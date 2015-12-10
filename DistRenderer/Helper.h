#pragma once

#include <winsock2.h>
#include <thread>
#include <iostream>
#include <chrono>
#include <iphlpapi.h>
#include <ws2tcpip.h>
#include <glm\glm.hpp>
#include "msg.pb.h"

#pragma comment(lib, "Ws2_32.lib")

#define BUFLEN 50000
#define VIEW_RECVPORT 1234
#define IPFAMILY AF_INET
#define PIXEL_PER_MSG 10		// number of pixel sent per packet, tune this for optimization

#define WIDTH 800
#define HEIGHT 800
#define HALF_WIDTH (WIDTH/2)
#define HALF_HEIGHT (HEIGHT/2)
#define YAW_SENSITIVITY 0.001f
#define PITCH_SENSITIVITY 0.001f

#define RETRY_ATTEMPT 5
#define TEST_SEND_ITERATION 10

#define MAXCONN 8
#define PACKET_HEADER_SIZE 5

#define IP_Port std::pair<std::string, unsigned int>

std::string getSelfIP();
void initWinSock();
void printInfo(std::string ip, int port);