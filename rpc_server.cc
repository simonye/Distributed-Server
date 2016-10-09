#include "rpc.h"
#include "rpc_debug.h"
#include "rpc_message.h"
#include "rpc_setup.h"
#include "rpc_entry.h"
#include "rpc_receiver.h"
#include "rpc_sender.h"
#include "rpc_argtypes.h"
#include <iostream>
#include <cstdlib>
#include <stdio.h>
#include <stdlib.h>
#include <cstring>
#include <string>
#include <cctype>
#include <list>
#include <unordered_map>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
#include <pthread.h>

#define RC_ERROR -1
#define RC_SUCCESS 0

#define DEBUG_ON 1

#define SERVER_ON 0
#define SERVER_OFF -1
#define SERVER_READY 1

#define REGISTER 1
#define REGISTER_SUCCESS 0
#define REGISTER_FAILURE -1
#define LOC_REQUEST 2
#define LOC_SUCCESS 0
#define LOC_FAILURE -1
#define EXECUTE 3
#define EXECUTE_SUCCESS 0
#define EXECUTE_FAILURE -1
#define TERMINATE 4

#define SERVER_ERROR_UNINIT -20
#define SERVER_ERROR_UNREG -21
#define SERVER_ERROR_ALREADY_INIT -22
#define SERVER_ERROR_BINDER_LOST -23
#define SERVER_ERROR_BINDER_INVALID_MESSAGE -24
#define SERVER_ERROR_BINDER_CONNECT -25
#define SERVER_ERROR_CLIENTS_SETUP -26



static int binder;
static int clients;
static int highsock;
static int server_port;
static char server_address[128];
static int server_status = SERVER_OFF;
static std::list<int> sock_list;
static fd_set socks;
static std::unordered_map<char *, LocalEntry> localDB;

struct socketMessage {                                                   
    int * connection;                                                   
    message * msg;                                                                                                                      
};   



int rpcInit() {

	if (server_status == SERVER_OFF) {
		
		// setup socket for listening clients
		println("server is starting up socket for clients.");
		clients = serverSetupClients(server_port);
		if (clients < 0) {
			println("server failed to start up socket for clients.");
			return RC_ERROR;
		}
		if (clients > highsock) {
			highsock = clients;
		}
		gethostname(server_address, sizeof(server_address));
		print("SERVER_ADDRESS ");
		println(server_address);
		print("SERVER_PORT ");
		println(server_port);


		// connect server with binder
		println("server is starting up socket for binder.");
		binder = serverSetupBinder();
		if (binder < 0) {
			println("server failed to start up socket for binder.");
			return RC_ERROR;
		}
		if (binder > highsock) {
			highsock = binder;
		}

		server_status = SERVER_READY;

		println("server initializes successfully.");

		return RC_SUCCESS;

	} else {
		println("server had already been initialized.");
		return RC_ERROR;
	}

}



int rpcRegister(char* name, int* argTypes, skeleton f) {
	
	if (server_status == SERVER_OFF) {
		println("server has not been initialized yet. function cannot be registered.");
		return RC_ERROR;
	}
	
	int rc;

	int request_type = REGISTER;	

	ArgTypes * at = readArgTypes(argTypes);
	
	int request_length = 128 + 4 + 64 + (at->count + 1) * 4;
	void * request_msg_addr = malloc(request_length);
	
	
	char * server_address_addr = (char *)request_msg_addr;
	char * server_port_addr = server_address_addr + 128;
	char * name_addr = server_port_addr + 4;
	char * argTypes_addr = name_addr + 64;
	

	memcpy(server_address_addr, server_address, sizeof(server_address));
	memcpy(server_port_addr, &server_port, sizeof(server_port));
	memcpy(name_addr, name, sizeof(name));
	memcpy(argTypes_addr, at->types, (at->count + 1) * 4);
	
	for (auto it = localDB.begin(); it != localDB.end(); ++it ) {
		std::string a(it->first);
		std::string b(name);
		if (a == b) {
			if (argTypesCompare((it->second).argTypes, at)) {
				//free(it->first);
				//delete((it->second).argTypes);
				localDB.erase(it);
				LocalEntry newEntry = LocalEntry(at, f);
				char * n = (char *)malloc(64);
				memcpy(n, name, sizeof(name));
				localDB.insert(std::pair<char *, LocalEntry>(n, newEntry));
				println("this server is already providing the same function, overrided skeleton");
				return RC_SUCCESS;
			
			}
		}
	}

	message * outgoing = new message(request_length, request_type, request_msg_addr);

	println("server is sending a register message to binder.");
	rc = rpcSend(binder, outgoing);
	if (rc < 0) {
		println("server failed to send register message to binder.");
		return RC_ERROR;
	}
	//delete(outgoing);
	println("server has sent a register message to binder. waiting for response.");

	message * incoming = rpcReceive(binder);
	if (incoming != NULL) {
		try {
			int recv_length = incoming->length;
			int recv_type = incoming->type;
			if (recv_length == sizeof(int) && recv_type == REGISTER_SUCCESS) {
				LocalEntry newEntry = LocalEntry(at, f);
				char * n = (char *)malloc(64);
				memcpy(n, name, sizeof(name));
				localDB.insert(std::pair<char *, LocalEntry>(n, newEntry));
				if (server_status == SERVER_READY) {
					server_status = SERVER_ON;
				}
				println("server registers function successfully.");
				rc = RC_SUCCESS;
			} else if (recv_length == sizeof(int) && recv_type == REGISTER_FAILURE) {
				println("server registers function failed.");
				rc = RC_ERROR;
			} else {
				println("server receives invalid message while receiving registration confirmation from binder.");
				rc = RC_ERROR;
			}
		} catch (int e) {
			println("server catches exception while reading function registration confirmation from binder.");
			rc = RC_ERROR;
		}
		
		if (rc == RC_SUCCESS) {
			//delete(incoming);
			return RC_SUCCESS;
		} else {
			return RC_ERROR;
		}
		
	} else {
		println("server failed to receive function registration confirmation from binder.");
		return RC_ERROR;
	}

	
}



void serverDropConn(int connection, int dropOnly) {
	close(connection);
	print("Connection lost: ");
	print("FD=");
	println(connection);
	sock_list.remove(connection);
}


int serverHandleExecute(int connection, message * m) {
	
	try {
		
		int rc;
	
		int request_length = m->length;
		void * request_msg = m->msg;
		
		int reply_length;
		int reply_type;
		void * reply_msg;
		
		char * name;
		int * argTypes;
		
		ArgTypes * at;
		
		int reasonCode;
		

		char * request_name_addr = (char *)request_msg;
		name = (char *)malloc(64);
		memcpy(name, request_name_addr, 64);

		char * request_argTypes_addr = request_name_addr + 64;
		at = readArgTypes((void *)request_argTypes_addr);
		argTypes = at->types;
		
		char * request_args_addr = request_argTypes_addr + (at->count + 1) * 4;
			
		char * arg_buffer = request_args_addr;
		
		void * args[at->count];

		for (int i = 0; i < at->count; i++) {
			
			int argtype = argTypes[i];
			int argArrayLength = argtype & 65535;
			int argdatatype = (argtype >> 16) & 255;

			int argsize = 0;
			if (argdatatype == ARG_CHAR) {
				argsize = sizeof(char);
			} else if (argdatatype == ARG_SHORT) {
				argsize = sizeof(short);
			} else if (argdatatype == ARG_INT) {
				argsize = sizeof(int);
			} else if (argdatatype == ARG_LONG) {
				argsize = sizeof(long);
			} else if (argdatatype == ARG_DOUBLE) {
				argsize = sizeof(double);
			} else if (argdatatype == ARG_FLOAT) {
				argsize = sizeof(float);
			} else {
				println("something bad happended while analysing data types in argtypes.");
				return RC_ERROR;
			}
			
			if (argArrayLength > 0) {
				argsize = argsize * argArrayLength;
			}

			void * arg = malloc(argsize);
			
			memcpy(arg, arg_buffer, argsize);
			
			args[i] = arg;
			
			arg_buffer += argsize;
			
		}
		
		skeleton f;
		int found = 0;
		for (auto it = localDB.begin(); it != localDB.end(); ++it ) {
			std::string a(it->first);
			std::string b(name);
			if (a == b) {
				if (argTypesCompare((it->second).argTypes, at)) {
					found = 1;
					f = (it->second).function;
					break;
				}
			}
		}
		
		
		
		if (found) {
			println("server found a matched function to execute this rpccall.");
			
			rc = f((int *)argTypes, args);
			if (rc < 0) {
				reasonCode = -1;
				reply_type = EXECUTE_FAILURE;
			} else {
				reply_type = EXECUTE_SUCCESS;
			}

		} else {
			reasonCode = -1;
			reply_type = EXECUTE_FAILURE;
			println("server receives a rpccall with unregistered/mismatch function name/argTypes from client.");
		}
		
		int args_length = 0;
		int newArgTypes[at->count + 1];

		if (reply_type == EXECUTE_SUCCESS) {
			
			for (int i = 0; i < at->count; i++) {
				
				void * arg = args[i];
				int argtype = argTypes[i];
				int argdatatype = (argtype >> 16) & 255;
				int argArrayLength = argtype & 65535;
				int newArgArrayLength = 0;
				
				if (argdatatype == ARG_CHAR) {
					int argsize = sizeof(char);
					if (argArrayLength > 0) {
						char * newarg = (char *)arg;
						while(newarg[newArgArrayLength] && newArgArrayLength < argArrayLength){
							newArgArrayLength++;
						}
						args_length += argsize * newArgArrayLength;
						newArgTypes[i] = (argtype & (65535 << 16)) | argArrayLength;	
					} else {
						args_length += argsize;
						newArgTypes[i] = argtype;
					}
					
				} else if (argdatatype == ARG_SHORT) {
					int argsize = sizeof(short);
					if (argArrayLength > 0) {
						short * newarg = (short *)arg;
						while(newarg[newArgArrayLength] && newArgArrayLength < argArrayLength){
							newArgArrayLength++;
						}
						args_length += argsize * newArgArrayLength;
						newArgTypes[i] = (argtype & (65535 << 16)) | argArrayLength;	
					} else {
						args_length += argsize;
						newArgTypes[i] = argtype;
					}
				} else if (argdatatype == ARG_INT) {
					int argsize = sizeof(int);
					if (argArrayLength > 0) {
						int * newarg = (int *)arg;
						while(newarg[newArgArrayLength] && newArgArrayLength < argArrayLength){
							newArgArrayLength++;
						}
						args_length += argsize * newArgArrayLength;
						newArgTypes[i] = (argtype & (65535 << 16)) | argArrayLength;	
					} else {
						args_length += argsize;
						newArgTypes[i] = argtype;
					}
					
				} else if (argdatatype == ARG_LONG) {
					int argsize = sizeof(long);
					if (argArrayLength > 0) {
						long * newarg = (long *)arg;
						while(newarg[newArgArrayLength] && newArgArrayLength < argArrayLength){
							newArgArrayLength++;
						}
						args_length += argsize * newArgArrayLength;
						newArgTypes[i] = (argtype & (65535 << 16)) | argArrayLength;	
					} else {
						args_length += argsize;
						newArgTypes[i] = argtype;
					}
					
				} else if (argdatatype == ARG_DOUBLE) {
					int argsize = sizeof(double);
					if (argArrayLength > 0) {
						double * newarg = (double *)arg;
						while(newarg[newArgArrayLength] && newArgArrayLength < argArrayLength){
							newArgArrayLength++;
						}
						args_length += argsize * newArgArrayLength;
						newArgTypes[i] = (argtype & (65535 << 16)) | argArrayLength;	
					} else {
						args_length += argsize;
						newArgTypes[i] = argtype;
					}
					
				} else if (argdatatype == ARG_FLOAT) {
					int argsize = sizeof(float);
					if (argArrayLength > 0) {
						float * newarg = (float *)arg;
						while(newarg[newArgArrayLength] && newArgArrayLength < argArrayLength){
							newArgArrayLength++;
						}
						args_length += argsize * newArgArrayLength;
						newArgTypes[i] = (argtype & (65535 << 16)) | argArrayLength;	
					} else {
						args_length += argsize;
						newArgTypes[i] = argtype;
					}
					
				} else {
					println("something bad happended while analysing data types in argtypes of result.");
					return RC_ERROR;
				}
			
				
			}
			
			
			newArgTypes[at->count] = 0;

		}
		
		
		if (reply_type == EXECUTE_SUCCESS) {

			reply_length = 64 + (at->count + 1) * 4 + args_length;
			reply_msg = malloc(reply_length);

			char * name_addr = (char *)reply_msg;
			char * argTypes_addr = name_addr + 64;
			char * args_addr = argTypes_addr + (at->count + 1) * 4;

			memcpy(name_addr, name, 64);
			memcpy(argTypes_addr, newArgTypes, (at->count + 1) * 4);
			

			for(int i = 0;i < at->count; i++){
				int arraylength = newArgTypes[i] & 65535;
				int type = (newArgTypes[i] >> 16) & 255;
				if(arraylength == 0){
					arraylength = 1;
				}
				void * arg = args[i];

				if(type == ARG_CHAR){
					char * chars = (char *)arg;
					for(int j = 0; j < arraylength; j++){
						char * c = new char();
						*c = chars[j];
						memcpy(args_addr,c,sizeof(char));
						args_addr = args_addr + sizeof(char);
					}

				}
				if(type == ARG_SHORT){
					short * shorts = (short *)arg;
					for(int j = 0; j < arraylength; j++){
						short * s = new short();
						*s = shorts[j];
						memcpy(args_addr,s,sizeof(short));
						args_addr = args_addr + sizeof(short);
					}
				}
				if(type == ARG_INT){
					int * chars = (int *)arg;
					for(int j = 0; j < arraylength; j++){
						int * c = new int();
						*c = chars[j];
						memcpy(args_addr,c,sizeof(int));
						args_addr = args_addr + sizeof(int);
					}
				}
				if(type == ARG_LONG){
					long * chars = (long *)arg;
					for(int j = 0; j < arraylength; j++){
						long * c = new long();
						*c = chars[j];
						memcpy(args_addr,c,sizeof(long));
						args_addr = args_addr + sizeof(long);
					}
				}
				if(type == ARG_DOUBLE){
					double * chars = (double *)arg;
					for(int j = 0; j < arraylength; j++){
						double * c = new double();
						*c = chars[j];
						memcpy(args_addr,c,sizeof(double));
						args_addr = args_addr + sizeof(double);
					}
				}
				if(type == ARG_FLOAT){
					float * chars = (float *)arg;
					for(int j = 0; j < arraylength; j++){
						float * c = new float();
						* c = chars[j];
						memcpy(args_addr,c,sizeof(float));
						args_addr = args_addr + sizeof(float);
					}
				}


			}
			

		} else if (reply_type == EXECUTE_FAILURE) {

			reply_length = sizeof(int);
			reply_msg = malloc(reply_length);
			memcpy(reply_msg, &reasonCode, sizeof(int));

		} else {
			println("oooooops, the server encounters a coding bug, please blame Tian. (reply_type==else)");
			return RC_ERROR;
		}
		
		message * reply_message = new message(reply_length, reply_type, reply_msg);
		rc = rpcSend(connection, reply_message);
		if (rc < 0) {
			println("server failed to send reply to client.");
		}
		//free(reply_msg);
		//delete(reply_message);
		println("server sent reply back to client successfully.");
		return RC_SUCCESS;


	} catch (int e) {
		println("server catches exception while sending reply back to a client.");
		return RC_ERROR;
	}


}


void * executeThread(void * arg) {
	
	int rc;
	
	struct socketMessage * sm = (struct socketMessage *)arg;
	message * m = sm->msg;
	int connection = *(sm->connection);	
	
	rc = serverHandleExecute(connection, m);
	if (rc == RC_ERROR) {
		println("server handles client request failed.");
		//delete(m);
		serverDropConn(connection, 0);
		pthread_exit((void *)RC_ERROR);
	}
	//delete(m);
	serverDropConn(connection, 0);
	pthread_exit(NULL);
}



int rpcExecute() {

	int rc;
	
	if (server_status == SERVER_OFF) {
		println("Server has not been initialize yet.");
		return RC_ERROR;
	} else if (server_status == SERVER_READY) {
		println("Server has not registered any function yet.");
		return RC_ERROR;
	}

	pthread_t newthread;
	while (server_status == SERVER_ON) {

		FD_ZERO(&socks);
		FD_SET(binder,&socks);
		FD_SET(clients,&socks);
		for (std::list<int>::const_iterator iterator = sock_list.begin(), end = sock_list.end(); iterator != end; ++iterator) {
			FD_SET(*iterator, &socks);
		}

		int num_socks = select(highsock + 1, &socks, NULL, NULL, NULL);
		if (num_socks < 0) {
			println("server selects error.");

		} else if (num_socks > 0) {

			for (int i = 0; i <= highsock; i++) {

				if (FD_ISSET(i, &socks)) {

					if (i == binder) {

						println("server receives a message from binder.");

						message * m = rpcReceive(binder);
						if (m == NULL) {
							println("server called receive() returns NULL.");
							server_status = SERVER_OFF;
						} else if (m->type == TERMINATE && m->length == 0) {
							println("server receives shutdown message from binder.");
							server_status = SERVER_OFF;
							//delete(m);
							break;
						} else {
							println("server receives invalid type from binder.");
							server_status = SERVER_OFF;
							//delete(m);
						}
						

					} else if (i == clients) {

						println("server receives a new client connection.");

						int connection = accept(clients, NULL, NULL);
						if (connection <= 0) {
							println("server accepts client error.");
						} else {
							sock_list.push_back(connection);
							if (connection > highsock) {
								highsock = connection;
							}
							print("Connection accepted: ");
							print("FD=");
							println(connection);
						}

					} else {
						
						println("server receives a message from a existing client.");
						println("A seperate thread is started to handle this message/request.");
						
						message * m = rpcReceive(i);

						if (m != NULL) {
							if (m->type == EXECUTE && m->length > 0) {
								struct socketMessage * sm = new struct socketMessage;
								int * x  = new int;
								*x = i;
								sm -> msg = m;
								sm -> connection = x;
								rc = pthread_create(&newthread, NULL, executeThread, (void *)sm);
								if (rc){
									println("pthread error.");
								}
							} else {
								println("server receives invalid request from client.");
								//delete(m);
								serverDropConn(i, 0);
							}
						} else {
							println("server called receive() returns NULL.");
							serverDropConn(i, 0);
						}
						
						
					}
					
					
				}
			}
		}

	}

	println("server is closing all sockets.");
	for (std::list<int>::const_iterator iterator = sock_list.end(), begin = sock_list.begin(); iterator != begin; --iterator) {
		serverDropConn(*iterator, 1);
	}
	close(clients);
	close(binder);
	println("server has closed all sockets, goodbye.");

	return RC_SUCCESS;

}
