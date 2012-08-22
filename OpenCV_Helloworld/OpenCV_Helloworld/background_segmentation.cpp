
#include "stdafx.h"
#include "background_segmentation.h"

pixel_data** background_model = NULL;
bool* segment_background;

void setSegmentBackgroundFlag(bool* flag)
{
	segment_background = flag;
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

IplImage* getMask(IplImage* frame, IplImage* oldMask)
{
	IplImage* ret;
	if(oldMask == NULL)
		ret = cvCreateImage(cvGetSize(frame), IPL_DEPTH_8U, 1);
	else
		ret = oldMask;

	cvSet(ret, CV_RGB(255,255,255));

	if(background_model == NULL || !(*segment_background)) //if the background model hasn't been initialized yet
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