#pragma once

#include "Helper.h"
#include "msg.pb.h"

enum PacketType {
	EMPTY = 0,
	INIT,
	ACK,
	START_RENDER,
	PIXEL,
	FILE_DATA
};

class Packet{
	public:
		Packet();
		~Packet();
		
		PacketType get_type();
		Message::INIT* get_init();
		Message::ACK* get_ack();
		Message::START_RENDER* get_start_render();
		Message::PIXEL* get_pixel();

		void set_init(Message::INIT *msg);
		void set_ack(Message::ACK *msg);
		void set_start_render(Message::START_RENDER *msg);
		void set_pixel(Message::PIXEL *msg);

		bool serializePacket(char* buf, int& len);
		bool parseFromArray(PacketType type_, char* buf, int& len);

	private:
		PacketType type;
		Message::INIT *init_msg;
		Message::ACK *ack_msg;
		Message::START_RENDER *startRender_msg;
		Message::PIXEL *pixel_msg;

};