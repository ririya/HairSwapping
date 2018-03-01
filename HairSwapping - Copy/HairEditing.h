#ifndef HAIR_EDITING_H
#define HAIR_EDITING_H

#include <unordered_set>

Mat swapHair(Hair hair, Face face, Mat modelImg, Mat targetImg, Mat synthesizedFace);

Mat findBestScaleAndPosition(Mat synthesizedFace, Mat hairPixels, Face face, vector<Point> contours, int refPointX, int refPointY, int refTx, int refTy);

int calculateEnergyHoles(Mat hairSwap, Mat hairMask, vector<Point> contours, Face face, Mat skinPixels);
int calculateEnergyHairOverlap(Mat hairMask, Face face, Mat skinPixels);

void searchHoles(Mat hairSwap, Mat faceMask, Mat hairMask, int x, int y, std::unordered_set<int> *holes_hashSet);

void insertIntoHashSet(int y, int x, int nCols, std::unordered_set<int> *holes_hashSet);

int searchForEnclosingHair(Mat hairMask, int x_interest, int y_interest, int contourType);

void searchLineForHoles(Mat hairMask, int x_interest, int y_interest, int lastHairPixelInd, std::unordered_set<int> *holes_hashSet, int contourType);

Mat convertTo3channels(Mat mat);

Mat trySwapHair(Mat synthesizedFace, Mat scaledHair, Mat scaledHairMask, Face face, Mat skinPixels, vector<Point> contours, int* energyHoles, int* energyHairOverlap);


Mat scaleHairOld(Mat imgRGB, int refPointX, int refPointY, double scaleX, double scaleY, Scalar backgroundColor);

Mat scaleHair(Mat img, int refPointX, int refPointY, int refTx, int refTy, double scaleX, double scaleY, Scalar backgroundColor);

Mat createAlphaImage(Mat mat, Mat alpha);

static const int CONTOUR_TYPE_LEFTPIXEL = 0;
static const int CONTOUR_TYPE_RIGHTPIXEL = 1;
static const int CONTOUR_TYPE_TOPPIXEL = 2;

static const int BACKGROUND_HAIR_B = 0;
static const int BACKGROUND_HAIR_G = 0;
static const int BACKGROUND_HAIR_R = 0;

static const int ALPHA_THRESHOLD = 128;

static const int NUMBER_OF_FACE_COLUMNS_ALLOWED_HAIR = 10;

static const double ENERGY_WEIGHT = 2.0;

static const int MAX_TX = 10;
static const int MAX_TY = 30;
static const int STEP_T = 5;
static const double MIN_SCALE = 0.6;
static const double MAX_SCALE = 1.2;
static const double STEP_S = 0.1;

#endif
