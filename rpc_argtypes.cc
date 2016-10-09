#include "rpc_argtypes.h"
#include "rpc_debug.h"
#include <cstring>
#include <cstdlib>


ArgTypes::ArgTypes() {
	ArgTypes::count = 0;
	ArgTypes::types = NULL;
}

ArgTypes::ArgTypes(int c, int * t) {
	ArgTypes::count = c;
	ArgTypes::types = (int *)(malloc((c + 1) * 4));
	memcpy(ArgTypes::types, t, (c + 1) * 4);
}


ArgTypes::~ArgTypes() {
	if (ArgTypes::types != NULL) {
		free(ArgTypes::types);		
	}
}


int argTypesCompare(ArgTypes * at1, ArgTypes * at2) {
	if (at1->count == at2->count) {
		for (int i = 0; i < at1->count; i++) {
			
			int a = (at1->types[i]) & (65535 << 16);
			int b = (at2->types[i]) & (65535 << 16);
			if (a != b) {
				return 0;
			}
			
		}
		if ((at1->types[at1->count] == 0) && (at2->types[at2->count] == 0)) {
			return 1;
		}
		return 0;
	}
	return 0;
}

ArgTypes * readArgTypes(void * msg) {
	
	char * argTypes_addr = (char *)msg;
	int count = 0;
	while (1) {
		int argType = *((int *)argTypes_addr);
		if (argType != 0) {
			count ++;
			argTypes_addr += sizeof(int);
		} else {
			break;
		}
	}
	
	int argTypes[count + 1];
	
	argTypes_addr = (char *)msg;
	
	for (int i = 0; i <= count; i++) {
		argTypes[i] = *((int *)argTypes_addr);
		argTypes_addr += sizeof(int);
	}
	
	return new ArgTypes(count, argTypes);
	
}


ArgTypes * readArgTypes(int * argTypes) {
	int count = 0;
	int * array = argTypes;
	while (1) {
		int argType = *array;
		if (argType != 0) {
			count ++;
			array ++;
		} else {
			break;
		}
	}
	return new ArgTypes(count, argTypes);
}


void printArgTypes(ArgTypes * at) {
	
	int count = at->count;
	int * types = at->types;
	
	int * array = types;
	
	print("this ArgTypes has count=");
	println(count);
	
	for (int i = 0; i <= count; i++) {
		int argType = *array;
		println(argType);
		array ++;
	}
	
	
}
