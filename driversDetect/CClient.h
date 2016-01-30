#pragma once
#ifndef _DRIVER_DETECT_CCLIENT_H_
#define _DRIVER_DETECT_CCLIENT_H_

#include <numeric>
#include <memory>
#include <exception>
#include "CServer.h"

class CClient{

	void serealize();

public:
	dataStruct::dataStructT data;

	void send();
	void load();

	CClient(const std::string &host, const std::string &port);
};


#endif

