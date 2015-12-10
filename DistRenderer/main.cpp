#include <glm/gtx/transform.hpp>

#include "Renderer.h"
#include "Leader.h"
#include "Viewer.h"
#include "FrontEnd.h"

#define NO_DUMMYRENDERER 1
#define DUMMYRECVPORT 12345

#define debug true

void debugFunc(const char *dirPath, const char* listFile);

int main(int argc, char** argv) {
	if (argc != 3) {
		std::cout << "Argument List:" << std::endl;
		std::cout << "1: The directory containing the scene file" << std::endl;
		std::cout << "2: The text file containing all the file associated" << 
			"with the target scene, the first file in the list is " <<
			"the SCENEFILE.txt itself." << std::endl;

		return 1;
	}

	initWinSock();

	if (debug)
		debugFunc(argv[1], argv[2]);

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
	printInfo(getSelfIP(), DUMMYRECVPORT);
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