#include "main.h"
#include "Export.h"
#include "IniWriter.h"
#include "IniReader.h"
#include "shopDetector.h"
#include "RoI_Information.h"
#include "DBConnection.h"

#include <Windows.h>
#include <opencv\cv.h>
#include <opencv\highgui.h>

#include <opencv2\opencv.hpp>
#include <vector>

#ifdef KINECT
#include <NuiApi.h>
#include <NuiImageCamera.h>
#include <NuiSkeleton.h>
#include <NuiSensor.h>
#endif

#include <stdio.h>
#include <iostream>	// for standard I/O
#include <string>   // for strings
#include <iomanip>  // for controlling float print precision
#include <sstream>  // string to number conversion
#include <omp.h>
#include <opencv\cv.h>
#include <opencv\highgui.h>
#include <math.h>
#include "DataInput.h"

using namespace cv;
using namespace std;

double sft_clock(void)
{
	
#ifdef _WIN32
	/* _WIN32: use QueryPerformance (very accurate) */
	LARGE_INTEGER freq , t ;
	/* freq is the clock speed of the CPU */
	QueryPerformanceFrequency(&freq) ;
	/* cout << "freq = " << ((double) freq.QuadPart) << endl; */
	/* t is the high resolution performance counter (see MSDN) */
	QueryPerformanceCounter ( & t ) ;
	return ( t.QuadPart /(double) freq.QuadPart ) ;
#else
	/* Unix or Linux: use resource usage */
	struct rusage t;
	double procTime;
	/* (1) Get the rusage data structure at this moment (man getrusage) */
	getrusage(0,&t);
	/* (2) What is the elapsed time ? - CPU time = User time + System time */
	/* (2a) Get the seconds */
	procTime = t.ru_utime.tv_sec + t.ru_stime.tv_sec;
	/* (2b) More precisely! Get the microseconds part ! */
	return ( procTime + (t.ru_utime.tv_usec + t.ru_stime.tv_usec) * 1e-6 ) ;
#endif
}
int maxFrames = 30000 ; 
int frameCount = maxFrames;

int frameSeq = 0;

int counter = 0;
bool saveCSV = false;

bool canProcess = false;
bool first = true;
int recordFreq = 1;
int inputMode = 0;

bool storeRGBVideo = false;
bool storeDepthVideo = false;
char* outputRGBVideo ;
char* outputDepthVideo;

char* dbFilePath;

int frame = 1;

static CvVideoWriter* gbWriterRGB = NULL;
static CvVideoWriter* gbWriterDepth = NULL;

using namespace std;

GLubyte data[width*height*cBytesPerPixel];
GLubyte dataRGB[width*height*cBytesPerPixel];

// Kinect variables
HANDLE depthStream;
HANDLE rgbStream;
INuiSensor* sensor;

HANDLE   m_hNextDepthFrameEvent;
HANDLE   m_pDepthStreamHandle;

HANDLE   m_hNextRGBFrameEvent;
HANDLE   m_pRGBStreamHandle;
BYTE*                   m_depthRGBX;

IplImage* depth ;
IplImage* rgb ;

/*
HRESULT initKinect() {
	// Get a working kinect sensor
	INuiSensor * pNuiSensor;
	HRESULT hr;

	int iSensorCount = 0;
	hr = NuiGetSensorCount(&iSensorCount);
	if (FAILED(hr))
	{
		return hr;
	}

	// Look at each Kinect sensor
	for (int i = 0; i < iSensorCount; ++i)
	{
		// Create the sensor so we can check status, if we can't create it, move on to the next
		hr = NuiCreateSensorByIndex(i, &pNuiSensor);
		if (FAILED(hr))
		{
			continue;
		}

		// Get the status of the sensor, and if connected, then we can initialize it
		hr = pNuiSensor->NuiStatus();
		if (S_OK == hr)
		{
			sensor = pNuiSensor;
			break;
		}

		// This sensor wasn't OK, so release it since we're not using it
		pNuiSensor->Release();
	}

	if (NULL != sensor)
	{
		// Initialize the Kinect and specify that we'll be using depth
		hr = sensor->NuiInitialize(NUI_INITIALIZE_FLAG_USES_DEPTH | NUI_INITIALIZE_FLAG_USES_COLOR ); 
		if (SUCCEEDED(hr))
		{
			// Create an event that will be signaled when depth data is available
			m_hNextDepthFrameEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

			m_hNextRGBFrameEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

			hr = sensor->NuiImageStreamOpen(NUI_IMAGE_TYPE_COLOR, // Depth camera or rgb camera?
				NUI_IMAGE_RESOLUTION_640x480,    // Image resolution
				0,		// Image stream flags, e.g. near mode
				2,		// Number of frames to buffer
				m_hNextRGBFrameEvent,   // Event handle
				&rgbStream);

			// Open a depth image stream to receive depth frames
			hr += sensor->NuiImageStreamOpen(
				NUI_IMAGE_TYPE_DEPTH,
				NUI_IMAGE_RESOLUTION_640x480,
				0,
				2,
				m_hNextDepthFrameEvent,
				&m_pDepthStreamHandle);
		}
	}

	if (NULL == sensor || FAILED(hr))
	{
		//  SetStatusMessage(L"No ready Kinect found!");
		return E_FAIL;
	}
	//sensor = m_pNuiSensor;
	return hr;
}

void getKinectRGBData(GLubyte* dest) 
{
	NUI_IMAGE_FRAME imageFrame;
	NUI_LOCKED_RECT LockedRect;
	if (sensor->NuiImageStreamGetNextFrame(rgbStream, 0, &imageFrame) < 0) return;
	INuiFrameTexture* texture = imageFrame.pFrameTexture;
	texture->LockRect(0, &LockedRect, NULL, 0);
	if (LockedRect.Pitch != 0)
	{
		const BYTE* curr = (const BYTE*) LockedRect.pBits;
		const BYTE* dataEnd = curr + (width*height)*4;
		BYTE * rgbrun = (BYTE*)dest;

		while (curr < dataEnd) 
		{
			for (int i=0; i<3 ;i++)
				*rgbrun++ = *curr++;
			if (cBytesPerPixel == 3 )
				*curr++;
		}
	}
	texture->UnlockRect(0);
	sensor->NuiImageStreamReleaseFrame(rgbStream, &imageFrame);

	cvSetData(rgb,dest,width * cBytesPerPixel);
}
*/

static byte CalculateIntensityFromDistance(int distance,BOOL nearMode)
{
	int  MaxDepthDistanceOffset = 3150;
	// This will map a distance value to a 0 - 255 range
	// for the purposes of applying the resulting value
	// to RGB pixels.
	// Get the min and max reliable depth for the current frame
	int minDepth = (nearMode ? NUI_IMAGE_DEPTH_MINIMUM_NEAR_MODE : NUI_IMAGE_DEPTH_MINIMUM) >> NUI_IMAGE_PLAYER_INDEX_SHIFT;
	int maxDepth = (nearMode ? NUI_IMAGE_DEPTH_MAXIMUM_NEAR_MODE : NUI_IMAGE_DEPTH_MAXIMUM) >> NUI_IMAGE_PLAYER_INDEX_SHIFT;


	int newMax = min( (int)MaxDepthDistanceOffset , (int)(distance - minDepth));
	if (newMax > 0)
		return  static_cast<BYTE>(255 -  ( (255 * newMax) / (MaxDepthDistanceOffset)) );
	else
		return  static_cast<BYTE>(0);
}


/// <summary>
/// Handle new depth data
/// </summary>
void getKinectData(GLubyte* dest) 
{
	HRESULT hr;
	NUI_IMAGE_FRAME imageFrame;

	// Attempt to get the depth frame
	hr = sensor->NuiImageStreamGetNextFrame(m_pDepthStreamHandle, 0, &imageFrame);
	if (FAILED(hr))
	{
		return;
	}

	BOOL nearMode;
	INuiFrameTexture* pTexture;

	// Get the depth image pixel texture
	hr = sensor->NuiImageFrameGetDepthImagePixelFrameTexture(
		m_pDepthStreamHandle, &imageFrame, &nearMode, &pTexture);
	if (FAILED(hr))
	{
		goto ReleaseFrame;
	}

	NUI_LOCKED_RECT LockedRect;

	// Lock the frame data so the Kinect knows not to modify it while we're reading it
	pTexture->LockRect(0, &LockedRect, NULL, 0);

	// Make sure we've received valid data
	if (LockedRect.Pitch != 0)
	{
		// Get the min and max reliable depth for the current frame
		int minDepth = (nearMode ? NUI_IMAGE_DEPTH_MINIMUM_NEAR_MODE : NUI_IMAGE_DEPTH_MINIMUM) >> NUI_IMAGE_PLAYER_INDEX_SHIFT;
		int maxDepth = (nearMode ? NUI_IMAGE_DEPTH_MAXIMUM_NEAR_MODE : NUI_IMAGE_DEPTH_MAXIMUM) >> NUI_IMAGE_PLAYER_INDEX_SHIFT;

		BYTE * rgbrun = (BYTE*)dest;
		const NUI_DEPTH_IMAGE_PIXEL * pBufferRun = reinterpret_cast<const NUI_DEPTH_IMAGE_PIXEL *>(LockedRect.pBits);

		// end pixel is start + width*height - 1
		const NUI_DEPTH_IMAGE_PIXEL * pBufferEnd = pBufferRun + (cDepthWidth * cDepthHeight);

		while ( pBufferRun < pBufferEnd )
		{
			// discard the portion of the depth that contains only the player index
			USHORT depth = pBufferRun->depth;

			// To convert to a byte, we're discarding the most-significant
			// rather than least-significant bits.
			// We're preserving detail, although the intensity will "wrap."
			// Values outside the reliable depth range are mapped to 0 (black).

			// Note: Using conditionals in this loop could degrade performance.
			// Consider using a lookup table instead when writing production code.

			// BYTE intensity = static_cast<BYTE>(depth >= minDepth && depth <= maxDepth ? (255 * depth) / maxDepth : 0);

			// int newMax = depth - NUI_IMAGE_DEPTH_MINIMUM;
			// BYTE intensity = static_cast<BYTE>(depth >= minDepth && depth <= maxDepth ? (255 - (255 * newMax / (3000))) : 0);


			BYTE intensity = CalculateIntensityFromDistance(depth,nearMode);

			// if (depth > minDepth && depth < maxDepth)
			// {
			// Write out blue byte
			*(rgbrun++) = intensity;

			// Write out green byte
			*(rgbrun++) = intensity;

			// Write out red byte
			*(rgbrun++) = intensity;
			// We're outputting BGR, the last byte in the 32 bits is unused so skip it
			// If we were outputting BGRA, we would write alpha here.
			// ++rgbrun;


			// }


			// Increment our index into the Kinect's depth buffer
			++pBufferRun;
		}



	}

	// We're done with the texture so unlock it
	pTexture->UnlockRect(0);

	pTexture->Release();

	cvSetData(depth,dest,width * cBytesPerPixel);


ReleaseFrame:
	// Release the frame
	sensor->NuiImageStreamReleaseFrame(m_pDepthStreamHandle, &imageFrame);
}

void saveVideos(CvVideoWriter* writer,IplImage* frame)
{
	cvWriteFrame(writer, frame);
}



int sequence = 0;
int mode = 1 | 2;
string pathFile = "";
string pathCSV;
string srcFile = "";

void parseCommandLineArguments(int argc, char *argv[])
{
	char* pathF = NULL;



	if (argc >= 1) 
	{
		for (int i=1; i < argc; i++) 
		{
			int bFirstArgIsParam = false;
			int string_start = 0;
			while (argv[i][string_start] == '-')
				string_start++;
			char *string_argv = &argv[i][string_start];

			if (!_stricmp(string_argv, "help")) {

				exit(0);
			}

			if (!_strnicmp(string_argv, "mxFrames",7)) {
				maxFrames = atoi(&string_argv[9]);
				continue;
			}

			if (!_strnicmp(string_argv, "seq",3)) {
				sequence = atoi(&string_argv[5]);
				continue;
			}

			if (!_strnicmp(string_argv, "mode",4)) {
				mode = atoi(&string_argv[6]);
				continue;
			}

			if (!_strnicmp(string_argv, "fpsJump",4)) {
				recordFreq = atoi(&string_argv[6]);
				continue;
			}

			if (!_strnicmp(string_argv, "input",4)) {
				inputMode = atoi(&string_argv[6]);
				continue;
			}

			if (!_strnicmp(string_argv, "path",4)) {
				pathF = &string_argv[5];
				continue;
			}

			if (!_strnicmp(string_argv, "file",4)) {
				srcFile = &string_argv[5];
				continue;
			}


		}
	}

	if (pathF)
	{
		pathFile = pathF;
	}
	else
	{
		std::string filename(argv[0]);

		const size_t last_slash_idx = filename.rfind('\\');
		if (std::string::npos != last_slash_idx)
		{
			pathFile = filename.substr(0, last_slash_idx);
		}
		else
			pathFile = "";
		string cpth(pathFile);
		pathFile = "";

		for (unsigned int i=0 ; i<cpth.length() ; i++)
			if (cpth[i] == '\\') 
				pathFile += '/';
			else
				pathFile+= cpth[i];

	}
	cout<< "Path File " << pathFile <<"\n";
}



bool isInRange(Blob* blob){ 

	//si pertenece al rango 
	if ((blob->maxX < RoI_Information::xMinLineRight) && (blob->minX > RoI_Information::xMinLineLeft)){
		return true;
	}

	//si en el frame anterior estaba en el rango
	if (blob->blobsHistory.size() > 0)
		if (blob->blobsHistory[blob->blobsHistory.size() -1]->visible)
			return true;

	return false;

}

//-----------------------------------------------------------------------------
//  generateOutput : genera la salida a c# en forma de array
//---------------------------------------------------------------------------
int * generateOutput(Vector<Blob*> blobsDetected, int &sizearr){
	/*
	* size = sizeArray + nframe + nBlobs*( id, parentId, rect(xmin,xmax,ymin,ymax), z ) 
	*			1       +  1     +   n * ( 1 +   1     +        1  +  1 + 1 + 1   +  1 )
	*                    2       +   n  * 7
	*/
	int size = 1 ;

	//calculo el tamaño del arreglo
	for (unsigned int i=0; i<blobsDetected.size() ; i++)
	{
		int blobChildSize =  blobsDetected[i]->blobsChild.size();
		size += blobChildSize + 11;
	}

	int *  arrayResult = new int[size];
	arrayResult[0] = frame;

	unsigned int j = 1;
	for (unsigned int i=0; i<blobsDetected.size() ; i++)
	{
		arrayResult[j] = blobsDetected[i]->globalID();
		if (blobsDetected[i]->parent == NULL)
			arrayResult[j+1] = -1;
		else
			arrayResult[j+1] = blobsDetected[i]->parent->globalID();
		arrayResult[j+2] = blobsDetected[i]->minX;
		arrayResult[j+3] = blobsDetected[i]->minY;
		arrayResult[j+4] = blobsDetected[i]->maxX;
		arrayResult[j+5] = blobsDetected[i]->maxY;
		arrayResult[j+6] = blobsDetected[i]->depth;
		arrayResult[j+7] = blobsDetected[i]->visible;
		arrayResult[j+8] = blobsDetected[i]->activated;
		arrayResult[j+9] = blobsDetected[i]->inROIGondola;


		//addchildBlobID
		int blobChildSize =  blobsDetected[i]->blobsChild.size();
		arrayResult[j+10] = blobChildSize;
		for (int k = 0; k < blobChildSize; k++){
			arrayResult[j+11+k] =  blobsDetected[i]->blobsChild[k]->globalID();
		}

		j= j + 11 + blobChildSize;
	}
	sizearr = size;
	return arrayResult;
}




void getBlobHandsInfo(Blob* head,vector<Blob*>* hands){
	for (unsigned int i = 0; i < head->blobsChild.size(); i++) {
		if (head->blobsChild[i]->inROIGondola)
			hands->push_back(head->blobsChild[i]);            
	}
}



DataInput* globalki = NULL;
ShopDetector* globalSD = NULL;
// extern "C" necesario para exportar el metodo como dll
extern "C"{

	IplImage* gdepthImageC ;
	IplImage* grgbImageC ;
	int gframeCount = 0;

	__declspec(dllexport) extern int itsAlive()
	{
	   return 1;
	}
	

	// Inicializa el Kinect
	__declspec(dllexport) extern int initFromKinect(char* rgbVideo, char* depthVideo, bool saveCSV)
	{
		globalki  = new KinectInput(DEPTH_MODE_TRUNC_8_16);
		// Demo 1
	
		globalSD = new ShopDetector(1,"", saveCSV, "", dbFilePath);
		gdepthImageC = cvCreateImage( cvSize(640,480),8,3);
		grgbImageC = cvCreateImage( cvSize(640,480),8,3);

		if (globalki->init()) return 0;
		else return -1;
	}

	// Inicializa desde Video
	__declspec(dllexport) extern int initFromVideo(char* rgbVideo, char* depthVideo, bool saveCSV)
	{
		globalki= new VideoDataInput(rgbVideo, depthVideo);

		globalSD = new ShopDetector(1,"", saveCSV, "c:/temp/orco.CSV", dbFilePath);
		gdepthImageC = cvCreateImage( cvSize(640,480),8,3);
		grgbImageC = cvCreateImage( cvSize(640,480),8,3);

		if (globalki->init()) return 0;
		else return -1;
	}

	__declspec(dllexport) extern int emptyInit()
	{
	   gdepthImageC = cvCreateImage( cvSize(640,480),8,3);
		grgbImageC = cvCreateImage( cvSize(640,480),8,3);
		cvShowImage("test", grgbImageC);
		return 0;
	}
	// Retorna cuantos BLOBS detecto
	__declspec(dllexport) extern int newBlobsDetected()
	{
		return globalSD->newBlobsDetected;
	}

	/// Define el nombre de un nuevo archivo en base a la fecha de creación
	string definirNuevoArchivoDeVideo(string directory, string prefix, string suffix){
		time_t t = time(0);
		struct tm* now  = localtime(&t);
		string result(directory);
		result.append("\\");
		result.append(prefix);
		result.append(to_string(now->tm_year+1900));
		result.append("-");
		result.append(to_string(now->tm_mon+1).c_str());
		result.append("-");
		result.append(to_string(now->tm_mday).c_str());
		result.append("___");
		result.append(to_string(now->tm_hour).c_str());
		result.append("-");
		result.append(to_string(now->tm_min).c_str());
		result.append(suffix);
		return result;
	}

	// Comienza a almacenar el video
	__declspec(dllexport) extern int startStoringRGBVideo(int w, int h)
	{
		int xvidCoded = 1145656920;
		string path = RoI_Information::video_directory_path;
		char newFileName[100];
		string pathVideoAux = definirNuevoArchivoDeVideo(path,"VideoRGB",".avi");
		cout << "Video RGB se va a guardar en "<<pathVideoAux<<"\n\r";
		strcpy(newFileName, pathVideoAux.c_str());
		gbWriterRGB = cvCreateVideoWriter(newFileName, xvidCoded, 20, cvSize((int)w,(int)h), 1);
		if (!gbWriterRGB)  return -1;
		storeRGBVideo = true;
		cout << "Video RGB OK\n\r";
		return 0;
	}

	__declspec(dllexport) extern void stopStoringRGBVideo()
	{
		storeRGBVideo = false;
	}
	

	__declspec(dllexport) extern int startStoringDepthVideo(int w, int h)
	{
		int xvidCoded = 1145656920;
		string path = RoI_Information::video_directory_path;
		char* newFileName = new char[100];
		string pathVideoAux = definirNuevoArchivoDeVideo(path,"VideoDEPTH",".avi");
		cout << "Video DEPTH se va a guardar en "<<pathVideoAux<<"\n\r";
		strcpy(newFileName, pathVideoAux.c_str());
		gbWriterDepth = cvCreateVideoWriter(newFileName, xvidCoded, 20, cvSize((int)w,(int)h), 1);
		if (!gbWriterDepth) return -1;
		storeDepthVideo = true;
		cout << "Video DEPTH OK\n\r";
		return 0;
	}

	__declspec(dllexport) extern void stopStoringDepthVideo()
	{
		storeDepthVideo = false;
	}

	// Actualiza el procesamiento
	__declspec(dllexport) extern void processCapture()
	{

		// MODO PROCESAMIENTO
		if (globalki->readImages() )
		{

			int size;   
			
		//	cvShowImage("input Depth", globalki->iDepth);
		//	cvShowImage("input RGB", globalki->iRGB);
			cvCopy(globalki->iDepth, gdepthImageC);
			cvCopy(globalki->iRGB, grgbImageC);

			if (storeRGBVideo)
			{
				saveVideos(gbWriterRGB , globalki->iRGB);
			}

			if (storeDepthVideo)
			{
				saveVideos(gbWriterDepth , globalki->iDepth);
			}

			globalSD->processBlobs(globalki->iDepth,globalki->iRGB,size, RoI_Information::canDraw, RoI_Information::threshold,gframeCount);
			globalSD->associateBlobsToPickUps(gdepthImageC,  grgbImageC,gframeCount);
			globalSD->discardInvalid(gframeCount);
			

			gframeCount++;
			cvWaitKey(10);

			if  (gframeCount % 100 == 0)		
				std::cout <<" Running frame"<< gframeCount << " Blobs "<<globalSD->allBlobs.size() << " PUs "<< globalSD->allPickUps.size() <<"\n";

		}

	}

	__declspec(dllexport) extern int eof()
	{
		//globalki  = new KinectInput(DEPTH_MODE_TRUNC_8_16);
		return globalki->eof();
	}

	__declspec(dllexport) extern int readDetectedBlobsCount()
	{
		//globalki  = new KinectInput(DEPTH_MODE_TRUNC_8_16);
		return globalSD->peopleblobs.size() ;		 
	}

	// Retorna cuantos PICK UPS se detectaron
	__declspec(dllexport) extern int readPUcount()
	{
		return globalSD->allPickUps.size();
	}

	// Retorna un puntero a un PICK UP
	__declspec(dllexport) extern PickUp* readPU(int index)
	{
		return globalSD->allPickUps[index];
	}

	// Lee propiedades de los PU

	__declspec(dllexport) extern int readPUx(PickUp* pu)
	{
		return pu->x;
	}

	__declspec(dllexport) extern int readPUwidth(PickUp* pu)
	{
		return pu->width;
	}

	__declspec(dllexport) extern int readPUdepth(PickUp* pu)
	{
		return pu->depth;
	}

	// Retorna un puntero a BLOB y se definen metodos para leer atributos
	__declspec(dllexport) extern Blob* readBlob(int index)
	{
		//globalki  = new KinectInput(DEPTH_MODE_TRUNC_8_16);
		return globalSD->peopleblobs[index];		 
	}


	__declspec(dllexport) extern int readBlobID(Blob* b)
	{
		//globalki  = new KinectInput(DEPTH_MODE_TRUNC_8_16);
		return b->globalID();		 
	}

	__declspec(dllexport) extern int readBlobFirstFrame(Blob* b)
	{
		//globalki  = new KinectInput(DEPTH_MODE_TRUNC_8_16);
		if (b->blobsHistory.size() == 0) return 0;		
		else return (b->blobsHistory[0]->frameNumber);	 
	}


	__declspec(dllexport) extern int readBlobHistoryCount(Blob* b)
	{
		//globalki  = new KinectInput(DEPTH_MODE_TRUNC_8_16);
		return b->blobsHistory.size() ;	 
	}

	__declspec(dllexport) extern int readBlobLastFrame(Blob* b)
	{
		//globalki  = new KinectInput(DEPTH_MODE_TRUNC_8_16);
		if (b->blobsHistory.size() == 0) return 0;		
		else return (b->blobsHistory[b->blobsHistory.size()-1]->frameNumber);	 
	}

	__declspec(dllexport) extern int readBlobPathX(Blob* b, int pathIndex)
	{
		//globalki  = new KinectInput(DEPTH_MODE_TRUNC_8_16);
		if (b->blobsHistory.size() == 0) return -1;		
		else return (b->blobsHistory[pathIndex]->posX);	 
	}

	__declspec(dllexport) extern int readBlobPathY(Blob* b, int pathIndex)
	{
		//globalki  = new KinectInput(DEPTH_MODE_TRUNC_8_16);
		if (b->blobsHistory.size() == 0) return -1;		
		else return (b->blobsHistory[pathIndex]->posY);	 
	}

	// Retorna un puntero a lo datos de las imagenes en RGB
	__declspec(dllexport) extern int obtenerRGBFrame(char *data)
	{
		if (!globalki) return -2;

		_IplImage* img = globalki->iRGB;

		if (!img) return -1;

		try
		{
			memcpy(data,img->imageData,img->imageSize);
			return 0;
		}
		catch(...)
		{
			return -3;
		}
	}

	// Retorna un puntero a lo datos de las imagenes en DEPTH
	__declspec(dllexport) extern int obtenerDepthFrame(char *data)
	{
		if (!globalki) return -2;

		_IplImage* img = globalki->getimgDepth();
		if (!img) return -1;

		try
		{
			memcpy(data,img->imageData,img->imageSize);
			return 0;
		}
		catch(...)
		{
			return -3;
		}
	}


	//-----------------------------------------------------------------------------
	//  array_release : para liberar el array generado con los datos de blobs
	//---------------------------------------------------------------------------
	__declspec(dllexport) extern void ReleaseImage(IplImage * img)
	{
		cvReleaseImage(&img);
	}

	//-----------------------------------------------------------------------------
	//  array_release : para liberar el array generado con los datos de blobs
	//---------------------------------------------------------------------------
	__declspec(dllexport) extern void array_release(int* pArray)
	{
		delete[] pArray;
	}

	}


void saveImage(string imagePath){
	// Inicia el análisis desde kinect
	initFromKinect("", "", RoI_Information::save_csv_file);
	for(;;){
		globalki->readImages();
		string title = string("Presione Enter para guardar la imagen en ");
		title = title.append(imagePath);
		cvShowImage(title.c_str(), globalki->iRGB);
		int key = cvWaitKey(100);
		if (key==13)
			break;
	}
	cvSaveImage(imagePath.c_str(), globalki->iRGB);
	cvWaitKey(-1);
}

string imagePathToSave;
bool saveImageToFile = false;
DBConnection* myDBConnection;

void procesarArgumentos(int argc, char* argv[]){
	// Comienzo desde el 1 porque el 0 es el nombre del archivo ejecutado
	for(int i=1;i<argc; i++){
		char* argumento = argv[i];
		if (argumento[0] == '-'){ // Si es un atributo
			switch (argumento[1])
			{
			case 'C': // Path del lugar donde se encuentra el archivo Settings.ini
				RoI_Information(argumento+=2);
				break;
			case 'D': // Path del lugar donde se encuentra el archivo DB.ini
				dbFilePath=argumento+=2;
				myDBConnection = new DBConnection(dbFilePath);
				break;
			case 'S': // Toma una foto desde la kinect
				saveImageToFile = true;
				imagePathToSave = string(argumento+=2);
			default:
				break;
			}
		}
	}
}

/// Compara dos horas
int compareTime(struct tm* t1, struct tm* t2){
	if (t1->tm_hour > t2->tm_hour)
		return 1;
	else if (t1->tm_hour < t2->tm_hour)
		return -1;
	else if (t1->tm_min > t2->tm_min)
		return 1;
	else if (t1->tm_min < t2->tm_min)
		return -1;
	else if (t1->tm_sec > t2->tm_sec)
		return 1;
	else if (t1->tm_sec < t2->tm_sec)
		return -1;
	return 0;
}

/// Inicia el procesamiento hasta que se acaba la entrada o hasta que se llega a la hora de cierre (finish time)
/*
void ejecutar(struct recordSettings* dataRecord){
*/
void ejecutar(struct tm* finish_time){
	if (!RoI_Information::readFromKinect){
		char* rgb_path_aux = new char[RoI_Information::rgb_video_in.length() +1];
		strcpy(rgb_path_aux, RoI_Information::rgb_video_in.c_str());
		char* depth_path_aux = new char[RoI_Information::depth_video_in.length() +1];
		strcpy(depth_path_aux, RoI_Information::depth_video_in.c_str());
		//Inicia el análisis desde video
		initFromVideo(rgb_path_aux, depth_path_aux, RoI_Information::save_csv_file);
	}else{
		int intentos = 0;
		int initKinect_ok = -1;
		do{
			// Inicia el análisis desde kinect
			initKinect_ok = initFromKinect("", "", RoI_Information::save_csv_file);
			if(initKinect_ok == -1){
				intentos++;
				cout << "Error iniciando Kinect (Intento "<<intentos<<")\n";
				Sleep(10000);
			}
		}while(initKinect_ok==-1 && intentos<10);
	}

	//graba solo si rec_video es true
	//if ((dataRecord->rec_video == 1) && RoI_Information::save_video)
	if (RoI_Information::save_video){
		// Grabar los videos en archivos
		startStoringDepthVideo(640,480);
		startStoringRGBVideo(640,480);
	}

	int cantPU = 0;
	time_t now = time(0);
	struct tm* now_time  = localtime(&now);
	while (!eof() && compareTime(finish_time, now_time)>0)
	{
		processCapture();
		cvWaitKey(RoI_Information::sleepTime);
		cantPU = globalSD->allPickUps.size();
		now = time(0);
		now_time  = localtime(&now);
	} 

	//detiene la grabacion solo si rec_video es true
	//if ((dataRecord->rec_video == 1) && RoI_Information::save_video)
	if (RoI_Information::save_video){
		stopStoringDepthVideo();
		stopStoringRGBVideo();
	}
}


///estructura para almacenar setting de base de datos
typedef struct recordSettings{
	struct tm* time_table;
	int rec_video;
	int rec_db;
} ; 

bool salir = false;
/// Bucle que se queda esperando hasta que comience el horario de análisis
void esperarInicio(){
	cout << "Esperando hasta que comience la próxima hora de análisis\n";
	while(!salir){
		time_t t = time(0);
		struct tm* now  = localtime(&t);
		//struct tm* finish = myDBConnection->startProcessing(now);
		
		recordSettings* data_record;
		data_record = myDBConnection->startProcessing(now);
		struct tm* finish = data_record->time_table;

		if (finish != NULL){
			cout << "comienza el análisis hasta las "<<finish->tm_hour<<":"<<finish->tm_min<<" horas\n";
			ejecutar(finish);
			//ejecutar(data_record);
			cout << "Esperando hasta que comience la próxima hora de análisis\n";
		}else{
			//cvWaitKey(60000);
			Sleep(15000);
		}
		cout << ".";
	}
}

int main(int argc, char* argv[]) 
{
	procesarArgumentos(argc, argv);

	if (saveImageToFile){
		saveImage(imagePathToSave);
		exit(0);
	}else{
		esperarInicio();
	}

	return -1;
}
