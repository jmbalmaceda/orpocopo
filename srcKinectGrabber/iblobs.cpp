#include "iblobs.h"

using namespace std;

int Blob::globalID() 
{ 
	return gID ;
}



int Blob::globalIDGenerator = 0;

Blob::Blob(int frameNum)  
	{ 
		gID = globalIDGenerator++;

		numPixels = 0;
		visible = true;
		activated = true;
		inROIGondola = false;
		saveCapture = false;
		minX =	minY = 10000;
		maxX =	maxY = -10000;	
		depth = 0;
		bestMatch = NULL; 
		parent = NULL; 
		activated = 1;
		bestMatchValue = MATCHING_TOLERANCE;
		frameDate = time(0);
		frameNumber = frameNum;
		// convert now to string form
       //char* dt = ctime(&frameDate);
		
	}

	void  Blob::calcCenter()
	{
       if ( blobsHistory.size() > 0)
		{
			int i = blobsHistory.size()-1 ;
			Blob* bh = blobsHistory[i];
			posX = (bh->maxX + bh->minX)  / 2;
		    posY = (bh->maxY + bh->minY)  / 2;
		}
	    else
	    {
		    posX = (maxX + minX)  / 2;
		    posY = (maxY + minY)  / 2;
	   }
	}

	double Blob::historyLength(int index)
	{
		double in = 0;
		int to = min((int)index, (int)blobsHistory.size());

		for (int i = 1 ; i< to ; i++)		
		{			
			Blob* b = blobsHistory[i];
			Blob* bh = blobsHistory[i-1];
			in += sqrt((double)(b->posX - bh->posX)* (b->posX - bh->posX) + 
			            (double)(b->posY - bh->posY)* (b->posY - bh->posY));			
		}

		return in ; 
	}

	void Blob::addPixel(int x, int y)
	{
		if (x < minX) minX = x;
		if (x > maxX) maxX = x;
		
		if (y < minY) minY = y;
		if (y > maxY) maxY = y;
		posX = (maxX + minX)  / 2;
		posY = (maxY + minY)  / 2;

		numPixels ++;
	}

	int Blob::contains(Blob* b)
	{
		bool noOverlap = this->minX > b->maxX ||
                     b->minX > this->maxX ||
					 this->minY > b->maxY ||
                     b->minY > this->maxY;

       return !noOverlap;
	}


	int Blob::isJoined(Blob* b)
	{
		bool noOverlap = this->minX > b->maxX ||
                     b->minX > this->maxX ||
					 this->minY > b->maxY ||
                     b->minY - 1 > this->maxY +1;

       return !noOverlap;
	}

	double Blob::match(Blob* b)
	{
		double in = sqrt((double)(b->posX - this->posX)* (b->posX - this->posX) + 
			            (double)(b->posY - this->posY)* (b->posY - this->posY));
		
		//for (int i = blobsHistory.size()-1 ; i>=0; i--)
		if ( blobsHistory.size() > 0)
		{
			int i = blobsHistory.size()-1 ;
			Blob* bh = blobsHistory[i];
			double bp = sqrt((double)(b->posX - bh->posX)* (b->posX - bh->posX) + 
			            (double)(b->posY - bh->posY)* (b->posY - bh->posY));
			in = min(bp , in);
		}
		
		return in;

	}

	void Blob::dotag()
	{		
		if (this->parent) return ; 
		// Sort childs
		for (unsigned int i = 0; i<blobsChild.size() ;i++)
		for (unsigned int j = i+1; j<blobsChild.size() ;j++)
		{
			if (blobsChild[i]->depth < blobsChild[j]->depth)
			{
				Blob* aux = blobsChild[i];
				blobsChild[i]  =blobsChild[j];
				blobsChild[j] = aux;
			}
		}
		// Assign a Child
		for (unsigned int i = 0; i<blobsChild.size() ;i++)
		{
			Blob* bs = blobsChild[i];
			if ( i == 0) 
				bs->tag = TAG_HEAD;
			else
			{
				Blob* bant = blobsChild[i-1];
				// La altura es menor, tomo el tag siguiente
				if (bs->depth < bant->depth) 
					bs->tag = bant->tag +1 ;
				// Sino, tomo el mismo tag
				else
					if (bs->depth == bant->depth) 
						bs->tag = bant->tag ;//si es igual y no es i=0 -> puede ser mano
					else
					bs->tag = bant->tag  ;

			}
		}
		
	}

	int Blob::computeAngle()
	{
		int xmin, xmax, ymin, ymax;
		xmin = ymin = 10000;
		xmax = ymax = -1000;
		
		for (unsigned int i = 0; i<blobsChild.size() ;i++)
		{
			Blob* bs = blobsChild[i];
			if (bs->tag == TAG_SHOULDER) 
			{
				xmin = min(xmin , bs->minX);
				ymin = min(ymin , bs->minY);
				
				xmax = max(xmax , bs->maxX);
				ymax = max(ymax , bs->maxY);
			}
		}
		//--- retorno el angulo
		if (( xmax - xmin) > (ymax - ymin) )
			return 0;
		else 
			return 90;
	}

	void Blob::setROIGondola(int xMin,int yMin,int xMax,int yMax){
		bool noOverlap = this->minX > xMax ||
                     xMin > this->maxX ||
					 this->minY > yMax ||
                    yMin > this->maxY;

       	this->inROIGondola = !noOverlap;
	}
   /* calcCenter();

    //si el centro del rectangulo del blob está dentro de ROI -> lo agrego a la lista
    if (this->posX > xMin)
		 if (this->posX < xMax) 
			if (this->posY > yMin)
				if (this->posY < yMax)
				{
					this->inROIGondola = true;
				}*/
//}

//-----------------------------------------------------------------------------
//  matchBlobs : Hace un match entre los "Blobs Nuevos" y los "Historicos" (SOLO SE USA EN MODO DEBUG, LA DLL NO LO UTILIZA)
//---------------------------------------------------------------------------
void matchBlobs(vector<Blob*> newBlobs, vector<Blob*>* allBlobs, int xMinR, int yMinR, int xMaxR, int yMaxR)
{
	
	for (unsigned int j=0; j<newBlobs.size() ; j++)
	{
		newBlobs[j]->calcCenter();
		newBlobs[j]->dotag();
	}

	for (unsigned  int i=0 ;  i< allBlobs->size() ; i++)
	{			
		Blob* bo = allBlobs->at(i);
		bo->flag = 0;
		bo->bestMatchValue = MATCHING_TOLERANCE;
	} 

	// Busco los mejores matchs
	for (unsigned int j=0; j<newBlobs.size() ; j++)
	{
		int matched = -1;
		double bestMatch = MATCHING_TOLERANCE;
        Blob* bn = newBlobs[j];
		if (!bn->visible) continue;

		for (unsigned int i=0 ;  i< allBlobs->size() ; i++)
		{			
			Blob* bo = allBlobs->at(i);
			if (!bo->activated) continue;
			// Ya se utilizo
			//if (bo->flag) continue;
			double match = bo->match(bn);
			// Esta muy alejado, lo descarto
			if (match > MATCHING_TOLERANCE ) continue; 

			if (match > bo->bestMatchValue){
				bn->visible = false;
				continue;
			}

			if (bestMatch > match)
			{
				bestMatch = match;
				matched = i;
				//marco al blob de allblob cual es el mejor valor del match
				bo->bestMatchValue = match;
			}			
		}
		// Encontro un matching
		if (matched>=0)
		{
			bn->bestMatch = allBlobs->at(matched);
			// Marco como usado
			allBlobs->at(matched)->flag = 1;			
			//allBlobs[matched]->blobsHistory.push_back(bn);
		}
   }

	// Actualizo la lista de Blobs
	for (unsigned int j=0; j<newBlobs.size() ; j++)
	{
		 Blob* bn = newBlobs[j];
		 if (!bn->visible) continue;

		 //verifica si el blob esta en la ROI
		 bn->setROIGondola(xMinR,yMinR,xMaxR,yMaxR);
		 			 
		 //if (bn->minX > 610)// verificar si el blob esta dentro de la region de aceptacion para que sea valido

		 if (bn->bestMatch )
			 bn->bestMatch->blobsHistory.push_back(bn); 
		 else{
			 if (allBlobs->size() == 2){
				string a = "";
			 }

			 // Agrego uno nuevo
			 allBlobs->push_back(bn);
		 }
	}
}

//	(SOLO SE USA EN MODO DEBUG, LA DLL NO LO UTILIZA)
void updateBlobsState(vector<Blob*>* allBlobs)
{
		/// Desactivo los blobs q no se marcaron
	for (unsigned int i=0 ;  i< allBlobs->size() ; i++)
		{			
			Blob* bo = allBlobs->at(i);
			if ((!bo->flag) && (bo->activated >0)) 
				bo->activated --;

	    } 

}



