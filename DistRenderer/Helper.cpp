#include "Helper.h"

#define ADAPTERFLAG 0
#define OUT_BUFFER_SIZE 15000

//adapted from MS example code
std::string getSelfIP(){
	ULONG flags = GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_MULTICAST | GAA_FLAG_SKIP_DNS_SERVER;
	ULONG outBufLen = OUT_BUFFER_SIZE; 
	PIP_ADAPTER_ADDRESSES ip_addr = (IP_ADAPTER_ADDRESSES *)malloc(outBufLen);

	DWORD result = GetAdaptersAddresses(IPFAMILY, flags, NULL, ip_addr, &outBufLen);
	
	char buff[BUFLEN];
	for (; ip_addr; ip_addr = ip_addr->Next){
		PIP_ADAPTER_UNICAST_ADDRESS pUnicast = ip_addr->FirstUnicastAddress;
		if (pUnicast != NULL && ip_addr->OperStatus == IF_OPER_STATUS::IfOperStatusUp) {
			sockaddr_in *sa_in = (sockaddr_in *)pUnicast->Address.lpSockaddr;
			std::string ip_out = inet_ntop(AF_INET, &(sa_in->sin_addr), buff, BUFLEN);

			if (ip_out.compare("127.0.0.1") != 0)
				return ip_out;
		}
	}

	return "127.0.0.1";
}

void initWinSock(){
	WSADATA wsaData;
	int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (result != 0){
		std::cout << "WSAStartup failed: " + result << std::endl;
		exit(EXIT_FAILURE);
	}
}

void printInfo(std::string ip, int port){
	std::cout << "This machine's Address:\t" << ip << ":" << port << std::endl;
	std::cout << std::endl;
}