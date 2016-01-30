#include "CSocket.h"
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>

static const char endTransmissionSignal[] = { "\nEndThisPacket\n" };

std::atomic<int> CSocket::connection::defaultSignal = 0;
const std::chrono::milliseconds CSocket::connection::defaultWaitTime(50);

bool CSocket::initialize()
{
	nativeResult = std::unique_ptr<char[]>((char*)new WSADATA);

	WSADATA &nativeData = *(WSADATA*)nativeResult.get();

	funcresult = WSAStartup(MAKEWORD(2, 2), &nativeData);

	if (funcresult != 0)
	{
		throw CSocketException(std::string("WSAStartup failed: ") + std::to_string(funcresult));
	}

	return true;
}

CSocket::CSocket()
	: nativeResultSize(sizeof(WSADATA)),
	nativeResult(nullptr)
{
	funcresult = (uintptr_t)nullptr;
	
	hintsn = std::unique_ptr<char[]>((char*)new ADDRINFOA);
	ADDRINFOA &hints = *(ADDRINFOA*)hintsn.get();

	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_flags = AI_PASSIVE;

}

CSocket::~CSocket()
{
	WSACleanup();
}

CSocket::connection::connection()
{
	threadRunning = true;
	tranfBytesSize = 0;
	totalBytesSize = 0;
	sendTypeR = 0;
}

void CSocket::connection::resetSndRcvStats()
{
	tranfBytesSize = 0;
	totalBytesSize = 0;
}

int CSocket::connection::sendfile(const std::string &file, fcallback_t func, size_t partlen, std::atomic<int> &usrSignal)
{
	sendTypeR = 1;
	std::fstream fstrm(file, std::ios::in | std::ios::binary);

	if (!fstrm.is_open())
	{
		return -1;
	}

	return sendfile(fstrm, func, partlen, usrSignal);
}

int CSocket::connection::sendfile(std::fstream &fstrm, fcallback_t func, size_t partlen, std::atomic<int> &usrSignal)
{
	resetSndRcvStats();
	sendTypeR = 1;

	if (!fstrm.is_open())
	{
		return -1;
	}

	if (partlen == 0 || partlen == (~0) )
	{
		return send(fstrm, [&](size_t tranfBytesSize, connection &conn) { return func? func(fstrm, tranfBytesSize, conn.totalBytesSize, conn, usrSignal) : 0; });
	}
	else
	{
		return sendbits(fstrm, [&](size_t tranfBytesSize, connection &conn) { return func ? func(fstrm, tranfBytesSize, conn.totalBytesSize, conn, usrSignal) : 0; }, partlen);
	}

	return 0;
}

int CSocket::connection::sendbits(std::iostream &stream, iocallback_t func, size_t partlen)
{
	tranfBytesSize = 0;
	totalBytesSize = 0;

	if (partlen == 0)
	{
		return -1;
	}

	int snd = 0;
	
	stream.seekg(0, std::ios::end);
	std::streamsize s = stream.tellg();
	stream.seekg(0, std::ios::beg);

	totalBytesSize = s;

	if(func) func(tranfBytesSize, *this);

	if (partlen >= s) {
		snd = send(stream, func);
	}
	else {

		std::streamsize parts = (s / partlen);
		std::streamsize partSize = partlen;
		std::streamsize pos = 0;

		std::unique_ptr<char[]> ch(new char[partSize]);

		for (std::streamsize i = 0; i < parts + 1; i++) {
			std::streamsize readsize = partSize, diff = s - pos;

			if (diff == 0) {
				//std::cout << 0 << std::endl;
				break;
			}

			if (diff < readsize)
			{
				readsize = diff;
			}

			stream.read(ch.get(), readsize);

			snd = ::send(socket, ch.get(), readsize, 0);

			pos += readsize;

			tranfBytesSize = pos;
			if (func) func(tranfBytesSize, *this);
			//std::cout << readsize << "      " << stream.good() << std::endl;
		}

		std::this_thread::sleep_for(std::chrono::milliseconds(1));
		snd = ::send(socket, endTransmissionSignal, sizeof(endTransmissionSignal), 0);
	}

	stream.seekg(0, std::ios::beg);

	return snd;
}

/* TODO: callback????*/
int CSocket::connection::send(std::iostream &stream, iocallback_t func)
{
	stream.seekg(0, std::ios::end);
	std::streamsize s = stream.tellg();
	stream.seekg(0, std::ios::beg);

	totalBytesSize = s;

	if (func) func(tranfBytesSize, *this);

	std::unique_ptr<char[]> ch(new char[s + 4]);

	stream.read(ch.get(), s);

	int snd = ::send(socket, ch.get(), s, 0);

	std::this_thread::sleep_for(std::chrono::milliseconds(1));

	::send(socket, endTransmissionSignal, sizeof(endTransmissionSignal), 0);
	
	return snd;
}

int CSocket::connection::autoSendFile(const std::string &name, std::fstream &fstrm, fcallback_t func, size_t partlen, std::atomic<int> &usrSignal, const std::chrono::milliseconds &waitTime)
{
	sendTypeR = 0;
	auto internalCallback = [&](size_t tranfBytesSize, connection &conn)
	{
		return func ? func(fstrm, tranfBytesSize, conn.totalBytesSize, conn, usrSignal) : 0;
	};

	if (name.size() == 0)
	{
		return -1;
	}

	fstrm.seekg(0, std::ios::end);
	std::streamsize size = fstrm.tellg();
	fstrm.seekg(0, std::ios::beg);

	std::this_thread::sleep_for(waitTime / 5);
	{
		std::stringstream ssname(name);

		int res = 0;
		sendTypeR = 2;

		if ((res = send(ssname, internalCallback)) == 0 || res == -1)
		{
			return res;
		}
	}

	{
		std::stringstream okSign;
		receive(okSign, 0x400);
	}

	std::this_thread::sleep_for(waitTime / 5);

	{
		std::stringstream sssize(std::to_string(size));

		int res = 0;

		sendTypeR = 3;
		if ((res = send(sssize, internalCallback)) == 0 || res == -1)
		{
			return res;
		}
	}

	{
		std::stringstream okSign;
		receive(okSign, 0x400);
	}

	std::this_thread::sleep_for(waitTime / 5);

	sendTypeR = 1;
	int result = sendfile(fstrm, func, partlen, usrSignal);

	{
		std::stringstream okSign;
		receive(okSign, 0x400);
	}
	std::this_thread::sleep_for(waitTime / 5);

	return result;
}

int CSocket::connection::receive(std::iostream &stream, size_t input_max, iocallback_t func)
{
	size_t bytesall = 0;
	char buffer[4096] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
	tranfBytesSize = 0;

	while (bytesall < input_max)
	{
		size_t bytesReceived = recv(socket, buffer, sizeof(buffer) / sizeof(char), MSG_PARTIAL);

		if (bytesReceived == sizeof(endTransmissionSignal))
		{
			if (strcmp(endTransmissionSignal, buffer) == 0)
			{
				break;
			}
		}

		int err = WSAGetLastError();

		if (err == 10035 || bytesReceived == -1)
		{
			break;
		}

		bool subtract = false;
		size_t b = bytesReceived - sizeof(endTransmissionSignal);

		if (err == 0 && bytesReceived > sizeof(endTransmissionSignal) && strcmp(endTransmissionSignal, &buffer[b]) == 0)
		{
			subtract = true;
			bytesReceived = b;
		}

		stream.write(buffer, bytesReceived);
		bytesall += bytesReceived;

		tranfBytesSize = bytesall;

		if (func)
		{
			func(bytesall, *this);
		}

		if (subtract)
		{
			break;
		}
	}

	stream.seekg(0, std::ios::beg);

	return bytesall;
}

bool CServerSocket::initialize(const std::string &lport, svcallbackf_t func)
{
	port = lport;
	CSocket::initialize();

	clientFunction = func;

	PADDRINFOA result = *(PADDRINFOA*)&this->result;;

	ADDRINFOA &hints = *(ADDRINFOA*)hintsn.get();

	int serverResult = getaddrinfo(NULL, port.c_str(), &hints, (PADDRINFOA*)&result);
	if (serverResult != 0) {
		WSACleanup();
		throw CSocketException(std::string("getaddrinfo failed: ") + std::to_string(serverResult));

		return false;
	}

	listenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
	if (listenSocket == INVALID_SOCKET) {
		throw CSocketException(std::string("socket failed: ") + std::to_string(WSAGetLastError()));
		return false;
	}

	serverResult = bind(listenSocket, result->ai_addr, (int)result->ai_addrlen);
	if (serverResult == SOCKET_ERROR) {
		throw CSocketException(std::string("bind failed: ") + std::to_string(WSAGetLastError()));
		return false;
	}

	if (listen(listenSocket, SOMAXCONN) == SOCKET_ERROR) {
		throw CSocketException(std::string("listen failed: ") + std::to_string(WSAGetLastError()));
		return false;
	}

	listerThread = std::thread(socketListen, std::ref(*this));

	return true;
}

void CServerSocket::clientMGR(CServerSocket &sv, connection &conn)
{
	svcallbackf_t call = conn.callb;

	try {
		call(std::ref(conn), nullptr, (size_t)0, std::ref(sv));
	}
	catch (const std::exception &e)
	{
		throw CSocketException(std::string("In client thread: ") + e.what());
	}

}

void CServerSocket::socketListen(CServerSocket &sv)
{
	while (sv.continueListeningSockets)
	{
		SOCKET clientSocket = accept(sv.listenSocket, NULL, NULL);

		if (clientSocket == INVALID_SOCKET)
		{
			if (!sv.continueListeningSockets)
			{
				break;
			}

			std::cout << "INVALID_SOCKET" << std::endl;
			break;
		}

		sv.writeClient.lock();
		auto &cli = sv.clients[sv.clients.size()];

		cli.socket = clientSocket;
		cli.callb = sv.clientFunction;
		cli.threadRunning = true;
		cli.fThread = std::thread(clientMGR, std::ref(sv), std::ref(cli));

		sv.writeClient.unlock();
	}
}

CServerSocket::CServerSocket() : CSocket()
{
	continueListeningSockets = true;
	clientFunction = nullptr;
	listenSocket = NULL;

}

CServerSocket::~CServerSocket()
{
	continueListeningSockets = false;
	closesocket(listenSocket);

	for (auto &ths : clients)
	{
		auto &t = ths.second;

		t.threadRunning = false;

		if (t.fThread.joinable())
		{
			t.fThread.join();
		}
	}

	if (listerThread.joinable())
	{
		listerThread.join();
	}

	CSocket::~CSocket();
}

bool CClientSocket::initialize()
{
	CSocket::initialize();

	ADDRINFOA &hints = *(ADDRINFOA*)hintsn.get();
	iResult = getaddrinfo(address.c_str(), port.c_str(), &hints, (PADDRINFOA*)&result);
	addrinfo *ptr = nullptr;

	for (ptr = (addrinfo*)result; ptr != NULL; ptr = ptr->ai_next) {
		conn.socket = socket(ptr->ai_family, ptr->ai_socktype,
			ptr->ai_protocol);
		if (conn.socket == INVALID_SOCKET) {
			printf("socket failed with error: %ld\n", WSAGetLastError());
			WSACleanup();
			return false;
		}

		// Connect to server.
		iResult = connect(conn.socket, ptr->ai_addr, (int)ptr->ai_addrlen);
		if (iResult == SOCKET_ERROR) {
			closesocket(conn.socket);
			conn.socket = INVALID_SOCKET;
			continue;
		}
		// std::cout << "Connected\n";
		break;
	}

	result = (uintptr_t)ptr;

	return true;
}


CClientSocket::CClientSocket(const std::string &svaddress, const std::string &svport) : CSocket()
{
	address = svaddress;
	port = svport;

	conn.socket = INVALID_SOCKET;

	iResult = SOCKET_ERROR;
}

CClientSocket::~CClientSocket()
{
	if (conn.socket != INVALID_SOCKET) {
		closesocket(conn.socket);
		//shutdown(conn.socket);
	}

	CSocket::~CSocket();
}

