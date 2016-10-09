#include "rpc_setup.h"
#include "rpc_debug.h"
#include "rpc.h"
#include <cstdlib>
#include <cstring>
#include <sys/socket.h>
#include <unistd.h>
#include <netdb.h>

#define RC_ERROR -1


int serverSetupBinder() {

	int rc;

	// create a socket to connect with binder
	int binder = socket(AF_INET, SOCK_STREAM, 0);
	if (binder < 0)
	{
		println("server creates binder socket error.");
		return RC_ERROR;
	}

	// connect binder
	static char * BINDER_PORT = getenv("BINDER_PORT");
	static char * BINDER_ADDRESS = getenv("BINDER_ADDRESS");
	struct addrinfo hints;
	struct addrinfo * servinfo;
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	getaddrinfo(BINDER_ADDRESS, BINDER_PORT, &hints, &servinfo);
	rc = connect(binder, servinfo->ai_addr, servinfo->ai_addrlen);
	if (rc < 0) {
		println("server connects binder error.");
		freeaddrinfo(servinfo);
		return RC_ERROR;
	}
	freeaddrinfo(servinfo);

	println("server connects binder successfully.");
	return binder;
}


int serverSetupClients(int &server_port) {

	int rc;

	// create a socket to listening for connection from clients
	int clients = socket(AF_INET, SOCK_STREAM, 0);
	if (clients < 0)
	{
		println("server creates clients socket error.");
		return RC_ERROR;
	}

	// bind a random port
	struct sockaddr_in saddr;
	saddr.sin_family = AF_INET;
	saddr.sin_addr.s_addr = htonl(INADDR_ANY);
	saddr.sin_port = htons(0);
	rc = bind(clients, (struct sockaddr *)&saddr, sizeof(saddr));
	if (rc < 0) {
		println("server binds port error.");
		return RC_ERROR;
	}

	// start listening the clients sockets
	rc = listen(clients, SOMAXCONN);
	if (rc < 0) {
		println("server listens for clients error.");
		return RC_ERROR;
	}

	socklen_t saddrsize = sizeof(saddr);
	getsockname(clients, (struct sockaddr *)&saddr, &saddrsize);
	server_port = ntohs(saddr.sin_port);

	return clients;

}


int binderSetup() {

	int rc;

	// create a socket to listening for connection from clients/servers
	int binder = socket(AF_INET, SOCK_STREAM, 0);
	if (binder < 0)
	{
		println("binder creates socket error.");
		return RC_ERROR;
	}

	// bind a random port
	struct sockaddr_in saddr;
	saddr.sin_family = AF_INET;
	saddr.sin_addr.s_addr = htonl(INADDR_ANY);
	saddr.sin_port = htons(0);
	rc = bind(binder, (struct sockaddr *)&saddr, sizeof(saddr));
	if (rc < 0) {
		println("binder binds port error.");
		return RC_ERROR;
	}

	// start listening the clients sockets
	rc = listen(binder, SOMAXCONN);
	if (rc < 0) {
		println("binder listens socket error.");
		return RC_ERROR;
	}

	char binder_address[128];
	gethostname(binder_address, sizeof binder_address);
	socklen_t saddrsize = sizeof(saddr);
	getsockname(binder, (struct sockaddr *)&saddr, &saddrsize);
	int binder_port = ntohs(saddr.sin_port);
	std::cout << "BINDER_ADDRESS " << binder_address << std::endl;
	std::cout << "BINDER_PORT    " << binder_port << std::endl;

	return binder;

}
