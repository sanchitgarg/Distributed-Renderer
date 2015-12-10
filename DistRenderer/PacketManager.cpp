#include "PacketManager.h"

PacketManager::PacketManager(uint32_t port_){
	ip = getSelfIP();
	port = port_;

	//init and bind the receiving socket
	sRecv = socket(IPFAMILY, SOCK_STREAM, IPPROTO_TCP);
	if (sRecv == INVALID_SOCKET){
		std::cout << "Create recv socket at port " << port << " failed: " << WSAGetLastError() << std::endl;
		exit(EXIT_FAILURE);
	}

	struct sockaddr_in recv;
	recv.sin_family = IPFAMILY;
	recv.sin_addr.S_un.S_addr = INADDR_ANY;
	recv.sin_port = htons(port);
	int bindResult = bind(sRecv, (struct sockaddr*)&recv, sizeof(recv));
	if (bindResult == SOCKET_ERROR){
		std::cout << "Binding port " << port << " failed: " + WSAGetLastError() << std::endl;
		exit(EXIT_FAILURE);
	}
}

PacketManager::~PacketManager(){
	closesocket(sRecv);
}

std::thread PacketManager::getThread(){
	return std::thread(&PacketManager::run, this);
}

Packet* PacketManager::getPacket(){
	if (qRecv.size() == 0)
		return nullptr;

	Packet* pOut;
	mutexRecv.lock();
	{
		pOut = qRecv.front();
		qRecv.pop();
	}
	mutexRecv.unlock();

	return pOut;
}

void PacketManager::push(Message::INIT* msg){
	Packet* p = new Packet();
	p->set_init(msg);
	qSend.push(p);
}

void PacketManager::push(Message::ACK *msg){
	Packet* p = new Packet();
	p->set_ack(msg);
	qSend.push(p);
}

void PacketManager::push(Message::START_RENDER *msg){
	Packet* p = new Packet();
	p->set_start_render(msg);
	qSend.push(p);
}

void PacketManager::push(Message::PIXEL *msg){
	Packet* p = new Packet();
	p->set_pixel (msg);
	qSend.push(p);
}
void PacketManager::push(Message::DONE *msg){
	Packet* p = new Packet();
	p->set_done(msg);
	qSend.push(p);
}

void PacketManager::push(Message::HALT *msg){
	Packet* p = new Packet();
	p->set_halt(msg);
	qSend.push(p);
}

void PacketManager::push(Message::CAM_MOVE *msg){
	Packet* p = new Packet();
	p->set_cam_move(msg);
	qSend.push(p);
}

void PacketManager::push(std::string dir, std::vector<std::string> filename){
	Packet* p = new Packet();
	p->set_fileData(dir, filename);
	qSend.push(p);
}

//send all of the packets in the queue through TCP.
bool PacketManager::sendPackets(const std::string ip, const uint32_t port){

	std::vector<char> buffer;

	//0. init and bind the sending socket
	SOCKET sSend = socket(IPFAMILY, SOCK_STREAM, IPPROTO_TCP);
	if (sSend == INVALID_SOCKET){
		std::cout << "Create send socket failed: " << WSAGetLastError() << std::endl;
		exit(EXIT_FAILURE);
	}

	// 1. Connect to the receiver.
	sockaddr_in saddr = createSOCKADDR(ip, port);
	int retryConnect = 0;
	while (connect(sSend, (sockaddr*)&saddr, sizeof(saddr)) == SOCKET_ERROR){
		retryConnect++;
		if (retryConnect == RETRY_ATTEMPT){
			std::cout << "Unable to connect to " << ip << ":" << port << " " << WSAGetLastError() << std::endl;
			return false;
		}
	}

	// 2. Send all the packets in the queue.
	int bufLen = BUFLEN;
	while (qSend.size() != 0){
		Packet* next = qSend.front();
		next->serializePacket(buffer);

		int retrySend = 0;
		while (send(sSend, &buffer[0], buffer.size(), 0) == SOCKET_ERROR){
			retrySend++;

			if (retrySend == RETRY_ATTEMPT){
				std::cout << "Cannot connect to " << ip << ":" << port << ":" << WSAGetLastError() << std::endl;
				return false;
			}
		}

		delete next;
		qSend.pop();
	}

	// 3. Disconnect.
	shutdown(sSend, SD_SEND);
	closesocket(sSend);

	return true;
}

void PacketManager::run(){

	std::vector<char> buffer;
	Packet* packet = new Packet();

	while(true) {
		// 1. Lister for a connection
		listen(sRecv, MAXCONN);

		// 2. Accept a connection
		SOCKET sd = accept(sRecv, nullptr, nullptr);

		// 3. Receive data until the sender shuts down the connection.
		int bufLen = 0;
		bool stillConnect = true;
		while(stillConnect){
			//get type
			stillConnect = readStreamToBuffer(sd, buffer, sizeof(char));
			if (stillConnect){
				PacketType pType = (PacketType)buffer[0];

				//get size
				readStreamToBuffer(sd, buffer, sizeof(int));
				int pSize = 0;
				for (int i = 0; i < sizeof(int); i++){
					pSize |= unsigned char(buffer[i]);
					if (i != sizeof(int) - 1)
						pSize <<= 8;
				}

				//read data
				if (readStreamToBuffer(sd, buffer, pSize)){
					if (packet->parseFromArray(pType, buffer, pSize)){

						mutexRecv.lock();
						{
							qRecv.push(packet);
						}
						mutexRecv.unlock();
					}

					if (pType == PacketType::FILE_DATA){
						//read the file and save it!
						Message::FILE_DATA *fData = packet->get_fileData();
						std::string dir = fData->dirpath();
						mkdir((dir + "/" + std::to_string(port)).c_str());
						mkdir((dir + "/" + std::to_string(port) + "/objs").c_str());

						for (int i = 0; i < fData->filename_size(); i++)
						{
							std::string fileName = fData->filename(i);
							pSize = fData->filesize(i);

							//SAVE FILE YAY!
							if (readStreamToBuffer(sd, buffer, pSize)){
								std::string path = dir + "/" + std::to_string(port) + "/" + fileName;
								std::ofstream file(path, std::ios::out | std::fstream::trunc);
								if (file.is_open())
								{
									for (int j = 0; j < pSize; j++){
										file << buffer[j];
									}
									file.close();
								}
								else
									std::cout << "[PacketManager::run] Unable to create file:" << path << std::endl;
							}
							else{
								//TODO: error handling -> connection lost
							}
						}
					}

					packet = new Packet();
				}
				else{
					//TODO: error handling -> connection lost
				}
			}
		} 

		// 4. Close a connection
		shutdown(sd, SD_RECEIVE);
		closesocket(sd);
	}
}

std::string PacketManager::getIP(){
	return ip;
}

unsigned int PacketManager::getPort(){
	return port;
}


sockaddr_in PacketManager::createSOCKADDR(std::string ip, uint32_t port){
	sockaddr_in saddr;
	saddr.sin_family = IPFAMILY;
	saddr.sin_port = htons(port);

	if (inet_pton(IPFAMILY, ip.c_str(), &saddr.sin_addr) == 0){
		std::cout << "Not a valid IP address." << std::endl;
		exit(EXIT_FAILURE);
	}

	return saddr;
}


bool PacketManager::readStreamToBuffer(SOCKET sd, 
	std::vector<char> &buf, int targetSize)
{
	int delta = 0;
	int toBeRead = targetSize;

	buf.resize(targetSize);

	while (toBeRead > 0){
		delta = recv(sd, &buf[targetSize - toBeRead], toBeRead, 0);

		//the sender terminates the connection
		if (delta <= 0)
			return false; 

		toBeRead -= delta;
	}

	return true;
}