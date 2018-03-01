#ifndef FACE_RECOGNITION_H
#define FACE_RECOGNITION_H

using namespace cv;

class Face
{
	Mat faceMask;
	Mat skinMask;
	Mat facePixels;

	int upperPointX; //Jx
	int upperPointY; //Jy

	int leftEdge;	
	int rightEdge;

	int leftEdgeEye;
	int rightEdgeEye;
	int bottomEye;
	int topEye;

	int hairTypicalBottom;	

	Rect regionA;
	Rect regionB;
	Rect regionC;


public:

	Face(Mat p_faceMask, Mat p_skinMask, int p_leftEdge, int p_rightEdge, int p_upperPointX, int p_upperPointY, int p_leftEdgeEye, int p_rightEdgeEye, int p_bottomEye, int p_topEye, int p_hairTypicalBottom,  Rect p_regionA, Rect p_regionB, Rect p_regionC)
	{
		faceMask = p_faceMask;
		skinMask = p_skinMask;

		leftEdge = p_leftEdge;
		rightEdge = p_rightEdge;

		upperPointX = p_upperPointX;
		upperPointY = p_upperPointY;
		leftEdgeEye = p_leftEdgeEye;
		rightEdgeEye = p_rightEdgeEye;
		bottomEye = p_bottomEye;
		topEye = p_topEye;
		hairTypicalBottom = p_hairTypicalBottom;

		regionA = p_regionA;
		regionB = p_regionB;
		regionC = p_regionC;

	}

	Mat getFaceMask()
	{
		return faceMask;
	}

	Mat getSkinMask()
	{
		return skinMask;
	}

	int getLeftEdge()
	{
		return leftEdge;
	}

	int getRightEdge()
	{
		return rightEdge;
	}

	int getUpperPointX()
	{
		return upperPointX;
	}

	int getUpperPointY()
	{
		return upperPointY;
	}

	int getLeftEdgeEye()
	{
		return leftEdgeEye;
	}

	int getRightEdgeEye()
	{
		return rightEdgeEye;
	}

	int getTopEye()
	{
		return topEye;
	}

	int getBottomEye()
	{
		return bottomEye;
	}

	int getHairTypicalBottom()
	{
		return hairTypicalBottom;
	}

	Rect getRegionA()
	{
		return regionA;
	}

	Rect getRegionB()
	{
		return regionB;
	}

	Rect getRegionC()
	{
		return regionC;
	}

	void setFacePixels(Mat p_facePixels)
	{
		facePixels = p_facePixels;
	}

	Mat getFacePixels()
	{
		return facePixels;
	}

	//	int middlePosY = landmarks[6 * 2 + 1];
};

static const int IND_LOWER_BOUND_MIDDLE = 6;
static const int BASE_OF_NOSE_IND = 56;
static const int LEFT_EDGE_IND = 0;
static const int RIGHT_EDGE_IND = 12;

static const int BASE_OF_LEFT_EYE_IND = 36;
static const int LEFT_EDGE_OF_LEFT_EYE_IND = 18;

static const int BASE_OF_RIGHT_EYE_IND = 46;
static const int RIGHT_EDGE_OF_RIGHT_EYE_IND = 25;
static const int LEFT_EDGE_OF_NOSE_IND = 58;
static const int RIGHT_EDGE_OF_NOSE_IND = 54;

static const int TOP_LEFT_EYEBROW_IND = 17;
static const int TOP_RIGHT_EYEBROW_IND = 17;

static const int TOP_OF_MOUTH_IND = 63;

static const int noseIndSequence[] = { 48,49,50,58,57,56,55,54 };

Face detectFace(Mat_<unsigned char> img, const char * path, const char* dataDir);

Mat detectUpperBoundaries(Mat_<unsigned char> img, float landmarks[], int *upperPoint_X, int *upperPoint_Y);

Mat  detectSkinMask(Mat img, float landmarks[]);

#endif //FACE_RECOGNITION_H