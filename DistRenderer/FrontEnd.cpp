#include "FrontEnd.h"

FrontEnd::FrontEnd(PacketManager* pMgr_, 
	std::string dirPath_, std::string listTxt_){
	pMgr = pMgr_;
	dirPath = dirPath_;
	listTxt = listTxt_;
	state = FrontEndState::IDLE;
	leaderIP = "";
	leaderPort = 0;
	display = new GLDisplay();
}

FrontEnd::~FrontEnd(){
	delete display;
}

std::thread FrontEnd::getThread(){
	return std::thread(&FrontEnd::run, this);
}

void FrontEnd::setLeaderIPPort(std::string ip_, unsigned int port_){
	leaderIP = ip_;
	leaderPort = port_;
}

void FrontEnd::step(){
	if (state == FrontEndState::IDLE){
		if (leaderIP != "")
			checkRendererAvailability();
	}
	else if (state == FrontEndState::WAITFORGREENLIGHT){
		waitForLeader();
	}
	else if (state == FrontEndState::SENDSCENE){
		sendSceneInfo();
	}
	else if (state == FrontEndState::READY){
		fetchPixels();
		display->update();
		display->draw();
	}
}

void FrontEnd::run(){
	while (true){
		step();
	}
}

// try sending out REQ and wait for a response.
void FrontEnd::checkRendererAvailability(){
	Message::INIT *imsg = new Message::INIT();
	imsg->set_viewer_ip(getSelfIP());
	imsg->set_viewer_port(pMgr->getPort());

	pMgr->push(imsg);
	bool sent = pMgr->sendPackets(leaderIP, leaderPort);

	if (!sent){
		std::cout << "[checkRendererAvailability] Can't reach the leader.... The program is quitting" << std::endl;
		exit(EXIT_FAILURE);
	}
	else{
		std::cout << "[checkRendererAvailability] send successfully." << std::endl;
		this->state = FrontEndState::WAITFORGREENLIGHT;
	}
}

//if the leader is available, it will return an OK signal
//if not, try again
void FrontEnd::waitForLeader(){
	Packet* pkt = nullptr;
	while (pkt = pMgr->getPacket()){
		if (pkt->get_type() == PacketType::ACK){
			Message::ACK* amsg = pkt->get_ack();

			if (amsg->ack_code() == 1){ // GREEN LIGHT
				std::cout << "[checkRendererAvailability] There are resources available! Start sending scenes!" << std::endl;
				this->state = FrontEndState::SENDSCENE;
			}
			else if (amsg->ack_code() == 2){ // RED LIGHT
				std::cout << "[checkRendererAvailability] No resource is available. Try again later." << std::endl;
				this->state = FrontEndState::IDLE;
			}
		}
	}
}

void FrontEnd::sendSceneInfo(){
	//get the list of the scene and all the files
	std::vector<std::string> fileList;

	std::string line;
	std::ifstream  myfile(dirPath + "/" + listTxt);
	if (myfile.is_open())
	{
		while (getline(myfile, line))
		{
			//pack file and send
			//the first one in the file should be the scene file!
			fileList.push_back(line);
		}
		myfile.close();

	}

	pMgr->push(dirPath, fileList);
	bool sent = pMgr->sendPackets(leaderIP, leaderPort);

	if (!sent){
		std::cout << "[sendSceneInfo] Can't reach the leader.... The program is quitting" << std::endl;
		exit(EXIT_FAILURE);
	}
	else{
		std::cout << "[sendSceneInfo] Scenes sent." << std::endl;
		this->state = FrontEndState::READY;
	}
}

void FrontEnd::fetchPixels(){
	if (display == nullptr) return;

	Packet* pkt = nullptr;
	Message::PIXEL* px;

	while (pkt = pMgr->getPacket()){
		px = pkt->get_pixel();
		int offset = px->firstpixelptr();

		for (int i = 0; i < px->color_size(); i++){
			const Message::Color clr = px->color(i);

			int y = offset / WIDTH;
			int x = offset - (y * WIDTH);

			display->setPixelColor(x, y, clr.r(), clr.g(), clr.b());

			offset += px->pixeloffset();
		}

		delete pkt;
	}

}