#include "Packet.h"

Packet::Packet(){
	type = PacketType::EMPTY;
}

Packet::~Packet(){
	delete init_msg;
	delete ack_msg;
	delete startRender_msg;
	delete pixel_msg;
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

bool Packet::serializePacket(char* buf, int& len){
	if (type == PacketType::EMPTY) return false;

	bool parseOK = false;

	if(type == PacketType::INIT){
		len = init_msg->ByteSize();
		parseOK = init_msg->SerializeToArray(&buf[PACKET_HEADER_SIZE], len);
	}
	else if (type == PacketType::ACK){
		len = ack_msg->ByteSize();
		parseOK = ack_msg->SerializeToArray(&buf[PACKET_HEADER_SIZE], len);
	}
	else if (type == PacketType::START_RENDER){
		len = startRender_msg->ByteSize();
		parseOK = startRender_msg->SerializeToArray(&buf[PACKET_HEADER_SIZE], len);
	}
	else if (type == PacketType::PIXEL){
		len = pixel_msg->ByteSize();
		parseOK = pixel_msg->SerializeToArray(&buf[PACKET_HEADER_SIZE], len);
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

bool Packet::parseFromArray(PacketType type_, char* buf, int& len){
	bool resultOK = false;
	if (type_ == PacketType::INIT){
		if (init_msg == nullptr)
			init_msg = new Message::INIT();
		resultOK = init_msg->ParseFromArray(buf, len);
	}
	if (type_ == PacketType::ACK){
		if (ack_msg == nullptr)
			ack_msg = new Message::ACK();
		resultOK = ack_msg->ParseFromArray(buf, len);
	}
	if (type_ == PacketType::START_RENDER){
		if (startRender_msg == nullptr)
			startRender_msg = new Message::START_RENDER();
		resultOK = startRender_msg->ParseFromArray(buf, len);
	}
	if (type_ == PacketType::PIXEL){
		if (pixel_msg == nullptr)
			pixel_msg = new Message::PIXEL();
		resultOK = pixel_msg->ParseFromArray(buf, len);
	}

	if (resultOK)
		type = type_;

	return resultOK;
}