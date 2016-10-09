CXX = g++

all:
	${CXX} -std=c++0x -pthread -c rpc_message.cc -o rpc_message.o
	${CXX} -std=c++0x -pthread -c rpc_debug.cc -o rpc_debug.o
	${CXX} -std=c++0x -pthread -c rpc_setup.cc -o rpc_setup.o
	${CXX} -std=c++0x -pthread -c rpc_entry.cc -o rpc_entry.o
	${CXX} -std=c++0x -pthread -c rpc_argtypes.cc -o rpc_argtypes.o
	${CXX} -std=c++0x -pthread -c rpc_receiver.cc -o rpc_receiver.o
	${CXX} -std=c++0x -pthread -c rpc_sender.cc -o rpc_sender.o
	${CXX} -std=c++0x -pthread -c rpc_server.cc -o rpc_server.o
	${CXX} -std=c++0x -pthread -c rpc_client.cc -o rpc_client.o
	ar rcs librpc.a rpc_server.o rpc_client.o rpc_setup.o rpc_message.o rpc_debug.o rpc_entry.o rpc_receiver.o rpc_sender.o rpc_argtypes.o
	
	${CXX} -std=c++0x -pthread -c rpc_binder.cc -o rpc_binder.o
	${CXX} -std=c++0x -pthread -L. rpc_binder.o -lrpc -o binder
	


clean:
	rm librpc.a rpc_message.o rpc_debug.o rpc_setup.o rpc_entry.o rpc_argtypes.o rpc_receiver.o rpc_sender.o rpc_server.o rpc_client.o binder
