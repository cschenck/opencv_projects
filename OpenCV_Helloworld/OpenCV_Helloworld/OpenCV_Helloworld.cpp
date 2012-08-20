// OpenCV_Helloworld.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

#include <stdio.h>

#include <cv.h>
#include <cxcore.h>
#include <highgui.h>

#define MAX_POINTS 500
#define MAIN_WINDOW "Display"
#define OPTIONS_WINDOW "Options"
#define FEATURES_MIN_DISTANCE 0.9
#define FEATURES_QUALITY_LEVEL 0.01
#define MIN_DEVIATION_FROM_BACKGROUND 4
#define NUM_BACKGROUND_TRAINING_FRAMES 30

static const double pi = 3.14159265358979323846;

class RunningStat
{
public:
    RunningStat() : m_n(0) {}

    void Clear()
    {
        m_n = 0;
    }

    void Push(double x)
    {
        m_n++;

        // See Knuth TAOCP vol 2, 3rd edition, page 232
        if (m_n == 1)
        {
            m_oldM = m_newM = x;
            m_oldS = 0.0;
        }
        else
        {
            m_newM = m_oldM + (x - m_oldM)/m_n;
            m_newS = m_oldS + (x - m_oldM)*(x - m_newM);
    
            // set up for next iteration
            m_oldM = m_newM; 
            m_oldS = m_newS;
        }
    }

    int NumDataValues() const
    {
        return m_n;
    }

    double Mean() const
    {
        return (m_n > 0) ? m_newM : 0.0;
    }

    double Variance() const
    {
        return ( (m_n > 1) ? m_newS/(m_n - 1) : 0.0 );
    }

    double StandardDeviation() const
    {
        return sqrt( Variance() );
    }

private:
    int m_n;
    double m_oldM, m_newM, m_oldS, m_newS;
};

float max(float a, float b) {return (a > b ? a : b);}
float min(float a, float b) {return (a < b ? a : b);}

template<class T> class Image
{
  private:
  IplImage* imgp;
  public:
  Image(IplImage* img=0) {imgp=img;}
  ~Image(){imgp=0;}
  void operator=(IplImage* img) {imgp=img;}
  inline T* operator[](const int rowIndx) {
    return ((T *)(imgp->imageData + rowIndx*imgp->widthStep));}
};

typedef struct{
  unsigned char b,g,r;
} RgbPixel;

typedef struct{
  float b,g,r;
} RgbPixelFloat;

typedef Image<RgbPixel>       RgbImage;
typedef Image<RgbPixelFloat>  RgbImageFloat;
typedef Image<unsigned char>  BwImage;
typedef Image<float>          BwImageFloat;

void RGB2HSV(int r, int g, int b, int &h, int &s, int &v) // Point conversion
{
	static float F = 255;
	static float Fudge = (float) .0001;
	float R = ((float)r)/F, G = ((float)g)/F, B = ((float)b)/F, x, f, H, S, V;
	int i;
	V = max(max(R, G), B);
	x = min(min(R, G), B);
	if(V == x) // fudge otherwise invalid mapping
	{
		if(x < Fudge) 
		{ 
			x = G = B = 0; 
			V = R = Fudge; 
		}
		else if(V + Fudge > 1) 
		{ 
			V = G = B = 1; 
			x = R = 1 - Fudge; 
		}
		else
		{
			x -= Fudge; R = x; // a little less red
			V += Fudge; B = V; // a little more blue
		}
	}
	f = (R == x) ? G - B : ((G == x) ? B - R : R - G);
	i = (R == x) ? 3 : ((G == x) ? 5 : 1);
	H = i - f/(V-x);
	S = (V-x)/V;
	h = (int)(H * 30. + 0.5);
	s = (int)(S * F + 0.5);
	v = (int)(V * F + 0.5);
}

typedef struct {
	RunningStat h;
	RunningStat s;
	RunningStat v;
	void addDataPoint(int r, int g, int b) {
		int H, S, V;
		RGB2HSV(r, g, b, H, S, V);
		h.Push(H);
		s.Push(S);
		v.Push(V);
	}
	bool outlyer(int r, int g, int b) {
		int H, S, V;
		RGB2HSV(r, g, b, H, S, V);
		if(abs(H - h.Mean()) > MIN_DEVIATION_FROM_BACKGROUND*h.StandardDeviation()
			|| abs(S - s.Mean()) > MIN_DEVIATION_FROM_BACKGROUND*s.StandardDeviation())
			return true;
		else
			return false;
	}
} pixel_data;

pixel_data** background_model = NULL;
bool segment_background = false;

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

void trainBackgroundModel(CvCapture* capture)
{
	CvSize frame_size;
	frame_size.height = (int) cvGetCaptureProperty( capture, CV_CAP_PROP_FRAME_HEIGHT );
	frame_size.width = (int) cvGetCaptureProperty( capture, CV_CAP_PROP_FRAME_WIDTH );

	if(background_model != NULL)
		delete background_model;
	background_model = new pixel_data*[frame_size.height];
	for(int i = 0; i < frame_size.height; i++)
		background_model[i] = new pixel_data[frame_size.width];

	const char* title = "Training Background Model";
	std::vector<const char*> lines;

	for(int i = 5; i >= 0; i--)
	{
		char buffer[400];
		sprintf(buffer, "Starting training in %d ...", i);
		lines.clear();
		lines.push_back(buffer);

		setOptions(title, lines);
		cvWaitKey(1000);
	}

	for(int num_frames = 0; num_frames < NUM_BACKGROUND_TRAINING_FRAMES; num_frames++)
	{
        
		char buffer[400];
		sprintf(buffer, "%g%%", ((double)num_frames/NUM_BACKGROUND_TRAINING_FRAMES*100.0));
		lines.clear();
		lines.push_back(buffer);
		setOptions(title, lines);

		/* get a frame */
		IplImage* frame = cvQueryFrame( capture );

		RgbImage img(frame);
		for(int i = 0; i < frame->height; i++)
		{
			for(int j = 0; j < frame->width; j++)
			{
				background_model[i][j].addDataPoint(img[i][j].r, img[i][j].g, img[i][j].b);
			}
		}
 
		/* always check */
		if( !frame ) break;

		cvShowImage(MAIN_WINDOW, frame);
		
		cvWaitKey(10);
    }


	lines.push_back("Done");
	setOptions(title, lines);
	cvWaitKey(3000);
}

IplImage* getMask(IplImage* frame, IplImage* oldMask = NULL)
{
	IplImage* ret;
	if(oldMask == NULL)
		ret = cvCreateImage(cvGetSize(frame), IPL_DEPTH_8U, 1);
	else
		ret = oldMask;

	cvSet(ret, CV_RGB(255,255,255));

	if(background_model == NULL || !segment_background) //if the background model hasn't been initialized yet
		return ret;
	
	RgbImage img(frame);
	BwImage mask(ret);
	for(int i = 0; i < frame->height; i++)
	{
		for(int j = 0; j < frame->width; j++)
		{
			if(!background_model[i][j].outlyer(img[i][j].r, img[i][j].g, img[i][j].b))
				mask[i][j] = 0;
		}
	}

	return ret;
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