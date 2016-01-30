#pragma once
#ifndef _DB_CADBNSQL_H_
#define _DB_CADBNSQL_H_
#define _CRT_SECURE_NO_WARNINGS 1

#include <string>
#include <deque>
#include <map>
#include <cstdint>
#include <cstdio>
#include <vector>
#include <mutex>
#include <cstdlib>
#include <cstring>
#include <algorithm>
#include <cmath>
#include <exception>
#include <fstream>

namespace DB{
	enum fieldType{integer, real, varchar};

	class CADBNSQLExcept : std::exception{
		const std::string excpt;

	public:
		inline const char *what(){
			return excpt.c_str();
		}

		inline CADBNSQLExcept(const std::string &e) : excpt(e), std::exception(){

		}
	};

	class CADBNSQL{
		FILE							*DBFile;
		std::string						dataBaseFileName;
		size_t							DBSize;

		size_t							getDBFileSize();
		void							loadTablesFromDBFile();

		const std::string				defaultDataFolder;
		std::string						tableFolderFormat(std::string tableName);
		std::string						dbFolderFormat(std::string dbName);
		std::string						dbFolderFormatWOUTEXT(std::string dbName);

		struct DBHeader{
			char realName[256];
			uint32_t numTables;

			inline DBHeader(){
				memset(realName, 0, sizeof(realName));
				numTables = 0;
			}
		} fHeader;
		
		struct Table{
			char name[256];
			uint32_t numFields;
			uint64_t fieldsPosInFile;
			uint32_t numRows;
			uint64_t rowsPosInFile;

			inline Table(){
				memset(name, 0, sizeof(name));
				numFields = 0;
				numRows = 0;
			}
		};

		struct Field{
			char name[256];
			uint32_t size;
			uint32_t type;

			inline Field(){
				memset(name, 0, sizeof(name));
				size = 0;
				type = 0;
			}
		};

	public:
		/*
		Database file's wrappers:

		*/
		class Tabledata;

		struct RowData{
			std::string data;

			inline RowData(const std::string &data){
				this->data = data;
			}

			inline RowData(const char *data){
				this->data = data;
			}

			inline RowData(){};
		};

		class FieldData{
			std::string					name;
			size_t						size;
			uint32_t					type;
			Tabledata					*MainTable;

			std::deque<RowData>			data;

		protected:
			inline void					addRow(const RowData &Row){data.push_back(Row);};
			friend						class CADBNSQL;
			friend						class Tabledata;

		public:
			inline RowData				&getRow(uint64_t pos){return data[pos];};
			bool						setData(const RowData &data, size_t pos);
			inline const int			getType() const{return type;}
			inline const size_t			getSize() const{return size;}
			inline void					setSize(size_t size){this->size = size;}
			inline void					setType(fieldType type){this->type = type;}
			inline const std::string	&getName() const{return name;}

			FieldData(const std::string &flName, Tabledata *Table);
		};

		class Tabledata{
			CADBNSQL					*MainDBPtr;
			std::string					name;
			size_t						rows;
			bool						loaded;
			void						loadDataFromDBFile();

		protected:
			std::deque<FieldData>		Fields;
			friend						class CADBNSQL;

		public:
			class wrapper
			{
				Tabledata &tbl;
				const int workingRow;

			public:
				inline std::string		operator[](const std::string &field) const{
					return tbl.getRowFromField(tbl.getFieldByName(field), workingRow);
				}

				wrapper(Tabledata &table, const int wRow);
			};

			int							getFieldByName(std::string name);
			std::string					getRowFromField(int fieldID, int rowID);
			bool						addRow(const std::deque<RowData> &dataForTheFields);
			void						addField(FieldData &Field);
			void						addField(std::string name, fieldType type, size_t size);
			inline const std::string	getName() const{return name;}
			inline const size_t			getNumRows() const{return rows;}
			inline const size_t			getNumFields() const{return Fields.size();}
			wrapper						operator[](const int row);
			inline void					truncate() { rows = 0; loaded = true; for (auto &f : Fields) { f.data.clear(); }; }
			Tabledata(const std::string &tbName, CADBNSQL *MainDBPtr);
		};

		std::deque<Tabledata> Tables;

		void flush(bool useMutex = true);
		uint32_t createTable(const Tabledata &table);
		int getTableByName(std::string name);

		inline Tabledata				&operator[](const std::string &name){
			int tblID = getTableByName(name);
			return Tables[(tblID == -1)? createTable(Tabledata(name, this)) : tblID];
		}

		CADBNSQL(const char *databaseFileName, std::string defaultDataFolder = "SAVE");
		CADBNSQL(std::string defaultDataFolder = "SAVE");
		CADBNSQL();

		~CADBNSQL();
	};
}

#endif