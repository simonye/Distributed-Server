#include "rpc.h"
#include "rpc_debug.h"
#include "rpc_message.h"
#include "rpc_setup.h"
#include "rpc_entry.h"
#include "rpc_receiver.h"
#include "rpc_sender.h"
#include <iostream>
#include <cstdlib>
#include <cstring>
#include <string>
#include <cctype>
#include <list>
#include <map>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <netdb.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>

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



#define SOCKET_OPEN_FAIL -2
#define SERVER_UNKNOW_HOST -3
#define SOCKET_CONNECTION_FAIL -4
#define FUNCTION_NAME_NOT_MATCH -5
#define GET_BINDERADDRESS_FAIL -6
#define GET_BINDERPORT_FAIL -7
#define SEND_MESSAGETOBINDER_FAIL -8
#define SEND_MESSAGETOSERVER_FAIL -9
#define SEND_EXECUTINGMESSAGE_FAIL -10
#define GET_EXECTUINGMESSAGE_FAIL -11

using namespace std;
int final_result;
char * binder_address;
char * binder_port;
int socketfd = -1;
char * server_address;
int server_port;

//called by client
//looking for right server (contact with binder)
//send request to server
//handle server's return
//handle all kind of error/exception



int setSocket(char * address, int port){
	
	int mysocket;
	struct hostent *server;
	struct sockaddr_in serverAddressStruct;

	mysocket = socket(AF_INET, SOCK_STREAM, 0);
	if(mysocket < 0){
		println("Socket open fail");
		return SOCKET_OPEN_FAIL;
	}


	server = gethostbyname(address);
	if (server == NULL) {
        println("Sever Unknow host");
		return SERVER_UNKNOW_HOST;
    }
    memset((char *) &serverAddressStruct, 0,sizeof(serverAddressStruct));
    serverAddressStruct.sin_family = AF_INET;
    bcopy((char *)server->h_addr,
         (char *)&serverAddressStruct.sin_addr.s_addr,
         server->h_length);
    serverAddressStruct.sin_port = htons(port);
    int rc = connect(mysocket,(struct sockaddr *) &serverAddressStruct,sizeof(serverAddressStruct));

	if(rc < 0){
		println("Socket connection fail");
		return SOCKET_CONNECTION_FAIL;
	}
    return mysocket;
}


int rpcTerminate(void) {
	// send terminate message to binder
	int result;
	int binder_port_num = atoi(binder_port);
	socketfd = setSocket(binder_address,binder_port_num);
    void * null_pointer = NULL;
    message * terminate_binder = new message(0, TERMINATE, null_pointer);
    result = rpcSend(socketfd, terminate_binder);
    
	// send terminate message to server
   
    delete(terminate_binder);   
	return result;
}



void getArgumentsMessage(int * argTypes, void** args, char * arg_buffer){
	int argtype_size = 0;
	while(argTypes[argtype_size]!=0){
		argtype_size++;
	}
	for(int i = 0; i < argtype_size; i++){
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
			}

			if (argArrayLength > 0) {
				argsize = argsize * argArrayLength;
			}

			void * arg = malloc(argsize);


			memcpy(arg, arg_buffer, argsize);
			char * tmp_result = (char *)arg;
			
			args[i] = arg;

			arg_buffer += argsize;
	}

}

//called by client
//informs binder the system is going to be shuted down
char * mutateArgumentTypes(int * argTypes, char * buf,void** args){
	int argtype_size = 0;
	while(argTypes[argtype_size]!=0){
		argtype_size++;
	}
	argtype_size++;

	memcpy(argTypes, buf, sizeof(int) * argtype_size);


	buf = buf + (sizeof(int) * argtype_size);
	getArgumentsMessage(argTypes, args, buf);
	return buf;
}

int rpcGetExecuteResult(char* name, int * argTypes, void** args){
	//char * buffer = (char *)buf;
	message * return_m = rpcReceive(socketfd);
	println("EXECUTE TYPE:");
	println(return_m->type);
	if(return_m->type == EXECUTE_SUCCESS){
		println("EXECUTE SUCCESS");
		char * buffer = (char *)return_m->msg;
		char recieve_name[64];
		for(int i = 0; i < 64; i++){
			recieve_name[i] = * buffer;
			buffer = buffer + 1;
		}

		string functionName(recieve_name);
		if(strcmp(functionName.c_str(), name) == 0)
        {
        	println("function name the same");
        	char  * buffer2 =  mutateArgumentTypes(argTypes,buffer,args);
           // getArgumentsMessage(argTypes, args, buffer2);
        }
        else
        {
        	println("function name not the same");
            return FUNCTION_NAME_NOT_MATCH;
        }

		return 0;

	}
	if(return_m->type == EXECUTE_FAILURE){
				println("EXECUTE FAIL");
				int reason_fail = * ((int *)(return_m->msg));
				
				return reason_fail;

			}
}





int rpcSendExecuteRequest(char* name, int * argTypes, void** args){

		int args_length = 0;
		int argtype_size = 0;
		while(argTypes[argtype_size]!=0){
			argtype_size++;
		}
		argtype_size++;
		for(int i = 0;i < argtype_size -1 ; i++){
			int argtype = argTypes[i];
			int arraylength = argtype & 65535;
			if(arraylength==0)arraylength = 1;
			int type = (argtype >> 16) & 255;
			if(type == ARG_CHAR){
				arraylength = arraylength * sizeof(char);
			}
			else if(type == ARG_SHORT){
				arraylength = arraylength * sizeof(short);
			}
			else if(type == ARG_INT){
				arraylength = arraylength * sizeof(int);
			}
			else if(type == ARG_LONG){
				arraylength = arraylength * sizeof(long);
			}
			else if(type == ARG_DOUBLE){
				arraylength = arraylength * sizeof(double);
			}
			else {
				arraylength = arraylength * sizeof(float);
			}
			args_length = args_length + arraylength;

		}
		char function_name[64];
		strncpy(function_name,name,sizeof(name));

		int length = sizeof(function_name) + 4 * argtype_size + args_length;
		void * msg_addr = malloc(length);

		char * client_name_address = (char *)msg_addr;
		char * argTypes_addr = client_name_address + sizeof(function_name);

		char * args_addr = argTypes_addr + 4 * argtype_size;
		memcpy(client_name_address, function_name, sizeof(function_name));

		memcpy(argTypes_addr, argTypes, 4 * argtype_size);

		for(int i = 0;i < argtype_size - 1; i++){
			int argtype = argTypes[i];
			int arraylength = argtype & 65535;
			int type = (argtype >> 16) & 255;
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
		message * m = new message(length, 3, msg_addr);
    	int result = rpcSend(socketfd, m);

		if(result<0){
			return result;
		}

		return result;
}
int sendMessageToServer(message *m){


	char * buffer = (char *)m->msg;

	if(m->type == LOC_FAILURE) {
		println("Request to binder fail");
		int reasoncode = * ((int *)(buffer));
		return reasoncode;


	} else	if (m->type == LOC_SUCCESS) {



		char * server_address_adr = (char *)m->msg;
		char * server_port_addr =  server_address_adr + 128;
		server_address = (char *)malloc(128);
		memcpy(server_address, server_address_adr, 128);

		server_port = *((int *)server_port_addr);

		socketfd = setSocket(server_address, server_port);
		return 0;

	}

}

int rpcSendRequest(char* name, int * argTypes, void** args){

	int result;

                                    //setup socket
		binder_address = getenv ("BINDER_ADDRESS");
		binder_port = getenv("BINDER_PORT");
		if(binder_address == NULL){
			println("Get binder address fail");
			result = GET_BINDERADDRESS_FAIL;
			return result;
		}
		if(binder_port == NULL){
			println("Get binder port fail");
			result = GET_BINDERPORT_FAIL;
			return result;
		}
		int binder_port_num = atoi(binder_port);
		socketfd = setSocket(binder_address,binder_port_num);

	
		//send message to binder
	int argtype_size = 0;
		while(argTypes[argtype_size]!=0){
			argtype_size++;
		}
		argtype_size++;
		char function_name[64];
		strncpy(function_name,name,sizeof(name));
		int length = sizeof(function_name) + 4 * argtype_size;
		void * msg_addr = malloc(length);

		char * client_name_address = (char *)msg_addr;
		char * argTypes_addr = client_name_address + 64;
		memcpy(client_name_address, function_name, sizeof(function_name));

		memcpy(argTypes_addr, argTypes, 4 * argtype_size);


    	message * m = new message(length, 2, msg_addr);
    	result = rpcSend(socketfd, m);
    	if(result < 0){
    		println("Send message to binder fail");
    		return SEND_MESSAGETOBINDER_FAIL;
    	}
    	delete(m);




		message * return_m = rpcReceive(socketfd);
		result = sendMessageToServer(return_m);
		if(result < 0){
			println("Sending message to server fail");
			return SEND_MESSAGETOSERVER_FAIL;
		}

		delete(return_m);

		result = rpcSendExecuteRequest(name, argTypes, args);
		if(result < 0){
			println("Sending executing message fail");
			return SEND_EXECUTINGMESSAGE_FAIL;
		}

		result = rpcGetExecuteResult(name, argTypes, args);
		if(result < 0){
			println("Getting executing message fail");
			return GET_EXECTUINGMESSAGE_FAIL;
		}
	

	return result;
}

int rpcCall(char* name, int* argTypes, void** args) {
	int result = rpcSendRequest(name, argTypes, args);
	close(socketfd);
  	return result;
}
