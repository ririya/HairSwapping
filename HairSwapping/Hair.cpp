#include <opencv2//core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>

#include "Hair.h"

Hair::Hair(Mat p_hairMask, Mat p_hairPixels, Mat p_hairMaskNoMatting, int p_hairConnectionPointLocationX, int p_hairConnectionPointLocationY, int p_hairConnectionPointDistanceToJ_X, int p_hairConnectionPointDistanceToJ_Y)
{
	hairMaskNoMatting = p_hairMaskNoMatting;
	hairMask = p_hairMask;
	hairPixels = p_hairPixels;
	hairConnectionPointLocationX = p_hairConnectionPointLocationX;
	hairConnectionPointLocationY = p_hairConnectionPointLocationY;
	hairConnectionPointDistanceToJ_X = p_hairConnectionPointDistanceToJ_X;
	hairConnectionPointDistanceToJ_Y = p_hairConnectionPointDistanceToJ_Y;

	meanStdDev(hairPixels, hairMean, hairStd, hairMask);

}

Mat Hair::getHairPixels()
{
	return hairPixels;
}

Mat Hair::getHairMask()
{
	return hairMask;
}

Mat Hair::getHairMaskNoMatting()
{
	return hairMaskNoMatting;
}

int Hair::getHairConnectionPointLocationX()
{
	return hairConnectionPointLocationX;
}

int Hair::getHairConnectionPointLocationY()
{
	return hairConnectionPointLocationY;
}

int Hair::getHairConnectionPointDistanceToJ_X()
{
	return hairConnectionPointDistanceToJ_X;
}

int Hair::getHairConnectionPointDistanceToJ_Y()
{
	return hairConnectionPointDistanceToJ_Y;
}

Scalar Hair::getHairMean()
{
	return hairMean;
}

Scalar Hair::getHairStd()
{
	return hairStd;
}