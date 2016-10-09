#include "rpc_receiver.h"
#include "rpc_message.h"
#include "rpc_debug.h"
#include <sys/socket.h>
#include <cstdlib>
#include <cstring>


/* Receive a buffer from socket, read its length, type and message part */
/* A message object will be return if no exception, otherwise NULL */
message * rpcReceive(int connection) {

	int rc;
	int length;
	int type;
	void * msg = NULL;
	
	message * m = NULL;

	// receive length of buffer
	try {
		char buffer_length[sizeof(int)];
		rc = recv(connection, buffer_length, sizeof(int), 0);
		if (rc < 0) {
			println("receives length of buffer failed.");
			return NULL;
		}
		length = *((int *)buffer_length);
	} catch (int e) {
		println("exception catched while receiving length of buffer");
		return NULL;
	}

	// receive type of buffer
	try {
		char buffer_type[sizeof(int)];
		rc = recv(connection, buffer_type, sizeof(int), 0);
		if (rc < 0) {
			println("receives type of buffer failed.");
			return NULL;
		}
		type = *((int *)buffer_type);
	} catch (int e) {
		println("exception catched while receiving type of buffer");
		return NULL;
	}
	
	
	// receive message of buffer
	try {
		if (length == 0) {
			msg = NULL;
		} else if (length > 0) {
			void * buffer_message = malloc(length);
			rc = recv(connection, buffer_message, length, 0);
			if (rc < 0) {
				println("receives message of buffer failed.");
				if (msg != NULL) {
					free(msg);		
				}
				return NULL;
			}
			msg = buffer_message;
			
		} else {
			println("invalid message length.");
			return NULL;
		}
	} catch (int e) {
		if (msg != NULL) {
			free(msg);		
		}
		println("exception catched while receiving message of buffer");
		return NULL;
	}
	
	m = new message(length, type, msg);

	if (msg != NULL) {
		free(msg);		
	}
	
	return m;
	
}
