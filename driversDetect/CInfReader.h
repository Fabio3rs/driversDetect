#pragma once
#ifndef __CXX__CINFREADER_H__
#define __CXX__CINFREADER_H__

#include <string>
#include <fstream>
#include <sstream>
#include <regex>
#include <deque>
#include <algorithm>

class CInfReader {
	static std::deque<std::string> explode(const std::string &s, char delim);
	std::string str_replace(const std::string& search, const std::string& replace, std::string& subject);

	std::string filePath, fileName;

	std::deque<std::string> data;
	std::deque<std::regex> rules;

public:
	const std::deque<std::string> &getData();

	bool parse();

	CInfReader(const std::string &iniPath, const std::string &infFile);
	CInfReader();

	~CInfReader() = default;
};


#endif


