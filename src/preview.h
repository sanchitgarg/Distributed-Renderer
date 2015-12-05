#pragma once

extern GLuint pbo;

std::string currentTimeString();
bool init();
void mainLoop(PacketManager* pMgr, std::string client_ip);
