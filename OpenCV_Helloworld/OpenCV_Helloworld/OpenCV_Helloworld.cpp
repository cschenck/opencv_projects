// OpenCV_Helloworld.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

#include <stdio.h>

#include <cv.h>
#include <cxcore.h>
#include <highgui.h>

#include "my_utils.h"
#include "background_segmentation.h"
#include "optical_flow.h"



void displayMainMenuOptions()
{
	std::vector<const char*> options;
	options.push_back("\'o\' = run optical flow");
	options.push_back("\'b\' = toggle background segmentation");
	options.push_back("\'t\' = train background model");
	options.push_back("\'q\' = quit");
	setOptions("Main Menu", options);
}


int _tmain(int argc, _TCHAR* argv[])
{
	CvCapture * capture;
    IplImage * frame;
    int key = 0;
	bool segment_background = false;
	setSegmentBackgroundFlag(&segment_background);
    
	//initialize camera
    capture = cvCaptureFromCAM( 0 );
 
    //just to be sure
    assert(capture );

	cvNamedWindow(MAIN_WINDOW, CV_WINDOW_AUTOSIZE);
	cvNamedWindow(OPTIONS_WINDOW, CV_WINDOW_AUTOSIZE);

	displayMainMenuOptions();
	IplImage* mask = NULL;
	IplImage *mFrame;
	CvSize frame_size;
	frame_size.height = (int) cvGetCaptureProperty( capture, CV_CAP_PROP_FRAME_HEIGHT );
	frame_size.width = (int) cvGetCaptureProperty( capture, CV_CAP_PROP_FRAME_WIDTH );
	mFrame = cvCreateImage(frame_size, IPL_DEPTH_8U, 3);

	while( key != 'q' ) {
       
        key = cvWaitKey( 10 );

		if(key == 'o')
		{
			detectOpticalFlow(capture);
			displayMainMenuOptions();
		}
		else if(key == 'b')
		{
			segment_background = !segment_background;
			if(segment_background)
				printf("Segmenting the background\n");
			else
				printf("Not segmenting the background\n");
		}
		else if(key == 't')
		{
			trainBackgroundModel(capture);
			displayMainMenuOptions();
		}
		else
		{
			 /* get a frame */
			frame = cvQueryFrame( capture );
			cvSet(mFrame, CV_RGB(0,0,0));
 
			/* always check */
			if( !frame ) break;

			mask = getMask(frame, mask);
			cvCopy(frame, mFrame, mask);

			cvShowImage(MAIN_WINDOW, mFrame);
		}
    }
 
    /* free memory */
    cvReleaseCapture( &capture );
    cvDestroyWindow(MAIN_WINDOW);


	return 0;
}