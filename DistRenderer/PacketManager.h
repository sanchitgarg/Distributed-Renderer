#pragma once

#include <direct.h>
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

		std::string	getIP();
		uint32_t	getPort();
		Packet*		getPacket();
		std::thread	getThread();

		//push packets into the queue, then send all of them altogether once.
		void push(Message::INIT* msg);
		void push(Message::ACK *msg);
		void push(Message::START_RENDER *msg);
		void push(Message::PIXEL *msg);
		void push(Message::DONE *msg);
		void push(Message::HALT *msg);
		void push(Message::CAM_MOVE *msg);
		void push(std::string orig_dir, std::string target_dir, 
			std::vector<std::string> filename);

		//return true if the sending is successful.
		bool sendPackets(const std::string ip, const uint32_t port);

	private:
		void run();
		bool readStreamToBuffer(SOCKET sd, std::vector<char> &buf, 
			int targetSize);
		sockaddr_in createSOCKADDR(std::string ip, uint32_t port);

		std::string ip;
		uint32_t port;
		SOCKET sRecv;

		//use pointer to avoid copying -> reducing "lock time"
		std::queue<Packet*> qSend;
		std::queue<Packet*> qRecv;
		std::mutex mutexRecv;
};