#ifndef SKIN_SYNTHESIS_H
#define SKIN_SYNTHESIS_H

#include "ColorEstimate.h"

using namespace cv;

Mat synthesizeSkin(Mat imgRGB, Face face, Mat hairMask);
int findBrigthestColumnB(Mat B_L);
//Mat findIndForhead(Mat facePixels, Mat faceMask, Mat hairMask, int* firstForeheadRow, int* lastForeheadRow, int* firstForeheadCol, int* lastForeheadCol);
Vec3b findClosestSkinPixel(int i, int j, int maxNeighboordSize, Mat facePixels, ColorEstimate cEst);
bool findHairOnNeighbors(int i, int j, int neighboorHoodSize, Mat hairMask);
Mat findIndForhead(Mat facePixels, Mat faceMask, Mat hairMask, int* firstForeheadRow, int* lastForeheadRow, int* firstForeheadCol, int* lastForeheadCol, ColorEstimate cEst, int posTopEye);
void updateForeheadPixels(int i, uchar firstL, uchar lastL, int firstJ, int lastJ, int numberOfPoints, Scalar avgSkinColor, Mat facePixels_Lab, Mat foreHeadMask);
void updateForeheadPixels2(int i, uchar firstL, uchar lastL, int firstUpdateJ, int lastUpdateJ, int numberOfPoints, int firstForeheadCol, int lastForeheadCol, Scalar avgSkinColor, Mat facePixels_Lab, Mat foreHeadMask);
void updateForeheadPixelsWithFixedValue(int i, uchar firstL, uchar lastL, int firstJ, int lastJ, Scalar avgSkinColor, Mat facePixels_Lab, uchar L, uchar a, uchar b);
Mat sinthesizeTexture(Mat textureReference, int blockSize, int nRowsForehead, int nColsForehead);


static const int FIRST_COL = 0;
static const int LAST_COL = 1;

static const int BACKGROUND_SKIN_B = 255;
static const int BACKGROUND_SKIN_G = 255;
static const int BACKGROUND_SKIN_R = 255;

static const int FORHEAD_TRANSITION_HEIGHT = 30;

#endif
