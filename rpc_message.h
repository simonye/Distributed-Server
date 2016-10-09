#ifndef RPC_MESSAGE_H
#define RPC_MESSAGE_H

class message
{

public:

	int length;
	int type;
	void * msg;
	
	message(int l, int t, void * m);
	~message();

};

#endif
