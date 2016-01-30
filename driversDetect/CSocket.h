#pragma once
#ifndef _CXX_CSOCKET_H_
#define _CXX_CSOCKET_H_

#include <thread>
#include <memory>
#include <cstdint>
#include <exception>
#include <string>
#include <atomic>
#include <mutex>
#include <chrono>
#include <map>
#include <sstream>
#include <functional>
#include <fstream>

class CSocketException : std::exception{
	const std::string exception_str;

public:
	const char *what() const{
		return exception_str.c_str();
	}

	CSocketException(const std::string &error)
		: std::exception(error.c_str()),
		exception_str(error)
	{

	}
};

class CSocket {
	std::unique_ptr<char[]> nativeResult;
	const size_t nativeResultSize;
	uintptr_t funcresult;

protected:
	std::string address, port;

	std::unique_ptr<char[]>							hintsn;
	uintptr_t										result;

public:
	class connection;

	typedef uintptr_t				SocketType;

	typedef std::function<int (connection &conn, char *recvbytes, size_t size, CSocket &sock)> callbackf_t;

	class connection {
		static std::atomic<int> defaultSignal;
		static const std::chrono::milliseconds defaultWaitTime;

	public:
		std::atomic<int>			sendTypeR;

		SocketType					socket;

		std::thread					fThread;
		std::atomic<bool>			threadRunning;

		callbackf_t					callb;
		
		std::atomic<size_t>			tranfBytesSize, totalBytesSize;

		typedef std::function<int(size_t, connection&)> iocallback_t;
		typedef std::function<int(std::fstream &file, std::streamsize tranfsize, std::streamsize size, connection&, std::atomic<int> &usrSignal)> fcallback_t;

		void resetSndRcvStats();
		int send(std::iostream &stream, iocallback_t func = nullptr);
		int sendbits(std::iostream &stream, iocallback_t func = nullptr, size_t partlen = 1024 * 64);
		int sendfile(std::fstream &fstrm, fcallback_t func = nullptr, size_t partlen = 1024 * 64, std::atomic<int> &usrSignal = defaultSignal);
		int sendfile(const std::string &file, fcallback_t func = nullptr, size_t partlen = 1024 * 64, std::atomic<int> &usrSignal = defaultSignal);
		int receive(std::iostream &stream, size_t input_max, iocallback_t func = nullptr);

		/* sends file name */
		int autoSendFile(const std::string &name, std::fstream &fstrm, fcallback_t func = nullptr, size_t partlen = 1024 * 64, std::atomic<int> &usrSignal = defaultSignal, const std::chrono::milliseconds &waitTime = defaultWaitTime);

		connection();
	};

	bool initialize();

	CSocket();
	~CSocket();

	CSocket(CSocket&) = delete;
};


class CServerSocket : public CSocket
{
public:
	typedef std::function<int(connection &conn, char *recvbytes, size_t size, CServerSocket &sock)> svcallbackf_t;

	class connection : public CSocket::connection {
	public:
		svcallbackf_t					callb;

		connection() = default;
	};

private:
	SocketType										listenSocket;

	std::atomic<bool>								continueListeningSockets;
	static void										socketListen(CServerSocket &sv);

	std::map<int, connection>						clients;

	std::thread										listerThread;

	static void										clientMGR(CServerSocket &sv, connection &conn);
	std::mutex										writeClient;

	svcallbackf_t									clientFunction;

public:

	bool											initialize(const std::string &lport, svcallbackf_t func);

	CServerSocket();
	~CServerSocket();
};


class CClientSocket : public CSocket {
	int												iResult;

public:
	connection										conn;
	bool											initialize();

	CClientSocket(const std::string &svaddress, const std::string &svport);
	~CClientSocket();
};


#endif
