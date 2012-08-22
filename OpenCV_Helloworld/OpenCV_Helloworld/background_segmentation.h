#ifndef __BACKGROUND_SEGMENTATION__H__
#define __BACKGROUND_SEGMENTATION__H__

#include "stdafx.h"

#include <stdio.h>

#include <cv.h>
#include <cxcore.h>
#include <highgui.h>

#include "my_utils.h"


#define MIN_DEVIATION_FROM_BACKGROUND 4
#define NUM_BACKGROUND_TRAINING_FRAMES 30


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

void setSegmentBackgroundFlag(bool* flag);

void trainBackgroundModel(CvCapture* capture);

IplImage* getMask(IplImage* frame, IplImage* oldMask = NULL);

#endif