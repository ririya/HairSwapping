#include "faceRecognition.h"
#include "face.h"
#include "Hair.h"

using namespace std;
using namespace cv;

#ifndef HAIR_EXTRACTION_H
#define HAIR_EXTRACTION_H

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
Hair findConnectionPoint(Mat hairMask, Mat hairImageMaskNoMatting, Mat hairPixels, Face face);
static const int OFFSET_HAIR = 20;
static const int N_CENTERS = 4;

static const int BACKGROUND_CENTER_INDEX = 0;
static const int CLOTHES_CENTER_INDEX = 3;
static const int SKIN_CENTER_INDEX = 2;
static const int HAIR_CENTER_INDEX = 1;
static const int HAIR_CLOTHES_DIST_THRESHOLD = 10;
static const int USE_MATTING = 1;

static const int EROSION_SIZE = 11;

#endif // HAIR_EXTRACTION_H
