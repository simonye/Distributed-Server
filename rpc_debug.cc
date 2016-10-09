#include "rpc_debug.h"
#include <iostream>

#define DEBUG_ON 0

void println(std::string str) {
	if (DEBUG_ON) std::cout << str << std::endl;

}

void println(int n) {
	if (DEBUG_ON) std::cout << n << std::endl;
}

void println(char c) {
	if (DEBUG_ON) std::cout << c << std::endl;
}

void print(std::string str) {
	if (DEBUG_ON) std::cout << str;
}

void print(int n) {
	if (DEBUG_ON) std::cout << n;
}

void print(char c) {
	if (DEBUG_ON) std::cout << c;
}
