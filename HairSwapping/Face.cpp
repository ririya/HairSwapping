#include <opencv2//core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include "Face.h"

Face::Face(Mat p_faceMask, Mat p_skinMask, int p_leftEdge, int p_rightEdge, int p_upperPointX, int p_upperPointY, int p_leftEdgeEye, int p_rightEdgeEye, int p_bottomEye, int p_topEye, int p_hairTypicalBottom, Rect p_regionA, Rect p_regionB, Rect p_regionC)
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

	Mat Face::getFaceMask()
	{
		return faceMask;
	}

	Mat Face::getSkinMask()
	{
		return skinMask;
	}

	int Face::getLeftEdge()
	{
		return leftEdge;
	}

	int Face::getRightEdge()
	{
		return rightEdge;
	}

	int Face::getUpperPointX()
	{
		return upperPointX;
	}

	int Face::getUpperPointY()
	{
		return upperPointY;
	}

	int Face::getLeftEdgeEye()
	{
		return leftEdgeEye;
	}

	int Face::getRightEdgeEye()
	{
		return rightEdgeEye;
	}

	int Face::getTopEye()
	{
		return topEye;
	}

	int Face::getBottomEye()
	{
		return bottomEye;
	}

	int Face::getHairTypicalBottom()
	{
		return hairTypicalBottom;
	}

	Rect Face::getRegionA()
	{
		return regionA;
	}

	Rect Face::getRegionB()
	{
		return regionB;
	}

	Rect Face::getRegionC()
	{
		return regionC;
	}

	void Face::setFacePixels(Mat p_facePixels)
	{
		facePixels = p_facePixels;
	}

	Mat Face::getFacePixels()
	{
		return facePixels;
	}

	