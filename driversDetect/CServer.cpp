#define _CRT_SECURE_NO_WARNINGS 1


#include "CServer.h"
#include "CSocket.h"
#include <string>
#include <fstream>
#include <exception>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstdarg>
#include <ctime>
#include <stdio.h>
#include <time.h>
#include <algorithm>
#include <Windows.h>


bool CServer::debug = false;
static CServerSocket *svsocket = nullptr;

CDrivers *dvrs = nullptr;

//std::atomic<size_t> recvBytesSize = 0;

void CServer::requestFinish()
{
	if (svsocket)
	{
		//svsocket->stop();
	}
}

CServer::~CServer()
{
	if (svsocket)
	{
		requestFinish();


		delete svsocket;

		svsocket = nullptr;
	}
}

void CServer::receiveUncerealize(const char *data, size_t size, dataStruct::dataStructT &dataStruct)
{
	std::stringstream str;

	str.write(data, size);
	cereal::PortableBinaryInputArchive input(str);

	input(dataStruct);

	/*for (auto &teste : dataStruct)
	{
		std::cout << teste.first << std::endl;
		for (auto &t2 : teste.second)
		{
			std::cout << t2.first << "  ";
			std::cout << t2.second << std::endl << std::endl;
		}
		std::cout << "-----------------------------------------------" << std::endl;
	}*/
}

int rec(size_t bytes, CSocket::connection &conn)
{
	//recvBytesSize = bytes;
	return 0;
}

void thrd(std::atomic<size_t> &s, size_t size, std::atomic<bool> &contnthr)
{
	if (size == 0)
		return;

	double b = size;

	std::cout << "[" << std::string(100, ' ') << "]\r";

	while (contnthr)
	{
		double a = s, calc = ((a / b) * 100.0);
		std::cout << "[";
		for (int i = 0; i < 100; i++)
		{
			if (i < calc){
				std::cout << "#";
			}
			else{
				std::cout << " ";
			}
		}

		std::cout << "]";

		std::cout << "\r";
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}
	std::cout << "\n";
}

static std::mutex svdrvsmutex;

static int tstsendfile(std::fstream &file, std::streamsize tranfsize, std::streamsize size, CSocket::connection& conn, std::atomic<int> &usrSignal)
{
	if (conn.sendTypeR == 1) usrSignal = 1;
	return 0;
}


void thrdsnd(std::atomic<size_t> &s, const CSocket::connection& conn, std::atomic<bool> &contnthr, std::atomic<int> &usrSignal)
{
	if (CServer::debug) {
		while (usrSignal == 0 && contnthr) {
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
		}

		size_t size = conn.totalBytesSize;

		double b = size;

		std::cout << "[" << std::string(100, ' ') << "]\r";

		double lcalc = -1;

		while (contnthr)
		{
			double a = s, calc = ((a / b) * 100.0);

			if (lcalc != calc) {
				std::string toCout("[");

				for (int i = 0; i < 100; i++)
				{
					if (i < calc) {
						toCout += "#";
					}
					else {
						toCout += " ";
					}
				}

				toCout += "]";

				std::cout << toCout + "\r";

				lcalc = calc;
			}
			if (contnthr) std::this_thread::sleep_for(std::chrono::milliseconds(5));
		}
		std::cout << "\n";
	}
}

int callback(CSocket::connection &conn, char *recvbytes, size_t size, CServerSocket &sock)
{
	//std::this_thread::sleep_for(std::chrono::seconds(1));

	static const char okSignal[] = { "OK Signal" };

	std::stringstream ss0;

	ss0.write(okSignal, sizeof(okSignal));

	conn.send(ss0);
	
	std::stringstream ss/*(std::ios::binary | std::ios::in | std::ios::out)*/;

	conn.receive(ss, 0x7FFFF);
	dataStruct::dataStructT data2;


	cereal::PortableBinaryInputArchive input(ss);

	input(data2);

	std::time_t tt = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());

	std::string str = std::string(ctime(&tt));

	str.pop_back();

	std::replace(str.begin(), str.end(), ':', '_');

	std::fstream file(str + ".txt", std::ios::out | std::ios::in | std::ios::binary | std::ios::trunc);

	//std::cout << "Arquivo => " << str + ".txt" << std::endl;

	cereal::JSONOutputArchive out(file);

	out(cereal::make_nvp("PC", data2));

	
	conn.send(ss0);

	auto removeduplicates = [](std::deque<std::string> &deq)
	{
		//std::cout << deq.size() << std::endl;

		/*std::sort(deq.begin(), deq.end());
		deq.erase(std::unique(deq.begin(), deq.end()), deq.end());*/

		std::set<std::string> s;
		unsigned size = deq.size();
		for (unsigned i = 0; i < size; ++i) s.insert(deq[i]);
		deq.assign(s.begin(), s.end());

		//std::cout << deq.size() << std::endl;
	};

	std::deque<std::string> sendToUser;

	for (auto &i : data2) {
		for (auto &asd : i.second) {
			if (asd.first.find("Hardware IDs") != std::string::npos)
			{
				svdrvsmutex.lock();
				auto list = dvrs->getDriverPath(asd.second);
				svdrvsmutex.unlock();

				if (list.size() == 0)
					continue;

				removeduplicates(list);

				sendToUser.insert(sendToUser.end(), list.begin(), list.end());
				break;
			}
		}
	}


	auto sendFolder = [&](const std::string &path) {
		HANDLE hFind;
		WIN32_FIND_DATA data;

		auto pos = path.find_last_of('/');

		std::string folder = (pos != std::string::npos)? std::string(&(path.c_str()[++pos])) : std::to_string(rand());

		if (folder.size() == 0) {
			folder = std::to_string(rand());
		}


		sendFoldersAndFiles f(1, folder);
		std::stringstream fstr(std::ios::binary | std::ios::in | std::ios::out);
		cereal::PortableBinaryOutputArchive output(fstr);
		output(f);

		hFind = FindFirstFile((path + "/*").c_str(), &data);
		if (hFind != INVALID_HANDLE_VALUE)
		{
			do
			{
				std::string cFileName = data.cFileName;
				std::string fullPath = path + std::string("/") + cFileName;

				if (cFileName != "." && cFileName != "..") {
					if ((data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0)
					{
						std::fstream fs(fullPath, std::ios::in | std::ios::binary);

						if (!fs.is_open()) {
							std::cout << "Falha ao abrir " << fullPath << std::endl;
							continue;
						}

						if(CServer::debug) std::cout << "header" << std::endl;
						conn.send(fstr);
						std::this_thread::sleep_for(std::chrono::milliseconds(10));

						{
							std::stringstream okSign;
							conn.receive(okSign, 0x400);
							std::this_thread::sleep_for(std::chrono::milliseconds(10));
						}

						if (CServer::debug) std::cout << cFileName << std::endl;

						std::atomic<int> signal = 0;
						std::atomic<bool> contnthr = true;

						std::thread sndthr(thrdsnd, std::ref(conn.tranfBytesSize), std::ref(conn), std::ref(contnthr), std::ref(signal));
						conn.autoSendFile(cFileName, fs, tstsendfile, 1024 * 64, signal);

						std::this_thread::sleep_for(std::chrono::milliseconds(15));
						contnthr = false;
						//std::this_thread::sleep_for(std::chrono::milliseconds(25));

						if (sndthr.joinable()) {
							sndthr.join();
						}
					}
				}
			} while (FindNextFile(hFind, &data));
			FindClose(hFind);
		}
	};

	removeduplicates(sendToUser);



	for (auto &l : sendToUser) {
		sendFolder(l);
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}

	{
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
		sendFoldersAndFiles f(1, "/");

		std::stringstream ssendstr(std::ios::binary | std::ios::in | std::ios::out);

		cereal::PortableBinaryOutputArchive output(ssendstr);

		output(f);



		conn.send(ssendstr);
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}
	return 0;
}


CServer::CServer(const std::string &port, CDrivers &d) : dvrs(d)
{
	::dvrs = &d;

	svsocket = new CServerSocket();

	svsocket->initialize(port, callback);
}
