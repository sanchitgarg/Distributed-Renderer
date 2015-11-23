#pragma once

extern GLuint pbo;

std::string currentTimeString();
bool init();
void mainLoop(PacketListener* pRecv, PacketSender* pSend, std::string client_ip);
