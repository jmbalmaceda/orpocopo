#include "shopDetector.h"
#include "DBConnection.h"
#include "Export.h"


// Get current date/time, format is YYYY-MM-DD.HH:mm:ss
const std::string currentDateTime() {
	time_t     now = time(0);
	struct tm  tstruct;
	char       buf[80];
	tstruct = *localtime(&now);

	strftime(buf, sizeof(buf), "%d/%m/%Y  %X", &tstruct);

	return buf;
}

const std::string currentDateTime(char* format) {
	time_t     now = time(0);
	struct tm  tstruct;
	char       buf[80];
	tstruct = *localtime(&now);
	// Visit http://en.cppreference.com/w/cpp/chrono/c/strftime
	// for more information about date/time format
	strftime(buf, sizeof(buf), format, &tstruct);

	return buf;
}


/*
Version iterativa
*/
int ShopDetector::iterativeVisit(IplImage *img, int x, int y, int W, int H, int value, IplImage *visited,int ID , int depth)
{   
	stack[0].x = x; stack[0].y = y; 
	int counter = 0;
	int index =1 ;
	while (index > 0)
	{
		int x0 , y0 ; 

		x0 = stack[index-1].x;
		y0 = stack[index-1].y;

		index--;
		if ((x0<=0) || (y0<=0) || (x0>=W) || (y0>=H)) continue;
		// TODO: Por qué 15000 ?
		if (index >= 15000) continue;

		uchar* row =  (uchar*)( img->imageData + y0 * img->widthStep ) ;
		uchar nvalue = row[3*x0];   
		if ( nvalue <= 1)   continue; 
		// Si es menor la diferencia que la tolerancia, lo acepto
		if (abs(nvalue - value) < DEPTH_TOLERANCE)
		{
			uchar* irow =  ( uchar*)( visited->imageData + y0 * visited->widthStep ) ;
			// Fue visitado
			if (irow[x0]) continue;         
			irow[x0] = ID;
			counter++;

			stack[index].x = x0+1; stack[index].y = y0; index++;
			stack[index].x = x0; stack[index].y = y0+1; index++;
			stack[index].x = x0-1; stack[index].y = y0; index++;
			stack[index].x = x0; stack[index].y = y0-1; index++;
		}

	}

	return counter;
}

int ShopDetector::iterativeClear(IplImage *img, int x, int y, int W, int H, int value, IplImage *visited,int ID , int depth)
{   
	stack[0].x = x; stack[0].y = y; 
	int counter = 0;
	int index =1 ;
	while (index > 0)
	{
		int x0 , y0 ; 

		x0 = stack[index-1].x;
		y0 = stack[index-1].y;

		index--;
		if ((x0<=0) || (y0<=0) || (x0>=W) || (y0>=H)) continue;
		if (index >= 15000) continue;

		uchar* irow =  ( uchar*)( visited->imageData + y0 * visited->widthStep ) ;
		// Fue visitado. Limpio el flag
		if (irow[x0] != ID ) continue;         
		irow[x0] = 0;

		stack[index].x = x0+1; stack[index].y = y0; index++;
		stack[index].x = x0; stack[index].y = y0+1; index++;
		stack[index].x = x0-1; stack[index].y = y0; index++;
		stack[index].x = x0; stack[index].y = y0-1; index++;
	}

	return counter;
}

/*
Version recursiva
*/

int ShopDetector::visit(IplImage *img, int x, int y, int W, int H, int value, IplImage *visited,int ID , int depth)
{
	if ((x<=0) || (y<=0) || (x>=W) || (y>=H)) return 0; 
	if (depth > 15000) return 0;
	//  if ( ( (x-x0) * (x-x0) + (y-y0)*(y-y0)) > maxR * maxR ) return 0;
	uchar* row =  (uchar*)( img->imageData + y * img->widthStep ) ;
	uchar nvalue = row[3*x];   
	if ( nvalue <= 1)   return 0; 

	// Si es menor la diferencia que la tolerancia, lo acepto
	if (abs(nvalue - value) < DEPTH_TOLERANCE)
	{
		uchar* irow =  ( uchar*)( visited->imageData + y * visited->widthStep ) ;
		// Fue visitado
		if (irow[x]) return 0;         
		irow[x] = ID;

		int counter = visit(img, x+1,y,W,H,value,visited, ID, depth+1) + 
			visit(img, x,y+1,W,H,value,visited, ID,depth+1) +
			visit(img, x-1,y,W,H,value,visited, ID,depth+1) +
			visit(img, x,y-1,W,H,value,visited, ID, depth+1)  + 
			// Visita las diagonales
			/*visit(img, x-1,y-1,W,H,value,visited, ID, depth+1)  +
			visit(img, x-1,y+1,W,H,value,visited, ID, depth+1)  +
			visit(img, x+1,y-1,W,H,value,visited, ID, depth+1)  +
			visit(img, x+1,y+1,W,H,value,visited, ID, depth+1)   */
			+ 1; 
		return counter;
	}
	else
		return 0;
}

int ShopDetector::recursiveClear(IplImage *img, int x, int y, int W, int H, int value, IplImage *visited,int ID , int depth)
{
	if ((x<=0) || (y<=0) || (x>=W) || (y>=H)) return 0; 
	if (depth > 15000) return 0;

	uchar* irow =  ( uchar*)( visited->imageData + y * visited->widthStep ) ;
	// Fue visitado. Limpio el flag
	if (irow[x] != ID ) return 0;         
	irow[x] = 0;

	int counter = recursiveClear(img, x+1,y,W,H,value,visited, ID, depth+1) + 
		recursiveClear(img, x,y+1,W,H,value,visited, ID,depth+1) +
		recursiveClear(img, x-1,y,W,H,value,visited, ID,depth+1) +
		recursiveClear(img, x,y-1,W,H,value,visited, ID, depth+1) +
		// Visita las diagonales
		/*  recursiveClear(img, x-1,y-1,W,H,value,visited, ID, depth+1) + 
		recursiveClear(img, x-1,y+1,W,H,value,visited, ID,depth+1) +
		recursiveClear(img, x+1,y-1,W,H,value,visited, ID,depth+1) +
		recursiveClear(img, x+1,y+1,W,H,value,visited, ID, depth+1)  */
		+1; 
	return counter;
}

//
//devuelve un blob dado un localID
//
Blob* ShopDetector::getBlobFromlID(int lID)
{
	for (unsigned int i = 0; i<blobs.size() ;i++)
	{ 
		if (blobs[i]->lID == lID)
			return (blobs[i]);
	}
	return 0;
}


//-----------------------------------------------------------------------------
//  dropFill : Algoritmo de deteccion de blobs por niveles. Estilo "drop", aunque diferente
//   * version  11_Junio_2013 : 60 FPS . Podrian eliminarse la creacion de las imagenes temporales. No se borran los blobs Creados
// Este algoritmo actualmente incluye la vinculacion de manos con heads. haciendo los filtros correspondientes 
//---------------------------------------------------------------------------
std::vector<Blob*>  ShopDetector::dropFill(IplImage *img , IplImage *rgb, int frameNro )
{
	//cvAddS(img, cvScalar(70,70,70), img);
	//cvSetImageROI(img,cvRect(30,0,610,480));
	if (!imgClone)
		imgClone = cvCreateImage(cvSize(img->width, img->height),img->depth,img->nChannels);	
	if (!visited)
		visited = cvCreateImage(cvSize(img->width, img->height),img->depth ,1);


	cvScale(visited, visited, 0);
	cvScale(imgClone, imgClone, 0);


	int i = 0;
	//int R = 15;
	int ID = 1;

	// TODO: Reemplazo de los límites utilizando información del archivo de configuración
	// Detecto los BLOBS, omitiendo bordes R
	//for (int y = R; y<img->height - R ; y++)
	for (int y = RoI_Information::yMinLineLeft; y<RoI_Information::yMaxLineLeft ; y++)
	{
		uchar* row =  (uchar*)( img->imageData + y * img->widthStep ) ;
		//for (int x = R; x<img->width - R ; x++)
		for (int x = RoI_Information::xMaxLineLeft; x<RoI_Information::xMaxLineRight; x++)
		{
			//	int x = 125 , y = 404 ; 
			int value ; 

			value = row[3*x];   

			if (!value) continue;			

			//int v = visit(img, x,y, img->width, img->height,value, visited, ID,0);			
			int v = this->iterativeVisit(img, x,y, img->width, img->height,value, visited, ID,120);			
			if ((v>0) && (v<MIN_BLOB_SIZE))
				this->iterativeClear(img, x,y, img->width, img->height,value, visited, ID,0);			
			//recursiveClear(img, x,y, img->width, img->height,value, visited, ID,0);			
			//clear(visited,0,0, img->width, img->height, ID) ;
			else
				if (v>=MIN_BLOB_SIZE)
					ID = ID + 1;	
		}
	}


	for (int i=0; i<ID-1 ; i++)
	{
		Blob* bs = new Blob(frameNro);		
		blobs.push_back(bs);
	}

	newBlobsDetected = ID - 1;

	if (newBlobsDetected == 0) return blobs;

	// Asigno los colores	
	for (int y = 0; y<visited->height  ; y++)
	{
		uchar* row =  (uchar*)( visited->imageData + y * visited->widthStep ) ;
		uchar* irow =  (uchar*)( img->imageData + y * img->widthStep ) ;
		uchar* dstrow =  (uchar*)( imgClone->imageData + y * imgClone->widthStep ) ;

		for (int x = 0; x<visited->width   ; x++)
		{			 
			int iID = row[x];   
			if (iID <= 0) continue; 
			if ((iID-1) >= (int) blobs.size() ) continue;
			blobs[iID-1]->addPixel(x,y);
			blobs[iID-1]->lID = iID;

			//row =  (uchar*)( img->imageData + y * img->widthStep ) ;
			blobs[iID-1]->depth = irow[3*x+0];

			dstrow[3*x+0] = (iID * 453 ) % 255;   
			dstrow[3*x+1] = (iID * 599 ) % 255;   
			dstrow[3*x+2] = (iID * 771 ) % 255;   

		}
	}

	// ordeno por altura, de menor a mayor
	for (unsigned int i = 0; i<blobs.size() ;i++)
		for (unsigned int j = i+1; j<blobs.size() ;j++)
		{
			if (blobs[i]->depth < blobs[j]->depth)
			{
				Blob* aux = blobs[i];
				blobs[i]  =blobs[j];
				blobs[j] = aux;
			}
		}

		//asigna los blobs child
		// Elimino los blobs que contienen a otros blobs
		for (unsigned int i = 0; i<blobs.size() ;i++)
		{   
			//agrego el localID al blob
			blobs[i]->calcCenter();
			char convB[10];
			_itoa(blobs[i]->lID,convB,10);
			std::string s2(convB);
			cvPutText(imgClone , s2.c_str(),cvPoint( blobs[i]->posX,blobs[i]->posY), &font , cvScalar(0,0,0));

			blobs[i]->blobsChild.push_back(blobs[i]);
			for (unsigned int j = 0; j<blobs.size() ;j++)
			{
				if (i == j) continue;
				// Elimino los blobs con muy pocos pixeles
				if (!blobs[j]->visible) continue;

				//se fija si el blob pertenece al ROI gondola. Setea inROIGondola de cada blob
				blobs[j]->setROIGondola(RoI_Information::xMinROI,RoI_Information::yMinROI,RoI_Information::xMaxROI,RoI_Information::yMaxROI);

				if ( (blobs[i]->depth > blobs[j]->depth) && (blobs[i]->contains(blobs[j])) )
				{
					// dejamos el seteo de visibles para despues de vincular las manos con las cabezas.
					// De esta forma las manos no estarían visibles

					//blobs[j]->visible = false;
					blobs[j]->parent = blobs[i];
					blobs[i]->blobsChild.push_back(blobs[j]);				
				}
				else{ //seteo parents para casos especiales
					if ( (blobs[i]->depth == blobs[j]->depth) && (blobs[i]->isJoined(blobs[j])))
					{

						// dejamos el seteo de visibles para despues de vincular las manos con las cabezas.
						// De esta forma las manos no estarían visibles

						//blobs[j]->visible = false;
						blobs[j]->parent = blobs[i];
						blobs[i]->blobsChild.push_back(blobs[j]);				
					}
				}

			}
		}


		// vinculo las manos que estan en la ROI de gondolas con las cabezas
		// verifico que el blob se encuentre dentro de la escena
		setBlobsFamily(&blobs);

		//seteo visibles
		/*
		for (unsigned int i = 0; i<blobs.size() ;i++)
		{ 

			for (unsigned int j = 0; j<blobs[i]->blobsChild.size() ;j++)
			{			
				//if (blobs[i]->lID == blobs[i]->blobsChild[j]->lID) continue;
				//	blobs[i]->blobsChild[j]->visible = false;
			}

		}
		*/

		// Mostrar información de la profundidad
		/*
		for (unsigned int i = 0 ; i< puDs.size() ; i++)
			cvRectangle(imgClone, cvPoint(puDs[i]->roi.x,puDs[i]->roi.y),cvPoint(puDs[i]->roi.x+puDs[i]->roi.width,puDs[i]->roi.y+puDs[i]->roi.height), cvScalar(120, 120, 255), 3, 8, 0);

		//draw LineRight
		cvRectangle(imgClone, cvPoint(RoI_Information::xMinLineRight,RoI_Information::yMinLineRight),cvPoint(RoI_Information::xMaxLineRight,RoI_Information::yMaxLineRight), cvScalar(0, 0, 255), 3, 8, 0);

		//draw LineLeft
		cvRectangle(imgClone, cvPoint(RoI_Information::xMinLineLeft,RoI_Information::yMinLineLeft),cvPoint(RoI_Information::xMaxLineLeft,RoI_Information::yMaxLineLeft), cvScalar(0, 0, 255), 3, 8, 0);

		cvShowImage("Visited", imgClone);
		//*/
		return (blobs);
}


//
// Dada la lista de potenciales hands, me devuelve las verdaderas hands.
// Elimina los blobs que estan en la zona de gondola y tienen parents (childs o parents) en zona de gondola más cercanos a la góndola. 
// Se queda sólo con el blob más cercano a la góndola 
//
void  ShopDetector::getBlobHands(vector<Blob*> potentialHands,vector<Blob*>* hands){

	if (potentialHands.size() == 0) 
		return;
	unsigned int j = 0;

	while (j < potentialHands.size())
	{		
		Blob* bHand = potentialHands[j];
		bool majorBlob = true;

		// controla si el blob actual tiene algun padre o hijo que esté más cerca a la góndola

		//me fijo si el padre esta en roi gondola
		if (bHand->parent != NULL && bHand->parent->inROIGondola)
		{
			//veo cual esta mas cerca de la gondola
			if (bHand->maxY < bHand->parent->maxY){
				majorBlob = false;
				bHand->inROIGondola = false;
			}
		}

		//me fijo si tiene algun hijo en roi gondola
		for (unsigned int i=0;i<bHand->blobsChild.size();i++){
			if (bHand->blobsChild[i]->lID == bHand->lID) continue;
			if (!bHand->blobsChild[i]->inROIGondola) continue;
			//si tiene algun hijo en roi gondola

			if (bHand->maxY < bHand->blobsChild[i]->maxY){
				majorBlob = false;
				bHand->inROIGondola = false;
			}
		}


		if (majorBlob == true){
			hands->push_back(bHand);
		}

		j++;
	}
}

void ShopDetector::getHeads(vector<Blob*>* heads)
{
	for (unsigned int i = 0; i<allBlobs.size() ;i++)
	{ 
		//si no esta en ROIGondoa y no tiene padre -> es cabeza
		if (!allBlobs[i]->inROIGondola){
			if (allBlobs[i]->parent == NULL)
				heads->push_back(allBlobs[i]);			
		}
	}
}

//
//me devuelve una lista de heads y las potenciales hands (blobs que se encuentra en roi gondolas)
//
void  ShopDetector::getPotentialsParents(vector<Blob*> heads, vector<Blob*> potentialHands)
{
	for (unsigned int i = 0; i<blobs.size() ;i++)
	{ 
		//si no esta en ROIGondoa y no tiene padre -> es cabeza
		if (!blobs[i]->inROIGondola){
			if (blobs[i]->parent == NULL)
				heads.push_back(blobs[i]);				
		}else{//si esta en zona de gondola es potencial cabeza
			potentialHands.push_back(blobs[i]);	
		}
	}
}

//
// vincula las manos que estan en ROI gondolas con las posibles cabezas
//
void ShopDetector::setBlobsFamily(vector<Blob*>* blobs)
{

	vector<Blob*> heads,potentialHands,hands;

	//devuelve lista de blobs heads y los blobs que estan en la ROI gondola (posibles hands)
	getPotentialsParents(heads,potentialHands);

	//determina cuales blobs que estan en la gondola son hands
	if (potentialHands.size() > 0)
		getBlobHands(potentialHands,&hands);

	//std::cout << frame << " heads size: " <<  heads.size() << " potencial hands size: " << potentialHands.size() << " hands size: " << hands.size()<<endl;


	//vinculo la lista de hands con las cabezas
	if ((hands.size() > 0)&&(heads.size() > 0)){

		//si hay una mano y una cabeza
		if ((hands.size() == 1) && (heads.size() == 1)){
			Blob* b = getBlobFromlID(hands[0]->lID);
			//si la mano no contiene a la cabeza -> hago el vinculo
			if (!isExist( heads[0]->blobsChild,hands[0]->lID)){
				b->parent = heads[0];
				b->parent->blobsChild.push_back(b);
			}
		}else{
			//recorro todas las manos y obtengo su familia y 
			// las comparo con la familia de cada cabeza
			//Despues me fijo si son parientes para vincular la mano con la cabeza
			for (unsigned int i=0;i<hands.size();i++)
			{
				vector<Blob*> handsFamily;
				getFamily(hands[i],&handsFamily);
				bool isParent = false;
				for (unsigned int j=0;j<heads.size() && !isParent;j++)
				{
					if (!isExist( heads[j]->blobsChild,hands[i]->lID)){
						vector<Blob*> headsFamily;
						getFamily(heads[j],&headsFamily);
						if (AreParents(handsFamily,headsFamily)){
							Blob* b = getBlobFromlID(hands[i]->lID);
							b->parent = heads[j];
							b->parent->blobsChild.push_back(b);
							isParent = true;
						}
					}
				}
			}
		}
	}

}

//
//algoritmo recursivo que me devuelve su familia (childs), dado un blob
//
void ShopDetector::getFamily(Blob* b, vector<Blob*>* family)
{

	family->push_back(b);

	if (b->blobsChild.size() == 1){
		return;
	}

	for(unsigned int i=0; i<b->blobsChild.size();i++){		
		if (!isExist(*family,b->blobsChild[i]->lID)){
			getFamily(b->blobsChild[i],family);
			//family->push_back(b);
		}
	}

}




//
// dado dos listas de blobs me dice si tiene algun blob en comun
//
bool ShopDetector::AreParents(vector<Blob*> handsFamily,vector<Blob*> headsFamily)
{
	for (unsigned int i=0;i<handsFamily.size();i++)
	{
		for (unsigned int j=0;j<headsFamily.size();j++)
		{
			if ((handsFamily[i]->lID == headsFamily[j]->lID))
				return true;
		}
	}
	return false;
}

bool ShopDetector::isInRange(Blob* blob)
{ 

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



//
//dado un localId me dice si existe o no en una lista de blobs
//
bool ShopDetector::isExist(vector<Blob*> blobs,int lID)
{
	for (unsigned int i=0;i<blobs.size();i++)
	{
		if (blobs[i]->lID == lID)
			return true;
	}
	return false;
}


//-----------------------------------------------------------------------------
//  processBlobs : realiza todo el proceso del blob dada las imagenes
//  -- canDraw: variable que indica si puede dibujar. Esta activo solo en "modo imagenes" fuera de la DLL
//---------------------------------------------------------------------------
void ShopDetector::processBlobs(IplImage *imgD,IplImage *imgRGB,int &size, bool canDraw, int threshold, int numFrame)
{
	if (invert1)
	{
		for (int y = 0; y<imgD->height  ; y++)
		{
			uchar* row =  (uchar*)( imgD->imageData + y * imgD->widthStep ) ;
			for (int x = 0; x<imgD->width * imgD->nChannels ; x++)
			{
				if (row[x]>0)
					row[x] =  255 - row[x]*10;
			}
		}

	}
	else
	{
		/*
		for (int y = 0; y<imgD->height  ; y++)
		{
		uchar* row =  (uchar*)( imgD->imageData + y * imgD->widthStep ) ;
		for (int x = 0; x<imgD->width * imgD->nChannels ; x++)
		{
		if (row[x]< threshold)
		row[x] =  0 ;
		}
		}
		*/
		cvThreshold(imgD , imgD, threshold, 255,CV_THRESH_TOZERO);//160
	}


	if (!iDepth)
		iDepth = cvCreateImage(cvSize(imgD->width, imgD->height),imgD->depth ,1);
	cvConvertImage(imgD,iDepth,CV_BGR2GRAY);

	//TODO: Para Debug: muestra la imagen a procesar
	/*
	cvShowImage("ImageThres", imgD);
	cvWaitKey(-1);
	//*/

	// Calculo los nuevos blobs
	blobs.clear();
	//  double startT = sft_clock();

	//Algoritmo de seguimiento de blob. USADO EN LA DLL

	//TODO: Las líneas de abajo ponen en negro toda la zona de góndola
	/*
	CvRect rect = cvRect(0, 350, 640, 480);
	cvSetImageROI(imgD, rect);
	cvZero(imgD);
	cvResetImageROI(imgD);
	//*/
	//CvRect rect = cvRect(RoI_Information::xMinROI, RoI_Information::yMinROI, RoI_Information::xMaxROI-RoI_Information::xMinROI, RoI_Information::yMaxROI-RoI_Information::yMinROI);

	//TODO: Para Debug: muestra la imagen luego de poner en negro la región de la góndola
	/*
	cvShowImage("ImageThres", imgD);
	cvWaitKey(-1);
	//*/

	dropFill(imgD, imgRGB, numFrame);

	//Dibuja y actualiza los blobs. NO USADO EN LA DLL
	drawAndUpdate(imgD , imgRGB,numFrame, canDraw);

	if (saveCSV)
	{
		SaveToCSV(imgRGB, pathCSV, numFrame);
		//vector<string> linesToSave = SaveToCSV(imgRGB,pathCSV,numFrame);
		//exportar a csv	  
		//for (unsigned int i=0; i<linesToSave.size(); i++)
		//	Export::ExportToCSV(pathCSV,linesToSave[i], true);//true -> append	  
	}

	//array_release(result);

}


IplImage* cropAndSave( IplImage* src,  CvRect roi, char* path)
{

	// Must have dimensions of output image
	IplImage* cropped = cvCreateImage( cvSize(roi.width,roi.height), src->depth, src->nChannels );

	// Say what the source region is
	cvSetImageROI( src, roi );

	// Do the copy
	cvCopy( src, cropped );
	cvResetImageROI( src );

	// cvShowImage("cropped",cropped);
	if (path)
	{
		cvSaveImage (path , cropped);
		cvReleaseImage(&cropped);
		return NULL;
	}
	//cvWaitKey(20);

	return cropped;
}



vector<string> ShopDetector::SaveToCSV(IplImage* imgRGB, string pathFile, int frameNro)
{	

	vector<string> dataToRecord;

	// convert now to string form
	string currenT = currentDateTime();

	for (unsigned int i=0; i<allBlobs.size() ; i++)
	{
		if (!allBlobs[i]->visible) continue;
		if (!allBlobs[i]->activated) continue;

		// if (MainWindow.allBlobs[i].Tag == MainWindow.TAG_HEAD)


		// dataToRecord.Add("Frame; Current Time; Blob ID; BPos X; BPos Y; BDepth; Hand ID; HPos X; HPos Y; HDepth;");

		string countBlob = IntToString(allBlobs.size());
		string frameSequencia = IntToString(frameNro);

		// Saco lo de abajo porque parece que no hace nada
		/*
		if ( allBlobs[i]->blobsHistory.size() > 2 )
		{
			int index = allBlobs[i]->blobsHistory.size()-1;
			int px = allBlobs[i]->posX-60;
			int py = MAX(0,allBlobs[i]->posY-60);
			//Si no se salvo y esta en el centro de la pantalla
			if ((!allBlobs[i]->saveCapture) && (px>100) && (px<500)  && (py < 350))
			{
				//string captureFile = pathFile + "capture"+IntToString(allBlobs[i]->globalID())+".jpg"; 
				//allBlobs[i]->imgCapture = cropAndSave(imgRGB,   cvRect(px ,py, 130,130), (char*)captureFile.c_str());
				//allBlobs[i]->saveCapture = true;
			}
		}
		//*/

		if (allBlobs[i]->blobsHistory.size() == 0)
		{
			vector<Blob*> bHands;
			getBlobHandsInfo(allBlobs[i],&bHands);

			if (bHands.size() == 0)
			{
				//dataToRecord.push_back(frameSequencia + ";" + currenT + ";" + countBlob + ";" + IntToString(allBlobs[i]->globalID()) + ";" + IntToString(allBlobs[i]->posX) + ";" + IntToString(allBlobs[i]->posY) + ";" + IntToString(allBlobs[i]->depth) + ";-;-;-;-");
				dbConnection->insertPickUpInformation(frameNro, allBlobs.size(), allBlobs[i]->globalID(), allBlobs[i]->posX, allBlobs[i]->posY, allBlobs[i]->depth);
			}
			else
			{
				for (unsigned int j=0; j<bHands.size() ; j++)
					//dataToRecord.push_back(frameSequencia + ";" + currenT + ";" + countBlob + ";" + IntToString(allBlobs[i]->globalID()) + ";" +  IntToString(allBlobs[i]->posX) + ";" + IntToString(allBlobs[i]->posY) + ";" + IntToString(allBlobs[i]->depth) + ";" + IntToString(bHands[j]->globalID()) + ";" + IntToString(bHands[j]->posX) + ";" + IntToString(bHands[j]->posY) + ";" + IntToString(bHands[j]->depth));
					dbConnection->insertPickUpInformation(frameNro, allBlobs.size(), allBlobs[i]->globalID(), allBlobs[i]->posX, allBlobs[i]->posY, allBlobs[i]->depth, bHands[j]->globalID(), bHands[j]->posX, bHands[j]->posY, bHands[j]->depth);
			}
		}
		else
		{
			vector<Blob*> bHands;
			getBlobHandsInfo(allBlobs[i]->blobsHistory[allBlobs[i]->blobsHistory.size() - 1],&bHands);
			if (bHands.size() == 0)
			{
				//dataToRecord.push_back(frameSequencia + ";" + currenT + ";" + countBlob + ";" + IntToString(allBlobs[i]->globalID()) + ";" + IntToString(allBlobs[i]->blobsHistory[allBlobs[i]->blobsHistory.size() - 1]->posX) + ";" + IntToString(allBlobs[i]->blobsHistory[allBlobs[i]->blobsHistory.size() - 1]->posY) + ";" + IntToString(allBlobs[i]->blobsHistory[allBlobs[i]->blobsHistory.size() - 1]->depth) + ";-;-;-;-");
				dbConnection->insertPickUpInformation(frameNro, allBlobs.size(), allBlobs[i]->globalID(), allBlobs[i]->blobsHistory[allBlobs[i]->blobsHistory.size() - 1]->posX, allBlobs[i]->blobsHistory[allBlobs[i]->blobsHistory.size() - 1]->posY, allBlobs[i]->blobsHistory[allBlobs[i]->blobsHistory.size() - 1]->depth);
			}
			else
			{
				for (unsigned int j=0; j<bHands.size() ; j++)
					//dataToRecord.push_back(frameSequencia + ";" + currenT + ";" + countBlob  + ";" +  IntToString(allBlobs[i]->globalID()) + ";" + IntToString(allBlobs[i]->blobsHistory[allBlobs[i]->blobsHistory.size() - 1]->posX) + ";" + IntToString(allBlobs[i]->blobsHistory[allBlobs[i]->blobsHistory.size() - 1]->posY) + ";" + IntToString(allBlobs[i]->blobsHistory[allBlobs[i]->blobsHistory.size() - 1]->depth) + ";" + IntToString(bHands[j]->globalID()) + ";" + IntToString(bHands[j]->posX) + ";" + IntToString(bHands[j]->posY) + ";" + IntToString(bHands[j]->depth));
					dbConnection->insertPickUpInformation(frameNro, allBlobs.size(), allBlobs[i]->globalID(), allBlobs[i]->blobsHistory[allBlobs[i]->blobsHistory.size() - 1]->posX, allBlobs[i]->blobsHistory[allBlobs[i]->blobsHistory.size() - 1]->posY, allBlobs[i]->blobsHistory[allBlobs[i]->blobsHistory.size() - 1]->depth,  bHands[j]->globalID(), bHands[j]->posX, bHands[j]->posY, bHands[j]->depth);
			}
		}
		//MainWindow.dataToRecord.Add(blobINFO[0].ToString() + ";" + DateTime.Now.ToString() + ";" + MainWindow.allBlobs.Count.ToString() + ";" + MainWindow.allBlobs[i].GlobalID + ";" + MainWindow.allBlobs[i].Tag + ";" + MainWindow.allBlobs[i].InROI);
	}

	return dataToRecord;
}

void ShopDetector::getBlobHandsInfo(Blob* head,vector<Blob*>* hands)
{
	for (unsigned int i = 0; i < head->blobsChild.size(); i++) {
		if (head->blobsChild[i]->inROIGondola)
			hands->push_back(head->blobsChild[i]);            
	}
}

//-----------------------------------------------------------------------------
//  drawAndUpdate : genera el entorno gráfico. (SOLO SE USA EN MODO DEBUG, LA DLL NO LO UTILIZA)
//---------------------------------------------------------------------------
void  ShopDetector::drawAndUpdate(IplImage *imgD,IplImage *imgRGB, int frameNro, bool canDraw)
{
	// Asocio con los que ya tenia

	if (invert1)
		drawBlobs(blobs, imgRGB,true,frameNro); 
	else
	{
		//double startT = sft_clock();
		matchBlobs(blobs,&allBlobs, RoI_Information::xMinROI,RoI_Information::yMinROI,RoI_Information::xMaxROI,RoI_Information::yMaxROI);		
		updateBlobsState(&allBlobs);	

		//elimino de all blobs los blobs que estan sobre los extremmos
		//para los casos que las personas estan paradas en los extremos de la escena
		removeBlobsOutRange();

		if (canDraw)
			drawBlobs(allBlobs, imgRGB,true,frameNro);
		//std::cout << frame << " Req. time for matching " <<  sft_clock() - startT << " secs " << endl;

		//   cvWaitKey(-1);
	}

}


//-----------------------------------------------------------------------------
//  drawBlobs :  (SOLO SE USA EN MODO DEBUG, LA DLL NO LO UTILIZA)
//---------------------------------------------------------------------------
void ShopDetector::drawBlobs(vector<Blob*> allBlobs , IplImage *imgRGB , bool drawLines , int frameNro)
{
	int blobsInROI = 0;

	char conv[10];
	char convA[10];
	// Muestro los visibles
	for (unsigned int i=0; i<allBlobs.size() ; i++)
	{
		if (!allBlobs[i]->visible) continue;
		if (!allBlobs[i]->activated) continue;
		// cvRectangle(imgRGB, cvPoint(allBlobs[i]->minX,allBlobs[i]->minY),cvPoint(allBlobs[i]->maxX,allBlobs[i]->maxY), cvScalar(255,0,0));
		allBlobs[i]->calcCenter();

		_itoa( allBlobs[i]->computeAngle(),convA,10);
		_itoa( allBlobs[i]->globalID(),conv,10);
		std::string si(conv);            
		std::string si2(convA); 
		std::string sr = si +" "+si2;
		cvPutText(imgRGB , sr.c_str(),cvPoint( allBlobs[i]->posX,allBlobs[i]->posY), &font , cvScalar(205,125,125));

		CvScalar blobColor =  cvScalar ( ((i+31) * 1112+3211 ) % 255 ,  ((i+54) * 475  ) % 255 ,  (i * 771 + 9988) % 255 );   
		if ( allBlobs[i]->blobsHistory.size() <= 5)
			cvCircle(imgRGB, cvPoint(allBlobs[i]->posX,allBlobs[i]->posY),10,cvScalar(0,0,255));
		else
			for (unsigned int j = 0; j<allBlobs[i]->blobsHistory.size() ; j++)
			{
				Blob* bh = allBlobs[i]->blobsHistory[j];				
				// Dibuja los rectangulos
				//cvRectangle(imgRGB, cvPoint(bh->minX,bh->minY),cvPoint(bh->maxX,bh->maxY), cvScalar(0,255,0));
				// Dibuja las lineas
				if (drawLines)
				{
					//if ( j == 0)
					//  cvLine(imgRGB ,cvPoint(allBlobs[i]->posX,allBlobs[i]->posY), cvPoint(allBlobs[i]->blobsHistory[0]->posX,allBlobs[i]->blobsHistory[0]->posY), blobColor);
					//else
					if (j >0 ) 
						cvLine(imgRGB ,cvPoint(bh->posX,bh->posY), cvPoint(allBlobs[i]->blobsHistory[j-1]->posX,allBlobs[i]->blobsHistory[j-1]->posY),   blobColor);
				}  

				// Es el ultimo añadido
				if ( j ==  allBlobs[i]->blobsHistory.size() - 1)
				{
					cvCircle(imgRGB, cvPoint(bh->posX,bh->posY),10,cvScalar(0,0,255));
					for (unsigned int k = 0; k<bh->blobsChild.size() ; k++)
					{
						Blob* bh2 = bh->blobsChild[k];
						if ( bh2->inROIGondola)
							blobsInROI++;

						if (bh2->tag == TAG_HEAD && !bh2->inROIGondola){
							//cvRectangle(imgRGB, cvPoint(bh2->minX,bh2->minY),cvPoint(bh2->maxX,bh2->maxY), cvScalar(255,255,0));

						}else
							if (bh2->tag == TAG_SHOULDER && !bh2->inROIGondola) {}
							//cvRectangle(imgRGB, cvPoint(bh2->minX,bh2->minY),cvPoint(bh2->maxX,bh2->maxY), cvScalar(255,0,255));
					}
				}
			}
	}

	possiblePeople = 0;
	int rejectedBlobs = 0;

	peopleblobs.clear();

	for (unsigned int i=0; i<allBlobs.size();i++)
		if ( allBlobs[i]->blobsHistory.size() > 2 )
		{
			peopleblobs.push_back(allBlobs[i]);
			possiblePeople++;
		}
		else
			rejectedBlobs++;

	//people count
	_itoa(possiblePeople,conv,10);
	std::string sp(conv);
	sp = "ple:" + sp;

	//detecta personas que entran en la gondola
	_itoa(rejectedBlobs,conv,10);
	std::string sr(conv);
	sr = "dtct:" + sr;

	//frames
	char convC[10];
	_itoa(frameNro,convC,10);
	std::string s3(convC);
	s3 = "frm:" + s3;

	//blobs
	char convB[10];
	_itoa(blobsInROI,convB,10);
	std::string s2(convB);
	s2 = "bls:" + s2;

	//fondo negro de datos
	cvRectangle(imgRGB, cvPoint(10,5),cvPoint(265,18), cvScalar(0, 0, 0), 18, 8, 0);

	//fecha y horario
	cvPutText(imgRGB, dbConnection->getCurrentTimeAsString().c_str(), cvPoint(6,18), &font, cvScalar(0,255,255));	

	//numero de frame
	//cvPutText(imgRGB , s3.c_str(),cvPoint(280,15), &font , cvScalar(0,255,255));
	
	//cantidad de blobs in roi
	//cvPutText(imgRGB, s2.c_str(), cvPoint(420,15), &font, cvScalar(0, 255, 255));

	//blobs count
	//cvPutText(imgRGB , sp.c_str(),cvPoint(500,15), &font , cvScalar(0,255,255));

	//rejected blobs count
	//cvPutText(imgRGB , sr.c_str(),cvPoint(570,15), &font , cvScalar(0,255,255));

	//draw ROI gondola
	for (unsigned int i=0; i<puDs.size(); i++)
		cvRectangle(imgRGB, cvPoint(puDs[i]->roi.x,puDs[i]->roi.y),cvPoint(puDs[i]->roi.x+puDs[i]->roi.width,puDs[i]->roi.y+puDs[i]->roi.height), cvScalar(0, 255, 255), 1, 8, 0);

	//draw LineRight
	cvRectangle(imgRGB, cvPoint(RoI_Information::xMinLineRight,RoI_Information::yMinLineRight),cvPoint(RoI_Information::xMaxLineRight,RoI_Information::yMaxLineRight), cvScalar(0, 0, 255), 3, 8, 0);

	//draw LineLeft
	cvRectangle(imgRGB, cvPoint(RoI_Information::xMinLineLeft,RoI_Information::yMinLineLeft),cvPoint(RoI_Information::xMaxLineLeft,RoI_Information::yMaxLineLeft), cvScalar(0, 0, 255), 3, 8, 0);

	//#ifdef _DEBUG
	cvShowImage("ImageRGB", imgRGB);
	//cvShowImage("ImageDepth", iDepth);
	//#endif
	//cvWaitKey(10);
}

void ShopDetector::drawBlobsInGondola(vector<Blob*> blobs , IplImage *imgRGB)
{
	int blobsInROI = 0;

	for (unsigned int i=0; i<blobs.size() ; i++){
		if ( blobs[i]->inROIGondola)
			blobsInROI++;
	}

	char convB[10];
	_itoa(blobsInROI,convB,10);
	std::string s2(convB);

	cvPutText(imgRGB, s2.c_str(), cvPoint(300,30), &font, cvScalar(255, 255, 255));
	cvRectangle(imgRGB, cvPoint(RoI_Information::xMinROI,RoI_Information::yMinROI),cvPoint(RoI_Information::xMaxROI,RoI_Information::yMaxROI), cvScalar(0, 255, 255), 3, 8, 0);

}

void ShopDetector::saveAllDetections(string output)
{
	string data = "ID;firstFrame;lastFrame;inputX;inputY";
	Export::ExportToCSV(output,data,false);
	for (unsigned int i = 0; i < allBlobs.size(); i++)
	{
		int fF = 0; // First Frame
		int lF = 1; // Last Frame
		int iX = 0; // input X
		int iY = 0; // input Y
		data = IntToString( fF)+";"+ IntToString( allBlobs[i]->globalID())+";"+IntToString( lF)
			+";"+IntToString( iX)+";"+IntToString( iY);

		Export::ExportToCSV(output,data,true);
	}
}






