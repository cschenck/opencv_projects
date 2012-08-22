
#include "stdafx.h"
#include "optical_flow.h"
#include "my_utils.h"
#include "background_segmentation.h"

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
	
	IplImage* mask = NULL;
	mask = getMask(frame);
	cvCopy(frame, frame, mask);

	gray_frame = cvCreateImage(cvGetSize(frame), IPL_DEPTH_8U, 1);
	last_gray_frame = cvCreateImage(cvGetSize(frame), IPL_DEPTH_8U, 1);
	cvCvtColor(frame, gray_frame, CV_BGR2GRAY);

	IplImage *mFrame;
	CvSize frame_size;
	frame_size.height = (int) cvGetCaptureProperty( capture, CV_CAP_PROP_FRAME_HEIGHT );
	frame_size.width = (int) cvGetCaptureProperty( capture, CV_CAP_PROP_FRAME_WIDTH );
	mFrame = cvCreateImage(frame_size, IPL_DEPTH_8U, 3);

	eig_image = cvCreateImage(frame_size, IPL_DEPTH_32F, 1);
	temp_image = cvCreateImage(frame_size, IPL_DEPTH_32F, 1);
	pyramid1 = cvCreateImage(frame_size, IPL_DEPTH_8U, 1);
	pyramid2 = cvCreateImage(frame_size, IPL_DEPTH_8U, 1);

	while( key != 's' ) {
        /* get a frame */
        frame = cvQueryFrame( capture );
 
        /* always check */
        if( !frame ) break;
		
		mask = getMask(frame, mask);
		cvSet(mFrame, CV_RGB(0,0,0));
		cvCopy(frame, mFrame, mask);

		cvCopyImage(gray_frame, last_gray_frame);
		cvCvtColor(mFrame, gray_frame, CV_BGR2GRAY);

		numPoints = MAX_POINTS;
		cvGoodFeaturesToTrack(last_gray_frame, eig_image, temp_image, prev_features, &numPoints, FEATURES_QUALITY_LEVEL, FEATURES_MIN_DISTANCE, 0); 

		CvSize optical_flow_window = cvSize(3,3);
		char optical_flow_found_feature[MAX_POINTS];
		float optical_flow_feature_error[MAX_POINTS];
		CvTermCriteria optical_flow_termination_criteria = cvTermCriteria( CV_TERMCRIT_ITER | CV_TERMCRIT_EPS, 20, .3 );

		cvCalcOpticalFlowPyrLK(last_gray_frame, gray_frame, pyramid1, pyramid2, prev_features, next_features,
			numPoints, optical_flow_window, 5, optical_flow_found_feature, optical_flow_feature_error, optical_flow_termination_criteria, 0);

		drawOpticalFlowLines(mFrame, prev_features, next_features, optical_flow_found_feature, numPoints);

		

        cvShowImage(MAIN_WINDOW, mFrame);

        key = cvWaitKey( 10 );
    }
}
