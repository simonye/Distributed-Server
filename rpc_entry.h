#ifndef ENTRY_H
#define ENTRY_H

#include "rpc.h"
#include "rpc_argtypes.h"

class LocalEntry
{

public:
	ArgTypes * argTypes;
	skeleton function;
	LocalEntry(ArgTypes * at, skeleton f);

};


class GlobalEntry
{

public:
	ArgTypes * argTypes;
	char * server_address;
	int server_port;
	int socket;
	int used;
	GlobalEntry(ArgTypes * at, char * sd, int sp, int s);

};



#endif
