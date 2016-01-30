#pragma once
#ifndef _DRIVERS_CDRIVERS_H_
#define _DRIVERS_CDRIVERS_H_

#include "CADBNSQL.h"
#include <string>

class CDrivers {
	DB::CADBNSQL &dDb;

public:

	void search(const std::string &path, bool recursive = true);
	std::deque<std::string> getDriverPath(const std::string &hwid);

	CDrivers(DB::CADBNSQL &db);
};


#endif


