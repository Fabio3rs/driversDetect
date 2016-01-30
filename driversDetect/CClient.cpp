#include <fstream>
#include "CClient.h"
#include "CSocket.h"

#include <windows.h>
#include <devguid.h>    // for GUID_DEVCLASS_CDROM etc
#include <setupapi.h>
#include <cfgmgr32.h>   // for MAX_DEVICE_ID_LEN, CM_Get_Parent and CM_Get_Device_ID



#define INITGUID
#include <tchar.h>
#include <stdio.h>
#include <thread>

static CClientSocket *clientSocket = nullptr;

extern void ListDevices(CONST GUID *pClassGuid, LPCTSTR pszEnumerator, dataStruct::dataStructT &cliData);

void thrdrecv(std::atomic<size_t> &s, size_t size, std::atomic<bool> &contnthr)
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
			if (i < calc) {
				std::cout << "#";
			}
			else {
				std::cout << " ";
			}
		}

		std::cout << "]";

		std::cout << "\r";
		if(contnthr) std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}
	std::cout << "\n";
}

void thrd(std::atomic<size_t> &s, const CSocket::connection& conn, std::atomic<bool> &contnthr, std::atomic<int> &usrSignal)
{

	while (usrSignal == 0 && contnthr) {
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}

	size_t size = conn.totalBytesSize;

	double b = size;

	std::cout << "[" << std::string(100, ' ') << "]\r";

	while (contnthr)
	{
		double a = s, calc = ((a / b) * 100.0);
		std::cout << "[";
		for (int i = 0; i < 100; i++)
		{
			if (i < calc) {
				std::cout << "#";
			}
			else {
				std::cout << " ";
			}
		}

		std::cout << "]";

		std::cout << "\r";
		if (contnthr) std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}
	std::cout << "\n";
}

int snd(size_t s, CSocket::connection& conn) {
	return 0;
}

int tstsendfile(std::fstream &file, std::streamsize tranfsize, std::streamsize size, CSocket::connection& conn, std::atomic<int> &usrSignal)
{
	if(conn.sendTypeR == 1) usrSignal = 1;
	return 0;
}

void CClient::send()
{
	serealize();

	auto rec = [](size_t bytes, CSocket::connection &conn)
	{
		//recvBytesSize = bytes;
		return 0;
	};

	auto recvfile = [&](CSocket::connection &conn, const std::string &recvpath){
		std::stringstream fname(std::ios::in | std::ios::out);
		conn.receive(fname, 4096);

		{
			std::stringstream okSign("Ok signal");
			clientSocket->conn.send(okSign);
		}


		std::string str;

		fname >> str;

		if (CServer::debug) std::cout << str << std::endl;

		if (str.size() > 0 && str[0] != 0xFF)
		{
			if (CServer::debug) std::cout << "Recebendo arquivo...\n";

			std::this_thread::sleep_for(std::chrono::milliseconds(5));

			std::stringstream fsize(std::ios::in | std::ios::out);
			conn.receive(fsize, 4096);

			{
				std::stringstream okSign("Ok signal");
				clientSocket->conn.send(okSign);
			}

			std::this_thread::sleep_for(std::chrono::milliseconds(5));

			std::string size;

			try {
				fsize >> size;
			}
			catch (const std::exception &e) {
				std::cout << e.what() << std::endl;
				return 1;
			}
			catch (...) {
				std::cout << "Erro desconhecido" << std::endl;
				return 1;
			}


			if (size.size() == 0)
			{
				std::cout << "Tamanho do arquivo não recebido." << std::endl;
				return 1;
			}

			size_t sizeInteger = 0;

			try {
				if (CServer::debug) std::cout << "t" << size << "t" << std::endl;
				int s = std::stoi(size);
				sizeInteger = std::stoll(size);
			}
			catch (...) {
				std::cout << "O valor de tamanho informado não é valido" << std::endl;
				std::cout << size << std::endl;
				return 1;
			}

			if (CServer::debug) std::cout << "Tamanho: " << size << " bytes" << std::endl;

			std::fstream fs(recvpath + std::string("/") + str, std::ios::out | std::ios::binary | std::ios::trunc);

			if (!fs.is_open()) {
				std::cout << "Não foi possível escrever o arquivo." << std::endl;
				return 1;
			}

			std::atomic<bool> contnthr = true;

			conn.tranfBytesSize = 0;

			std::thread thr(thrdrecv, std::ref(conn.tranfBytesSize), (size_t)std::stoll(size), std::ref(contnthr));

			conn.receive(fs, sizeInteger + 32, rec);

			{
				std::stringstream okSign("Ok signal");
				clientSocket->conn.send(okSign);
			}

			std::this_thread::sleep_for(std::chrono::milliseconds(22));
			contnthr = false;
			std::this_thread::sleep_for(std::chrono::milliseconds(22));

			if (CServer::debug) std::cout << "Finalizado";

			if (thr.joinable())
			{
				thr.join();
			}
			std::cout << std::endl;
		}
	};

	std::this_thread::sleep_for(std::chrono::milliseconds(100));

	while (true) {
		std::this_thread::sleep_for(std::chrono::milliseconds(10));

		std::stringstream ss(std::ios::binary | std::ios::in | std::ios::out);
		sendFoldersAndFiles data2;

		try {
			while (clientSocket->conn.receive(ss, sizeof(sendFoldersAndFiles)+64) == 0)
			{
				std::this_thread::sleep_for(std::chrono::milliseconds(2));
			}

			cereal::PortableBinaryInputArchive input(ss);

			input(data2);

			if (data2.getName() == "/") {
				break;
			}

			CreateDirectoryA(data2.getName().c_str(), nullptr);
			//std::cout << data2.getName() << std::endl;

			
		}
		catch (const std::exception &e) {
			MessageBoxA(0, e.what(), "", 0);
		}

		{
			std::stringstream okSign("Ok signal");
			clientSocket->conn.send(okSign);
		}

		std::this_thread::sleep_for(std::chrono::milliseconds(5));
		if (CServer::debug) std::cout << "recv" << std::endl;
		recvfile(clientSocket->conn, data2.getName());

		std::this_thread::sleep_for(std::chrono::milliseconds(5));
	}

	std::this_thread::sleep_for(std::chrono::milliseconds(50));
	std::this_thread::sleep_for(std::chrono::milliseconds(50));

	std::cout << "Finalizado";
	std::cout << std::endl;
}

void CClient::serealize()
{

	std::stringstream str(std::ios::binary | std::ios::in | std::ios::out);

	cereal::PortableBinaryOutputArchive output(str); // stream to cout

	output(cereal::make_nvp("DriverP2PIdentifier", data));

	clientSocket->conn.send(str);

	std::stringstream ss(std::ios::binary | std::ios::in | std::ios::out);

	clientSocket->conn.receive(ss, 4096);

	std::string string;
	ss >> string;

	std::cout << string << std::endl;
}


void CClient::load()
{
	dataStruct d;

	ListDevices(nullptr, nullptr, data);

	d.set(data);
}

CClient::CClient(const std::string &host, const std::string &port)
{
	dataStruct::setDefauts(data);

	clientSocket = new CClientSocket(host.c_str(), port.c_str());

	clientSocket->initialize();

	std::stringstream ss(std::ios::binary | std::ios::in | std::ios::out);

	clientSocket->conn.receive(ss, 4096);

	std::string string;
	ss >> string;

	std::cout << string << std::endl;

}
