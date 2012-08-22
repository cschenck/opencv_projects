#ifndef __MY_UTILS__H__
#define __MY_UTILS__H__

#include "stdafx.h"

#include <stdio.h>

#include <cv.h>
#include <cxcore.h>
#include <highgui.h>

#define OPTIONS_WINDOW "Options"
#define MAIN_WINDOW "Display"

#define max(a,b) (a > b ? a : b)
#define min(a,b) (a < b ? a : b)

//float max(float a, float b) {return (a > b ? a : b);}
//float min(float a, float b) {return (a < b ? a : b);}

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



void RGB2HSV(int r, int g, int b, int &h, int &s, int &v); // Point conversion

void setOptions(const char* title, std::vector<const char*> options);

#endif
