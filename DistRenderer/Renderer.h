#pragma once

#include "CUDAPathTracer.h"
#include "Viewer.h"
#include "PacketManager.h"

enum RendererState { STANDBY, WAITFORASSIGN, RENDER, RDONE};

class Renderer{
	public:
		Renderer(PacketManager *pMgr_, bool hasViewer_ = false);
		~Renderer();
		std::thread	getThread();
		void step();	//one step of run(), for local testing purpose (run on the main thread to share GPU)
		void run();	

	private:
		//states
		void rendererStandby();
		void initRenderer();
		void rendering();
		void renderDone();
		void sendPixel();

		void cleanup();
		void setViewerIPPort(std::string ip_, unsigned int port_);
		void setLeaderIPPort(std::string ip_, unsigned int port_);
		void setCamera(float theta, float phi, glm::vec3 cammove);

		Scene* scn;
		PacketManager* pMgr;
		CUDAPathTracer* cudaEngine;
		Viewer* view;

		bool hasViewer;

		RendererState state;

		std::string viewerIP;
		unsigned int viewerPort;
		std::string leaderIP;
		unsigned int leaderPort;

		int iteration;
		int assigned_no;
		int no_renderer;
		int sendIteration;
};