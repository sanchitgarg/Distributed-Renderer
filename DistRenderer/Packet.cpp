#include "Packet.h"

Packet::Packet(){
	type = PacketType::EMPTY;

	init_msg = nullptr;
	ack_msg = nullptr;
	startRender_msg = nullptr;
	pixel_msg = nullptr;
	done_msg = nullptr;
	halt_msg = nullptr;
	file_msg = nullptr;
}

Packet::~Packet(){
	delete init_msg;
	delete ack_msg;
	delete startRender_msg;
	delete pixel_msg;
	delete done_msg;
	delete halt_msg;
	delete file_msg;
}

PacketType Packet::get_type(){
	return type;
}

Message::INIT* Packet::get_init(){
	if (type == PacketType::INIT){
		return init_msg;
	}
	
	return nullptr;
}

Message::ACK* Packet::get_ack(){
	if (type == PacketType::ACK){
		return ack_msg;
	}

	return nullptr;
}

Message::START_RENDER* Packet::get_start_render(){
	if (type == PacketType::START_RENDER){
		return startRender_msg;
	}

	return nullptr;
}

Message::PIXEL* Packet::get_pixel(){
	if (type == PacketType::PIXEL){
		return pixel_msg;
	}

	return nullptr;
}

Message::DONE* Packet::get_done(){
	if (type == PacketType::DONE){
		return done_msg;
	}

	return nullptr;
}

Message::HALT* Packet::get_halt(){
	if (type == PacketType::HALT){
		return halt_msg;
	}

	return nullptr;
}

Message::CAM_MOVE* Packet::get_cam_move(){
	if (type == PacketType::CAM_MOVE){
		return cam_msg;
	}

	return nullptr;
}

Message::FILE_DATA* Packet::get_fileData(){
	if (type == PacketType::FILE_DATA){
		return file_msg;
	}

	return nullptr;
}

void Packet::set_init(Message::INIT *msg){
	type = PacketType::INIT;
	init_msg = msg;
}

void Packet::set_ack(Message::ACK *msg){
	type = PacketType::ACK;
	ack_msg = msg;
}

void Packet::set_start_render(Message::START_RENDER *msg){
	type = PacketType::START_RENDER;
	startRender_msg = msg;
}

void Packet::set_pixel(Message::PIXEL *msg){
	type = PacketType::PIXEL;
	pixel_msg = msg;
}

void Packet::set_done(Message::DONE *msg){
	type = PacketType::DONE;
	done_msg = msg;
}

void Packet::set_halt(Message::HALT *msg){
	type = PacketType::HALT;
	halt_msg = msg;
}

void Packet::set_cam_move(Message::CAM_MOVE *msg){
	type = PacketType::CAM_MOVE;
	cam_msg = msg;
}

void Packet::set_fileData(std::string dir, std::vector<std::string> fileName){
	type = PacketType::FILE_DATA;
	file_msg = new Message::FILE_DATA();
	file_msg->set_dirpath(dir);
	for (int i = 0; i < fileName.size(); i++)
	{
		std::ifstream in(dir + "/" + fileName[i]);
		in.seekg(0, std::ios::end);
		int fileSize = in.tellg();
		in.close();

		if (fileSize == 0){
			std::cout << "[Packet::set_fileData] This file has 0 byte size:" << fileName[i] << std::endl;
			continue;
		}

		file_msg->add_filename(fileName[i]);
		file_msg->add_filesize(fileSize);
	}
}

bool Packet::serializePacket(std::vector<char> &buf){
	if (type == PacketType::EMPTY) return false;

	bool parseOK = false;

	buf.clear();
	int len = 0;

	if(type == PacketType::INIT){
		len = init_msg->ByteSize();
		buf.resize(PACKET_HEADER_SIZE + len);
		parseOK = len == 0 || init_msg->SerializeToArray(&buf[PACKET_HEADER_SIZE], len);
	}
	else if (type == PacketType::ACK){
		len = ack_msg->ByteSize();
		buf.resize(PACKET_HEADER_SIZE + len);
		parseOK = len == 0 || ack_msg->SerializeToArray(&buf[PACKET_HEADER_SIZE], len);
	}
	else if (type == PacketType::START_RENDER){
		len = startRender_msg->ByteSize();
		buf.resize(PACKET_HEADER_SIZE + len);
		parseOK = len == 0 || startRender_msg->SerializeToArray(&buf[PACKET_HEADER_SIZE], len);
	}
	else if (type == PacketType::PIXEL){
		len = pixel_msg->ByteSize();
		buf.resize(PACKET_HEADER_SIZE + len);
		parseOK = len == 0 || pixel_msg->SerializeToArray(&buf[PACKET_HEADER_SIZE], len);
	}
	else if (type == PacketType::DONE){
		len = done_msg->ByteSize();
		buf.resize(PACKET_HEADER_SIZE + len);
		parseOK = len == 0 || done_msg->SerializeToArray(&buf[PACKET_HEADER_SIZE], len);
	}
	else if (type == PacketType::HALT){
		len = halt_msg->ByteSize();
		buf.resize(PACKET_HEADER_SIZE + len);
		parseOK = len == 0 || halt_msg->SerializeToArray(&buf[PACKET_HEADER_SIZE], len);
	}
	else if (type == PacketType::CAM_MOVE){
		len = cam_msg->ByteSize();
		buf.resize(PACKET_HEADER_SIZE + len);
		parseOK = len == 0 || cam_msg->SerializeToArray(&buf[PACKET_HEADER_SIZE], len);
	}
	else if (type == PacketType::FILE_DATA){
		len = file_msg->ByteSize();

		//TODO: parse file to the aaaarrray!
		int totalOffset = 0;
		for (int i = 0; i < file_msg->filename_size(); i++)
			totalOffset += file_msg->filesize(i);

		buf.resize(PACKET_HEADER_SIZE + len + totalOffset);
		parseOK = len == 0 || file_msg->SerializeToArray(&buf[PACKET_HEADER_SIZE], len);

		int offset = PACKET_HEADER_SIZE + len;
		for (int i = 0; i < file_msg->filename_size(); i++)
		{
			std::string fileName = file_msg->filename(i);
			std::string dir = file_msg->dirpath();
			int fSize = file_msg->filesize(i);

			if (fSize == 0){
				std::cout << "[Packet::serializePacket] This file has 0 byte size:" << fileName << std::endl;
				continue;
			}

			std::ifstream file(dir + "/" + fileName);
			if (file.is_open())
			{
				file.read(&buf[offset], fSize);
				file.close();
			}
			else
				std::cout << "[Packet::serializePacket] Unable to open file:" << (dir + "/" + fileName) << std::endl;

			offset += fSize;
		}
	}

	if (parseOK){
		//packing msgType
		buf[0] = type;	//char: 0-255

		//packing the size of the packet
		int tLen = len;
		for (int i = sizeof(int) - 1; i >= 0; i--){
			buf[i + 1] = unsigned char(tLen);

			if (i != 0)
				tLen >>= 8;
		}

		len += PACKET_HEADER_SIZE;
	}

	return parseOK;
}

bool Packet::parseFromArray(PacketType type_, 
	std::vector<char> &buf, int len)
{
	bool resultOK = false;

	if (type_ == PacketType::INIT){
		if (init_msg == nullptr)
			init_msg = new Message::INIT();
		resultOK = len == 0 || init_msg->ParseFromArray(&buf[0], len);
	}
	else if(type_ == PacketType::ACK){
		if (ack_msg == nullptr)
			ack_msg = new Message::ACK();
		resultOK = len == 0 || ack_msg->ParseFromArray(&buf[0], len);
	}
	else if(type_ == PacketType::START_RENDER){
		if (startRender_msg == nullptr)
			startRender_msg = new Message::START_RENDER();
		resultOK = len == 0 || startRender_msg->ParseFromArray(&buf[0], len);
	}
	else if(type_ == PacketType::PIXEL){
		if (pixel_msg == nullptr)
			pixel_msg = new Message::PIXEL();
		resultOK = len == 0 || pixel_msg->ParseFromArray(&buf[0], len);
	}
	else if(type_ == PacketType::DONE){
		if (done_msg == nullptr)
			done_msg = new Message::DONE();
		resultOK = len == 0 || done_msg->ParseFromArray(&buf[0], len);
	}
	else if(type_ == PacketType::HALT){
		if (halt_msg == nullptr)
			halt_msg = new Message::HALT();
		resultOK = len == 0 || halt_msg->ParseFromArray(&buf[0], len);
	}
	else if(type_ == PacketType::CAM_MOVE){
		if (cam_msg == nullptr)
			cam_msg = new Message::CAM_MOVE();
		resultOK = len == 0 || cam_msg->ParseFromArray(&buf[0], len);
	}
	else if (type_ == PacketType::FILE_DATA){
		if (file_msg == nullptr)
			file_msg = new Message::FILE_DATA();
		resultOK = len == 0 || file_msg->ParseFromArray(&buf[0], len);
	}

	if (resultOK)
		type = type_;

	return resultOK;
}