#include "rpc_sender.h"
#include "rpc_message.h"
#include "rpc_debug.h"
#include <sys/socket.h>
#include <cstdlib>
#include <cstring>

#define RC_ERROR -1
#define RC_SUCCESS 0

/* Send a message object to the socket */
/* A message object will be reassambled as continuous bytes */
/* A return code will be returned to indicate the function success or not */
int rpcSend(int connection, message * m) {
	
	void * buffer = NULL;
	
	try {
		
		int rc;
		
		int length = m->length;
		int type = m->type;
		void * msg = m->msg;
		
		int buffer_length = sizeof(length) + sizeof(type) + length;

		buffer = malloc(buffer_length);

		void * length_addr = (char *)buffer;
		void * type_addr = (char *)length_addr + 4;
		void * msg_addr = (char *)type_addr + 4;
				
		memcpy(length_addr, &length, 4);
		memcpy(type_addr, &type, 4);
		
		if (msg != NULL && length > 0) {
			memcpy(msg_addr, msg, length);
		}
				
		rc = send(connection, buffer, buffer_length, 0);
		if (rc < 0) {
			println("sending buffer failed");
			return RC_ERROR;
		}

		if (buffer != NULL) {
			//free(buffer);
		}
		return RC_SUCCESS;
		
	} catch (int e) {
		if (buffer != NULL) {
			//free(buffer);
		}
		println("exception catched while sending buffer");
		return RC_ERROR;
	}

}
