#pragma once

#include <mutex>
#include <queue>

#include "Packet.h"
#include "Helper.h"

/*
	Packet Frame:
	1st byte : MSGType
	2-5th: Message/Packet/File size.
*/

class PacketManager{
	public:
		PacketManager(uint32_t port_);
		~PacketManager();

		uint32_t	getPort();
		std::thread	getThread();
		Packet*		getPacket();

		//push packets into the queue, then send all of them altogether once.
		void push(Message::INIT* msg);
		void push(Message::ACK *msg);
		void push(Message::START_RENDER *msg);
		void push(Message::PIXEL *msg);

		void sendPackets(const std::string ip, const uint32_t port);

	private:
		void run();
		bool readStreamToBuffer(SOCKET sd, char* buf, int targetSize);
		sockaddr_in createSOCKADDR(std::string ip, uint32_t port);

		uint32_t port;
		SOCKET sRecv;

		//use pointer to avoid copying -> reducing "lock time"
		std::queue<Packet*> qSend;
		std::queue<Packet*> qRecv;
		std::mutex mutexRecv;
};