#include "rpc_entry.h"
#include "rpc_argtypes.h"
#include "rpc.h"

LocalEntry::LocalEntry(ArgTypes * at, skeleton f) {
	LocalEntry::argTypes = at;
	LocalEntry::function = f;
}


GlobalEntry::GlobalEntry(ArgTypes * at, char * sd, int sp, int s) {
	GlobalEntry::argTypes = at;
	GlobalEntry::server_address = sd;
	GlobalEntry::server_port = sp;
	GlobalEntry::socket = s;
	GlobalEntry::used = 0;
}


