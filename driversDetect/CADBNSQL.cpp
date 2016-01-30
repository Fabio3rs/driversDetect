#include "CADBNSQL.h"
#include <iostream>

#ifdef _WIN32
#include <Windows.h> // For CreateDirectoryA
#endif

int DB::CADBNSQL::getTableByName(std::string name){
	int i = 0;
	for(auto &dbTable : Tables){
		//std::cout << name << "     " << dbTable.name << std::endl;
		if(name == dbTable.name){
			return i;
		}
		i++;
	}
	return -1;
}

int DB::CADBNSQL::Tabledata::getFieldByName(std::string name){
	int i = 0;
	for(auto &dbField : Fields){
		if(name == dbField.name){
			return i;
		}
		i++;
	}

	return -1;
}

std::string DB::CADBNSQL::dbFolderFormatWOUTEXT(std::string dbName){
	char result[2048];
	
	size_t p = dbName.find_first_of('.');

	if(p != std::string::npos){
		dbName.resize(p);
	}

	sprintf(result, "./%s/%s", defaultDataFolder.c_str(), dbName.c_str());

#ifdef _DEBUG
	std::cout << result << std::endl;
#endif

	return result;
}

std::string DB::CADBNSQL::dbFolderFormat(std::string dbName){
	char result[2048];

	sprintf(result, "./%s/%s", defaultDataFolder.c_str(), dbName.c_str());

#ifdef _DEBUG
	std::cout << result << std::endl;
#endif

	return result;
}

std::string DB::CADBNSQL::tableFolderFormat(std::string tableName){
	char result[2048];

	std::string databaseFName = dataBaseFileName;
	
	size_t p = databaseFName.find_first_of('.');

	if(p != std::string::npos){
		databaseFName.resize(p);
	}

	sprintf(result, "./%s/%s-tables/%s", defaultDataFolder.c_str(), databaseFName.c_str(), tableName.c_str());

#ifdef _DEBUG
	std::cout << result << std::endl;
#endif

	return result;
}

size_t DB::CADBNSQL::getDBFileSize(){
	fseek(DBFile, 0, SEEK_END);

	DBSize = ftell(DBFile);

	rewind(DBFile);

	return DBSize;
}

void DB::CADBNSQL::Tabledata::loadDataFromDBFile(){
	if(!loaded){
		FILE *tableDataFile = fopen(MainDBPtr->tableFolderFormat(getName()).c_str(), "rb+");
		
		if(tableDataFile == nullptr){
			tableDataFile = fopen(MainDBPtr->tableFolderFormat(getName()).c_str(), "wb+");
		}

		for(int l = 0, lsize = getNumRows(); l < lsize; l++){
			for(int j = 0, jsize = Fields.size(); j < jsize; j++){
				char *rowTmp = new char[Fields[j].getSize() + 1];
				memset(rowTmp, 0, Fields[j].getSize() + 1);

				fread(rowTmp, 1, Fields[j].getSize(), tableDataFile);

				RowData newRow;
				newRow.data = rowTmp;
				//newRow.data.insert(0, rowTmp, Fields[j].getSize());

				Fields[j].data.push_back(newRow);

				delete[] rowTmp;
			}
		}

		if(tableDataFile) fclose(tableDataFile);
		loaded = true;
	}
}

DB::CADBNSQL::Tabledata::wrapper::wrapper(Tabledata &table, const int wRow) : tbl(table), workingRow(wRow){

}

DB::CADBNSQL::Tabledata::wrapper DB::CADBNSQL::Tabledata::operator[](const int row){
	return wrapper(*this, row);
}

void DB::CADBNSQL::Tabledata::addField(FieldData &Field){
	for(auto &field : Fields){
		if(field.name == Field.name){
			return;
		}
	}

	loadDataFromDBFile();

	Fields.push_back(Field);
}

void DB::CADBNSQL::Tabledata::addField(std::string name, fieldType type, size_t size){
	for(auto &field : Fields){
		if(field.name == name){
			std::cout << name << std::endl;
			return;
		}
	}

	loadDataFromDBFile();

	FieldData Field(name, this);
	Field.setType(type);
	Field.setSize(size);

	Fields.push_back(Field);
}

bool DB::CADBNSQL::Tabledata::addRow(const std::deque<RowData> &dataForTheFields){
	loadDataFromDBFile();

	if(dataForTheFields.size() != Fields.size()){
		return false;
	}

	for(int i = 0, size = dataForTheFields.size(); i < size; i++){
		Fields[i].addRow(dataForTheFields[i]);
	}

	rows++;

	return true;
}
std::string DB::CADBNSQL::Tabledata::getRowFromField(int fieldID, int rowID){
	loadDataFromDBFile();

	std::string result = Fields[fieldID].data[rowID].data;

	return result;
}

void DB::CADBNSQL::flush(bool useMutex){

	DBFile = freopen(dbFolderFormat(dataBaseFileName).c_str(), "wb+", DBFile);
	fseek(DBFile, 0, SEEK_SET);
	fHeader.numTables = Tables.size();
	fwrite(&fHeader, sizeof(fHeader), 1, DBFile);

	size_t aaa;

	std::vector<Table> tableArray;
	std::vector<Field> fieldArray;

	// Tables ADD
	for(int i = 0, size = Tables.size(); i < size; i++){
		Table newTable;

		strcpy(newTable.name, Tables[i].getName().c_str());
		newTable.numFields = Tables[i].Fields.size();
		/*
		if(Tables[i].Fields.size() > 0){
			Tables[i].rows = Tables[i].Fields[0].data.size();
		}*/

		newTable.numRows = Tables[i].getNumRows();
		newTable.fieldsPosInFile = -1L;
		newTable.rowsPosInFile = -1L;

		tableArray.push_back(newTable);
	}

	// Fields ADD
	for(int i = 0, size = Tables.size(); i < size; i++){
		uint64_t pos = tableArray.size() * sizeof(Table) + sizeof(Field) * fieldArray.size();
		
		tableArray[i].fieldsPosInFile = pos;

		for(int j = 0, jsize = Tables[i].Fields.size(); j < jsize; j++){
			Field newField;

			strcpy(newField.name, Tables[i].Fields[j].getName().c_str());
			newField.type = Tables[i].Fields[j].getType();
			newField.size = Tables[i].Fields[j].getSize();

			fieldArray.push_back(newField);
		}
	}
	
	/*std::cout << */fwrite(&tableArray[0], sizeof(Table), tableArray.size(), DBFile)/* << std::endl*/;
	/*std::cout << */fwrite(&fieldArray[0], sizeof(Field), fieldArray.size(), DBFile)/* << std::endl*/;
	
	fflush(DBFile);

	// Row ADD
	for(int i = 0, size = Tables.size(); i < size; i++){
		if(Tables[i].loaded){
			FILE *tableDataFile = fopen(tableFolderFormat(Tables[i].getName()).c_str(), "wb+");

			if (tableDataFile == nullptr) {
				std::fstream table(tableFolderFormat(Tables[i].getName()), std::ios::in | std::ios::out | std::ios::trunc);
				table.flush();
				table.close();

				tableDataFile = fopen(tableFolderFormat(Tables[i].getName()).c_str(), "wb+");
			}

			for(int l = 0, lsize = Tables[i].getNumRows(); l < lsize; l++){
				for(int j = 0, jsize = Tables[i].Fields.size(); j < jsize; j++){
					char *rowTmp = new char[Tables[i].Fields[j].getSize() + 1];
					memset(rowTmp, 0, Tables[i].Fields[j].getSize() + 1);

					memcpy(rowTmp, Tables[i].Fields[j].getRow(l).data.c_str(), Tables[i].Fields[j].getRow(l).data.size());

					fwrite(rowTmp, 1, Tables[i].Fields[j].getSize(), tableDataFile);

					fflush(tableDataFile);
					delete[] rowTmp;
				}
			}

			fclose(tableDataFile);
		}
	}
}

bool DB::CADBNSQL::FieldData::setData(const RowData &data, size_t pos){
	if(pos == -1L) return false;
	if(pos >= this->data.size()) return false;

	try{
		this->data[pos] = data;
	}catch(...){
		return false;
	}

	return true;
}

DB::CADBNSQL::FieldData::FieldData(const std::string &flName, Tabledata *Table){
	MainTable = Table;
	name = flName;

	size = 0;
	type = 0;
}

uint32_t DB::CADBNSQL::createTable(const Tabledata &table){
	int i = getTableByName(table.name);
	
	//std::cout << i << std::endl;

	if (i != -1)
		return i;

	Tables.push_back(table);
	Tables.back().MainDBPtr = this;

	return Tables.size() - 1;
}

DB::CADBNSQL::Tabledata::Tabledata(const std::string &tbName, CADBNSQL *MainDBPtr){
	name = tbName;

	rows = 0;

	loaded = false;
	this->MainDBPtr = MainDBPtr;
}

void DB::CADBNSQL::loadTablesFromDBFile(){
	Table *dbTables = new Table[fHeader.numTables];

	fread(dbTables, sizeof(Table), fHeader.numTables, DBFile);

	for(int i = 0; i < fHeader.numTables; i++){
		Tabledata myTable(dbTables[i].name, this);

		myTable.rows = dbTables[i].numRows;
		
		Field *dbFields = new Field[dbTables[i].numFields];
		
		//fseek(DBFile, dbTables[i].fieldsPosInFile, SEEK_SET);
		fread(dbFields, sizeof(Field), dbTables[i].numFields, DBFile);

		std::deque<FieldData> &Fields = myTable.Fields;

		for(int j = 0; j < dbTables[i].numFields; j++){
			FieldData dbField(dbFields[j].name, &myTable);
			dbField.type = dbFields[j].type;
			dbField.size = dbFields[j].size;

			Fields.push_back(dbField);
		}

		delete[] dbFields;

		Tables.push_back(myTable);
	}

	for(auto &table : Tables){
		for(auto &field : table.Fields){
			field.MainTable = &table;
		}
	}

	delete[] dbTables;
}

DB::CADBNSQL::CADBNSQL(const char *databaseFileName, std::string defaultDataFolder) : defaultDataFolder(defaultDataFolder){
#ifdef _WIN32
	CreateDirectoryA(defaultDataFolder.c_str(), 0);
#endif
	DBFile = fopen(dbFolderFormat(databaseFileName).c_str(), "rb+");

	if(!DBFile){
		std::fstream db(databaseFileName, std::ios::in | std::ios::out | std::ios::trunc);
		db.flush();
		db.close();

		DBFile = fopen(dbFolderFormat(databaseFileName).c_str(), "wb+");
	}

	dataBaseFileName = databaseFileName;

#ifdef _WIN32
	CreateDirectoryA((dbFolderFormatWOUTEXT(databaseFileName) + "-tables").c_str(), 0);
#endif

	getDBFileSize();

	if(DBSize >= sizeof(fHeader)){
		fread(&fHeader, sizeof(fHeader), 1, DBFile);

		loadTablesFromDBFile();
	}else{
		fHeader.numTables = 0;
		strcpy(fHeader.realName, "AAA");

		fwrite(&fHeader, sizeof(fHeader), 1, DBFile);
		fflush(DBFile);
	}
}

DB::CADBNSQL::CADBNSQL() : defaultDataFolder(defaultDataFolder){
	DBFile = nullptr;
	DBSize = 0;
}

DB::CADBNSQL::CADBNSQL(std::string defaultDataFolder) : defaultDataFolder(defaultDataFolder){
#ifdef _WIN32
	CreateDirectoryA(defaultDataFolder.c_str(), 0);
#endif
	DBFile = nullptr;
	DBSize = 0;
}

DB::CADBNSQL::~CADBNSQL(){
	flush();
}
