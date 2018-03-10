#ifndef SKIN_SYNTHESIS_H
#define SKIN_SYNTHESIS_H

#include "ColorEstimate.h"
#include "HairExtraction.h"
#include "Face.h"

using namespace cv;

Mat synthesizeSkin(Mat imgRGB, Face face, Hair hair);
int findbrightestColumnB(Mat B_L);
//Mat findIndForhead(Mat facePixels, Mat faceMask, Mat hairMask, int* firstForeheadRow, int* lastForeheadRow, int* firstForeheadCol, int* lastForeheadCol);
Vec3b findClosestSkinPixel(int i, int j, int maxNeighboordSize, Mat facePixels_lab, Mat facePixels, ColorEstimate cEst);
bool findHairOnNeighbors(int i, int j, int neighboorHoodSize, Mat hairMask);
void updateReplaceMask(int i, int j, Mat replaceMask, Mat facePixels, Hair hair, ColorEstimate cEst, Face face);
Mat findIndForhead(Mat replaceMask, Mat facePixels, Mat faceMask, Hair hair, int* firstForeheadRow, int* lastForeheadRow, int* firstForeheadCol, int* lastForeheadCol, ColorEstimate cEst, Face face);
void updateForeheadPixels(int i, uchar firstL, uchar lastL, int firstUpdateJ, int lastUpdateJ, int numberOfPoints, int firstForeheadCol, int lastForeheadCol, Scalar avgSkinColor, Mat facePixels_Lab, Mat foreHeadMask);
void updateForeheadPixelsWithFixedValue(int i, uchar firstL, uchar lastL, int firstJ, int lastJ, Scalar avgSkinColor, Mat facePixels_Lab, uchar L, uchar a, uchar b);
Mat synthesizeTexture(Mat textureReference, int blockSize, int nRowsForehead, int nColsForehead, double blockStep);
Mat synthesizeTextureRandom(Mat textureReference, int blockSize, int nRowsForehead, int nColsForehead);
void obtainReplaceMask(Mat replaceMask, Mat facePixels, Mat faceMask, Hair hair, ColorEstimate cEst, Face face, int firstForeheadRow, int lastForeheadRow, int firstForeheadCol, int lastForeheadCol);

static const int FIRST_COL = 0;
static const int LAST_COL = 1;

static const int BACKGROUND_SKIN_B = 255;
static const int BACKGROUND_SKIN_G = 255;
static const int BACKGROUND_SKIN_R = 255;

static const int TEXTURE_BLOCK_SIZE = 5;

static const int N_ROWS_ABOVE_REFERENCE_POINT = 10;

static const int BLUR_SIZE = 5;

static const double WEIGHT_COLOR_ESTIMATE = 1.5;

#endif
