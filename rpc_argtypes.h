#ifndef RPC_ARGTYPES_H
#define RPC_ARGTYPES_H

class ArgTypes{
	
public:
	int count;
	int * types;
	ArgTypes();
	~ArgTypes();
	ArgTypes(int c, int * t);
};

ArgTypes * readArgTypes(void * msg);

ArgTypes * readArgTypes(int * argTypes);

int argTypesCompare(ArgTypes * at1, ArgTypes * at2);

void printArgTypes(ArgTypes * at);

#endif
