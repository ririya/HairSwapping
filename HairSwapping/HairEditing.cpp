#include <stdio.h>
#include <stdlib.h>
#include <list>
#include <time.h>
#include <unordered_set>

// OpenCV
#include <opencv2//core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include "Face.h"
#include "Hair.h"
#include "HairEditing.h"

using namespace cv;

Mat swapHair(Hair hair, Face face, int modelHeadSize, Mat synthesizedFace)
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

	//Mat hairPixels(modelImg.rows, modelImg.cols, CV_8UC3, Scalar(BACKGROUND_HAIR_B, BACKGROUND_HAIR_G, BACKGROUND_HAIR_R));
	//modelImg.copyTo(hairPixels, hairMask);
	
	Mat hairMaskShifted;
	Mat hairPixelsShifted;

	

	//alpha value will hold the mask, and will prevent errors when the hair color == backgroundColor, or when scaling smoothes the background
	//mat hairAlpha = createAlphaImage(hairPixels, hairMask);

	Mat hairAlpha = hair.getHairPixels();

	//cv::namedWindow("hairAlpha", CV_WINDOW_AUTOSIZE);
	//cv::imshow("hairAlpha", hairAlpha);
	//cv::waitKey();

	Mat translationMatrix = (Mat_<double>(2, 3) << 1, 0, distModelTargetX, 0, 1, distModelTargetY);
	warpAffine(hairAlpha, hairPixelsShifted, translationMatrix, hairMask.size(), 1, 0, Scalar(BACKGROUND_HAIR_B, BACKGROUND_HAIR_G, BACKGROUND_HAIR_R,0));
	
	std::vector<std::vector<Point> > contours;

	Mat contourImg = face.getFaceMask().clone()*255;

	findContours(contourImg, contours, CV_RETR_LIST, CV_CHAIN_APPROX_SIMPLE);	

	Mat hairSwap = findBestScaleAndPosition(synthesizedFace, hairAlpha, face, modelHeadSize, contours[0], refPointX, refPointY, distModelTargetX, distModelTargetY);

	return hairSwap;

}

Mat convertTo3channels(Mat mat)
{
	Mat out;
	Mat in[] = { mat, mat, mat };
	cv::merge(in, 3, out);

	return out;

}


Mat trySwapHair(Mat synthesizedFace, Mat scaledHair, Mat scaledHairMask, Face face, Mat skinPixels, std::vector<Point> contours, int* energyHoles, int* energyHairOverlap)
{

	clock_t  tStartSwap, tEndSwap, tStartHoles, tEndHoles, tStartOverlap, tEndOverlap;

	Mat hairSwap;
	Mat weightedScaledHair;
	Mat weightedSynthesizedFace;
	Mat alphaMask;
	Mat background_mask;
	
	//int type0 = scaledHairMask.type();

	//tStartSwap = clock();

	scaledHairMask.convertTo(alphaMask, CV_32FC1, 1.0 / 255); // alpha mask
	alphaMask = convertTo3channels(alphaMask);

	scaledHair.convertTo(scaledHair, CV_32FC3);

	/*cv::namedWindow("scaledHair", CV_WINDOW_AUTOSIZE);
	cv::imshow("scaledHair", scaledHair/255);
	cv::waitKey();*/
	
	multiply(scaledHair , alphaMask, weightedScaledHair);

	weightedScaledHair.convertTo(weightedScaledHair, CV_8UC3);

	/*cv::namedWindow("weightedScaledHair", CV_WINDOW_AUTOSIZE);
	cv::imshow("weightedScaledHair", weightedScaledHair);
	cv::waitKey();*/

	background_mask = Scalar::all(1.0) - alphaMask;

	multiply(background_mask, synthesizedFace, weightedSynthesizedFace);

	weightedSynthesizedFace.convertTo(weightedSynthesizedFace, CV_8UC3);

	/*cv::namedWindow("weightedSynthesizedFace", CV_WINDOW_AUTOSIZE);
	cv::imshow("weightedSynthesizedFace", weightedSynthesizedFace);
	cv::waitKey();*/
	 
	add(weightedSynthesizedFace, weightedScaledHair, hairSwap);

	hairSwap.convertTo(hairSwap, CV_8UC3);

	//tEndSwap = clock();
	//
	//double dif = double (tEndSwap - tStartSwap) / CLOCKS_PER_SEC;
	//printf("Time swap = %.5lf seconds. ", dif);

	//cv::namedWindow("hairSwap", CV_WINDOW_AUTOSIZE);
	//cv::imshow("hairSwap", hairSwap);
	//cv::waitKey();

	//scaledHair.copyTo(hairSwap, scaledHairMask);
	
	//tStartHoles = clock();

	*energyHoles = calculateEnergyHoles(hairSwap, scaledHairMask, contours, face, skinPixels);

	/*tEndHoles = clock();

	dif = double(tEndHoles - tStartHoles) / CLOCKS_PER_SEC;
	printf("Time holes = %.5lf seconds. ", dif);

	tStartOverlap = clock();*/

	*energyHairOverlap = calculateEnergyHairOverlap(scaledHairMask, face, skinPixels);

	//tEndOverlap = clock();

	//dif = double(tEndOverlap - tStartOverlap) / CLOCKS_PER_SEC;
	//printf("Time overlap = %.5lf seconds. \n", dif);


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

Mat findBestScaleAndPosition(Mat synthesizedFace, Mat hairPixels, Face face, int modelHeadSize, std::vector<Point> contours, int refPointX, int refPointY, int refTx, int refTy)
{

	Mat floatSynthesizedFace;
	synthesizedFace.convertTo(floatSynthesizedFace, CV_32FC3); // needs to be float for alpha blending

	int hairBottom = face.getHairTypicalBottom();

	Mat skinPixels;
	synthesizedFace.copyTo(skinPixels, face.getSkinMask());

	Mat BestMatch;
	int minEnergy = synthesizedFace.cols*synthesizedFace.rows;
	int bestEnergyHoles = minEnergy;
	int bestEnergyHairOverlap = minEnergy;
	Mat bestScaledHair;

	double headSizeRatio = (double) face.getHeadSize() / modelHeadSize;

	double minScale = headSizeRatio - 0.1;
	double maxScale = headSizeRatio + 0.1;

	double bestParams[4]; //best tx, ty, sX, sY

	time_t start, end, tStartScale, tEndScale;

	time(&start);

	printf("Calculating best hair position...\n");

	for (double sX = MIN_SCALE; sX <= MAX_SCALE; sX += STEP_S)
	{
		Mat scaledHairX = scaleHair(hairPixels, refPointX, refPointY, refTx, refTy, sX, 1, Scalar(BACKGROUND_HAIR_B, BACKGROUND_HAIR_G, BACKGROUND_HAIR_R, 0));

		for (double sY = MIN_SCALE; sY <= MAX_SCALE; sY += STEP_S)
		{
			//Mat scaledHair = scaleHair(hairPixels, refPointX, refPointY, 0.5, 0.5, Scalar(BACKGROUND_HAIR_B, BACKGROUND_HAIR_G, BACKGROUND_HAIR_R, 0));
			//scaledHair = scaleHairOld(hairPixels, refPointX, refPointY, 2, 2, Scalar(BACKGROUND_HAIR_B, BACKGROUND_HAIR_G, BACKGROUND_HAIR_R, 0));

			//tStartScale = clock();

			Mat scaledHair = scaleHair(scaledHairX, refPointX, refPointY, refTx, refTy, 1, sY, Scalar(BACKGROUND_HAIR_B, BACKGROUND_HAIR_G, BACKGROUND_HAIR_R, 0));

			/*tEndScale = clock();

			double dif = double(tEndScale - tStartScale) / CLOCKS_PER_SEC;
			printf("Time scale = %.5lf seconds. ", dif);*/

			for (int tx = -MAX_TX; tx <= MAX_TX; tx += STEP_T)
			{
				for (int ty = -MAX_TY; ty <= MAX_TY; ty += STEP_T)
				{
					Mat translatedHair, currHairMask;
					Mat translationMatrix = (Mat_<double>(2, 3) << 1, 0, tx+refTx, 0, 1, ty + refTy);
					warpAffine(scaledHair, translatedHair, translationMatrix, hairPixels.size(), 1, 0, Scalar(BACKGROUND_HAIR_B, BACKGROUND_HAIR_G, BACKGROUND_HAIR_R, 0));
			
					std::vector<cv::Mat> matChannels;
					cv::split(translatedHair, matChannels);
					Mat hairAlphaMask = matChannels[3];
					matChannels.erase(matChannels.begin() + 3);
					Mat scaledTranslatedHair3D;
					merge(matChannels, scaledTranslatedHair3D);

					int energyHoles;
					int energyHairOverlap;

					Mat currHairSwap = trySwapHair(floatSynthesizedFace, scaledTranslatedHair3D, hairAlphaMask, face, skinPixels, contours, &energyHoles, &energyHairOverlap);

					int currEnergy = (int)(ENERGY_WEIGHT*energyHoles) + energyHairOverlap;

					if (currEnergy <= minEnergy) // if the same, preference for larger scale
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

	cv::namedWindow("BestMatch", CV_WINDOW_AUTOSIZE);
	cv::imshow("BestMatch", BestMatch);
	cv::waitKey();


	return BestMatch;
}

int calculateEnergyHairOverlap(Mat scaledHairMask, Face face, Mat skinPixels)
{
	Mat hairOnSkin;
	Mat skinMask = face.getSkinMask();
	
	int leftEdge = face.getLeftEdge();
	int rightEdge = face.getRightEdge();

	//get face roi, ignoring some columns on the edge of face
	Rect leftRect(0, 0, leftEdge + NUMBER_OF_FACE_COLUMNS_ALLOWED_HAIR, skinMask.rows);
	Rect rightRect(rightEdge - NUMBER_OF_FACE_COLUMNS_ALLOWED_HAIR, 0, skinMask.cols - (rightEdge - NUMBER_OF_FACE_COLUMNS_ALLOWED_HAIR), skinMask.rows);

	skinMask(leftRect) = Scalar(0, 0, 0);
	skinMask(rightRect) = Scalar(0, 0, 0);

	/*cv::namedWindow("skinMask", CV_WINDOW_AUTOSIZE);
	cv::imshow("skinMask", skinMask);
	cv::waitKey();*/
	

	scaledHairMask.copyTo(hairOnSkin, skinMask);

	Mat hairOnSkinNonTransparent(hairOnSkin.rows, hairOnSkin.cols, CV_8UC1, Scalar(0));
	hairOnSkinNonTransparent.setTo(1, hairOnSkin > ALPHA_THRESHOLD);

	Scalar Energy = sum(hairOnSkinNonTransparent);
	
	return Energy[0];
}

int calculateEnergyHoles(Mat hairSwap, Mat hairMask, std::vector<Point> contours, Face face, Mat skinPixels)
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
		searchLineForHoles(hairMask, x_interest, y_interest, lastHairPixelInd, holes_hashSet, contourType);
	}

	if (faceMask_topPixel == 0)   //this is a top edge,  search up
	{
		int x_interest = x;
		int y_interest = y-1;	
		int contourType = CONTOUR_TYPE_TOPPIXEL;

		int lastHairPixelInd = searchForEnclosingHair(hairMask, x_interest, y_interest, contourType);
		searchLineForHoles(hairMask, x_interest, y_interest, lastHairPixelInd, holes_hashSet, contourType);
	}

	if (faceMask_rightPixel == 0)   //this is a right edge,  search to the right
	{
		int x_interest = x + 1;
		int y_interest = y;	
		int contourType = CONTOUR_TYPE_RIGHTPIXEL;

		int lastHairPixelInd = searchForEnclosingHair(hairMask, x_interest, y_interest, contourType);	
		searchLineForHoles(hairMask, x_interest, y_interest, lastHairPixelInd, holes_hashSet, contourType);
	}
}

void searchLineForHoles(Mat hairMask, int x_interest, int y_interest, int lastHairPixelInd, std::unordered_set<int> *holes_hashSet, int contourType)
{
	if (lastHairPixelInd == -1) // no hair found
	{
		return;
	}

	if (contourType == CONTOUR_TYPE_LEFTPIXEL)
	{
		for (int j = lastHairPixelInd+1; j <= x_interest; j++)
		{
			//if (hairSwap.at<Vec3b>(y_interest, j)[0] == BACKGROUND_SKIN_B && hairSwap.at<Vec3b>(y_interest, j)[1] == BACKGROUND_SKIN_G && hairSwap.at<Vec3b>(y_interest, j)[2] == BACKGROUND_SKIN_R)
			if (hairMask.at<uchar>(y_interest, j) < ALPHA_THRESHOLD)
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

				insertIntoHashSet(y_interest, j, hairMask.cols, holes_hashSet);
			
			}
		}
	}

	if (contourType == CONTOUR_TYPE_RIGHTPIXEL)
	{
		for (int j = x_interest; j < lastHairPixelInd; j++)
		{
			//			if (hairSwap.at<Vec3b>(y_interest, j)[0] == BACKGROUND_SKIN_B && hairSwap.at<Vec3b>(y_interest, j)[1] == BACKGROUND_SKIN_G && hairSwap.at<Vec3b>(y_interest, j)[2] == BACKGROUND_SKIN_R)
			if (hairMask.at<uchar>(y_interest, j) < ALPHA_THRESHOLD)
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

				insertIntoHashSet(y_interest, j, hairMask.cols, holes_hashSet);
			}
		}
	}

	if (contourType == CONTOUR_TYPE_TOPPIXEL)
	{
		for (int i = lastHairPixelInd+1; i <= y_interest; i++)
		{
			//if (hairSwap.at<Vec3b>(i, x_interest)[0] == BACKGROUND_SKIN_B && hairSwap.at<Vec3b>(i, x_interest)[1] == BACKGROUND_SKIN_G && hairSwap.at<Vec3b>(i, x_interest)[2] == BACKGROUND_SKIN_R)
			if (hairMask.at<uchar>(i, x_interest) < ALPHA_THRESHOLD)
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

				insertIntoHashSet(i, x_interest, hairMask.cols, holes_hashSet);

			}			
		}
	}

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
			//if (hairMask.at<uchar>(y_interest, j) > 0)
			if (hairMask.at<uchar>(y_interest, j) >= ALPHA_THRESHOLD)
			{
				return j;
			}
		}
	}

	if (contourType == CONTOUR_TYPE_RIGHTPIXEL)
	{
		for (int j = x_interest; j < hairMask.cols; j++)
		{
			if (hairMask.at<uchar>(y_interest, j) >= ALPHA_THRESHOLD)
			//if (hairMask.at<uchar>(y_interest, j) > 0)
			{
				return j;
			}
		}
	}

	if (contourType == CONTOUR_TYPE_TOPPIXEL)
	{
		for (int i = y_interest; i >= 0; i--)
		{
			if (hairMask.at<uchar>(i, x_interest) >= ALPHA_THRESHOLD)
			//if (hairMask.at<uchar>(i, x_interest) > 0)
			{
				return i;
			}
		}
	}

	return  -1;
}

void setMarker(Mat img, int refPointX, int refPointY, Scalar markerColor)
{
	img.at<Vec4b>(refPointY, refPointX)[0] = markerColor[0];
	img.at<Vec4b>(refPointY, refPointX)[1] = markerColor[1];
	img.at<Vec4b>(refPointY, refPointX)[2] = markerColor[2];
	img.at<Vec4b>(refPointY, refPointX)[3] = 255;
}

Scalar findMarkerPosition(Mat img, Scalar markerColor)
{
	for (int i = 0;i < img.rows;i++)
	{
		for (int j = 0; j < img.cols; j++)
		{
			if (img.at<Vec4b>(i, j)[0] == markerColor[0] && img.at<Vec4b>(i, j)[1] == markerColor[1] && img.at<Vec4b>(i, j)[2] == markerColor[2])
			{
				return Scalar(i, j);
			}
		}
	}

	return Scalar(-1, -1);
}

Mat scaleHair(Mat img, int refPointX, int refPointY, int refTx, int refTy, double scaleX, double scaleY, Scalar backgroundColor)
{
	int origWidth = img.cols;
	int origHeight = img.rows;

	Mat translated;
	Mat scaled;
	Mat corrected;
	
	int scaledRefPointX = scaleX*refPointX;
	int scaledRefPointY = scaleY*refPointY;

	int tx = refPointX - scaledRefPointX;
	int ty = refPointY - scaledRefPointY;

	double newWidth = img.cols*scaleX;
	double newHeight = img.rows*scaleY;

	Scalar markerColor = Scalar(0, 0, 255);

	//setMarker(img, refPointX, refPointY, markerColor);

	Scalar positionOriginal = findMarkerPosition(img, markerColor);
	
	Mat scalingMatrix = (Mat_<double>(2, 3) << scaleX, 0, 0, 0, scaleY, 0);
	Mat translationMatrix = (Mat_<double>(2, 3) << 1, 0, tx, 0, 1, ty);

	warpAffine(img, scaled, scalingMatrix, Size(newWidth, newHeight), 1, 0, backgroundColor);

	/*Scalar positionScaled = findMarkerPosition(scaled, markerColor);
	Scalar positionTranslated;
	Scalar positionCorrected;*/

	//if (newHeight > origHeight) 
	//{
	//	warpAffine(scaled, translated, translationMatrix, Size(newWidth, newHeight), 1, 0, backgroundColor);

	//	positionTranslated = findMarkerPosition(translated, markerColor);

	//	Rect rectCrop(0, 0, origWidth, origHeight);
	//	corrected = Mat(translated, rectCrop);

	//	positionCorrected = findMarkerPosition(corrected, markerColor);

	//}
	//else
	//{

	//	Mat temp = Mat(img.size(), img.type(), backgroundColor);

	//	Rect rectCrop(0, 0, newWidth, newHeight);

	//	scaled.copyTo(temp(rectCrop));

	//	/*	cv::namedWindow("temp", CV_WINDOW_AUTOSIZE);
	//	cv::imshow("temp", temp);*/

	//	warpAffine(temp, translated, translationMatrix, Size(origWidth, origHeight), 1, 0, backgroundColor);

	//	corrected = translated;

	//	positionCorrected = findMarkerPosition(corrected, markerColor);

	//}
	
	if (newHeight > origHeight || newWidth > origWidth)
	{
		warpAffine(scaled, translated, translationMatrix, Size(newWidth, newHeight), 1, 0, backgroundColor);

		//positionTranslated = findMarkerPosition(translated, markerColor);

		Rect rectCrop(0, 0, origWidth, origHeight);
		corrected = Mat(translated, rectCrop);

		//positionCorrected = findMarkerPosition(corrected, markerColor);
				
	}
	else
	{
	
		Mat temp = Mat(img.size(), img.type(), backgroundColor);

		Rect rectCrop(0, 0, newWidth, newHeight);

		scaled.copyTo(temp(rectCrop));

	/*	cv::namedWindow("temp", CV_WINDOW_AUTOSIZE);
		cv::imshow("temp", temp);*/

		warpAffine(temp, translated, translationMatrix, Size(origWidth, origHeight), 1, 0, backgroundColor);
		
		corrected = translated;

		//positionCorrected = findMarkerPosition(corrected, markerColor);
	
	}

/*	Mat finalTranslation;

	Mat translationMatrixFinal = (Mat_<double>(2, 3) << 1, 0, refTx, 0, 1, refTy);

	warpAffine(corrected, finalTranslation, translationMatrixFinal, corrected.size(), 1, 0, Scalar(BACKGROUND_HAIR_B, BACKGROUND_HAIR_G, BACKGROUND_HAIR_R, 0));*/
	

	/*cv::namedWindow("original1", CV_WINDOW_AUTOSIZE);
	cv::imshow("original1", img);

	cv::namedWindow("scaled1", CV_WINDOW_AUTOSIZE);
	cv::imshow("scaled1", scaled);

	cv::namedWindow("translated1", CV_WINDOW_AUTOSIZE);
	cv::imshow("translated1", translated);
	
	cv::namedWindow("corrected1", CV_WINDOW_AUTOSIZE);
	cv::imshow("corrected1", corrected);*/

	//cv::namedWindow("finalTranslation", CV_WINDOW_AUTOSIZE);
	//cv::imshow("finalTranslation", finalTranslation);
	//cv::waitKey();
	
	return corrected;
}
