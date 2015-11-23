#include "PacketListener.h"

PacketListener::PacketListener(int recvPort){
	//init and bind the incoming socket
	sd = socket(IPFAMILY, SOCK_DGRAM, 0);
	if (sd == INVALID_SOCKET){
		std::cout << "Create socket at port " << recvPort << " failed: " << WSAGetLastError() << std::endl;
		exit(EXIT_FAILURE);
	}

	struct sockaddr_in recv;
	recv.sin_family = IPFAMILY;
	recv.sin_addr.S_un.S_addr = INADDR_ANY;
	recv.sin_port = htons(recvPort);
	int bindResult = bind(sd, (struct sockaddr*)&recv, sizeof(recv));
	if (bindResult == SOCKET_ERROR){
		std::cout << "Binding port " << recvPort << " failed: " + WSAGetLastError() << std::endl;
		exit(EXIT_FAILURE);
	}
}

PacketListener::~PacketListener(){
	closesocket(sd);
}

std::thread PacketListener::getThread(){
	return std::thread(&PacketListener::run, this);
}

Packet* PacketListener::getPacket(){
	if (pQueue.size() == 0)
		return nullptr;

	Packet* pOut;
	mutex.lock();
	{
		pOut = pQueue.front();
		pQueue.pop();
	}
	mutex.unlock();

	return pOut;
}

void PacketListener::run(){
	Packet* packet = new Packet();
	char buf[BUFLEN];
	int bufLen = BUFLEN;
	sockaddr_in si_from;
	int fromLen = sizeof(si_from);

	while(true) {
		memset(buf, '\0', bufLen);
		bufLen = recvfrom(sd, buf, BUFLEN, 0, (sockaddr*)&si_from, &fromLen);
		if (bufLen == SOCKET_ERROR){
			std::cout << "recvfrom() failed: " << WSAGetLastError() << std::endl;
			exit(EXIT_FAILURE);
		}
		else if (packet->ParseFromArray(buf, bufLen)){
			//std::cout << "Packet Received. Type: " << packet->type() << std::endl;
			mutex.lock();
			{
				pQueue.push(packet);
			}
			mutex.unlock();
			packet = new Packet();
		}
	}
}
