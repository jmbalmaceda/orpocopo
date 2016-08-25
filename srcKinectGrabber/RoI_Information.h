#pragma once
#include <string>
#include <sys/stat.h>

using namespace std;
class RoI_Information
{
public:
	/// tiempo en milisegundos que esperar entre captura y captura
	static int sleepTime;
	/// ROI de Gondola
	static int xMinROI, yMinROI, xMaxROI, yMaxROI;
	/// Lineas a Derecha
	static int xMinLineRight, yMinLineRight, xMaxLineRight, yMaxLineRight;
	/// Lineas a Izquierda
	static int xMinLineLeft, yMinLineLeft, xMaxLineLeft, yMaxLineLeft;
	/// CVS information
	static bool save_csv_file;
	static string csv_file_path;
	/// Video information
	static bool save_video_rgb;
	static bool save_video_depth;
	static string video_directory_path;
	static string video_rgb_path;
	static string video_depth_path;
	/// Leer desde kinect (true) o desde video (false)
	static bool readFromKinect;
	static string rgb_video_in;
	static string depth_video_in;
	/// Determina si se muestra en pantalla información de los blobs y los caminos
	static bool canDraw;
	///
	static int threshold;
	/// Determina si se debe respetar los horarios de análisis o se ejecuta sin importar el mismo.
	static bool ignoreTimes;


	RoI_Information(char* propertyFilePath);
	~RoI_Information(void);

	/// Verifica si existe el archivo
	inline bool fileCheck (const std::string& name) {
		struct stat buffer;   
		return (stat (name.c_str(), &buffer) == 0); 
	}
};

