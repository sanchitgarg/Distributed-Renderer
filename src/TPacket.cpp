#include "TPacket.h"

TPacket::TPacket(Packet* p_, std::string ip_, int port_){
	packet = p_;
	ip = ip_;
	port = htons(port_);
}

TPacket::~TPacket(){
	delete packet;
}

bool TPacket::serializePacket(char* buf, int& len){
	len = packet->ByteSize();
	if (len > BUFLEN){
		std::cout << "packet too large" << std::endl;
		exit(EXIT_FAILURE);
	}

	return packet->SerializeToArray(buf, len);
}

void TPacket::setIPandPort(sockaddr_in& si_target){
	si_target.sin_port = port;
	inet_pton(IPFAMILY, ip.c_str(), &si_target.sin_addr);
}
