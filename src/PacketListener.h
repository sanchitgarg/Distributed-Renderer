#pragma once

#include <mutex>
#include <queue>

#include "msg.pb.h"
#include "Helper.h"

class PacketListener{
	public:
		PacketListener(int recvPort);
		~PacketListener();
		std::thread	getThread();
		Packet* getPacket();

	private:
		void run();

		SOCKET sd; 
		std::mutex mutex;
		std::queue<Packet*> pQueue;	//use pointer to avoid copying -> reducing "lock time"
};