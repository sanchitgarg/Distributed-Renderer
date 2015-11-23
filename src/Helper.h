#pragma once

#include <winsock2.h>
#include <thread>
#include <iostream>
#include <iphlpapi.h>
#include <ws2tcpip.h>
#include "msg.pb.h"

#pragma comment(lib, "Ws2_32.lib")

#define BUFLEN 50000
#define RECVPORT 1234
#define IPFAMILY AF_INET
#define TILESIZE 20

std::string getSelfIP();
void initWinSock();
void printInfo(std::string ip, int port);