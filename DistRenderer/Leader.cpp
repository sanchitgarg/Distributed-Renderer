#include "Leader.h"


Leader::Leader(PacketManager *pMgr_){
	pMgr = pMgr_;

	viewerIP = "";
	viewerPort = 0;
	leaderIP = "";
	leaderPort = 0;

	state = LeaderState::WAITFORINIT;
}

std::thread Leader::getThread(){
	return std::thread(&Leader::run, this);
}

void Leader::step(){
	if (state == LeaderState::WAITFORINIT){
		leaderStandby();
	}
	else if (state == LeaderState::RECVSCENE){
		recvSceneFiles();
	}
	else if (state == LeaderState::START){
		initRenderers();
	}
	else if (state == LeaderState::MONITOR){
		monitorRenderers();
	}
}

void Leader::run(){
	while (true){
		step();
	}
}

void Leader::addRendererIPPort(std::string ip_, unsigned int port_){
	if (rendererMap.count({ ip_, port_ }) == 0){
		rendererMap.insert({ ip_, port_ });
		return;
	}

	std::cout << "addRendererIPPort " << ip_ << ":" << port_ <<
		"error. This set of ip:port already exists." << std::endl;
	exit(EXIT_FAILURE);
}

void Leader::setViewerIPPort(std::string ip_, unsigned int port_){
	viewerIP = ip_;
	viewerPort = port_;
}


void Leader::leaderStandby(){
	Packet* pkt;
	if (pkt = pMgr->getPacket()){
		if (pkt->get_type() == PacketType::INIT){
			std::cout << "Got INIT from the viewer." << std::endl;

			Message::INIT* imsg = pkt->get_init();
			setViewerIPPort(imsg->viewer_ip(), imsg->viewer_port());
			sendIteration = imsg->iteration();

			//TODO: for now, we will just assume that the leader
			//can only handle one rendering at a time.
			//so we will only send back a green light.
			//(No need to deal with the case that there are renderer that joins mid-way.

			Message::ACK *amsg = new Message::ACK();
			amsg->set_ack_code(1);
			pMgr->push(amsg);
			bool sent = pMgr->sendPackets(viewerIP, viewerPort);

			if (!sent){
				std::cout << "[leaderStandby] Try to contact back the viewer, but can't reach it." << std::endl;
				exit(EXIT_FAILURE);
			}
			else{
				std::cout << "[leaderStandby] Give a green light to the viewer, now waiting for scene files." << std::endl;
				state = LeaderState::RECVSCENE;
			}
			return;
		}
		delete pkt;
	}
}


void Leader::recvSceneFiles(){
	//receive scene info.

	Packet* pkt;
	if (pkt = pMgr->getPacket()){
		//This leader is in the middle of something, send back RED LIGHT
		if (pkt->get_type() == PacketType::INIT){
			Message::ACK *amsg = new Message::ACK();
			amsg->set_ack_code(2);
			pMgr->push(amsg);
			bool sent = pMgr->sendPackets(viewerIP, viewerPort);
		}
		else if (pkt->get_type() == PacketType::FILE_DATA){
			std::cout << "Got FILE_DATA from the viewer." << std::endl;

			//forward the scene to every renderers.
			Message::FILE_DATA *fmsg = pkt->get_fileData();
			std::string orig_dir = fmsg->targetdir();
			std::vector<std::string> fileList;
			for (int i = 0; i < fmsg->filename_size(); i++){
				fileList.push_back(fmsg->filename(i));
			}

			int count = 0;
			for (auto i = rendererMap.begin(); i != rendererMap.end(); i++){
				//start sending (including self!)
				std::string target_dir = fmsg->targetdir() + "/" + std::to_string(count);
				pMgr->push(orig_dir, target_dir, fileList);
				bool sent = pMgr->sendPackets(i->first, i->second);
				if (sent){
					activeRenderer[count] = { i->first, i->second };
					count++;
				}3
				else{
					std::cout << "[recvSceneFiles] can't reach this node: " <<
						i->first << ":" << i->second << std::endl;
				}
			}

			std::cout << "[recvSceneFiles] Distributed scene files to "
				<< count << "renderer nodes" << std::endl;

			//change mode to START
			state = LeaderState::START;
		}

		delete pkt;
	}
}


//assign job to each renders
void Leader::initRenderers(){
	//assign job to each renders
	for (int i = 0; i < activeRenderer.size(); i++){
		Message::START_RENDER *minit = new Message::START_RENDER();

		minit->set_viewer_ip(viewerIP);
		minit->set_viewer_port(viewerPort);
		minit->set_leader_ip(pMgr->getIP());
		minit->set_leader_port(pMgr->getPort());
		minit->set_assigned_no(i);
		minit->set_no_renderer(activeRenderer.size());
		minit->set_iteration(sendIteration);

		pMgr->push(minit);
		IP_Port target = activeRenderer[i];
		bool sent = pMgr->sendPackets(target.first, target.second);

		if (!sent){
			//TODO: come up with any error handling scheme.
			//(i.e. the node goes down after receiving scene files.
			std::cout << "[initRenderers] can't reach this node: " <<
				target.first << ":" << target.second << std::endl;
		}
	}

	state = LeaderState::MONITOR;
	finishedRendering = 0;
}


void Leader::monitorRenderers(){
	Packet* pkt;
	if (pkt = pMgr->getPacket()){
		//check if any INIT msg reaches the leader -> reply back with RED LIGHT.
		if (pkt->get_type() == PacketType::INIT){
			Message::ACK *amsg = new Message::ACK();
			amsg->set_ack_code(2);
			pMgr->push(amsg);
			bool sent = pMgr->sendPackets(viewerIP, viewerPort);
		}
		//check if the camera is changed.
		else if (pkt->get_type() == PacketType::CAM_MOVE){
			finishedRendering = 0;
			Message::CAM_MOVE *c_orig = pkt->get_cam_move();
			for (int i = 0; i < activeRenderer.size(); i++){
				Message::CAM_MOVE *cinit = new Message::CAM_MOVE();

				cinit->set_theta(c_orig->theta());
				cinit->set_phi(c_orig->phi());

				cinit->set_cammove_x(c_orig->cammove_x());
				cinit->set_cammove_y(c_orig->cammove_y());
				cinit->set_cammove_z(c_orig->cammove_z());

				pMgr->push(cinit);
				IP_Port target = activeRenderer[i];
				bool sent = pMgr->sendPackets(target.first, target.second);

				if (!sent){
					//TODO: come up with any error handling scheme.
					//(i.e. the node goes down after receiving scene files.
					std::cout << "[monitorRenderers] can't reach this node: " <<
						target.first << ":" << target.second << std::endl;
				}
			}
		}
		//check if any renderer sends a message saying.... IM DONE!
		else if (pkt->get_type() == PacketType::DONE){
			finishedRendering++;

			//check if the rendering is done.
			if (finishedRendering == activeRenderer.size()){
				std::cout << "[Leader] rendering done." << std::endl;

				Message::DONE *dmsg = new Message::DONE();
				pMgr->push(dmsg);
				pMgr->sendPackets(viewerIP, viewerPort);
			}

		}

		//TODO: check if any renderer sends a message saying.... 
		//I LOST CONTACT WITH THE VIEWER HENCE THIS RENDER SHOULD BE HALTED.
		delete pkt;
	}
}