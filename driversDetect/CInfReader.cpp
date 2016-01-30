#include "CInfReader.h"
#include <iostream>
#include <Windows.h>
#include <memory>

std::string CInfReader::str_replace(const std::string& search,
	const std::string& replace,
	std::string& subject)
{
	std::string str = subject;
	size_t pos = 0;
	while ((pos = str.find(search, pos)) != std::string::npos)
	{
		str.replace(pos, search.length(), replace);
		pos += replace.length();
	}

	subject = str;
	return str;
}

std::deque<std::string> CInfReader::explode(const std::string &s, char delim)
{
	std::deque<std::string> result;
	std::istringstream iss(s);

	for (std::string token; std::getline(iss, token, delim); )
	{
		result.push_back(std::move(token));
	}

	return result;
}

bool CInfReader::parse()
{
	std::fstream file(filePath + std::string("/") +fileName, std::ios::in | std::ios::binary);

	if (!file.is_open())
	{
		return false;
	}

	data.clear();

	file.seekg(0, std::ios::end);
	std::streamsize size = file.tellg();

	std::unique_ptr < char[] > c(new char[size + 4uL]);
	
	memset(c.get(), '\n', size + 4uL);

	c.get()[size + 2uL] = 0;
	c.get()[size + 3uL] = 0;

	file.seekg(0, std::ios::beg);

	file.read(c.get(), size);

	if(c.get()[3] == 0){
		wchar_t *wch = (wchar_t*)c.get();

		std::wstring str(wch, wch + size / 2);

		auto exp = explode(std::string(str.begin(), str.end()), '\n');

		for (auto &line : exp)
		{
			str_replace("\r", "", line);

			auto exp2 = explode(line, ' ');

			for (auto &e : exp2)
			{
				for (auto &rule : rules)
				{
					if (std::regex_match(e, rule))
					{
						data.push_back(e);
						break;
					}
				}
			}
		}
	}
	else {
		std::string str(c.get(), c.get() + size);

		auto exp = explode(str, '\n');

		for (auto &line : exp)
		{
			str_replace("\r", "", line);

			auto exp2 = explode(line, ' ');

			for (auto &e : exp2)
			{
				for (auto &rule : rules)
				{
					if (std::regex_match(e, rule))
					{
						data.push_back(e);
						break;
					}
				}
			}
		}
	}

	return true;
}

const std::deque<std::string> &CInfReader::getData()
{
	return data;
}

CInfReader::CInfReader(const std::string &iniPath, const std::string &infFile)
{
	filePath = iniPath;
	fileName = infFile;

	rules.push_back(std::regex("((USB)|(PCI)|(ACPI)|(HDAUDIO))?(\\\\)(VID_)[[:xdigit:]][[:xdigit:]][[:xdigit:]][[:xdigit:]](\\&PID_)[[:xdigit:]][[:xdigit:]][[:xdigit:]][[:xdigit:]](\\&REV_)[[:xdigit:]][[:xdigit:]][[:xdigit:]][[:xdigit:]](^\\\\)"));
	rules.push_back(std::regex("((USB)|(PCI)|(ACPI)|(HDAUDIO))?(\\\\)(VID_)[[:xdigit:]][[:xdigit:]][[:xdigit:]][[:xdigit:]](\\&PID_)[[:xdigit:]][[:xdigit:]][[:xdigit:]][[:xdigit:]](\\&REV_)[[:xdigit:]][[:xdigit:]][[:xdigit:]][[:xdigit:]]"));
	rules.push_back(std::regex("((USB)|(PCI)|(ACPI)|(HDAUDIO))?(\\\\)(VEN_)[[:xdigit:]][[:xdigit:]][[:xdigit:]][[:xdigit:]](\\&DEV_)[[:xdigit:]][[:xdigit:]][[:xdigit:]][[:xdigit:]](\\&CC_)[[:xdigit:]][[:xdigit:]][[:xdigit:]][[:xdigit:]]"));
	rules.push_back(std::regex("((USB)|(PCI)|(ACPI)|(HDAUDIO))?(\\\\)(VEN_)[[:xdigit:]][[:xdigit:]][[:xdigit:]][[:xdigit:]](\\&DEV_)[[:xdigit:]][[:xdigit:]][[:xdigit:]][[:xdigit:]]"));
	rules.push_back(std::regex("((USB)|(PCI)|(HDAUDIO))?(\\\\)(FUNC_01)[[:xdigit:]][[:xdigit:]][[:xdigit:]][[:xdigit:]](\\&DEV_)[[:xdigit:]][[:xdigit:]][[:xdigit:]][[:xdigit:]]"));
	rules.push_back(std::regex("((USB)|(PCI)|(ACPI)|(HDAUDIO))?(\\\\)(VEN_)[[:xdigit:]][[:xdigit:]][[:xdigit:]][[:xdigit:]](\\&DEV_)[[:xdigit:]][[:xdigit:]][[:xdigit:]][[:xdigit:]]((\\&CC_)|(\\&SUBSYS_))?(^\\\\)"));
	rules.push_back(std::regex("((USB)|(PCI)|(ACPI)|(SCSI))?(\\\\)(^\\\\)"));
}

CInfReader::CInfReader()
{


}
