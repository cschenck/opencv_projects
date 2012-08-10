// OpenCV_Helloworld.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

#include <stdio.h>

#include <cv.h>
#include <cxcore.h>
#include <highgui.h>

#define MAX_POINTS 400
#define MAIN_WINDOW "Display"
#define OPTIONS_WINDOW "Options"

static const double pi = 3.14159265358979323846;

void setOptions(const char* title, std::vector<const char*> options)
{
	CvFont font;
	double hScale=1.0;
	double vScale=1.0;
	int    lineWidth=1;
	cvInitFont(&font,CV_FONT_HERSHEY_TRIPLEX, hScale,vScale,0,lineWidth);

	CvSize size;
	int baseline;
	cvGetTextSize(title, &font, &size, &baseline);
	size.height += baseline;
	for(int i = 0; i < options.size(); i++)
	{
		CvSize s;
		cvGetTextSize(options[i], &font, &s, &baseline);
		size.height += baseline + s.height;
		if(s.width > size.width)
			size.width = s.width;
	}
	size.width += 10;

	IplImage* img;
	img = cvCreateImage(size, IPL_DEPTH_8U, 3);
	cvSet(img, CV_RGB(0,0,0));

	int height = 0;
	cvGetTextSize(title, &font, &size, &baseline);
	height = size.height;
	cvPutText(img,title,cvPoint(0,height), &font, cvScalar(0,0,255));
	height += baseline;

	for(int i = 0; i < options.size(); i++)
	{
		cvGetTextSize(options[i], &font, &size, &baseline);
		height += size.height;
		cvPutText(img,options[i],cvPoint(0,height), &font, cvScalar(0,0,255));
		height += baseline;
	}

	cvShowImage(OPTIONS_WINDOW, img);
	cvReleaseImage(&img);
}

void drawOpticalFlowLines(IplImage* img, CvPoint2D32f* previous, CvPoint2D32f* next, char* status, int numFeatures)
{
	/* For fun (and debugging :)), let's draw the flow field. */
    for(int i = 0; i < numFeatures; i++)
    {
		/* If Pyramidal Lucas Kanade didn't really find the feature, skip it. */
		if ( status[i] == 0 )  continue;
		int line_thickness;     line_thickness = 1;
		/* CV_RGB(red, green, blue) is the red, green, and blue components
		* of the color you want, each out of 255.
		*/
		CvScalar line_color;    line_color = CV_RGB(255,0,0);
		/* Let's make the flow field look nice with arrows. */
		/* The arrows will be a bit too short for a nice visualization because of the 
		high framerate
		*  (ie: there's not much motion between the frames).  So let's lengthen them 
		by a factor of 3.
		*/
		CvPoint p,q;
		p.x = (int) previous[i].x;
		p.y = (int) previous[i].y;
		q.x = (int) next[i].x;
		q.y = (int) next[i].y;
		double angle;   angle = atan2( (double) p.y - q.y, (double) p.x - q.x );
		double hypotenuse;  hypotenuse = sqrt( (double)(p.y - q.y)*(p.y - q.y) + (p.x - q.x)*(p.x - q.x) );

		/* Here we lengthen the arrow by a factor of three. */
		q.x = (int) (p.x - 3 * hypotenuse * cos(angle));
		q.y = (int) (p.y - 3 * hypotenuse * sin(angle));
		/* Now we draw the main line of the arrow. */
		/* "frame1" is the frame to draw on.
		*  "p" is the point where the line begins.
		*  "q" is the point where the line stops.
		*  "CV_AA" means antialiased drawing.
		*  "0" means no fractional bits in the center cooridinate or radius.
		*/
		cvLine( img, p, q, line_color, line_thickness, CV_AA, 0 );
		/* Now draw the tips of the arrow.  I do some scaling so that the
		* tips look proportional to the main line of the arrow.
		*/   
		p.x = (int) (q.x + 9 * cos(angle + pi / 4));
		p.y = (int) (q.y + 9 * sin(angle + pi / 4));
		cvLine( img, p, q, line_color, line_thickness, CV_AA, 0 );
		p.x = (int) (q.x + 9 * cos(angle - pi / 4));
		p.y = (int) (q.y + 9 * sin(angle - pi / 4));
		cvLine( img, p, q, line_color, line_thickness, CV_AA, 0 );
    }
}

void displayOpticalFlowOptions()
{
	std::vector<const char*> options;
	options.push_back("\'s\' = stop optical flow");
	setOptions("Optical Flow Menu", options);
}

void detectOpticalFlow(CvCapture* capture)
{
	displayOpticalFlowOptions();

	IplImage * frame;
	IplImage* gray_frame;
	IplImage* last_gray_frame;
	IplImage* eig_image;
	IplImage* temp_image;
	IplImage* pyramid1;
	IplImage* pyramid2;
    int key = 0;

	CvPoint2D32f prev_features[MAX_POINTS];
	CvPoint2D32f next_features[MAX_POINTS];
	int numPoints = MAX_POINTS;

	frame = cvQueryFrame( capture );
	gray_frame = cvCreateImage(cvGetSize(frame), IPL_DEPTH_8U, 1);
	last_gray_frame = cvCreateImage(cvGetSize(frame), IPL_DEPTH_8U, 1);
	cvCvtColor(frame, gray_frame, CV_BGR2GRAY);

	CvSize frame_size;
	frame_size.height = (int) cvGetCaptureProperty( capture, CV_CAP_PROP_FRAME_HEIGHT );
	frame_size.width = (int) cvGetCaptureProperty( capture, CV_CAP_PROP_FRAME_WIDTH );

	eig_image = cvCreateImage(frame_size, IPL_DEPTH_32F, 1);
	temp_image = cvCreateImage(frame_size, IPL_DEPTH_32F, 1);
	pyramid1 = cvCreateImage(frame_size, IPL_DEPTH_8U, 1);
	pyramid2 = cvCreateImage(frame_size, IPL_DEPTH_8U, 1);

	cvGoodFeaturesToTrack(gray_frame, eig_image, temp_image, next_features, &numPoints, 0.01, 0.01, NULL); 
	
	while( key != 's' ) {
        /* get a frame */
        frame = cvQueryFrame( capture );
 
        /* always check */
        if( !frame ) break;

		cvCopyImage(gray_frame, last_gray_frame);
		cvCvtColor(frame, gray_frame, CV_BGR2GRAY);

		numPoints = MAX_POINTS;
		cvGoodFeaturesToTrack(last_gray_frame, eig_image, temp_image, prev_features, &numPoints, 0.01, 0.01, NULL); 

		CvSize optical_flow_window = cvSize(3,3);
		char optical_flow_found_feature[MAX_POINTS];
		float optical_flow_feature_error[MAX_POINTS];
		CvTermCriteria optical_flow_termination_criteria = cvTermCriteria( CV_TERMCRIT_ITER | CV_TERMCRIT_EPS, 20, .3 );

		cvCalcOpticalFlowPyrLK(last_gray_frame, gray_frame, pyramid1, pyramid2, prev_features, next_features,
			numPoints, optical_flow_window, 5, optical_flow_found_feature, optical_flow_feature_error, optical_flow_termination_criteria, 0);

		drawOpticalFlowLines(frame, prev_features, next_features, optical_flow_found_feature, numPoints);

		

        cvShowImage(MAIN_WINDOW, frame);

        key = cvWaitKey( 10 );
    }
}

void displayMainMenuOptions()
{
	std::vector<const char*> options;
	options.push_back("\'o\' = run optical flow");
	options.push_back("\'q\' = quit");
	setOptions("Main Menu", options);
}


int _tmain(int argc, _TCHAR* argv[])
{
	CvCapture * capture;
    IplImage * frame;
    int key = 0;
    
	//initialize camera
    capture = cvCaptureFromCAM( 0 );
 
    //just to be sure
    assert(capture );

	cvNamedWindow(MAIN_WINDOW, CV_WINDOW_AUTOSIZE);
	cvNamedWindow(OPTIONS_WINDOW, CV_WINDOW_AUTOSIZE);

	displayMainMenuOptions();

	while( key != 'q' ) {
       
        key = cvWaitKey( 10 );

		if(key == 'o')
		{
			detectOpticalFlow(capture);
			displayMainMenuOptions();
		}
		else
		{
			 /* get a frame */
			frame = cvQueryFrame( capture );
 
			/* always check */
			if( !frame ) break;

			cvShowImage(MAIN_WINDOW, frame);
		}
    }
 
    /* free memory */
    cvReleaseCapture( &capture );
    cvDestroyWindow(MAIN_WINDOW);


	return 0;
}