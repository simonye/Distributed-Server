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

#define BINDER_ON 0
#define BINDER_OFF -1
#define BINDER_WARNING 1

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

#define BINDER_ERROR_SETUP -30


static int binder;
static int highsock;
static int binder_status = BINDER_OFF;
static std::list<int> sock_list;
static std::list<int> server_list;
static fd_set socks;
static std::unordered_map<char *, GlobalEntry> globalDB;


void removeEntry(int connection) {
	for (auto it = globalDB.begin(); it != globalDB.end(); ++it ) {
		if ((it->second).socket == connection) {
			free(it->first);
			free((it->second).server_address);
			delete((it->second).argTypes);
			globalDB.erase(it);
			removeEntry(connection);
			break;
		}
	}
}


void binderDropConn(int connection) {
	
	try {
		close(connection);
		print("Connection lost: ");
		print("FD=");
		println(connection);
		sock_list.remove(connection);
		server_list.remove(connection);
		removeEntry(connection);
	
	} catch (int e) {
		print("exception catched while dropping a connection");
	}
	
}


int binderHandleRegister(int connection, message * m) {

	
	int length = m->length;
	void * msg = m->msg;

	int rc;
	char * name;
	void * argTypes;
	char * server_address;
	int server_port;
	ArgTypes * at;

	try {
		char * server_address_adr = (char *)msg;
		char * server_port_addr =  server_address_adr + 128;
		char * name_addr = server_port_addr + 4;
		char * argTypes_addr = name_addr + 64;
		
		server_address = (char *)malloc(128);
		memcpy(server_address, server_address_adr, 128);
		server_port = *((int *)server_port_addr);
		name = (char *)malloc(64);
		memcpy(name, name_addr, 64);
		
		at = readArgTypes((void *)argTypes_addr);
		argTypes = at->types;

	} catch (int e) {
		println("binder catches exception while handling request from server.");
		return RC_ERROR;
	}

	int back_type;
	int back_reasonCode;

	back_type = REGISTER_SUCCESS;
	back_reasonCode = 0;
	
	if (back_type == REGISTER_SUCCESS) {
		GlobalEntry newEntry = GlobalEntry(at, server_address, server_port, connection);
		globalDB.insert(std::pair<char *, GlobalEntry>(name, newEntry));

		int inlist = 0;
		for (std::list<int>::const_iterator iterator = server_list.begin(), end = server_list.end(); iterator != end; ++iterator) {
			if (*iterator == connection) {
				inlist = 1;
			}
		}
		if (!inlist) {
			server_list.push_back(connection);
		}

		println("binder acceptes the function registration and added to globalDB.");
	}
	
	delete(m);

	try {
		
		int back_length = sizeof(int);
		void * msg_addr = malloc(back_length);
		memcpy(msg_addr, &back_reasonCode, sizeof(int));

		message * reply_message = new message(back_length, back_type, msg_addr);
		
		rc = rpcSend(connection, reply_message);
		if (rc < 0) {
			println("binder failed to send registration result back to server.");
			return RC_ERROR;
		}
		delete(reply_message);
		println("binder sent function registration result back to server.");

		return RC_SUCCESS;
		
	} catch (int e) {
		
		println("binder catches exception while sending reply back to server.");
		return RC_ERROR;
		
	}

}


int binderHandleLocRequest(int connection, message * m) {
	
	int length = m->length;
	void * msg = m->msg;

	int rc;
	char * name;
	void * argTypes;
	ArgTypes * at;
	int exist = 0;
	
	char * server_address;
	int server_port;
		
	try {
		
		char * name_addr = (char *)msg;
		char * argTypes_addr = name_addr + 64;
		name = (char *)malloc(64);
		memcpy(name, name_addr, 64);
		
		at = readArgTypes((void *)argTypes_addr);
		argTypes = at->types;
		

		
		for (auto it = globalDB.begin(); it != globalDB.end(); ++it ) {
			std::string a(it->first);
			std::string b(name);
			
			if (a == b) {
				if (argTypesCompare((it->second).argTypes, at)) {
					if ((it->second).used == 0) {
						exist = 1;
						server_address = (it->second).server_address;
						server_port = (it->second).server_port;
						break;
					}
				}
			}
		}
		
		if (exist) {
			for (auto it = globalDB.begin(); it != globalDB.end(); ++it ) {
				std::string a((it->second).server_address);
				std::string b(server_address);
				if (a == b) {
					(it->second).used = 1;
				}
			}
			
			
		} else {
			for (auto it = globalDB.begin(); it != globalDB.end(); ++it ) {
				(it->second).used = 0;
			}
			
				
			for (auto it = globalDB.begin(); it != globalDB.end(); ++it ) {
				std::string a(it->first);
				std::string b(name);
				
				if (a == b) {
					if (argTypesCompare((it->second).argTypes, at)) {
						if ((it->second).used == 0) {
							exist = 1;
							server_address = (it->second).server_address;
							server_port = (it->second).server_port;
							break;
						}
					}
				}
			}
			
			if (exist) {
				for (auto it = globalDB.begin(); it != globalDB.end(); ++it ) {
					std::string a((it->second).server_address);
					std::string b(server_address);
					if (a == b) {
						(it->second).used = 1;
					}
				}
			}
			
		}

		
	} catch (int e) {
		delete(m);
		println("binder catches exception while handling request from client.");
		return RC_ERROR;
		
	}

	int back_type;
	int back_reasonCode;
	int back_length;
	void * msg_addr;
	
	
	try {
		
		if (exist) {
			
			println("binder found match for this location request.");
			
			back_type = LOC_SUCCESS;
			back_length = 128 + 4;
			msg_addr = malloc(back_length);
			char * reply_server_address_addr = (char *)msg_addr;
			char * reply_server_port_addr = reply_server_address_addr + 128;
			memcpy(reply_server_address_addr, server_address, 128);
			memcpy(reply_server_port_addr, &server_port, 4);

		} else {
			
			println("binder receives a function_name/argTypes which is not registered.");
			back_type = LOC_FAILURE;
			back_reasonCode = -1;
			back_length = sizeof(int);
			msg_addr = malloc(back_length);
			memcpy(msg_addr, &back_reasonCode, sizeof(int));
		
		}
		
		message * reply_message = new message(back_length, back_type, msg_addr);
		
		rc = rpcSend(connection, reply_message);
		if (rc < 0) {
			println("binder failed to send location request result back to client.");
			delete(m);
			return RC_ERROR;
		}
		delete(reply_message);
		delete(m);
		println("binder sent location request result back to server.");
		
		return RC_SUCCESS;
	} catch (int e) {
		delete(m);
		println("binder catches exception while sending location request result back to server.");
		return RC_ERROR;
	}

}



int main(int argc, char *argv[]) {

	int rc;

	println("binder is starting up socket.");
	binder = binderSetup();
	if (binder < 0) {
		println("binder failed to start up socket.");
		return BINDER_ERROR_SETUP;
	}
	if (binder > highsock) {
		highsock = binder;
	}
	binder_status = BINDER_ON;

	println("binder initializes successfully. start receiving.");


	while (binder_status == BINDER_ON) {

		FD_ZERO(&socks);
		FD_SET(binder,&socks);
		for (std::list<int>::const_iterator iterator = sock_list.begin(), end = sock_list.end(); iterator != end; ++iterator) {
			FD_SET(*iterator, &socks);
		}

		int num_socks = select(highsock + 1, &socks, NULL, NULL, NULL);
		if (num_socks < 0) {
			println("binder selects error.");
		} else if (num_socks > 0) {

			for (int i = 0; i <= highsock; i++) {

				if (FD_ISSET(i, &socks)) {

					if (i == binder) {

						println("binder receives a new connection.");
						int connection = accept(binder, NULL, NULL);
						if (connection <= 0) {
							println("binder accepts client error.");
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

						println("binder receives a message from existing connection.");
						int keepConnection = 0;
						message * m = rpcReceive(i);
						if (m != NULL) {

							if (m->type == TERMINATE && m->length == 0) {
								println("binder receives a request from client to shutdown the system.");
								delete(m);
								binder_status = BINDER_OFF;
							} else if (m->type == REGISTER && m->length > 0) {
								println("binder receives a request from server to register a function.");
								rc = binderHandleRegister(i, m);
								if (rc == RC_ERROR) {
									println("binder handles server failed.");
								}
								keepConnection = 1;
							} else if (m->type == LOC_REQUEST && m->length > 0) {
								println("binder receives a request from client to retrieve the server location of a function.");
								rc = binderHandleLocRequest(i, m);
								if (rc == RC_ERROR) {
									println("binder handles client failed.");
								}
							} else {
								println("binder receives invalid request.");
							}

						} else {
							println("binder called receive() returns NULL.");						
						}
						
						if (!keepConnection) {
							binderDropConn(i);
						}

					}
				}

			}

		}

	}


	try {

		for (std::list<int>::const_iterator iterator = server_list.end(), begin = server_list.begin(); iterator != begin; --iterator) {
			int connection = *iterator;
			message * m = new message(0, TERMINATE, NULL);
			rc = rpcSend(connection, m);
			if (rc < 0) {
				println("binder failed to send TERMINATE message to a server.");
			}
			delete(m);
			binderDropConn(connection);
		}

	} catch (int e) {
		println("binder catches exception while sending TERMINATE message to servers.");
	}

	println("binder is closing all sockets.");
	for (std::list<int>::const_iterator iterator = sock_list.end(), begin = sock_list.begin(); iterator != begin; --iterator) {
		binderDropConn(*iterator);
	}
	close(binder);
	println("binder has closed all sockets, goodbye.");

	return RC_SUCCESS;

}
