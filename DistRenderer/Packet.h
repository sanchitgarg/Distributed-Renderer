#pragma once

#include "Helper.h"
#include "msg.pb.h"
#include <fstream>


enum PacketType {
	EMPTY = 0,
	INIT,
	ACK,
	START_RENDER,
	PIXEL,
	FILE_DATA,
	DONE,
	CAM_MOVE,
	HALT
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
		Message::DONE* get_done();
		Message::HALT* get_halt();
		Message::CAM_MOVE* get_cam_move();
		Message::FILE_DATA* get_fileData();

		void set_init(Message::INIT *msg);
		void set_ack(Message::ACK *msg);
		void set_start_render(Message::START_RENDER *msg);
		void set_pixel(Message::PIXEL *msg);
		void set_done(Message::DONE *msg);
		void set_halt(Message::HALT *msg);
		void set_cam_move(Message::CAM_MOVE *msg);
		void set_fileData(std::string orig_dir, std::string target_dir,
			std::vector<std::string> filename);

		bool serializePacket(std::vector<char> &buf);
		bool parseFromArray(PacketType type_, 
			std::vector<char> &buf, int len);

	private:
		PacketType type;
		Message::INIT *init_msg;
		Message::ACK *ack_msg;
		Message::START_RENDER *startRender_msg;
		Message::PIXEL *pixel_msg;
		Message::DONE *done_msg;
		Message::HALT *halt_msg;
		Message::FILE_DATA *file_msg;
		Message::CAM_MOVE *cam_msg;

};