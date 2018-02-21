#include <stdio.h>
#include <stdlib.h>
#include <list>
#include <time.h>
#include <unordered_set>

// OpenCV
#include <opencv2//core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/calib3d.hpp>
#include <opencv2/video/tracking.hpp>
#include "opencv/highgui.h"
#include "stasm_lib.h"
#include "FaceRecognition.h"
#include "HairExtraction.h"
#include "SkinSynthesis.h"
#include "HairEditing.h"

using namespace cv;

Mat swapHair(Hair hair, Face face, Mat modelImg, Mat targetImg, Mat synthesizedFace)
{	
	//initial Position estimation, connecting by position J in the two images
	int connXModel = hair.getHairConnectionPointLocationX();
	int connYModel = hair.getHairConnectionPointLocationY();

	int upperPointXTarget = face.getUpperPointX();
	int upperPointYTarget = face.getUpperPointY();

	int distModelTargetX = upperPointXTarget - connXModel;  //distance between hair connectionPoint in the model and top landmark in the target
	int distModelTargetY = upperPointYTarget - connYModel;

	distModelTargetX = distModelTargetX + hair.getHairConnectionPointDistanceToJ_X();  //add distance from connection point to top landmark in the model
	distModelTargetY = distModelTargetY + hair.getHairConnectionPointDistanceToJ_Y();

	int refPointX = connXModel + distModelTargetX;
	int refPointY = connYModel + distModelTargetY;

	Mat hairMask = hair.getHairMask();

	Mat hairPixels(modelImg.rows, modelImg.cols, CV_8UC3, Scalar(BACKGROUND_HAIR_B, BACKGROUND_HAIR_G, BACKGROUND_HAIR_R));
	modelImg.copyTo(hairPixels, hairMask);
	
	Mat hairMaskShifted;
	Mat hairPixelsShifted;

	Mat translationMatrix = (Mat_<double>(2, 3) << 1, 0, distModelTargetX, 0, 1, distModelTargetY);

	//alpha value will hold the mask, and will prevent errors when the hair color == backgroundColor, or when scaling smoothes the background
	Mat hairAlpha = createAlphaImage(hairPixels, hairMask);

	warpAffine(hairAlpha, hairPixelsShifted, translationMatrix, hairMask.size(), 1, 0, Scalar(BACKGROUND_HAIR_B, BACKGROUND_HAIR_G, BACKGROUND_HAIR_R,0));
	
	vector<vector<Point> > contours;

	Mat contourImg = face.getFaceMask().clone()*255;

	findContours(contourImg, contours, CV_RETR_LIST, CV_CHAIN_APPROX_SIMPLE);	

	Mat hairSwap = findBestScaleAndPosition(synthesizedFace, hairPixelsShifted, hairMask, face, contours[0], refPointX, refPointY);

	return hairSwap;

}

Mat trySwapHair(Mat synthesizedFace, Mat scaledHair, Mat scaledHairMask, Face face, Mat skinPixels, vector<Point> contours, int* energyHoles, int* energyHairOverlap)
{
	Mat hairSwap = synthesizedFace.clone();	

	scaledHair.copyTo(hairSwap, scaledHairMask);
		
	*energyHoles = calculateEnergyHoles(hairSwap, scaledHairMask, contours, face, skinPixels);
	*energyHairOverlap = calculateEnergyHairOverlap(hairSwap, face, skinPixels);

	return hairSwap;
}

Mat createAlphaImage(Mat mat, Mat alpha)
{
	std::vector<cv::Mat> matChannels;
	cv::split(mat, matChannels);

	matChannels.push_back(alpha);

	Mat alphaImage;

	cv::merge(matChannels, alphaImage);

	return alphaImage;
}

Mat findBestScaleAndPosition(Mat synthesizedFace, Mat hairPixels, Mat hairMask, Face face, vector<Point> contours, int refPointX, int refPointY)
{
	int hairBottom = face.getHairTypicalBottom();

	Mat skinPixels;
	synthesizedFace.copyTo(skinPixels, face.getSkinMask());

	Mat BestMatch;
	int minEnergy = synthesizedFace.cols*synthesizedFace.rows;
	int bestEnergyHoles = minEnergy; 
	int bestEnergyHairOverlap = minEnergy;
	Mat bestScaledHair;

	
	double bestParams[4]; //best tx, ty, sX, sY

	time_t start, end;

	time(&start);

	printf("Calculating best hair position...\n");
	
	for (int tx = -MAX_T; tx <= MAX_T; tx+= STEP_T)
	{
		for (int ty = -MAX_T; ty <= MAX_T; ty+= STEP_T)
		{
			Mat currHairPixels, currHairMask;
			Mat translationMatrix = (Mat_<double>(2, 3) << 1, 0, tx, 0, 1, ty);
			warpAffine(hairPixels, currHairPixels, translationMatrix, hairPixels.size(), 1, 0, Scalar(BACKGROUND_HAIR_B, BACKGROUND_HAIR_G, BACKGROUND_HAIR_R,0));
		
			for (double sX = MIN_SCALE; sX <= MAX_SCALE; sX += STEP_S)
			{
				for (double sY = MIN_SCALE; sY <= MAX_SCALE; sY += STEP_S)
				{

					Mat scaledHairX = scaleHair(currHairPixels, refPointX, refPointY, sX, 1, Scalar(BACKGROUND_HAIR_B, BACKGROUND_HAIR_G, BACKGROUND_HAIR_R,0));
					Mat scaledHair = scaleHair(scaledHairX, refPointX, refPointY, 1, sY, Scalar(BACKGROUND_HAIR_B, BACKGROUND_HAIR_G, BACKGROUND_HAIR_R,0));

					std::vector<cv::Mat> matChannels;
					cv::split(scaledHair, matChannels);
					Mat scaledHairMask= matChannels[3]; 
					matChannels.erase(matChannels.begin() + 3);
					Mat scaledHair3D;
					merge(matChannels, scaledHair3D);

					int energyHoles;
					int energyHairOverlap;

					Mat currHairSwap = trySwapHair(synthesizedFace, scaledHair3D, scaledHairMask, face, skinPixels, contours, &energyHoles, &energyHairOverlap);

					int currEnergy = (int)(ENERGY_WEIGHT*energyHoles) + energyHairOverlap;

					if (currEnergy < minEnergy)
					{
						bestScaledHair = scaledHair;
						bestEnergyHoles = energyHoles;
						bestEnergyHairOverlap = energyHairOverlap;
						minEnergy = currEnergy;
						BestMatch = currHairSwap;
						bestParams[0] = tx;
						bestParams[1] = ty;
						bestParams[2] = sX;
						bestParams[3] = sY;
					}

				/*	cv::namedWindow("currHairSwap", CV_WINDOW_AUTOSIZE);
					cv::imshow("currHairSwap", currHairSwap);
					cv::waitKey();*/
				}
			}
		}
	}

	time(&end);
	double dif = difftime(end, start);
	printf("Finished calculting best hair position in %.2lf seconds.\n", dif);
	printf("Type a key to continue...", dif);

	//cv::namedWindow("bestScaledHair", CV_WINDOW_AUTOSIZE);
	//cv::imshow("bestScaledHair", bestScaledHair);
	//cv::waitKey();

	cv::namedWindow("BestMatch", CV_WINDOW_AUTOSIZE);
	cv::imshow("BestMatch", BestMatch);
	cv::waitKey();

	return BestMatch;
}

int calculateEnergyHairOverlap(Mat hairSwap, Face face, Mat skinPixels)
{
	Mat hairOnSkin;
	Mat skinMask = face.getSkinMask();
	
	int leftEdge = face.getLeftEdge();
	int rightEdge = face.getRightEdge();

	Rect leftRect(0, 0, leftEdge + NUMBER_OF_FACE_COLUMNS_ALLOWED_HAIR, skinMask.rows);
	Rect rightRect(rightEdge - NUMBER_OF_FACE_COLUMNS_ALLOWED_HAIR, 0, skinMask.cols - (rightEdge - NUMBER_OF_FACE_COLUMNS_ALLOWED_HAIR), skinMask.rows);

	skinMask(leftRect) = Scalar(0, 0, 0);
	skinMask(rightRect) = Scalar(0, 0, 0);

	/*cv::namedWindow("skinMask", CV_WINDOW_AUTOSIZE);
	cv::imshow("skinMask", skinMask);
	cv::waitKey();*/
	

	hairSwap.copyTo(hairOnSkin, skinMask);
	Mat difference = hairOnSkin != skinPixels;
	cvtColor(difference, difference, CV_RGB2GRAY);

	int Energy = countNonZero(difference);

	return Energy;
}

int calculateEnergyHoles(Mat hairSwap, Mat hairMask, vector<Point> contours, Face face, Mat skinPixels)
{

	std::unordered_set<int> holes_hashSet;

	int Energy = 0;

	Mat faceMask = face.getFaceMask();
	faceMask = faceMask / 255 * 128;
	
	for (int c = 0; c < contours.size(); c++)
	{
		int x = contours[c].x;
		int y = contours[c].y;

		if (y > face.getHairTypicalBottom())
		{
			continue;
		}	

		searchHoles(hairSwap, faceMask, hairMask, x, y, &holes_hashSet);
	}

	Energy = holes_hashSet.size();

	return Energy;
}

void searchHoles(Mat hairSwap, Mat faceMask, Mat hairMask, int x, int y, std::unordered_set<int> *holes_hashSet)
{
	// look for background pixels enclosed by hair and skin
	//the hashset avoids double-counting background pixels

	int offset = 1;
	int leftPixelPosition = max(x - offset, 0);
	int rightPixelPosition = min(x + offset, faceMask.cols - 1);
	int topPixelPosition = max(y - offset, 0);

	uchar faceMask_rightPixel = faceMask.at<uchar>(y, rightPixelPosition);

	uchar faceMask_topPixel = faceMask.at<uchar>(topPixelPosition, x);

	uchar faceMask_leftPixel = faceMask.at<uchar>(y, leftPixelPosition);

	/*Mat currFaceMask = faceMask.clone();
	currFaceMask.at<uchar>(y, x) = 255;*/

	/*cv::namedWindow("hairSwap", CV_WINDOW_AUTOSIZE);
	cv::imshow("hairSwap", hairSwap);

	cv::namedWindow("faceMask", CV_WINDOW_AUTOSIZE);
	cv::imshow("faceMask", faceMask);
	cv::waitKey();*/
	
	if (faceMask_leftPixel == 0)   //this is a left edge,  search to the left
	{
		int x_interest = x - 1;
		int y_interest = y;		
		int contourType = CONTOUR_TYPE_LEFTPIXEL;

		int lastHairPixelInd = searchForEnclosingHair(hairMask, x_interest, y_interest, contourType);
		searchLineForHoles(hairSwap, x_interest, y_interest, lastHairPixelInd, holes_hashSet, contourType);
	}

	if (faceMask_topPixel == 0)   //this is a top edge,  search up
	{
		int x_interest = x;
		int y_interest = y-1;	
		int contourType = CONTOUR_TYPE_TOPPIXEL;

		int lastHairPixelInd = searchForEnclosingHair(hairMask, x_interest, y_interest, contourType);
		searchLineForHoles(hairSwap, x_interest, y_interest, lastHairPixelInd, holes_hashSet, contourType);
	}

	if (faceMask_rightPixel == 0)   //this is a right edge,  search to the right
	{
		int x_interest = x + 1;
		int y_interest = y;	
		int contourType = CONTOUR_TYPE_RIGHTPIXEL;

		int lastHairPixelInd = searchForEnclosingHair(hairMask, x_interest, y_interest, contourType);	
		searchLineForHoles(hairSwap, x_interest, y_interest, lastHairPixelInd, holes_hashSet, contourType);
	}
}

void searchLineForHoles(Mat hairSwap, int x_interest, int y_interest, int lastHairPixelInd, std::unordered_set<int> *holes_hashSet, int contourType)
{
	if (lastHairPixelInd == -1) // no hair found
	{
		return;
	}

	if (contourType == CONTOUR_TYPE_LEFTPIXEL)
	{
		for (int j = lastHairPixelInd+1; j <= x_interest; j++)
		{
			if (hairSwap.at<Vec3b>(y_interest, j)[0] == BACKGROUND_SKIN_B && hairSwap.at<Vec3b>(y_interest, j)[1] == BACKGROUND_SKIN_G && hairSwap.at<Vec3b>(y_interest, j)[2] == BACKGROUND_SKIN_R)
			{
				/*Mat currHairSwap = hairSwap.clone();
				currHairSwap.at<Vec3b>(y_interest, x_interest)[0] = 0;
				currHairSwap.at<Vec3b>(y_interest, x_interest)[1] = 0;
				currHairSwap.at<Vec3b>(y_interest, x_interest)[2] = 255;

				currHairSwap.at<Vec3b>(y_interest, lastHairPixelInd)[0] = 0;
				currHairSwap.at<Vec3b>(y_interest, lastHairPixelInd)[1] = 255;
				currHairSwap.at<Vec3b>(y_interest, lastHairPixelInd)[2] = 0;
			
				cv::namedWindow("currHairSwap", CV_WINDOW_AUTOSIZE);
				cv::imshow("currHairSwap", currHairSwap);
				cv::waitKey();*/

				insertIntoHashSet(y_interest, j, hairSwap.cols, holes_hashSet);
			
			}
		}
	}

	if (contourType == CONTOUR_TYPE_RIGHTPIXEL)
	{
		for (int j = x_interest; j < lastHairPixelInd; j++)
		{
			if (hairSwap.at<Vec3b>(y_interest, j)[0] == BACKGROUND_SKIN_B && hairSwap.at<Vec3b>(y_interest, j)[1] == BACKGROUND_SKIN_G && hairSwap.at<Vec3b>(y_interest, j)[2] == BACKGROUND_SKIN_R)
			{				

				/*Mat currHairSwap = hairSwap.clone();
				currHairSwap.at<Vec3b>(y_interest, x_interest)[0] = 0;
				currHairSwap.at<Vec3b>(y_interest, x_interest)[1] = 0;
				currHairSwap.at<Vec3b>(y_interest, x_interest)[2] = 255;

				currHairSwap.at<Vec3b>(y_interest, lastHairPixelInd)[0] = 0;
				currHairSwap.at<Vec3b>(y_interest, lastHairPixelInd)[1] = 255;
				currHairSwap.at<Vec3b>(y_interest, lastHairPixelInd)[2] = 0;

				cv::namedWindow("currHairSwap", CV_WINDOW_AUTOSIZE);
				cv::imshow("currHairSwap", currHairSwap);
				cv::waitKey();*/

				insertIntoHashSet(y_interest, j, hairSwap.cols, holes_hashSet);
			}
		}
	}

	if (contourType == CONTOUR_TYPE_TOPPIXEL)
	{
		for (int i = lastHairPixelInd+1; i <= y_interest; i++)
		{

			if (y_interest < 160) {

				if (hairSwap.at<Vec3b>(i, x_interest)[0] == BACKGROUND_SKIN_B && hairSwap.at<Vec3b>(i, x_interest)[1] == BACKGROUND_SKIN_G && hairSwap.at<Vec3b>(i, x_interest)[2] == BACKGROUND_SKIN_R)
				{
					/*Mat currHairSwap = hairSwap.clone();
					currHairSwap.at<Vec3b>(y_interest, x_interest)[0] = 0;
					currHairSwap.at<Vec3b>(y_interest, x_interest)[1] = 0;
					currHairSwap.at<Vec3b>(y_interest, x_interest)[2] = 255;

					currHairSwap.at<Vec3b>(lastHairPixelInd, x_interest)[0] = 0;
					currHairSwap.at<Vec3b>(lastHairPixelInd, x_interest)[1] = 255;
					currHairSwap.at<Vec3b>(lastHairPixelInd, x_interest)[2] = 0;

					cv::namedWindow("currHairSwap", CV_WINDOW_AUTOSIZE);
					cv::imshow("currHairSwap", currHairSwap);
					cv::waitKey();*/

					insertIntoHashSet(i, x_interest, hairSwap.cols, holes_hashSet);

				}
			}
		}
	}

	//int a = 1;

}

void insertIntoHashSet(int y, int x, int nCols, std::unordered_set<int> *holes_hashSet)
{
	int pixelInd = y*nCols + x;

	std::unordered_set<int>::const_iterator got = holes_hashSet->find(pixelInd);

	if (got == holes_hashSet->end())
	{
		//not in hashSet, add it
		holes_hashSet->insert(pixelInd);
	}
}

int searchForEnclosingHair(Mat hairMask, int x_interest, int y_interest, int contourType)
{
	if (contourType == CONTOUR_TYPE_LEFTPIXEL)
	{
		for (int j = x_interest; j >= 0; j--)
		{
			if (hairMask.at<uchar>(y_interest, j) > 0)
			{
				return j;
			}
		}
	}

	if (contourType == CONTOUR_TYPE_RIGHTPIXEL)
	{
		for (int j = x_interest; j < hairMask.cols; j++)
		{
			if (hairMask.at<uchar>(y_interest, j) > 0)
			{
				return j;
			}
		}
	}

	if (contourType == CONTOUR_TYPE_TOPPIXEL)
	{
		for (int i = y_interest; i >= 0; i--)
		{
			if (hairMask.at<uchar>(i, x_interest) > 0)
			{
				return i;
			}
		}
	}

	return  -1;
}

Mat scaleHair(Mat imgRGB, int refPointX, int refPointY, double scaleX, double scaleY, Scalar backgroundColor)
{
	int origWidth = imgRGB.cols;
	int origHeight = imgRGB.rows;	

	Mat translated;
	Mat scaled;

	int tx = origWidth / 2 - refPointX;
	int ty = origHeight / 2 - refPointY;

	double newWidth = imgRGB.cols*scaleX;
	double newHeight = imgRGB.rows*scaleY;

	Mat translationMatrix = (Mat_<double>(2, 3) << 1, 0, tx, 0, 1, ty);

	warpAffine(imgRGB, translated, translationMatrix, Size(origWidth, origHeight),1, 0, backgroundColor);

	Mat scalingMatrix = (Mat_<double>(2, 3) << scaleX, 0, 0, 0, scaleY, 0);

	warpAffine(translated, scaled, scalingMatrix, Size(newWidth, newHeight), 1, 0, backgroundColor);

	Mat rescaledCentered;
	if (newHeight > origHeight || newWidth > origWidth)
	{
		Rect rectCrop(cvRound(newWidth / 2) - cvRound(origWidth / 2), cvRound(newHeight / 2) - cvRound(origHeight / 2), origWidth, origHeight);
		rescaledCentered = Mat(scaled, rectCrop);
	}
	else
	{
		rescaledCentered = Mat(imgRGB.size(), imgRGB.type(), backgroundColor);

		Rect rectCrop(cvRound(origWidth / 2) - cvRound(newWidth / 2), cvRound(origHeight / 2) - cvRound(newHeight / 2), newWidth, newHeight);

		scaled.copyTo(rescaledCentered(rectCrop));
	}

	Mat translationMatrix2 = (Mat_<double>(2, 3) << 1, 0, -tx, 0, 1, -ty);

	Mat rescaledCorrected;

	warpAffine(rescaledCentered, rescaledCorrected, translationMatrix2, Size(origWidth, origHeight), 1, 0, backgroundColor);

	return rescaledCorrected;
}