#include "iblobs.h"
#include "pickDetection.h"
#include "RoI_Information.h"
#include "DBConnection.h"

#include <opencv\cv.h>
#include <opencv\highgui.h>

using namespace cv;


struct point 
{ 
public : 
	int x,y ; 
};

const std::string currentDateTime();
const std::string currentDateTime(char* format);


class ShopDetector
{
public : 

	DBConnection* dbConnection;
	vector<PickUpDetector*> puDs;
	vector<PickUp*> allPickUps;
	std::vector<Blob*> allBlobs;
	std::vector<Blob*>  blobs;
	std::vector<Blob*>  peopleblobs;
	point* stack;
	int newBlobsDetected ; 
	int possiblePeople ;
	CvFont font;
	bool invert1;

	bool saveCSV ;
	string pathCSV;

	void  getPotentialsParents(vector<Blob*> heads, vector<Blob*> potentialHands);
	void processBlobs(IplImage *imgD,IplImage *imgRGB,int &size, bool canDraw, int threshold, int numFrame);

	void getHeads(vector<Blob*>* heads);
	void  getBlobHands(vector<Blob*> potentialHands,vector<Blob*>* hands);
	void setBlobsFamily(vector<Blob*>* blobs);
	void saveAllDetections(string output);


	IplImage *imgClone,*visited, *iDepth;

	int  frameJump;

	ShopDetector(int configuracion, string pathFile, bool saveCSV, string pathCSV, char* dbFilePath)
	{  
		this->dbConnection = new DBConnection(dbFilePath);
		this->saveCSV = saveCSV;
		this->pathCSV = pathCSV;
		stack= new point[15004] ; imgClone = visited = iDepth = NULL; invert1 = false; 
		puDs.push_back( new PickUpDetector(pathFile, cvRect(RoI_Information::xMinROI,RoI_Information::yMinROI,RoI_Information::xMaxROI-RoI_Information::xMinROI,RoI_Information::yMaxROI - RoI_Information::yMinROI), "pickUps_2",1) );
		cvInitFont(&font, CV_FONT_HERSHEY_SIMPLEX, 0.6, 0.6, 0, 1, 8);
		invert1 = false;

	}

	void getFamily(Blob* b, vector<Blob*>* family);
	bool AreParents(vector<Blob*> handsFamily,vector<Blob*> headsFamily);

	std::vector<Blob*>  dropFill(IplImage *img , IplImage *rgb , int frameNro );
	bool isInRange(Blob* blob);
	bool isExist(vector<Blob*> blobs,int lID);
	int iterativeVisit(IplImage *img, int x, int y, int W, int H, int value, IplImage *visited,int ID , int depth);
	int iterativeClear(IplImage *img, int x, int y, int W, int H, int value, IplImage *visited,int ID , int depth);
	Blob* getBlobFromlID(int lID);
	int visit(IplImage *img, int x, int y, int W, int H, int value, IplImage *visited,int ID , int depth);
	int recursiveClear(IplImage *img, int x, int y, int W, int H, int value, IplImage *visited,int ID , int depth);

	vector<string> ShopDetector::SaveToCSV(IplImage* imgRGB, string pathFile, int frameNro);
	void  drawAndUpdate(IplImage *imgD,IplImage *imgRGB, int frameNro, bool canDraw);

	int findPerson(PickUpDetector* puD)
	{
		return 0;		
	}

	PickUp* findPU(int nframe, Rect zone)
	{
		for (unsigned int i=0; i<allPickUps.size() ; i++)
		{
			PickUp* pu = allPickUps[i];
			// Si lo encontre en el frame anterior
			if (pu->lFrame == nframe - 1)
			{
				Rect rect = pu->r & zone ;  
				if (rect.area()>0) return pu;
			}
		}
		return NULL;
	}
	void discardInvalid( int frameNo)
	{
		std::vector<PickUp*> copy;
		for (unsigned  int i=0; i<allPickUps.size() ; i++)
		{
			PickUp* pu = allPickUps[i];
			// Si lo encontre en el frame anterior
			if (pu->lFrame <= frameNo - 2)
			{
				// duro mas de 10 frames
				if ((pu->lFrame - pu->fFrame) > 10)
				{
					copy.push_back(pu);
				}
				
			}
			else
				// se encuentra activo
				copy.push_back(pu);
		}
		allPickUps.clear();
		allPickUps.swap(copy);
	}

	void associateBlobsToPickUps(IplImage* iDepth,IplImage* rgbImageC, int frameNo)
	{
		// xMinROI, yMinROI, xMaxROI, yMaxROI
		for (unsigned int i=0; i<puDs.size();i++)
		{
			PickUpDetector* puD = puDs[i];
			if ( puD->roi.width >0)
			{
				vector<PickCells*> vFrame = puD->onNewFrameD( iDepth,  rgbImageC,frameNo, &allPickUps);

				if (vFrame.size() > 0 )
				{
					for (unsigned  int i=0; i< vFrame.size() ; i++)
					{
						PickUp* pu = findPU(frameNo, vFrame[i]->r);
						if (pu)
						{
							pu->r = pu->r & vFrame[i]->r;
							pu->lFrame = frameNo;
						}
						else
						{
							pu = new PickUp();
							pu->x = vFrame[i]->r.x;
							pu->width = vFrame[i]->r.width;
							pu->depth = vFrame[i]->depth;
							pu->r = vFrame[i]->r;
							pu->fFrame = frameNo;
							pu->lFrame = frameNo;
							allPickUps.push_back(pu);
						}
					}

				}
			#ifdef _DEBUG
				puD->draw();
			#endif
				cvResetImageROI( iDepth); 
			}
			// busca la persona
			int idPerson = findPerson(puD);

		}
	}
	void getBlobHandsInfo(Blob* head,vector<Blob*>* hands);

	//
	//marca a la cabeza como no visible
	//
	void removeElement(Blob* head){
		int pos = -1;
		for (unsigned int j=0;j<allBlobs.size();j++)
		{
			if (allBlobs[j]->lID == head->lID)
				pos = j;
		}

		if (pos != -1)
			allBlobs.erase(allBlobs.begin()+pos);
	}


	//
	//verifica que el blob este dentro de line Right y line Left
	//
	bool noVerifyBlobsInScene(Blob* head){

		//verifica blobHead
		bool inRange= false;
		if (!isInRange(head)){
			//remover los elementos de blobs

			return true;
		}

		//verifica family blobHead
		vector<Blob*> headsFamily;
		getFamily(head,&headsFamily);
		bool inRangeFamily = false;
		for (unsigned int j=0;j<headsFamily.size() && !inRangeFamily;j++)
		{
			if (head->lID == headsFamily[j]->lID)
				continue;

			if (!isInRange(headsFamily[j])){
				//remover los elementos de blobs
				return true;
			}
		}	
		return inRange;
	}

	void drawBlobs(vector<Blob*> allBlobs , IplImage *imgRGB , bool drawLines, int frameNro );

	void drawBlobsInGondola(vector<Blob*> blobs , IplImage *imgRGB);

	void removeBlobsOutRange(){	

		vector<Blob*> heads;
		//devuelve lista de blobs heads y los blobs que estan en la ROI gondola (posibles hands=
		getHeads(&heads);

		//verifico que todo el blob este dentro de la escena
		for (unsigned int i=0;i<heads.size();i++)
		{
			if (noVerifyBlobsInScene(heads[i]))
				removeElement(heads[i]);
		}

	}

};