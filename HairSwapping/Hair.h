#ifndef HAIR_H
#define HAIR_H

#include <opencv2//core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>

using namespace cv;

class Hair
{

	Mat hairMaskNoMatting;
	Mat hairMask;
	Mat hairPixels;

	int hairConnectionPointLocationX;
	int hairConnectionPointLocationY;

	int hairConnectionPointDistanceToJ_X;
	int hairConnectionPointDistanceToJ_Y;

	Scalar hairMean;
	Scalar hairStd;

public:

	Hair()
	{

	}

	Hair(Mat p_hairMask, Mat p_hairPixels, Mat p_hairMaskNoMatting, int p_hairConnectionPointLocationX, int p_hairConnectionPointLocationY, int p_hairConnectionPointDistanceToJ_X, int p_hairConnectionPointDistanceToJ_Y);
	
	Mat getHairPixels();

	Mat getHairMask();

	Mat getHairMaskNoMatting();

	int getHairConnectionPointLocationX();

	int getHairConnectionPointLocationY();

	int getHairConnectionPointDistanceToJ_X();

	int getHairConnectionPointDistanceToJ_Y();

	Scalar getHairMean();

	Scalar getHairStd();
};





#endif
