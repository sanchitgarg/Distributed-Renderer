#include <glm/gtx/transform.hpp>

#include "Renderer.h"
#include "Leader.h"
#include "Viewer.h"
#include "FrontEnd.h"

#define NO_DUMMYRENDERER 4
#define DUMMYRECVPORT 12345

void debugFunc(const char *dirPath, const char* listFile);

int main(int argc, char** argv) {
	if (argc < 2) {
		std::cout << "Argument List:" << std::endl;
		std::cout << "1: Mode ('f' = frontend, 'l' = leader + renderer, 'r' = renderer only'" << std::endl;
		return 1;
	}

	initWinSock();
	printInfo(getSelfIP(), VIEW_RECVPORT);

	bool debug = true;
	if (debug){
		debugFunc(argv[2], argv[3]);
	}
	else{

		if (*argv[1] == 'f'){
			if (argc != 4){
				std::cout << "Argument Needed for FrontEnd Viewer:" << std::endl;
				std::cout << "2: The directory containing the scene file" << std::endl;
				std::cout << "3: The text file containing all the file associated" <<
					"with the target scene, the first file in the list is " <<
					"the SCENEFILE.txt itself." << std::endl;
			}

			PacketManager pMgr(VIEW_RECVPORT);
			FrontEnd fe(&pMgr, argv[2], argv[3]);

			//ask for the leader's IP:Port
			std::string ip;
			uint32_t port;

			std::cout << "Leader's IP: ";
			std::cin >> ip;

			std::cout << "Leader's port: ";
			std::cin >> port;

			fe.setLeaderIPPort(ip, port);

			std::thread pmThread = pMgr.getThread();
			std::thread feThread = fe.getThread();

			pmThread.join();
			feThread.join();
		}
		else if (*argv[1] == 'l'){

			PacketManager pMgrLeader(VIEW_RECVPORT);
			PacketManager pMgrRenderer(VIEW_RECVPORT - 1);
			Leader leader(&pMgrLeader);
			Renderer rend(&pMgrRenderer, true);

			leader.addRendererIPPort(pMgrRenderer.getIP(), pMgrRenderer.getPort());

			int rendNo;
			std::cout << "No of renderers: ";
			std::cin >> rendNo;

			std::string ip;
			uint32_t port;
			for (int i = 0; i < rendNo; i++){
				std::cout << "Renderer # " << rendNo << std::endl;
				std::cout << "IP: ";
				std::cin >> ip;

				std::cout << "port: ";
				std::cin >> port;

				leader.addRendererIPPort(ip, port);
			}

			std::cout << "Waiting for incoming connection... " << std::endl;
			std::thread pmOneThread = pMgrLeader.getThread();
			std::thread pmTwoThread = pMgrRenderer.getThread();
			std::thread lThread = leader.getThread();
			std::thread rThread = rend.getThread();

			pmOneThread.join();
			pmTwoThread.join();
			lThread.join();
			rThread.join();
		}
		else if (*argv[1] == 'r'){
			PacketManager pMgrRenderer(VIEW_RECVPORT);
			Renderer rend(&pMgrRenderer, true);

			std::thread pmThread = pMgrRenderer.getThread();
			std::thread rThread = rend.getThread();

			pmThread.join();
			rThread.join();
		}
	}

	WSACleanup();
	return 0;
}


void debugMainThread(Leader* leader, Renderer** dummy, FrontEnd* fEnd){
	while (true){
		leader->step();
		for (int i = 0; i < NO_DUMMYRENDERER; i++)
			dummy[i]->step();
		fEnd->step();
	}
}
void debugFunc(const char *dirPath, const char* listFile)
{
	std::vector<std::thread> threads;

	PacketManager pMgrLeader(DUMMYRECVPORT - 1);
	threads.push_back(pMgrLeader.getThread());
	Leader leader(&pMgrLeader);

	Renderer* dummy[NO_DUMMYRENDERER];
	for (int i = 0; i < NO_DUMMYRENDERER; i++){
		PacketManager *pMgrTmp = new PacketManager(DUMMYRECVPORT + i);
		threads.push_back(pMgrTmp->getThread());

		leader.addRendererIPPort(pMgrTmp->getIP(), pMgrTmp->getPort());

		dummy[i] = new Renderer(pMgrTmp, false);
	}

	PacketManager pMgr(VIEW_RECVPORT);
	FrontEnd fEnd(&pMgr, dirPath, listFile);
	fEnd.setLeaderIPPort(pMgrLeader.getIP(), pMgrLeader.getPort());

	threads.push_back(pMgr.getThread());
	threads.push_back(std::thread(debugMainThread, &leader, dummy, &fEnd));

	for (int i = 0; i < threads.size(); i++)
		threads[i].join();
}
