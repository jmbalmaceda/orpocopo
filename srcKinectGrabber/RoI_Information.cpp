#include "RoI_Information.h"
#include "IniWriter.h"
#include "IniReader.h"
#include <iostream>

using namespace std;
	int RoI_Information::sleepTime = 200;
    int RoI_Information::xMinROI = -1;
	int RoI_Information::xMaxROI = -1;
	int RoI_Information::yMinROI = -1;
	int RoI_Information::yMaxROI = -1;

	int RoI_Information::xMinLineLeft = -1;
	int RoI_Information::xMaxLineLeft = -1;
	int RoI_Information::yMinLineLeft = -1;
	int RoI_Information::yMaxLineLeft = -1;

	int RoI_Information::xMinLineRight = -1;
	int RoI_Information::xMaxLineRight = -1;
	int RoI_Information::yMinLineRight = -1;
	int RoI_Information::yMaxLineRight = -1;

	bool RoI_Information::save_csv_file = true;
	string RoI_Information::csv_file_path = "orco.csv";

	bool RoI_Information::save_video_rgb = true;
	bool RoI_Information::save_video_depth = false;
	string RoI_Information::video_directory_path = "videos";
	string RoI_Information::rgb_video_in = "video_RGB.avi";
	string RoI_Information::depth_video_in = "video_DEPTH.avi";

	bool RoI_Information::readFromKinect = true;

	bool RoI_Information::canDraw = false;
	int RoI_Information::threshold = 120;
	bool RoI_Information::ignoreTimes = false;

RoI_Information::RoI_Information(char *propertyFilePath)
{
	if (!fileCheck(propertyFilePath)){
		cout << "No existe el archivo: " << propertyFilePath;
		exit(EXIT_FAILURE);
	}
	//read .ini file
	CIniReader iniReader(propertyFilePath);
	sleepTime = iniReader.ReadInteger("TIMES", "sleepTime", 200);
	ignoreTimes = iniReader.ReadBoolean("TIMES", "ignoreTimes", false);
	xMaxROI = iniReader.ReadInteger("ROI", "xMaxROI", 0);
	xMinROI = iniReader.ReadInteger("ROI", "xMinROI", 0);
	yMinROI = iniReader.ReadInteger("ROI", "yMinROI", 0);
	yMaxROI = iniReader.ReadInteger("ROI", "yMaxROI", 0);

	xMinLineLeft = iniReader.ReadInteger("Setting", "xMinLineLeft", 0);
	xMaxLineLeft = iniReader.ReadInteger("Setting", "xMaxLineLeft", 0);
	yMinLineLeft = iniReader.ReadInteger("Setting", "yMinLineLeft", 0);
	yMaxLineLeft = iniReader.ReadInteger("Setting", "yMaxLineLeft", 0);

	xMinLineRight = iniReader.ReadInteger("Setting", "xMinLineRight", 0);
	xMaxLineRight = iniReader.ReadInteger("Setting", "xMaxLineRight", 0);
	yMinLineRight = iniReader.ReadInteger("Setting", "yMinLineRight", 0);
	yMaxLineRight = iniReader.ReadInteger("Setting", "yMaxLineRight", 0);

	canDraw = iniReader.ReadBoolean("Image", "canDraw", true);
	threshold = iniReader.ReadInteger("Image", "threshold", 120);

	save_csv_file = iniReader.ReadBoolean("CSV","SAVE_CSV_FILE",false); // sacar
	csv_file_path = iniReader.ReadString("CSV","CSV_FILE_PATH", "orco.csv"); // sacar

	//save_video = iniReader.ReadBoolean("VIDEO","SAVE_VIDEO",true);
	video_directory_path = iniReader.ReadString("VIDEO","VIDEO_DIRECTORY_PATH","c:\\Users\\cito\\orcoVideos");
	rgb_video_in = iniReader.ReadString("VIDEO", "VIDEO_RGB_IN", "video_RGB.avi");
	depth_video_in = iniReader.ReadString("VIDEO", "VIDEO_DEPTH_IN", "video_DEPTH.avi");

	readFromKinect = iniReader.ReadBoolean("KINECT", "READ_FROM_KINECT", true);
}


RoI_Information::~RoI_Information(void)
{
}
