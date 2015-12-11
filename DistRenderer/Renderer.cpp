#include "Renderer.h"

Renderer::Renderer(PacketManager *pMgr_, bool hasViewer_){
	pMgr = pMgr_;
	cudaEngine = new CUDAPathTracer();

	iteration = 0;
	viewerIP = "";
	viewerPort = 0;
	leaderIP = "";
	leaderPort = 0;

	state = RendererState::STANDBY;

	view = nullptr;
	hasViewer = hasViewer_;
}

Renderer::~Renderer(){
	cleanup();
}

void Renderer::cleanup(){
	delete scn;
	delete cudaEngine;
	delete view;

	iteration = 0;
	viewerIP = "";
	viewerPort = 0;
	leaderIP = "";
	leaderPort = 0;
}

std::thread Renderer::getThread(){
	return std::thread(&Renderer::run, this);
}

//TODO
void Renderer::step(){
	if (state == RendererState::STANDBY){
		rendererStandby();
	}
	else if (state == RendererState::WAITFORASSIGN){
		initRenderer();
	}
	else if (state == RendererState::RENDER){
		rendering();
	}
	else if (state == RendererState::RDONE){
		renderDone();
	}
}

void Renderer::run(){
	while (true){
		step();
	}
}

void Renderer::setViewerIPPort(std::string ip_, unsigned int port_){
	viewerIP = ip_;
	viewerPort = port_;
}

void Renderer::setLeaderIPPort(std::string ip_, unsigned int port_){
	leaderIP = ip_;
	leaderPort = port_;
}

void Renderer::setCamera(float theta, float phi, glm::vec3 cammove){
	if (state == RendererState::RENDER){
		cudaEngine->setCamera(theta, phi, cammove);
		iteration = 0;
		cudaEngine->pathtraceFree();
		cudaEngine->pathtraceInit(scn, assigned_no, no_renderer);
	}
}

void Renderer::rendererStandby(){
	Packet* pkt;
	if (pkt = pMgr->getPacket()){
		if (pkt->get_type() == PacketType::FILE_DATA){
			Message::FILE_DATA* fmsg = pkt->get_fileData();

			//initializing the renderer
			scn = new Scene(fmsg->dirpath() + '/' + fmsg->filename(0));
			cudaEngine = new CUDAPathTracer();

			state = RendererState::WAITFORASSIGN;
		}
		delete pkt;
	}
}
void Renderer::initRenderer(){

	Packet* pkt;
	if (pkt = pMgr->getPacket()){
		if (pkt->get_type() == PacketType::START_RENDER){
			Message::START_RENDER* smsg = pkt->get_start_render();
			
			sendIteration = smsg->iteration();
			assigned_no = smsg->assigned_no();
			no_renderer = smsg->no_renderer();

			setViewerIPPort(smsg->viewer_ip(), smsg->viewer_port());
			setLeaderIPPort(smsg->leader_ip(), smsg->leader_port());

			cudaEngine->pathtraceInit(scn, assigned_no, no_renderer);

			if (hasViewer)
				view = new Viewer(scn);

			state = RendererState::RENDER;
		}

		delete pkt;
	}
}
void Renderer::rendering(){
	Packet* pkt;
	if (pkt = pMgr->getPacket()){
		if (pkt->get_type() == PacketType::CAM_MOVE){
			Message::CAM_MOVE *cmsg = pkt->get_cam_move();
			setCamera(cmsg->theta(), cmsg->phi(),
				glm::vec3(cmsg->cammove_x(), cmsg->cammove_y(), cmsg->cammove_z()));
		}

		delete pkt;
	}

	if (cudaEngine->isActive() &&
		iteration < scn->state.iterations){
		if (view != nullptr){
			uchar4 *pbo_dptr = NULL;
			cudaGLMapBufferObject((void**)&pbo_dptr, view->getPBO());
			cudaEngine->pathtrace(pbo_dptr, iteration);
			cudaGLUnmapBufferObject(view->getPBO());
			view->update(iteration);
		}
		else{
			cudaEngine->pathtrace(nullptr, iteration);
		}

		if (iteration != 0 && iteration % sendIteration == 0){
			sendPixel();

			//TODO: check if IT LOST CONTACT WITH THE VIEWER HENCE THIS RENDER SHOULD BE HALTED.
		}

		std::cout << "Iteration: " << iteration << " rendered." << std::endl;
		iteration++;
	}
	else{
		std::cout << "Rendering Finished" << std::endl;
		state == RendererState::RDONE;

		cudaEngine->saveImage("Now", iteration);

		Message::DONE *dmsg = new Message::DONE();
		dmsg->set_assigned_no(assigned_no);
		pMgr->push(dmsg);
		pMgr->sendPackets(leaderIP, leaderPort);
	}
}

void Renderer::renderDone(){
	Packet* pkt;
	if (pkt = pMgr->getPacket()){
		if (pkt->get_type() == PacketType::CAM_MOVE){
			state = RendererState::RENDER;
			Message::CAM_MOVE *cmsg = pkt->get_cam_move();
			setCamera(cmsg->theta(), cmsg->phi(),
				glm::vec3(cmsg->cammove_x(), cmsg->cammove_y(), cmsg->cammove_z()));
		}

		delete pkt;
	}
}

void Renderer::sendPixel(){
	if (viewerIP == "") return;
	int offset = assigned_no;

	const std::vector<glm::vec3> pixels = cudaEngine->getPixels();

	int numPackets = glm::ceil(pixels.size() / (float)PIXEL_PER_MSG);

	for (int i = 0; i < numPackets; i++){
		Message::PIXEL* p = new Message::PIXEL();
		p->set_firstpixelptr(offset);
		p->set_pixeloffset(no_renderer);

		for (int ptr = 0; ptr < PIXEL_PER_MSG; ptr++){
			int px = ptr + i * PIXEL_PER_MSG;

			if (px >= pixels.size()){
				pMgr->push(p);
				pMgr->sendPackets(viewerIP, viewerPort);
				return;
			}

			glm::vec3 pix = pixels[px];
			Message::Color* c = p->add_color();
			c->set_r(glm::clamp((int)(pix.x / iteration * 255.0), 0, 255));
			c->set_g(glm::clamp((int)(pix.y / iteration * 255.0), 0, 255));
			c->set_b(glm::clamp((int)(pix.z / iteration * 255.0), 0, 255));
		}

		offset += PIXEL_PER_MSG * no_renderer;

		pMgr->push(p);
	}

	pMgr->sendPackets(viewerIP, viewerPort);

	//std::cout << viewerPort << std::endl;
}