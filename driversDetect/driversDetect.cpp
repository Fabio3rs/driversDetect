// driversDetect.cpp : Defines the entry point for the console application.
//

#include "CADBNSQL.h"
#include "CText.h"
#include "CDrivers.h"
#include <iostream>
#include "CClient.h"
#include "CServer.h"


int main(int argc, char *argv[])
{
	srand(time(0) + (~time(0)));
	{

		CText config("Config.txt",
			"host localhost\r\n"
			"port 7777\r\n"
			"driversFolderToScan drvsfolder\r\n\r\n", true);

		if (argc > 1)
		{
			DB::CADBNSQL db("myDB");

			int tablesSize = db.Tables.size();

			DB::CADBNSQL::Tabledata newTable("driversTable", &db);

			uint32_t table = db.createTable(newTable);
			int tablesSizeNew = db.Tables.size();

			if (tablesSizeNew != tablesSize) {
				std::cout << table << std::endl;
				db.Tables[table].addField("ID", DB::fieldType::integer, 11);
				db.Tables[table].addField("HWID", DB::fieldType::varchar, 128);
				db.Tables[table].addField("PATH", DB::fieldType::varchar, 4096);
			}

			CDrivers dvrs(db);




			bool detect = false, canDetect = true;
			if (std::string(argv[1]) == "-detect") {
				std::cout << "Detectando drivers das pastas\n";

				if (db.Tables[table].getNumRows() > 0) {
					std::cout << "Ja existe dados nessa tabela, deseja apagar tudo e redetectar? [[Y/N]/[S/N]]\n";

					int ch = 0;

					do {
						ch = getchar();
					} while (ch != 'Y' && ch != 'S' && ch != 'N');

					if (ch == 'Y' || ch == 'S') {
						db.Tables[table].truncate();
						std::cout << "Tabela de drivers limpa\n";
					}
					else {
						canDetect = false;
					}
				}

				std::cout << "Numero de registros: " << db.Tables[table].getNumRows() << std::endl;

				if (canDetect) {
					dvrs.search(config[""]["driversFolderToScan"]);

					std::cout << "Detectados\n";
				}
				else {
					std::cout << "A redetecção não foi feita\n";
				}
				detect = true;
			}

			if (std::string(argv[1]) == "-server" || detect)
			{
				CServer serv(config[""]["port"], dvrs);

				std::cout << "Digite sair para finalizar" << std::endl;

				std::string str;

				do
				{
					std::cin >> str;
				} while (str != "sair");
			}
		}
		else {
			CClient cli(config[""]["host"], config[""]["port"]);

			cli.load();
			cli.send();

			std::this_thread::sleep_for(std::chrono::seconds(2));

			std::cout << "Digite sair para finalizar" << std::endl;

			std::string str;

			do
			{
				std::cin >> str;
			} while (str != "sair");
		}
	}

	system("pause");
    return 0;
}

