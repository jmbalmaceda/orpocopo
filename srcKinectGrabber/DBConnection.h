#pragma once
#include "mysql.h"
#include <string>

using namespace std;

///estructura para almacenar setting de base de datos
struct recordSettings{
	struct tm* time_table;
	int rec_video;
	int rec_db;
} ; 

class DBConnection
{
private:
	MYSQL *conn;
	MYSQL_RES *res;
	MYSQL_ROW row;
public:
	DBConnection(char* propertyFilePath);
	~DBConnection(void);

	/// Formato ejemplo 2015-06-29 00:43:00
	string getCurrentTimeAsString();

	bool insertPickUpInformation(int frame, int count_blobs, int blob_id, int blob_x, int blob_y, int blob_depth, int blob_hand_id, int blob_hand_x, int blob_hand_y, int blob_hand_depth);

	bool insertPickUpInformation(int frame, int count_blobs, int blob_id, int blob_x, int blob_y, int blob_depth);

	struct recordSettings * startProcessing(struct tm*);
};

