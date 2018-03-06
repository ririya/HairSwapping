#ifndef FACE_H
#define FACE_H

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

	Face(Mat p_faceMask, Mat p_skinMask, int p_leftEdge, int p_rightEdge, int p_upperPointX, int p_upperPointY, int p_leftEdgeEye, int p_rightEdgeEye, int p_bottomEye, int p_topEye, int p_hairTypicalBottom, Rect p_regionA, Rect p_regionB, Rect p_regionC);

	Mat getFaceMask();

	Mat getSkinMask();

	int getLeftEdge();

	int getRightEdge();

	int getUpperPointX();

	int getUpperPointY();

	int getLeftEdgeEye();

	int getRightEdgeEye();

	int getTopEye();

	int getBottomEye();

	int getHairTypicalBottom();

	Rect getRegionA();

	Rect getRegionB();

	Rect getRegionC();

	void setFacePixels(Mat p_facePixels);

	Mat getFacePixels();
	
};

#endif
