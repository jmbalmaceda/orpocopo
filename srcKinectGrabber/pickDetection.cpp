#include "Export.h"
#include "pickDetection.h"

#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include <iostream>
#include <stdio.h>
#include <stdlib.h>

using namespace cv;
using namespace std;

#include <string>   // for strings
#include <sstream>  // string to number conversion
#include <omp.h>


int puID = 0;



PickUp::PickUp() 
{ 
	r = cvRect(0,0,0,0); 
	cps = new CvPoint[10000];
	index=0;	

	this->id = puID;

	puID++;

}

IplImage* PickUpDetector::ExtractBackgroundFromRGB(IplImage* imgRGB)
{
	int xMinROI = roi.x;
	int xMaxROI = roi.width;
	int yMinROI = roi.y;
	int yMaxROI = roi.height + roi.y;

	frame = imgRGB;
	frameRZ = frame(cv::Rect(roi.x,roi.y, roi.width, roi.height));
	bg.operator ()(frameRZ,fore);
	bg.getBackgroundImage(back);

	cv::erode(fore,fore,cv::Mat());
	cv::dilate(fore,fore,cv::Mat());
	//cv::findContours(fore,contours,CV_RETR_EXTERNAL,CV_CHAIN_APPROX_NONE);
	//cv::drawContours(frame,contours,-1,cv::Scalar(0,0,255),2);
	//cv::imshow("Frame",frameRZ);
	//cv::imshow("Background",back);

	IplImage* roiImage = cvCloneImage(&(IplImage)fore);
	return roiImage;
}

int readPixelValue(IplImage* img, int x, int y, int nchannel)
{
	uchar* row =  (uchar*)( img->imageData + y * img->widthStep ) ;
	return row[nchannel*x];   
}

void setPixelValue(IplImage* img, int x, int y, int nchannel, int value)
{
	uchar* row =  (uchar*)( img->imageData + y * img->widthStep ) ;
	row[nchannel*x] = value; 
	row[nchannel*x+1] = value; 
	row[nchannel*x+2] = value; 

}
vector<PickCells*> PickUpDetector::onNewFrameD(IplImage* imgD,IplImage* imgRGB,int frameNro, vector<PickUp*>* allPU)	
{

	int xMinROI = roi.x;
	int xMaxROI = roi.width;
	int yMinROI = roi.y;
	int yMaxROI = roi.height + roi.y;
	CvRect rect = cvRect(xMinROI, yMinROI+5, xMaxROI -xMinROI , yMaxROI-yMinROI);
	vector<PickCells*> res(0);
	if (!roiImageD)
	{
		roiImageD= cvCreateImage( cvSize(xMaxROI -xMinROI , yMaxROI-yMinROI) , 8, imgD->nChannels);
		meanImageD= cvCreateImage( cvSize(xMaxROI -xMinROI , yMaxROI-yMinROI) , 8, imgD->nChannels);
		diffImageD= cvCreateImage( cvSize(xMaxROI -xMinROI , yMaxROI-yMinROI) , 8, imgD->nChannels);
		cvXor(meanImageD,meanImageD,meanImageD);
		cvXor(diffImageD,diffImageD,diffImageD);
	}

	cvSetImageROI( imgD, rect ); 
	cvCopy(imgD, roiImageD); 
	cvResetImageROI( imgD);

	int maxHeight = 0;
	if (frameNro<150)
	{
		cvDilate(roiImageD,roiImageD,0,2);
		cvMax(roiImageD,meanImageD,meanImageD);
	//	cvShowImage("Fore Depth",roiImageD);
	//	cvShowImage("Max Depth",meanImageD);
	//	cvShowImage("Diff Depth",diffImageD);
		return res;
	}
	else
	{
		for (int x = 0 ; x < roiImageD->width ; x++)
			for (int y = 0 ; y < roiImageD->height ; y++)
			{
				int dh =  readPixelValue( roiImageD,x,y,3) - readPixelValue( meanImageD,x,y,3);
				if ( dh > 0)
				{
					maxHeight = MAX(maxHeight , readPixelValue( roiImageD,x,y,3));
					setPixelValue(diffImageD,x,y,3,255);
				}
				else
					setPixelValue(diffImageD,x,y,3,0);
			}

	}


	//cvShowImage("Fore Depth",roiImageD);
	//cvShowImage("Max Depth",meanImageD);
	cvErode(diffImageD,diffImageD,0,1);
	cvDilate(diffImageD,diffImageD,0,1);

	//cvShowImage("Diff Depth",diffImageD);

	vector<vector<Point> > contours;
	vector<Vec4i> hierarchy;

	/// Detect edges using canny
	Mat canny_output ;

	Canny( Mat(diffImageD), canny_output, 1.0, 2.0, 3 );
	/// Find contours
	findContours(  canny_output, contours, hierarchy, CV_RETR_TREE, CV_CHAIN_APPROX_SIMPLE, Point(0, 0) );

	/// Approximate contours to polygons + get bounding rects and circles
	vector<vector<Point> > contours_poly( contours.size() );
	vector<Rect> boundRect( contours.size() );

	for( int i = 0; i < contours.size(); i++ )
	{ 
		approxPolyDP( Mat(contours[i]), contours_poly[i], 3, true );
		boundRect[i] = boundingRect( Mat(contours_poly[i]) );
	}


	/// Draw polygonal contour + bonding rects + circles
	Mat drawing = Mat::zeros( canny_output.size(), CV_8UC3 );
	for( int i = 0; i< contours.size(); i++ )
	{
		Scalar color = Scalar( 255,0,0 );
		if (boundRect[i].area() < 50 ) continue;
		
		rectangle( drawing, boundRect[i].tl(), boundRect[i].br(), color, 2, 8, 0 );
		PickCells* pc = new PickCells();
		pc->r = boundRect[i];
		pc->depth = maxHeight;
		res.push_back(pc);
	}

	/// Show in a window
//	namedWindow( "Contours", CV_WINDOW_AUTOSIZE );
//	imshow( "Contours", drawing );
	return res;
}

void PickUpDetector::onNewFrame(IplImage* imgD,IplImage* imgRGB,int frameNro, vector<PickUp*>* allPU)	
{
	int xMinROI = roi.x;
	int xMaxROI = roi.width;
	int yMinROI = roi.y;
	int yMaxROI = roi.height + roi.y;
	nFrame = frameNro;

	CvRect rect = cvRect(xMinROI, yMinROI+5, xMaxROI -xMinROI , yMaxROI-yMinROI);


	// if (sourceImage == SRC_DEPTH)
	{
		if (!roiImageD)
			roiImageD= cvCreateImage( cvSize(xMaxROI -xMinROI , yMaxROI-yMinROI) , 8, imgD->nChannels);  

		cvSetImageROI( imgD, rect ); 
		cvCopy(imgD, roiImageD); 
		cvResetImageROI( imgD); 
	}
	//else
	{
		roiImage = ExtractBackgroundFromRGB(imgRGB);
		//   cvThreshold(roiImage , roiImage, 160, 255,CV_THRESH_TOZERO);//160
		//   cvErode(roiImage,roiImage,0,2);
		//   cvDilate(roiImage,roiImage,0,5);
	}
	//	cvShowImage("Gondola",roiImage);


	PickUp* pu = NULL;
	// Agrego este frame a la lista
	// vector<PickUp*> vFrame;
	//vall.push_back(vFrame);

	for (int b = 0; b<nBuckets ; b++)
		buckets[b].mask = 0;

	int blobID = 0;
	int maxDepth = 0;

	// Detecto los blobs
	for (int x = 0 ; x<nBuckets ; x++)
	{
		int meanD =isOccupied(roiImage, x, roiImage->nChannels, 160 ); 
		if ( meanD)
		{
			if (x<=1) { blobID =1; continue; }

			if (buckets[x-1].mask > 0) 
				buckets[x].mask = buckets[x-1].mask;
			else
			{
				blobID++;
				buckets[x].mask = blobID;
			}
			maxDepth = meanD;
			buckets[x].history ++;
		}
		else
		{
			if (buckets[x].history>0)
			{
				buckets[x].history--;
				if (buckets[x].history==0)
					buckets[x].index = -1;
			}
		}
	}

	if ( (blobID<1) || (nFrame<20) ){ cvReleaseImage(&roiImage); return; }

	// #ifdef _DEBUG
	//cvShowImage("Fore",roiImage);

	//cvShowImage("Fore Depth",roiImageD);
	//cvWaitKey(-1);
	//#endif

	// Los uno con los blobs historicos
	for (int x = 0 ; x<nBuckets ; x++)
	{
		if (buckets[x].history > 20)
		{


			// Creo uno nuevo
			if (buckets[x].index<0)
			{
				pu = new PickUp();

				pu->x = x*4;
				pu->width = 4;
				pu->depth = this->idRegion * 40;
				pu->frameNro = frameNro;
				vs.push_back(pu);

				buckets[x].index = allPU->size();
				allPU->push_back(pu);
				int ix = x;
				x++;
				// Calculo el rectangulo que lo contiene
				while ((x<nBuckets) && (buckets[x].mask>0) )
				{
					pu->width+= 4;
					buckets[x].history =  buckets[ix].history;
					buckets[x].index =  buckets[ix].index;
					x++;
				}

			}
		}


	}


#ifdef _DEBUG
	//	this->draw();
#endif
	//puD->draw();
	if (roiImage)
		cvReleaseImage(&roiImage);
}
