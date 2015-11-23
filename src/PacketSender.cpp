#include "PacketSender.h"

PacketSender::PacketSender(){
	//init and bind the incoming socket
	sd = socket(IPFAMILY, SOCK_DGRAM, IPPROTO_UDP);
	if (sd == INVALID_SOCKET){
		std::cout << "Create socket for PacketSender failed: " << WSAGetLastError() << std::endl;
		exit(EXIT_FAILURE);
	}
}

PacketSender::~PacketSender(){
}

std::thread PacketSender::getThread(){
	return std::thread(&PacketSender::run, this);
}

void PacketSender::sendPacket(Packet* p, const std::string ip, const int port){
	TPacket* pTarget = new TPacket(p, ip, port);
	mutex.lock();
	{
		pQueue.push(pTarget);
	}
	mutex.unlock();
}

void PacketSender::run(){
	TPacket* next = nullptr;

	struct sockaddr_in si_target;
	memset((char *)&si_target, 0, sizeof(si_target));
	si_target.sin_family = IPFAMILY;
	int targetLen = sizeof(si_target);

	char buf[BUFLEN];
	int bufLen = BUFLEN;

	while (true){
		mutex.lock();
		{
			if (pQueue.size() != 0){
				next = pQueue.front();
				pQueue.pop();
			}
		}
		mutex.unlock();

		if (next != nullptr){
			next->setIPandPort(si_target);
			next->serializePacket(buf, bufLen);

			bufLen = sendto(sd, buf, bufLen, 0, (sockaddr*)&si_target, targetLen);
			if (bufLen == SOCKET_ERROR){
				std::cout << "sendto() failed: " << WSAGetLastError() << std::endl;
				exit(EXIT_FAILURE);
			}

			//std::cout << "send" << std::endl;
		}

		delete next;
		next = nullptr;
	}
}
