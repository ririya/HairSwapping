#include "faceRecognition.h"
using namespace cv;

#ifndef HAIR_EXTRACTION_H
#define HAIR_EXTRACTION_H

class Hair
{
	Mat hairMask;
	Mat hairPixels;
	
	int hairConnectionPointLocationX;
	int hairConnectionPointLocationY;

	int hairConnectionPointDistanceToJ_X;
	int hairConnectionPointDistanceToJ_Y;

public:

	Hair()
	{

	}

	Hair(Mat p_hairMask,Mat p_hairPixels, int p_hairConnectionPointLocationX, int p_hairConnectionPointLocationY, int p_hairConnectionPointDistanceToJ_X, int p_hairConnectionPointDistanceToJ_Y)
	{
		hairMask = p_hairMask;
		hairPixels = p_hairPixels;
		hairConnectionPointLocationX = p_hairConnectionPointLocationX;
		hairConnectionPointLocationY = p_hairConnectionPointLocationY;
		hairConnectionPointDistanceToJ_X = p_hairConnectionPointDistanceToJ_X;
		hairConnectionPointDistanceToJ_Y = p_hairConnectionPointDistanceToJ_Y;
	}

	Mat getHairPixels()
	{
		return hairPixels;
	}

	Mat getHairMask()
	{
		return hairMask;
	}

	int getHairConnectionPointLocationX()
	{
		return hairConnectionPointLocationX;
	}

	int getHairConnectionPointLocationY()
	{
		return hairConnectionPointLocationY;
	}

	int getHairConnectionPointDistanceToJ_X()
	{
		return hairConnectionPointDistanceToJ_X;
	}

	int getHairConnectionPointDistanceToJ_Y()
	{
		return hairConnectionPointDistanceToJ_Y;
	}
};

int extractHair(Mat img, Face face, Mat *labels, Hair *hair);
void getBackgroundCenter(Mat img, Mat centers);
void getClothesCenter(Mat img, Mat centers);
void getSkinCenter(Mat img, Mat skinMask, Mat centers);
void getHairCenter(Mat img, int upperPointX, int upperPointY, Mat centers);
void viewCenters(Mat centers);
Mat prepareImageForKmeans(Mat img);
Mat segmentImage(Mat pixelSequence, Mat centers, int nRows, int nCols);
Mat performKmeans(Mat pixelSequence, Mat centers, int maxIterations, int nRows, int nCols);
Mat getLabelsImage(Mat labels, int nRows, int nCols);
Mat reconstructImage3D(Mat pixelSequence, int nRows, int nCols);
Mat reconstructImage1D(Mat pixelSequence, int nRows, int nCols);
Mat findHairPixels(Mat pixelSequence, Mat labels, int nRows, int nCols, int upperPointX, int upperPointY);
void FindBlobs(Mat &binary, vector < vector<Point2i> > &blobs);
int findHairBlob(vector<vector<Point2i>> blobs, int upperPointX, int upperPointY);
Mat performMatting(Mat *hairImageMask, Mat Image);
Hair findConnectionPoint(Mat hairMask, Mat hairPixels, Face face);
static const int OFFSET_HAIR = 20;
static const int N_CENTERS = 4;

static const int BACKGROUND_CENTER_INDEX = 0;
static const int CLOTHES_CENTER_INDEX = 3;
static const int SKIN_CENTER_INDEX = 2;
static const int HAIR_CENTER_INDEX = 1;
static const int USE_MATTING = 1;

static const int EROSION_SIZE = 10;

#endif // HAIR_EXTRACTION_H
