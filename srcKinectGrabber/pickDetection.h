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

#include <opencv2\opencv.hpp>


using namespace std;
using namespace cv;


#define SRC_RGB 1
#define SRC_DEPTH 0

class PickCells
{
public : 
	cv::Rect r ; 
	int depth ; 
} ;



class PickUp
{

public: 
	cv::Rect r;	
	CvPoint* cps;
	int index;
	int id ;
	int x, width, depth, frameNro,fFrame, lFrame;

	
	PickUp( ) ;

	int contains(CvRect br)
	{
		bool noOverlap = this->r.x > (br.x + br.width) ||
                         br.x > (this->r.x + this->r.width) ;

       return !noOverlap;
	}


	void add(int x, int t) 
	{ 		
	//	cps[index] = cvPoint(x,t);
		//index++;
		// es la primer vez
		if (r.height == 0)
		{
			r.height =1;
			r.width = 1;
			r.x = x;
			r.y = t;
		}
		else
		{
			int mnx = MIN(r.x, x);
			int mxx = MAX(r.width+r.x, x);
			if (r.y != t) 
				r.height = r.height+1;
			r.x = mnx;
			r.width = mxx - mnx;
		}		
     };

	void addRect(CvRect rn, int t) 
	{ 		
		for (int x=0; x<rn.width;x++)
			this->add( x + rn.x , t);
				
    };
} ;

class PickUpHistory
{
public :
	int firstFrame, lastFrame;
	vector<PickUp*> vpu ;
	void start(int ifr) {  firstFrame = ifr ; }
	void close(int ifr) {  lastFrame = ifr ; }

	int belongs(CvRect r , int frame)
	{
		if (vpu.size() ==0) return 0;
		if (frame > lastFrame) return 0;
		PickUp* vf = vpu[frame - firstFrame];
		// No tiene nada registrado
		if ( vf->contains(r) ) 
			return 1;

		return 0;

	}
	

};

struct Bucket 
{
	int mask, history, index;
};

class PickUpDetector
{
public :
	CvRect roi; 

	Bucket* buckets;
	int nFrame ;
	int nBuckets;	
	// Lista de todos los pickUps posibles
	vector<PickUp*> vs ;
	// Lista ordenada en el tiempo de los pickUps
	vector<vector<PickUp*> > vall ;
	IplImage* imgToRender;
	IplImage* imgToAnalize;
	IplImage* lastFrame;
	IplImage* roiImage;
	IplImage* roiImageD, *meanImageD, *diffImageD;

	string path;

	string name ; 

	int sourceImage ; 
	int idRegion ;

	cv::BackgroundSubtractorMOG2 bg;
	  cv::Mat frame;
		 cv::Mat frameRZ ;
		 cv::Mat fore;
		 cv::Mat back;
       

	PickUpDetector(string pth, CvRect iroi, string _name, int id) 
	{ 
		name = _name;
		nBuckets = 160;		
		buckets = new Bucket[nBuckets];
		
		for (int i=0; i<nBuckets ; i++) 
		{
				buckets[i].history = 0;
				buckets[i].index = -1;
		}

		nFrame = 0;
		roi = iroi;
		this->idRegion = id;
		imgToRender = cvCreateImage(cvSize(640, 40),8,3);	
		imgToAnalize= cvCreateImage(cvSize(iroi.width, iroi.height),8,3);	
		path = pth+".PU.CSV";
		this->sourceImage = SRC_RGB;
		this->roiImage = NULL;
		this->roiImageD = NULL;
	}

	int isOccupied(IplImage* img , int x, int nchannels, int threshold)
	{

		int jmp = img->width / nBuckets;
		int counter = 0;
		int meanD = 0;
		
		for (int yy=0; yy<img->height;yy++)
			{
				uchar* row =  (uchar*)( img->imageData + yy * img->widthStep ) ;
		        for (int xx =0; xx<jmp;xx++)
				   if (row[nchannels * (x *jmp + xx)] >threshold) 
					   {
						   counter++;
						   meanD += row[nchannels * (x *jmp + xx)];

				         }
			}
		if (counter < 20) return 0 ;
		else return meanD /counter ;
	}

	PickUp* findPreviousPU(CvRect r , int frame)
	{
		// es el primer frame
		if (frame <0) return NULL;
		vector<PickUp*> vf = vall[frame];
		// No tiene nada registrado
		if (vf.size() ==0) return NULL;

		for (unsigned int ib = 0; ib<vf.size();ib++)
		{
			// estan superpuestos
			if ( vf[ib]->contains(r) ) return vf[ib];
		}
		// no encontro nada
		return NULL;
	}
	IplImage* ExtractBackgroundFromRGB(IplImage* imgRGB);
	void onNewFrame(IplImage* imgD,IplImage* imgRGB,int frameNro, vector<PickUp*>* allPU)	;
	 vector<PickCells*> onNewFrameD(IplImage* imgD,IplImage* imgRGB,int frameNro, vector<PickUp*>* allPU)	;


	
	void draw()
	{
		cvRectangle(imgToRender, cvPoint(0,0),cvPoint(imgToRender->width, imgToRender->height),cvScalar(0,0,0));
		int acum[640];
		for (int i=0; i<640 ; i++)
		{
			acum[i] = 0;
		}

		for (unsigned int i=0; i<vs.size() ; i++)
		{
			PickUp* pu = vs[i];

			int iID = pu->id;
			for (int x = 0 ; x<pu->r.width ; x++)
				acum[ pu->r.x + x]++;
		   }

		for (int i=0; i<640 ; i++)
		{
			 CvScalar c = cvScalar( acum[i], acum[i], acum[i]);   
			 
			 cvRectangle(imgToRender, cvPoint(i,0),cvPoint(i+1,imgToRender->height),c,3);
		
		}

		cvShowImage(name.c_str(), imgToRender);

	}
    

} ;


