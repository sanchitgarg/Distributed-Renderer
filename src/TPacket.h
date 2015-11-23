#pragma once

#include <glm\glm.hpp>
#include <winsock2.h>
#include "msg.pb.h"
#include "Helper.h"

enum PacketType { 
	REQ = 0,
	PIXEL };

class TPacket{
	public:
		TPacket(Packet* p_, std::string ip_, int port_);
		~TPacket();
		bool serializePacket(char* buf, int& len);
		void setIPandPort(sockaddr_in& si_target);

	private:
		Packet* packet;
		std::string ip;
		u_short port;
};