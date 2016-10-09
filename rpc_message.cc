#include "rpc_message.h"
#include <cstring>
#include <cstdlib>

message::message(int l, int t, void * m) {
	
	if (l > 0 && m != NULL) {
		message::length = l;
		message::type = t;
		message::msg = malloc(length);
		memcpy(message::msg, m, length);
	} else if (l == 0 && m == NULL) {
		message::length = 0;
		message::type = t;
		message::msg = NULL;
	} else {
		message::length = 0;
		message::type = -2;
		message::msg = NULL;
	}
	
}


message::~message() {
	if (message::msg != NULL) {
		free(message::msg);
	}
}
