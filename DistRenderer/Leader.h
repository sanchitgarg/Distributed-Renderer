#pragma once

#include "PacketManager.h"

enum LeaderState { WAITFORINIT, RECVSCENE, START, MONITOR };

class Leader{
public:
	Leader(PacketManager *pMgr_);
	std::thread	getThread();
	void step();	//one step of run(), for local testing purpose (run on the main thread to share GPU)
	void run();

	void addRendererIPPort(std::string ip_, unsigned int port_);
	void setViewerIPPort(std::string ip_, unsigned int port_);

	private:
		//states
		void leaderStandby();
		void recvSceneFiles();
		void initRenderers();
		void monitorRenderers();

		PacketManager* pMgr;
		LeaderState state;

		int sendIteration;

		std::string viewerIP;
		unsigned int viewerPort;
		std::string leaderIP;
		unsigned int leaderPort;
		std::set<IP_Port> rendererMap;
		std::map<unsigned int, IP_Port> activeRenderer;	//when rendering, we know which nodes to update.
		int finishedRendering;
};