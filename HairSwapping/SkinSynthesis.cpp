#include <stdio.h>
#include <stdlib.h>
#include <list>
#include <random>

// OpenCV
#include <opencv2//core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/photo.hpp>"
#include "HairExtraction.h"
#include "face.h"
#include "skinSynthesis.h"
#include "ColorEstimate.h"


using namespace std;
using namespace cv;

Mat synthesizeSkin(Mat imgRGB, Face face, Mat hairMask)
{
	Mat A = imgRGB(face.getRegionA());
	Mat B = imgRGB(face.getRegionB());
	Mat C = imgRGB(face.getRegionC());
	
	/*cv::imshow("regionA", A);
	cv::imshow("regionB", B);
	cv::imshow("regionC", C);
	cv::waitKey();*/

	Mat facePixels_Lab;
	Mat A_Lab;
	Mat B_Lab;
	Mat C_Lab;

	cv::cvtColor(A, A_Lab, cv::COLOR_BGR2Lab);
	cv::cvtColor(B, B_Lab, cv::COLOR_BGR2Lab);
	cv::cvtColor(C, C_Lab, cv::COLOR_BGR2Lab);

	ColorEstimate colorEst(A_Lab, B_Lab, C_Lab);
	
	Mat facePixels(imgRGB.rows, imgRGB.cols, imgRGB.type(), Scalar(BACKGROUND_SKIN_B, BACKGROUND_SKIN_G, BACKGROUND_SKIN_R));

	Mat faceMask = face.getFaceMask();

	imgRGB.copyTo(facePixels, faceMask);

	cvtColor(facePixels, facePixels_Lab, cv::COLOR_BGR2Lab);
	//cvtColor(facePixels_Lab, facePixels, cv::COLOR_Lab2BGR);

	//cv::imshow("facePixels_Lab", facePixels_Lab);
	//cv::waitKey();

	//cv::imshow("facePixels1", facePixels);
	//cv::waitKey();

	/*cv::imshow("facePixels", facePixels);
	cv::waitKey();*/

	/*Mat hairpixels(imgRGB.rows, imgRGB.cols, CV_8UC3, Scalar(0, 0, 0));
	imgRGB.copyTo(hairpixels, hairMask);*/

	//cv::imshow("hairpixels", hairpixels);
	//cv::waitKey();	

	//define Brigthest col in B
	Mat B_Lab_Vec[3];
	split(B_Lab, B_Lab_Vec);
	Mat B_L = B_Lab_Vec[0];
	int brigthestColInd = findBrigthestColumnB(B_L);
	Mat brigthestCol_L = B_L.col(brigthestColInd);
	int middleInd = brigthestColInd + face.getRegionB().x;

	//define first col in A
	Mat A_Lab_Vec[3];
	split(A_Lab, A_Lab_Vec);
	Mat A_L = A_Lab_Vec[0];	
	//Mat firstCol_L = A_L.col(0);
	Mat firstCol_L = A_L.col(A_L.cols - 1); //rightmost column of A

	//define last col in C
	Mat C_Lab_Vec[3];
	split(C_Lab, C_Lab_Vec);
	Mat C_L = C_Lab_Vec[0];
	//Mat lastCol_L = C_L.col(C_L.cols-1);
	Mat lastCol_L = C_L.col(0); //leftmost column of C

	int firstForeheadRow;
	int lastForeheadRow;
	int firstForeheadCol;
	int lastForeheadCol;

	Mat facePixels_Lab_NoHair = facePixels_Lab.clone();

	Mat foreheadInd = findIndForhead(facePixels_Lab_NoHair, faceMask, hairMask, &firstForeheadRow, &lastForeheadRow, &firstForeheadCol, &lastForeheadCol, colorEst, face.getTopEye());

	Mat foreheadPixels_Lab = Mat(facePixels.rows, facePixels.cols, facePixels.type(), Scalar(0, 0, 0));


	std::default_random_engine gaussianGenerator;
	std::normal_distribution<double> gaussianDistribution_L_A(colorEst.getMeanA()[0], 1);
	std::normal_distribution<double> gaussianDistribution_L_B(colorEst.getMeanB()[0], 1);
	std::normal_distribution<double> gaussianDistribution_L_C(colorEst.getMeanC()[0], 1);

	double beta = 1.5;

	for (int i = 0; i < firstCol_L.rows; i++)
	{	
		if (firstCol_L.at<uchar>(i,0) < colorEst.getMeanA()[0] || lastCol_L.at<uchar>(i, 0) > beta*colorEst.getMeanA()[0] + colorEst.getStdA()[0])
		{
			//firstCol_L.at<uchar>(i,0) = (uchar)gaussianDistribution_L_A(gaussianGenerator);
			firstCol_L.at<uchar>(i, 0) = (uchar)colorEst.getMeanA()[0];
		}
	}

	for (int i = 0; i < lastCol_L.rows; i++)
	{
		if (lastCol_L.at<uchar>(i, 0) < colorEst.getMeanC()[0] || lastCol_L.at<uchar>(i, 0) > beta*(colorEst.getMeanC()[0] + colorEst.getStdC()[0]))
		{
			//lastCol_L.at<uchar>(i, 0) = (uchar)gaussianDistribution_L_C(gaussianGenerator);
			lastCol_L.at<uchar>(i, 0) = (uchar)colorEst.getMeanC()[0];
		}
	}

	for (int i = 0; i < brigthestCol_L.rows; i++)
	{
		if (brigthestCol_L.at<uchar>(i, 0) < colorEst.getMeanB()[0] || lastCol_L.at<uchar>(i, 0) > beta*colorEst.getMeanB()[0] + colorEst.getStdB()[0])
		{
			//brigthestCol_L.at<uchar>(i, 0) = (uchar)gaussianDistribution_L_B(gaussianGenerator);
			brigthestCol_L.at<uchar>(i, 0) = (uchar)colorEst.getMeanB()[0];
		}
	}

	//int regionRow = 0;
	//int updateRow = 1;
	//for (int i = firstForeheadRow;i <= lastForeheadRow; i++)

	Mat foreHeadMask = Mat(facePixels.rows, facePixels.cols, CV_8UC1, Scalar(0, 0, 0));
		
	int regionRow = face.getRegionA().height - 1;
	int updateRow = -1;	
	for (int i = lastForeheadRow - 1; i > 0; i--)
	{		
		int firstJ = foreheadInd.at<int>(i, FIRST_COL);
		int lastJ = foreheadInd.at<int>(i, LAST_COL);

		if (firstJ == 10000)  //no hair on this row;
		{
			continue;
		}

		//int regionRow = (i - firstForeheadRow) % face.getRegionA().height;  //modulus operator because forhead might be taller than regions A,B,C

		if (firstJ < middleInd)   // if first column less than middle column, ignore left side of face
		{
			uchar firstL = firstCol_L.at<uchar>(regionRow);  //always interpolate from first column of A to middle column of B
			uchar lastL = brigthestCol_L.at<uchar>(regionRow);

			//firstL = max((uchar)colorEst.getMeanA()[0], firstL);
			//lastL = max((uchar)colorEst.getMeanB()[0], lastL);		

			int currfirstJ = firstJ;
			int currlastJ = min(middleInd, lastJ);  //handles when last column is less than middle column and won't go to second part

			if (i > face.getTopEye())    //row is below eyebrows, don't update all columns
			{
				currlastJ = min(face.getLeftEdgeEye(), currlastJ);
			}

			int numberOfInterpolationPoints = middleInd - firstForeheadCol + 1;
			updateForeheadPixels2(i, firstL, lastL, currfirstJ, currlastJ, numberOfInterpolationPoints, firstForeheadCol, middleInd, colorEst.getAvgSkinColor(), foreheadPixels_Lab, foreHeadMask);

			//int numberOfPoints = currlastJ - currfirstJ + 1;
			//updateForeheadPixels(i, firstL, lastL, currfirstJ, currlastJ, numberOfInterpolationPoints, colorEst.getAvgSkinColor(), foreheadPixels_Lab, foreHeadMask);
			//updateForeheadPixels(i, 0, 255, currfirstJ, currlastJ, numberOfPoints, colorEst.getAvgSkinColor(), foreheadPixels_Lab, foreHeadMask);

			/*uchar AfirstL = A_Lab.at<Vec3b>(A_Lab.rows/2, A_Lab.cols / 2)[0];
			uchar Afirsta = A_Lab.at<Vec3b>(A_Lab.rows / 2, A_Lab.cols / 2)[1];
			uchar Afirstb = A_Lab.at<Vec3b>(A_Lab.rows / 2, A_Lab.cols / 2)[2];*/

			/*uchar AfirstL = A.at<Vec3b>(A.rows / 2, A.cols / 2)[0];
			uchar Afirsta = A.at<Vec3b>(A.rows / 2, A.cols / 2)[1];
			uchar Afirstb = A.at<Vec3b>(A.rows / 2, A.cols / 2)[2];*/
			
			//updateForeheadPixelsWithFixedValue(i, firstL, lastL, currfirstJ, currlastJ, colorEst.getAvgSkinColor(), facePixels_Lab, colorEst.getMeanA()[0],colorEst.getAvgSkinColor()[0], colorEst.getAvgSkinColor()[1]);
			//updateForeheadPixelsWithFixedValue(i, firstL, lastL, currfirstJ, currlastJ, colorEst.getAvgSkinColor(), facePixels_Lab, AfirstL, Afirsta, Afirstb);
			
		}

		if (middleInd < lastJ)   // if last column less than middle column, ignore right side of face
		{
			//uchar firstL = firstCol_L.at<uchar>(regionRow);  //always interpolate from first column of A to middle column of B
			uchar firstL = brigthestCol_L.at<uchar>(regionRow);    //always interpolate from middle column of B to last column of A;
			uchar lastL = lastCol_L.at<uchar>(regionRow);

			//firstL = max((uchar)colorEst.getMeanB()[0], firstL);
			//lastL = max((uchar)colorEst.getMeanC()[0], lastL);			

			int currfirstJ = max(middleInd, firstJ); // handles when first column is greater than middle column
			int currlastJ = lastJ;

			if (i > face.getTopEye())    //row is below eyebrows, don't update all columns
			{
				currfirstJ = max(face.getRightEdgeEye(), currfirstJ);
			}

			int numberOfInterpolationPoints = lastForeheadCol - middleInd + 1;
			updateForeheadPixels2(i, firstL, lastL, currfirstJ, currlastJ, numberOfInterpolationPoints, middleInd, lastForeheadCol, colorEst.getAvgSkinColor(), foreheadPixels_Lab, foreHeadMask);

			//int numberOfPoints = currlastJ - currfirstJ + 1;
			//updateForeheadPixels(i, 255, 0, currfirstJ, currlastJ, numberOfPoints, colorEst.getAvgSkinColor(), foreheadPixels_Lab, foreHeadMask);
			//updateForeheadPixels(i, firstL, lastL, currfirstJ, currlastJ, numberOfPoints, colorEst.getAvgSkinColor(), foreheadPixels_Lab, foreHeadMask);
			//updateForeheadPixelsWithFixedValue(i, firstL, lastL, currfirstJ, currlastJ, colorEst.getAvgSkinColor(), facePixels_Lab, AfirstL, Afirsta, Afirstb);
			//updateForeheadPixelsWithFixedValue(i, firstL, lastL, currfirstJ, currlastJ, colorEst.getAvgSkinColor(), facePixels_Lab, colorEst.getMeanB()[0], colorEst.getAvgSkinColor()[0], colorEst.getAvgSkinColor()[1]);
		}

		if (regionRow == face.getRegionA().height - 1)
		{
			updateRow = -1;
		}

		if (regionRow == 0)
		{
			regionRow = face.getRegionA().height - 1;
			//updateRow = 1;
		}

		regionRow = regionRow + updateRow;

	}
		//cv::cvtColor(facePixels_Lab, facePixels, cv::COLOR_Lab2BGR);

  /*  cv:namedWindow("x", CV_WINDOW_AUTOSIZE);
	cv::imshow("facePixels", facePixels);
	cv::waitKey();*/

	Mat foreheadPixels;

	cv::cvtColor(foreheadPixels_Lab, foreheadPixels, cv::COLOR_Lab2BGR);

	//Mat test(50, 50, facePixels.type(), Scalar(255,255,255));
	//Mat testMask(50, 50, CV_8UC1, Scalar(255));
	//
	//foreheadPixels.copyTo(facePixels, foreHeadMask);
	

	//Rect foreheadRoi = Rect(firstForeheadCol, lastForeheadRow - FORHEAD_TRANSITION_HEIGHT, lastForeheadCol - firstForeheadCol + 1, FORHEAD_TRANSITION_HEIGHT);
	Rect foreheadRoi = Rect(firstForeheadCol, firstForeheadRow, lastForeheadCol - firstForeheadCol + 1, lastForeheadRow - firstForeheadRow + 1);
	
	Mat facePixelsRect = facePixels(foreheadRoi);
	Mat foreheadPixelsRect = foreheadPixels(foreheadRoi);
	Mat foreheadMaskRect = foreHeadMask(foreheadRoi);
	//foreheadMaskRect = Mat(foreheadPixels.rows, foreheadPixels.cols, CV_8UC1, Scalar(255));

	//int nchannels = foreheadMaskRect.channels();
  /*  cv:namedWindow("foreheadPixels", CV_WINDOW_AUTOSIZE);
	cv::imshow("foreheadPixels", foreheadPixels);
	cv::imshow("foreHeadMask", foreheadMaskRect * 255);*/
	//cv::imshow("testMask", testMask * 255);
	//cv::imshow("test", test * 255);
	//cv::waitKey();

	Mat seamLessCloneOutput;
	Point center(firstForeheadCol + foreheadMaskRect.cols / 2, firstForeheadRow + foreheadMaskRect.rows/2);
	//Point center(firstForeheadCol + foreheadMaskRect.cols / 2, lastForeheadRow - FORHEAD_TRANSITION_HEIGHT + foreheadMaskRect.rows / 2);

	//facePixels.at<Vec3b>(center.x, center.y)[0] = 0;
	//facePixels.at<Vec3b>(center.x, center.y)[1] = 0;
	//facePixels.at<Vec3b>(center.x, center.y)[2] = 255;

	/*Mat facePixelsRect = facePixels(foreheadRoi);
	foreheadPixels.copyTo(facePixelsRect, foreheadMaskRect);*/

	//cv:namedWindow("facePixels", CV_WINDOW_AUTOSIZE);
	//cv::imshow("facePixels", facePixels);
	//cv::waitKey();

	/*foreheadPixels = foreheadPixels(Rect(0, 0, 50, 50));
	foreheadMaskRect = foreheadMaskRect(Rect(0, 0, 50, 50));*/

	Mat facePixels2 = facePixels.clone();

	foreheadPixels.copyTo(facePixels2, foreHeadMask);

  /*  cv:namedWindow("x", CV_WINDOW_AUTOSIZE);
	cv::imshow("facePixels2", facePixels2);
	cv::waitKey();*/

	Mat facePixelsNoHair;

	cv::cvtColor(facePixels_Lab_NoHair, facePixelsNoHair, cv::COLOR_Lab2BGR);

	seamlessClone(foreheadPixelsRect, facePixelsNoHair, foreheadMaskRect, center, seamLessCloneOutput, NORMAL_CLONE);
	//seamlessClone(facePixelsRect, facePixels2, foreheadMaskRect, center, seamLessCloneOutput, NORMAL_CLONE);

	int widthSmooth = 10;
	//Mat brigthRegionSmoothingLab = Mat(face.getTopEye() - firstForeheadRow + 1, widthSmooth, facePixels_Lab.type(), Scalar(colorEst.getMeanB()[0], colorEst.getMeanB()[1], colorEst.getMeanB()[2]));
	//Mat brigthRegionSmoothing;
	//cv::cvtColor(brigthRegionSmoothingLab, brigthRegionSmoothing, cv::COLOR_Lab2BGR);


	Mat brigthRegionSmoothing = Mat(face.getTopEye() - firstForeheadRow + 1, widthSmooth, facePixels_Lab.type(), Scalar(255, 255, 255));

	Mat cloneMask = Mat::ones(brigthRegionSmoothing.size(), CV_8UC1)*255;

	center = Point(brigthestColInd + face.getRegionB().x, (firstForeheadRow + face.getTopEye()) / 2);

	Mat seamLessCloneOutputSmooth;
	seamlessClone(brigthRegionSmoothing, seamLessCloneOutput, cloneMask, center, seamLessCloneOutputSmooth, NORMAL_CLONE);

	/*Rect transitionRoi = Rect(firstForeheadCol, lastForeheadRow - FORHEAD_TRANSITION_HEIGHT, lastForeheadCol - firstForeheadCol + 1, FORHEAD_TRANSITION_HEIGHT);

	Mat facePixelsTransition = facePixels2(transitionRoi);*/

	//cv:namedWindow("x", CV_WINDOW_AUTOSIZE);
	//cv::imshow("facePixelsTransition", facePixelsTransition);
	//cv::waitKey();
	
	/*Mat seamLessCloneTransition = seamLessCloneOutput(transitionRoi);*/

	//cv::imshow("seamLessCloneTransition", seamLessCloneTransition);

	//seamLessCloneTransition.copyTo(facePixelsTransition);

    cv:namedWindow("seamlessClone", CV_WINDOW_FREERATIO);
	cv::imshow("facePixelsNoHair", facePixelsNoHair);
	cv::imshow("Seamless Cloning", seamLessCloneOutput);	
	cv::imshow("Seamless Cloning2", seamLessCloneOutputSmooth);
	cv::imshow("Interpolation Only", facePixels2);
	cv::waitKey();
	//
	//face.setFacePixels(facePixels);

	return facePixels;

}

void updateForeheadPixelsWithFixedValue(int i, uchar firstL, uchar lastL, int firstJ, int lastJ, Scalar avgSkinColor, Mat facePixels_Lab, uchar L, uchar a, uchar b)
{
	for (int j = firstJ; j <= lastJ;j++)
	{
		facePixels_Lab.at<Vec3b>(i, j)[0] = L;
		facePixels_Lab.at<Vec3b>(i, j)[1] = a;
		facePixels_Lab.at<Vec3b>(i, j)[2] = b;		
	}
}


void updateForeheadPixels2(int i, uchar firstL, uchar lastL, int firstUpdateJ, int lastUpdateJ, int numberOfPoints, int firstForeheadCol, int lastForeheadCol, Scalar avgSkinColor, Mat facePixels_Lab, Mat foreHeadMask)
{
	

	double stepL = (double)(lastL - firstL) / numberOfPoints;
	
	for (int j = firstUpdateJ; j <= lastUpdateJ;j++)
	{

		double currL = firstL + (j - firstForeheadCol)*stepL;
		//double currL = firstL;

		facePixels_Lab.at<Vec3b>(i, j)[0] = (uchar)cvRound(currL);
		facePixels_Lab.at<Vec3b>(i, j)[1] = avgSkinColor[0];
		facePixels_Lab.at<Vec3b>(i, j)[2] = avgSkinColor[1];

		foreHeadMask.at<uchar>(i, j) = 255;
	}
}

void updateForeheadPixels(int i, uchar firstL, uchar lastL, int firstJ, int lastJ, int numberOfPoints, Scalar avgSkinColor, Mat facePixels_Lab, Mat foreHeadMask)
{
	double stepL = (double)(lastL - firstL) / numberOfPoints;

	double currL = firstL;

	for (int j = firstJ; j <= lastJ;j++)
	{
		facePixels_Lab.at<Vec3b>(i, j)[0] = (uchar)cvRound(currL);
		facePixels_Lab.at<Vec3b>(i, j)[1] = avgSkinColor[0];
		facePixels_Lab.at<Vec3b>(i, j)[2] = avgSkinColor[1];

		foreHeadMask.at<uchar>(i, j) = 255;

		currL = currL + stepL;
	}
}


bool findHairOnNeighbors(int i, int j, int neighboorHoodSize, Mat hairMask)
{
	int nRows = hairMask.rows;
	int nCols = hairMask.cols;

	for (int ni = -neighboorHoodSize; ni <= neighboorHoodSize; ni++)
	{
		if ((i + ni) < 0 || (i + ni) >= nRows)
		{
			continue;
		}
		
		for (int nj = -neighboorHoodSize; nj <= neighboorHoodSize; nj++)
		{

			if ((j + nj) < 0 || (j + nj) >= nCols)
			{
				continue;
			}

			if (hairMask.at<uchar>(i + ni, j + nj) > 0)
			{
				return true;
			}

		}
	}

	return false;
}

Mat sinthesizeTexture(Mat textureReference, int blockSize, int nRowsForehead, int nColsForehead)
{
	int nBlocksVertical = ceil((double) nRowsForehead / blockSize);
	int nBlocksHorizontal = ceil((double) nColsForehead / blockSize);

	Mat texture(nBlocksVertical*blockSize, nBlocksHorizontal*blockSize, CV_8UC3);

	int nBlocksVerticalRef = textureReference.rows / blockSize; //rouded down
	int nBlocksHorRef = textureReference.cols / blockSize;

	for (int v = 0; v < nBlocksVertical; v++)
	{
		for (int h = 0; h < nBlocksHorizontal; h++)
		{
			Rect rectTexture(v*blockSize, h*blockSize, blockSize, blockSize);
			
			Mat currBlock = texture(rectTexture);

			int chosenBlockV = round(nBlocksVerticalRef*rand());
			int chosenBlockH = round(nBlocksHorRef*rand());

			Rect rectRef(chosenBlockV*blockSize, chosenBlockH*blockSize, blockSize, blockSize);

			textureReference.copyTo(currBlock);
		}
	}

	return texture;
}

Vec3b findClosestSkinPixel(int i, int j, int maxNeighboordSize, Mat facePixels, ColorEstimate cEst)
{
	int neighboorHoodSize = 1;
	int nRows = facePixels.rows;
	int nCols = facePixels.cols;

	while (neighboorHoodSize < maxNeighboordSize)
	{

		for (int ni = -neighboorHoodSize; ni <= neighboorHoodSize; ni++)
		{
			if ((i + ni) < 0 || (i + ni) >= nRows)
			{
				continue;
			}

			for (int nj = -neighboorHoodSize; nj <= neighboorHoodSize; nj++)
			{

				if ((j + nj) < 0 || (j + nj) >= nCols)
				{
					continue;
				}

				uchar L = facePixels.at <Vec3b>(i + ni, j + nj)[0];
				uchar a = facePixels.at <Vec3b>(i + ni, j + nj)[1];
				uchar b = facePixels.at <Vec3b>(i + ni, j + nj)[2];

				double stddev_a = cEst.getStdA()[1];
				double mu_a = cEst.getMeanA()[1];
				double stddev_b = cEst.getStdA()[2];
				double mu_b = cEst.getMeanA()[2];

				bool closeColor = abs(a - mu_a) < stddev_a || abs(b - mu_b) < stddev_b; //check if color is close to the mean of region A

				if (closeColor)
				{
					return Vec3b(L,a,b);					
				}

			}
		
		}

		neighboorHoodSize++;
	}

	return Vec3b(0, 0, 0);
}

Mat findIndForhead(Mat facePixels, Mat faceMask, Mat hairMask, int* firstForeheadRow, int* lastForeheadRow, int* firstForeheadCol, int* lastForeheadCol, ColorEstimate cEst, int posTopEye)
{
	Mat foreheadIndFirstCol(faceMask.rows, 1, CV_32SC1, Scalar(10000)); //initialize matrix with first and last column of forehead for each row
	Mat foreheadIndLastCol(faceMask.rows, 1, CV_32SC1, Scalar(0));
	Mat foreheadInd;

	hconcat(foreheadIndFirstCol, foreheadIndLastCol, foreheadInd);

	*firstForeheadRow = 10000;
    *lastForeheadRow  = 0;

	*firstForeheadCol = 10000;
	*lastForeheadCol = 0;

	std::default_random_engine gaussianGenerator;
	std::normal_distribution<double> gaussianDistribution_L(cEst.getMeanA()[0], cEst.getStdA()[0]);
	std::normal_distribution<double> gaussianDistribution_a(cEst.getMeanA()[1], cEst.getStdA()[1]);
	std::normal_distribution<double> gaussianDistribution_b(cEst.getMeanA()[2], cEst.getStdA()[2]);

	//search for first and last col of each row on the forehead
	for (int i = 0;i < faceMask.rows; i++)
	{

		for (int j = 0;j < faceMask.cols;j++)
		{	
		/*	uchar a = facePixels.at <Vec3b>(i, j)[1];
			uchar b = facePixels.at <Vec3b>(i, j)[2];

			double stddev_a = cEst.getStdA()[1];
			double mu_a = cEst.getMeanA()[1];
			double stddev_b = cEst.getStdA()[2];
			double mu_b = cEst.getMeanA()[2];

			bool hasHairNeighbor = findHairOnNeighbors(i, j, 1, hairMask);

			bool differentColor = abs(a - mu_a) > stddev_a || abs(b - mu_b) > stddev_b;

			bool notBackground = faceMask.at<uchar>(i, j) != 0;

			if (hasHairNeighbor && differentColor && notBackground)
			{
				facePixels.at <Vec3b>(i, j)[0] = (uchar)gaussianDistribution_L(gaussianGenerator);
				facePixels.at <Vec3b>(i, j)[1] = (uchar)gaussianDistribution_a(gaussianGenerator);
				facePixels.at <Vec3b>(i, j)[2] = (uchar)gaussianDistribution_b(gaussianGenerator);
			}*/

			if (faceMask.at<uchar>(i, j) > 0 && hairMask.at<uchar>(i, j) > 0)
			{

				if (i < posTopEye) // row on the forehead, probably a lot of original skin pixels
				{
					facePixels.at <Vec3b>(i, j)[0] = (uchar)gaussianDistribution_L(gaussianGenerator);
					facePixels.at <Vec3b>(i, j)[1] = (uchar)gaussianDistribution_a(gaussianGenerator);
					facePixels.at <Vec3b>(i, j)[2] = (uchar)gaussianDistribution_b(gaussianGenerator);
				}

				else  //row is close to the eye, so it is side hair.
				{
					Vec3b skinColor = findClosestSkinPixel(i, j, 10, facePixels, cEst);

					if (skinColor[0] == 0 && skinColor[1] == 0 && skinColor[2] == 0)  // couldnt find nearby skin pixel
					{
						facePixels.at <Vec3b>(i, j) = Vec3b(cEst.getMeanA()[0], cEst.getMeanA()[1], cEst.getMeanA()[2]);
					}

					else
					{
						facePixels.at <Vec3b>(i, j) = skinColor;
					}
				}
				
				if (j < foreheadInd.at<int>(i, FIRST_COL))
				{
					foreheadInd.at<int>(i, FIRST_COL) = j;
					
				}

				if (j > foreheadInd.at<int>(i, LAST_COL))
				{
					foreheadInd.at<int>(i, LAST_COL) = j;
					
				}

				if (i > *lastForeheadRow)
				{
					*lastForeheadRow = i;
				}

				if (i < *firstForeheadRow)
				{
					*firstForeheadRow = i;
				}

				if (j < *firstForeheadCol)
				{
					*firstForeheadCol = j;
				}

				if (j > *lastForeheadCol)
				{
					*lastForeheadCol = j;
				}
			}			
		}
	}

	//update face rows above the first intersection of hair and face
	for (int i = 0;i < *firstForeheadRow; i++)
	{

		for (int j = 0;j < faceMask.cols;j++)
		{
			if (faceMask.at<uchar>(i, j) > 0)  
			{

				if (j < foreheadInd.at<int>(i, FIRST_COL))
				{
					foreheadInd.at<int>(i, FIRST_COL) = j;

				}

				if (j > foreheadInd.at<int>(i, LAST_COL))
				{
					foreheadInd.at<int>(i, LAST_COL) = j;

				}

				if (i > *lastForeheadRow)
				{
					*lastForeheadRow = i;
				}

				if (i < *firstForeheadRow)
				{
					*firstForeheadRow = i;
				}

				if (j < *firstForeheadCol)
				{
					*firstForeheadCol = j;
				}

				if (j > *lastForeheadCol)
				{
					*lastForeheadCol = j;
				}
			}
		}
	}

	return foreheadInd;
}

int findBrigthestColumnB(Mat B_L)
{	
	uchar brigthestValue = 0;

	int brigthestColInd = 0;

	for (int i = 0;i < B_L.rows; i++)
	{
		for (int j = 0;j < B_L.cols;j++)
		{
			if (B_L.at<uchar>(i, j) > brigthestValue)
			{
				brigthestValue = B_L.at<uchar>(i, j);
				brigthestColInd = j;
			}
		}
	}

	return brigthestColInd;
}





