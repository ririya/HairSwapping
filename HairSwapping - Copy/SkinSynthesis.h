#ifndef SKIN_SYNTHESIS_H
#define SKIN_SYNTHESIS_H

using namespace cv;

Mat synthesizeSkin(Mat imgRGB, Face face, Mat hairMask);
int findBrigthestColumnB(Mat B_L);
Mat findIndForhead(Mat facePixels, Mat faceMask, Mat hairMask, int* firstForeheadRow, int* lastForeheadRow);
void updateForeheadPixels(int i, uchar firstL, uchar lastL, int firstJ, int lastJ, int numberOfPoints, Scalar avgSkinColor, Mat facePixels_Lab);
void updateForeheadPixelsWithFixedValue(int i, uchar firstL, uchar lastL, int firstJ, int lastJ, Scalar avgSkinColor, Mat facePixels_Lab, uchar fixedValue);

static const int firstCol = 0;
static const int lastCol = 1;

static const int BACKGROUND_SKIN_B = 255;
static const int BACKGROUND_SKIN_G = 255;
static const int BACKGROUND_SKIN_R = 255;

#endif
