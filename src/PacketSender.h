#pragma once

#include <mutex>
#include <queue>

#include "msg.pb.h"
#include "Helper.h"
#include "TPacket.h"

class PacketSender{
	public:
		PacketSender();
		~PacketSender();
		std::thread getThread();
		void sendPacket(Packet* p, const std::string ip, const int port);

	private:
		void run();

		SOCKET sd;
		std::mutex mutex;
		std::queue<TPacket*> pQueue;
};