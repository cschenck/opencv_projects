
#include "stdafx.h"
#include "my_utils.h"


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