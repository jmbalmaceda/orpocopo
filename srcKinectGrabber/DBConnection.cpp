#include "DBConnection.h"
#include "mysql.h"
#include "IniWriter.h"
#include "IniReader.h"
#include <stdio.h>
#include <time.h>
#include <iostream>
#include <cstdlib>
#include <sys/timeb.h>

using namespace std;



DBConnection::~DBConnection(void)
{
	/* close connection */
	mysql_free_result(res);
	mysql_close(conn);
}

DBConnection::DBConnection(char* propertyFilePath)
{
	CIniReader reader = CIniReader(propertyFilePath);
	char *server = reader.ReadString("DB","SERVER","localhost");
	char *user = reader.ReadString("DB","USER","root");
	char *password = reader.ReadString("DB","PASS","");
	char *database = reader.ReadString("DB","DATABASE","orcodb");
  
	int intentos = 0;
	bool connection_ok = false;
	do{
		conn = mysql_init(NULL);

		// Connect to database
		if (!mysql_real_connect(conn, server, user, password, database, 0, NULL, 0)) {
			intentos++;
			fprintf(stderr, "Error conectando a la base de datos (Intento %d)\n%s\n\n", intentos, mysql_error(conn));
			Sleep(10000);
			//system("pause");
			//exit(1);
		}else{
			connection_ok = true;
		}
	}while(!connection_ok && intentos < 10);
}


int getMillis(){
	timeb tb;
	ftime(&tb);
	return tb.millitm;
}

string DBConnection::getCurrentTimeAsString(){
		time_t t = time(0);
		const long double systime = time(0);
		struct tm* now  = localtime(&t);
		string result(to_string(now->tm_year+1900));
		result.append("-");
		result.append(to_string(now->tm_mon+1).c_str());
		result.append("-");
		result.append(to_string(now->tm_mday).c_str());
		result.append(" ");
		result.append(to_string(now->tm_hour).c_str());
		result.append(":");
		result.append(to_string(now->tm_min).c_str());
		result.append(":");
		result.append(to_string(now->tm_sec).c_str());
		result.append(".");
		result.append(to_string(getMillis()).c_str());
		return result;
	}

bool DBConnection::insertPickUpInformation(int frame, int count_blobs, int blob_id, int blob_x, int blob_y, int blob_depth, int blob_hand_id, int blob_hand_x, int blob_hand_y, int blob_hand_depth){
		string sql("INSERT INTO `pickup` (`frame`, `current_time`, `count_blobs`, `blob_id`, `blob_x`, `blob_y`, `blob_depth`, `blob_hand_id`, `blob_hand_x`, `blob_hand_y`, `blob_hand_depth`) VALUES (");
		sql.append(to_string(frame).c_str());
		sql.append(", '");
		sql.append(getCurrentTimeAsString());
		sql.append("', ");
		sql.append(to_string(count_blobs).c_str());
		sql.append(", ");
		sql.append(to_string(blob_id).c_str());
		sql.append(", ");
		sql.append(to_string(blob_x).c_str());
		sql.append(", ");
		sql.append(to_string(blob_y).c_str());
		sql.append(", ");
		sql.append(to_string(blob_depth).c_str());
		sql.append(", ");
		sql.append(to_string(blob_hand_id).c_str());
		sql.append(", ");
		sql.append(to_string(blob_hand_x).c_str());
		sql.append(", ");
		sql.append(to_string(blob_hand_y).c_str());
		sql.append(", ");
		sql.append(to_string(blob_hand_depth).c_str());
		sql.append(");");
		if (mysql_query(conn, sql.c_str())){
			return false;
		}
	}

	bool DBConnection::insertPickUpInformation(int frame, int count_blobs, int blob_id, int blob_x, int blob_y, int blob_depth){
		string sql("INSERT INTO `pickup` (`frame`, `current_time`, `count_blobs`, `blob_id`, `blob_x`, `blob_y`, `blob_depth`) VALUES (");
		sql.append(to_string(frame).c_str());
		sql.append(", '");
		sql.append(getCurrentTimeAsString());
		sql.append("', ");
		sql.append(to_string(count_blobs).c_str());
		sql.append(", ");
		sql.append(to_string(blob_id).c_str());
		sql.append(", ");
		sql.append(to_string(blob_x).c_str());
		sql.append(", ");
		sql.append(to_string(blob_y).c_str());
		sql.append(", ");
		sql.append(to_string(blob_depth).c_str());
		sql.append(");");
		if (mysql_query(conn, sql.c_str())){
			return false;
		}
	}

	/// Dada un horario, obtiene en la base de datos si est� en un periodo de tiempo en el cual hay que analizar.
	/// Si esto es as�, devuelve hasta qu� hora hay que analizar. Si no se est� en un horario de an�lisis, devuelve null;
	struct tm* DBConnection::startProcessing(struct tm* time){
		struct tm* salida = NULL;
		string sql("select finish from times where week_day=");
		sql.append(to_string(time->tm_wday));
		sql.append(" and start <= '");
		sql.append(to_string(time->tm_hour));
		sql.append(":");
		sql.append(to_string(time->tm_min));
		sql.append("' and finish > '");
		sql.append(to_string(time->tm_hour));
		sql.append(":");
		sql.append(to_string(time->tm_min));
		sql.append("';");

		if (mysql_query(conn, sql.c_str())) {
			fprintf(stderr, "Error consultando a la base de datos: %s\n", mysql_error(conn));
		}else{
			res = mysql_use_result(conn);

			while ((row = mysql_fetch_row(res)) != NULL){
				salida = new tm();
				// Ac� estoy esperando algo del tipo hh:mm:ss
				// printf("%s\n", row[0]);
				const char* finish_str = row[0];
				char* h = new char[2];
				char* m = new char[2];
				char* s = new char[2];
				strncpy(h, finish_str, 2);
				salida->tm_hour = stoi(h);
				strncpy(m, finish_str+=3, 2);
				salida->tm_min = stoi(m);
				strncpy(s, finish_str+=3, 2);
				salida->tm_sec = stoi(s);
			}
		}
		return salida;
	}