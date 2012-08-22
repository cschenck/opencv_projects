

#ifndef __OPTICAL_FLOW__H__
#define __OPTICAL_FLOW__H__

#include "stdafx.h"

#include <stdio.h>

#include <cv.h>
#include <cxcore.h>
#include <highgui.h>



#define MAX_POINTS 500
#define FEATURES_MIN_DISTANCE 0.9
#define FEATURES_QUALITY_LEVEL 0.01

void drawOpticalFlowLines(IplImage* img, CvPoint2D32f* previous, CvPoint2D32f* next, char* status, int numFeatures);

void displayOpticalFlowOptions();

void detectOpticalFlow(CvCapture* capture);


#endif

