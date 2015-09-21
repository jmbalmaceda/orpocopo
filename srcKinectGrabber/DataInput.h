
#include <opencv\cv.h>
#include <opencv\highgui.h>

#include <NuiApi.h>
#include <NuiImageCamera.h>
#include <NuiSkeleton.h>
#include <NuiSensor.h>

#define DEPTH_MODE_TRUNC_8  1
// Salva el Depth replicado.. los primeros 8 son los resumidos, los siguientes sin comprimir
#define DEPTH_MODE_TRUNC_8_16 2

typedef unsigned char GLubyte;
static const int        cDepthWidth  = 640;
static const int        cDepthHeight = 480;
static const int        cBytesPerPixel = 3;
static const int        cStatusMessageMaxLen = MAX_PATH*2;


class DataInput
{
public : 
	CvSize frame_size; 
	CvCapture* cRGB;
	CvCapture* cDepth;

	IplImage* iRGB ;
	IplImage* iDepth ;

	
	int numFrames ;
	int actualFrame ;
	int width , height ;
	virtual void seek(int nframe) {  }
	virtual bool init() { return true; } 
	virtual bool eof() { return false; }
	virtual IplImage* getimgRGB() { return iRGB ; };
	virtual IplImage* getimgDepth(){ return iDepth ; };

	
byte CalculateIntensityFromDistance(int distance,BOOL nearMode)
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


	bool readImages() 
    {
       getimgRGB();
       getimgDepth();

	   return (iRGB!=NULL) && (iDepth!=NULL);

    }

} ;

class VideoDataInput : public DataInput
{
public: 	
	int depthMode ;
	IplImage* cvtDepth ;
	VideoDataInput(char *rgbFile, char* depthFile, int Depth_Mode = 1)
	{
     	cRGB = cvCreateFileCapture(rgbFile);
		

		// Read the video's frame size out of the AVI. 
		frame_size.height = (int) cvGetCaptureProperty( cRGB, CV_CAP_PROP_FRAME_HEIGHT ); 

        frame_size.width = (int) cvGetCaptureProperty( cRGB, CV_CAP_PROP_FRAME_WIDTH );

		cDepth = cvCreateFileCapture(depthFile);
        
        numFrames =MIN( (int) cvGetCaptureProperty(cRGB, CV_CAP_PROP_FRAME_COUNT) , 
			           (int) cvGetCaptureProperty(cDepth, CV_CAP_PROP_FRAME_COUNT)); 

		

		

        depthMode = Depth_Mode;

		cvtDepth = 0;
		
		actualFrame = 0;
	}

	void seek(int nframe)
	{
		cvSetCaptureProperty( cRGB, CV_CAP_PROP_POS_FRAMES, nframe ); 
		cvSetCaptureProperty( cDepth, CV_CAP_PROP_POS_FRAMES, nframe ); 
	}
	bool init()
	{
		iDepth = cvCreateImage(cvSize(cDepthWidth,cDepthHeight),IPL_DEPTH_8U,cBytesPerPixel);
	    iRGB = cvCreateImage(cvSize(cDepthWidth,cDepthHeight),IPL_DEPTH_8U,cBytesPerPixel);

		return (cRGB != NULL) && (cDepth!=NULL) ;
	}

	IplImage* getimgRGB() 
	{ 
		actualFrame++;
		iRGB = cvQueryFrame(cRGB); 
		 return  iRGB; 
	};
	
	IplImage* getimgDepth(){ 

		if (depthMode == 1)
		{
		  iDepth = cvQueryFrame(cDepth); 
		  return  iDepth;
		}
		else
		{
		//	if (!cvtDepth) 	cvCreateImage(cvSize(cDepthWidth,cDepthHeight),IPL_DEPTH_8U,cBytesPerPixel);
			
			cvtDepth = cvQueryFrame(cDepth);

		   if (!cvtDepth) return NULL;

		   for (int y = 0; y<cvtDepth->height ; y++)
	        {
		         uchar* rowSrc =  (uchar*)( cvtDepth->imageData + y * cvtDepth->widthStep ) ;
				 uchar* rowDst =  (uchar*)( iDepth->imageData + y * iDepth->widthStep ) ;
		         for (int x = 0; x<iDepth->width  ; x++)
		         {
					 int depth = rowSrc[3*x]   ;
					 BYTE intensity = CalculateIntensityFromDistance(depth,DEPTH_MODE_TRUNC_8);
					 rowDst[3*x] = depth;
					 rowDst[3*x+1] = depth;
					 rowDst[3*x+2] = depth;
		         }
			}
		  return iDepth;
		}
	};

	bool eof() 
		
	{ 		
		return (actualFrame == numFrames); 
	}


} ;

class KinectInput : public DataInput
{
public : 
	HANDLE depthStream;
	HANDLE rgbStream;
	INuiSensor* sensor;


	HANDLE   m_hNextDepthFrameEvent;
	HANDLE   m_pDepthStreamHandle;

	HANDLE   m_hNextRGBFrameEvent;
	HANDLE   m_pRGBStreamHandle;
	BYTE*                   m_depthRGBX;

	GLubyte *data;
    GLubyte *dataRGB;

	int depthImageMode ;

	KinectInput(int imageMode = DEPTH_MODE_TRUNC_8)
	{
		width = 640;
		height = 480;
		data = (GLubyte *)malloc(width*height*cBytesPerPixel);
        dataRGB = (GLubyte *)malloc(width*height*cBytesPerPixel);
		depthImageMode = depthImageMode;
	}


HRESULT initKinect() 
{
    // Get a working kinect sensor
    HRESULT hr;
	INuiSensor * pNuiSensor;

    int iSensorCount = 0;
    hr = NuiGetSensorCount(&iSensorCount);
    if (FAILED(hr))
    {
        return hr;
    }
	sensor = NULL;

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


bool init() {
	
	iDepth = cvCreateImage(cvSize(cDepthWidth,cDepthHeight),IPL_DEPTH_8U,cBytesPerPixel);
	iRGB = cvCreateImage(cvSize(cDepthWidth,cDepthHeight),IPL_DEPTH_8U,cBytesPerPixel);

	return initKinect() == S_OK ; 

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

	cvSetData(iRGB,dest,width * cBytesPerPixel);
}


IplImage* getimgRGB() 
{ 
	if ( WAIT_OBJECT_0 == WaitForSingleObject(m_hNextRGBFrameEvent, 0) )
    {
       getKinectRGBData(dataRGB);
	   return iRGB;
		//cvShowImage("rgb",rgb);
    }
	else
		return NULL;
}


byte CalculateIntensityFromDistance(int distance,BOOL nearMode)
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
			
			if (this->depthImageMode == DEPTH_MODE_TRUNC_8)
			{
	   		    BYTE intensity = CalculateIntensityFromDistance(depth,nearMode);
				// Write out blue byte
				*(rgbrun++) = intensity;
				// Write out green byte
				*(rgbrun++) = intensity;
				// Write out red byte
				*(rgbrun++) = intensity;
			}
			else
			{
				BYTE intensity = CalculateIntensityFromDistance(depth,nearMode);
				// Write out blue byte
				*(rgbrun++) = (BYTE)intensity;
				// Write out green byte
				*(rgbrun++) = (BYTE)(depth / 256);
				// Write out red byte
				*(rgbrun++) = (BYTE)(depth % 256);
			}
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

	cvSetData(iDepth,dest,width * cBytesPerPixel);
	

ReleaseFrame:
    // Release the frame
    sensor->NuiImageStreamReleaseFrame(m_pDepthStreamHandle, &imageFrame);
}


IplImage* getimgDepth()
{ 
	 if ( WAIT_OBJECT_0 == WaitForSingleObject(m_hNextDepthFrameEvent, 0) )
    {
       
       getKinectData(data);
      // int size;    

	   return iDepth;	  
    }
	 else
		  return nullptr;
};




} ;