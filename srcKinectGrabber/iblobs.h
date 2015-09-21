#include <iostream>	// for standard I/O
#include <string>   // for strings
#include <iomanip>  // for controlling float print precision
#include <sstream>  // string to number conversion
#include <omp.h>
#include <opencv\cv.h>
#include <opencv\highgui.h>
#include <string>
#include <math.h>
#include <ctime>

using namespace std;

#define DEPTH_TOLERANCE 10
#define MIN_BLOB_SIZE 400 // 100 pixeles
#define MATCHING_TOLERANCE 150 // Distancia de 200 pixeles
#define DELAY_TIME 50 // Tiempo de espera entre frames
#define TAG_HEAD 1
#define TAG_SHOULDER 2
#define TAG_ABDOMINAL 3
#define TAG_DOWN 4
#define TAG_HAND 10
#define SAVE_IF_LENGTH 20


class Blob
{
private : 
	int gID ;

public :
	static int globalIDGenerator ;
	int minX, maxX, minY, maxY ;
	int  lID;
	bool saveCapture;
	IplImage* imgCapture;
	
	int depth ;
	bool visible;
	bool inROIGondola;
	int posX, posY;
	int numPixels ; 
	int activated ; 
	Blob* bestMatch ; 
	int flag;
	int tag;
	double bestMatchValue;
	int frameNumber;
	time_t frameDate;
	
	std::vector<Blob*> blobsHistory;
	std::vector<Blob*> blobsChild;
	Blob* parent;
	int Blob::globalID() ;
	Blob(int frameNum)  ;
	
	void  calcCenter();
	
	double historyLength(int index);
	
	void addPixel(int x, int y);
	
	int contains(Blob* b);

	int isJoined(Blob* b);
	
	double match(Blob* b);
	
	void dotag();
	
	int computeAngle();

	void  setROIGondola(int xMin, int yMin, int xMax, int yMax);

	static void Blob::setROIBlobs(int xMinR,int yMinR,int xMaxR,int yMaxR);
	
};


 

void matchBlobs(vector<Blob*> newBlobs, vector<Blob*>* allBlobs, int xMinR, int yMinR, int xMaxR, int yMaxR);
void updateBlobsState(vector<Blob*>* allBlobs);