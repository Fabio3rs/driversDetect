#pragma once
#ifndef _DRIVER_DETECT_CSERVER_H_
#define _DRIVER_DETECT_CSERVER_H_

#include <numeric>
#include <memory>
#include <exception>
#include <atomic>

#include <thread>
#include <cstdint>
#include <atomic>
#include <chrono>

// type support
#include <cereal/types/map.hpp>
#include <cereal/types/vector.hpp>
#include <cereal/types/string.hpp>
#include <cereal/types/complex.hpp>

// for doing the actual serialization
#include <cereal/archives/json.hpp>
#include <cereal/archives/xml.hpp>
#include <cereal/archives/portable_binary.hpp>


#include <cereal\cereal.hpp>

#include <iostream>
#include <string>
#include <sstream>
#include <set>

#include "CDrivers.h"

template <class T>
std::streamsize stmsize(T &stream) {
	std::streamsize p = stream.tellg();
	stream.seekg(0, std::ios::end);
	std::streamsize s = stream.tellg();
	stream.seekg(p, std::ios::beg);

	return s;
}

class sendFoldersAndFiles{

public:
	int type;
	std::string str;

	friend class cereal::access;

	template <class Archive>
	void save(Archive & ar) const
	{
		ar(type, str);
	}

	template <class Archive>
	void load(Archive & ar)
	{
		ar(type, str);
	}

	std::string getName() {
		return str;
	}

	int getType() {
		return type;
	}

	sendFoldersAndFiles(int i, const std::string &s) {
		type = i;
		str = s;
	}

	sendFoldersAndFiles() {
		type = 0;
	}
};

class dataStruct
{
public:
	dataStruct() = default;

	typedef std::map < int, std::map<std::string, std::string> > dataStructT;

	void set(const dataStructT &d)
	{
		data = d;
	}

	static void setDefauts(dataStructT &d)
	{
		d[-1]["driverTestVersionString"] = "0.1a";
	}

private:
	dataStructT data;

	friend class cereal::access;

	template <class Archive>
	void serialize(Archive & ar)
	{
		ar(CEREAL_NVP(data));
	}

	template <class Archive>
	void save(Archive & ar)
	{
		ar(CEREAL_NVP(data));
	}

	template <class Archive>
	void load(Archive & ar)
	{
		ar(CEREAL_NVP(data));
	}
};

class CServer{
	CDrivers &dvrs;

public:
	static void receiveUncerealize(const char *data, size_t size, dataStruct::dataStructT &dataStruct);

	void requestFinish();

	CServer(const std::string &port, CDrivers &d);
	~CServer();

	static bool debug;
};


#endif

