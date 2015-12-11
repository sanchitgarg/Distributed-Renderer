#pragma once

#include "Helper.h"
#include "GLDisplay.h"
#include "PacketManager.h"

enum FrontEndState { IDLE = 0, WAITFORGREENLIGHT, SENDSCENE, READY };

class FrontEnd{
	public:
		FrontEnd(PacketManager* pMgr_, std::string dirPath_, 
			std::string listTxt_, int sendIteration_);
		~FrontEnd();
		void setLeaderIPPort(std::string ip_, uint32_t port_);
		std::thread	getThread();
		void step();	//one step of run(), for local testing purpose (run on the main thread to share GPU)
		void run();

	private:
		void initGL();
		void checkRendererAvailability();
		void waitForLeader();
		void sendSceneInfo();
		void fetchPixels();
		
		int sendIteration;

		FrontEndState state;
		std::string leaderIP;
		uint32_t leaderPort;

		PacketManager* pMgr;
		GLDisplay* display;

		std::string dirPath;
		std::string listTxt;	//text file containing all the files associated with this scene
};