#include "CDrivers.h"
#include "CInfReader.h"
#include <Windows.h>
#include <iostream>

int &funct() {
	static int abc = 0;
	return ++abc;
}

void CDrivers::search(const std::string &path, bool recursive)
{
	auto extension_from_filename = [](const std::string &fname)
	{
		size_t s = fname.find_last_of('.');
		return std::string((s != fname.npos) ? &(fname.c_str()[++s]) : "");
	};

	int tbl = dDb.getTableByName("driversTable");

	// TODO: change to STD FileSystem

	HANDLE hFind;
	WIN32_FIND_DATA data;

	hFind = FindFirstFile((std::string("./") + path + "/*").c_str(), &data);
	if (hFind != INVALID_HANDLE_VALUE)
	{
		do
		{
			std::string cFileName = data.cFileName;
			std::string fullPath = path + std::string("/") + cFileName;

			if (cFileName != "." && cFileName != "..") {
				if ((data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0)
				{
					if (extension_from_filename(cFileName) == "inf")
					{
						CInfReader in(path, cFileName);

						in.parse();


						//std::cout << in.getData().size() << std::endl;

						if (in.getData().size() > 0) {
							for (auto &d : in.getData())
							{
								std::deque<DB::CADBNSQL::RowData> dataForTheFields;

								dataForTheFields.push_back(std::to_string(rand()));
								dataForTheFields.push_back(d);
								dataForTheFields.push_back(path);

								dDb.Tables[tbl].addRow(dataForTheFields);
								funct();
							}
						}
					}
				}
				else {
					search(fullPath, true);
				}
			}
		} while (FindNextFile(hFind, &data));
		FindClose(hFind);
	}

	dDb.flush();
}

std::deque<std::string> CDrivers::getDriverPath(const std::string &hwid)
{
	int tbl = dDb.getTableByName("driversTable");
	std::deque<std::string> result;
	
	for (int i = 0, size = dDb.Tables[tbl].getNumRows(); i < size; i++) {
		if (hwid.find(dDb.Tables[tbl].getRowFromField(1, i)) != std::string::npos) {
			result.push_back(dDb.Tables[tbl].getRowFromField(2, i));
		}
	}

	return result;
}

CDrivers::CDrivers(DB::CADBNSQL &db) : dDb(db)
{


}
