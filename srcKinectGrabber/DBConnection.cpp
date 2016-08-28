#include "DBConnection.h"
#include "mysql.h"
#include "IniWriter.h"
#include "IniReader.h"
#include <stdio.h>
#include <time.h>
#include <iostream>
#include <cstdlib>
#include <sys/timeb.h>
#include "RoI_Information.h"

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

string DBConnection::getLastPickupId(){
	string sql("SELECT max(`id`) from `pickup`;");
	if (mysql_query(conn, sql.c_str())){
			return NULL;
		}
	res = mysql_use_result(conn);

	string salida = string();
	while ((row = mysql_fetch_row(res)) != NULL){
		if (row[0])
			salida = string(row[0]);
		else
			salida = "0";
	}
	mysql_free_result(res);
	return salida;
}

void replaceAll(std::string& str, const std::string& from, const std::string& to) {
    if(from.empty())
        return;
    size_t start_pos = 0;
    while((start_pos = str.find(from, start_pos)) != std::string::npos) {
        str.replace(start_pos, from.length(), to);
        start_pos += to.length(); // In case 'to' contains 'from', like replacing 'x' with 'yx'
    }
}

bool DBConnection::insertLog(string log){
	string maxId = getLastPickupId();
	string sql("INSERT INTO `log` (`fecha`, `texto`, `last_pickup_id`, `Video_rgb`, `Video_depth`) VALUES ('");	
	sql.append(getCurrentTimeAsString());
	sql.append("', '");
	sql.append(log.c_str());
	sql.append("', ");
	sql.append(maxId);
	sql.append(", '");
	string aux = string(RoI_Information::video_rgb_path);
	replaceAll(aux, "\\","\\\\");
	sql.append(aux);
	sql.append("', '");
	string aux2 = string(RoI_Information::video_depth_path);
	replaceAll(aux2, "\\","\\\\");
	sql.append(aux2);
	sql.append("');");
	if (mysql_query(conn, sql.c_str())){
		fprintf(stderr, "Error consultando a la base de datos: %s\n", mysql_error(conn));
	}
	return true;
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
		return true;
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
		return true;
	}

	/// Dada un horario, obtiene en la base de datos si está en un periodo de tiempo en el cual hay que analizar.
	/// Si esto es así, devuelve hasta qué hora hay que analizar. Si no se está en un horario de análisis, devuelve null;
	struct tm * DBConnection::startProcessing(struct tm* time){
		struct tm* timetable = NULL;
		int rec_video_rgb = 0;
		int rec_video_depth = 0;
		int rec_db = 0;
		string sql("select finish, rec_video_rgb, rec_video_depth, rec_db from times where week_day=");
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
				timetable = new tm();
				// Acá estoy esperando algo del tipo hh:mm:ss
				//printf("%s\n", row[0]);

				const char* finish_str = row[0];
				char* h = new char[2];
				char* m = new char[2];
				char* s = new char[2];
				strncpy(h, finish_str, 2);
				timetable->tm_hour = stoi(h);
				strncpy(m, finish_str+=3, 2);
				timetable->tm_min = stoi(m);
				strncpy(s, finish_str+=3, 2);
				timetable->tm_sec = stoi(s);

				///copio la segunda variable de la consulta, rec_video_rgb
				const char* str_video_rgb = row[1];
				char* vid_rgb = new char[1];
				strncpy(vid_rgb, str_video_rgb, 1);
				rec_video_rgb = stoi(vid_rgb);
				
				///copio la segunda variable de la consulta, rec_video_depth
				const char* str_video_depth = row[2];
				char* vid_depth = new char[1];
				strncpy(vid_depth, str_video_depth, 1);
				rec_video_depth = stoi(vid_depth);

				///copio la segunda variable de la consulta, rec_video
				const char* str_db = row[3];
				char* db = new char[1];
				strncpy(db, str_db, 1);
				rec_db = stoi(db);

			}
		}


		RoI_Information::save_video_rgb = rec_video_rgb == 1;
		RoI_Information::save_video_depth = rec_video_depth == 1;
		RoI_Information::save_csv_file = rec_db == 1;
		return timetable;
	}