#ifndef FACE_RECOGNITION_H
#define FACE_RECOGNITION_H

#include <opencv2//core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include "Face.h"

using namespace cv;

static const int stasm_NLANDMARKS = 77; // number of landmarks

static const char *STASM_FILE = "stasmResults.csv";

static const int IND_LOWER_BOUND_MIDDLE = 6;
static const int BASE_OF_NOSE_IND = 56;
static const int LEFT_EDGE_IND = 0;
static const int RIGHT_EDGE_IND = 12;

static const int FIRST_EYE_IND = 16;
static const int LAST_EYE_IND = 47;

static const int BASE_OF_LEFT_EYE_IND = 36;
static const int LEFT_EDGE_OF_LEFT_EYE_IND = 18;

static const int BASE_OF_RIGHT_EYE_IND = 46;
static const int RIGHT_EDGE_OF_RIGHT_EYE_IND = 25;

static const int EYE_EDGE_OFFSET_X = 1;
static const int EYE_EDGE_OFFSET_Y = 1;

static const int TOP_EYE_OFFSET_Y = 3;
static const int BOTTOM_EYE_OFFSET_Y = 5;

static const int LEFT_EDGE_OF_NOSE_IND = 58;
static const int RIGHT_EDGE_OF_NOSE_IND = 54;

static const int TOP_LEFT_EYEBROW_IND = 17;
static const int TOP_RIGHT_EYEBROW_IND = 24;

static const int TOP_OF_MOUTH_IND = 63;

static const int noseIndSequence[] = { 48,49,50,58,57,56,55,54 };

static const int REGION_A_OFFSET_FROM_NOSE = 2;
static const int REGION_C_OFFSET_FROM_NOSE = 2;


Face detectFace(Mat_<unsigned char> img, const char * path, const char* dataDir);

Mat detectUpperBoundaries(Mat_<unsigned char> img, float landmarks[], int *upperPoint_X, int *upperPoint_Y);

Mat  detectSkinMask(Mat img, float landmarks[]);

#endif //FACE_RECOGNITION_H