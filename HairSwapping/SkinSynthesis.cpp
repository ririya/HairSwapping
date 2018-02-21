#include <stdio.h>
#include <stdlib.h>
#include <list>

// OpenCV
#include <opencv2//core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/calib3d.hpp>
#include <opencv2/video/tracking.hpp>
#include "opencv/highgui.h"
#include "stasm_lib.h"
#include "HairExtraction.h"
#include "faceRecognition.h"
#include "skinSynthesis.h"

using namespace std;
using namespace cv;

Mat synthesizeSkin(Mat imgRGB, Face face, Mat hairMask)
{
	Mat A = imgRGB(face.getRegionA());
	Mat B = imgRGB(face.getRegionB());
	Mat C = imgRGB(face.getRegionC());
	
	//cv::imshow("regionA", A);
	//cv::waitKey();
	//cv::imshow("regionB", B);
	//cv::waitKey();
	//cv::imshow("regionC", C);
	//cv::waitKey();

	Mat facePixels_Lab;
	Mat A_Lab;
	Mat B_Lab;
	Mat C_Lab;

	cvtColor(A, A_Lab, cv::COLOR_BGR2Lab);
	cvtColor(B, B_Lab, cv::COLOR_BGR2Lab);
	cvtColor(C, C_Lab, cv::COLOR_BGR2Lab);

	Scalar avgSkinColor = getAvgSkinColor(A_Lab, C_Lab);

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
	Mat B_L = B_Lab_Vec[1];
	int brigthestColInd = findBrigthestColumnB(B_L);
	Mat brigthestCol_L = B_L.col(brigthestColInd);
	int middleInd = brigthestColInd + face.getRegionB().x;

	//define first col in A
	Mat A_Lab_Vec[3];
	split(A_Lab, A_Lab_Vec);
	Mat A_L = A_Lab_Vec[1];	
	Mat firstCol_L = A_L.col(0);

	//define last col in C
	Mat C_Lab_Vec[3];
	split(C_Lab, C_Lab_Vec);
	Mat C_L = C_Lab_Vec[1];
	Mat lastCol_L = C_L.col(C_L.cols-1);

	int firstForeheadRow;
	int lastForeheadRow;

	Mat foreheadInd = findIndForhead(facePixels, faceMask, hairMask, &firstForeheadRow, &lastForeheadRow);

	for (int i = firstForeheadRow;i <= lastForeheadRow; i++)
	{	
		//printf("i=%d\n", i);
		int firstJ = foreheadInd.at<int>(i, firstCol);
		int lastJ = foreheadInd.at<int>(i, lastCol);

		if (firstJ == 10000)  //no hair on this row;
		{
			continue;
		}

		facePixels.at<Vec3b>(i, firstJ)[0] = 0;
		facePixels.at<Vec3b>(i, firstJ)[1] = 0;
		facePixels.at<Vec3b>(i, firstJ)[2] = 255;

		facePixels.at<Vec3b>(i, lastJ)[0] = 0;
		facePixels.at<Vec3b>(i, lastJ)[1] = 255;
		facePixels.at<Vec3b>(i, lastJ)[2] = 0;

		/*if ((i - firstForeheadRow) % 10 == 0)
		{
			cv::namedWindow("hairpixels", CV_WINDOW_AUTOSIZE);
			cv::imshow("hairpixels", hairpixels);
			cv::namedWindow("facePixels", CV_WINDOW_AUTOSIZE);
			cv::imshow("facePixels", facePixels);
			cv::waitKey();
		}*/

		int regionRow = (i - firstForeheadRow) % face.getRegionA().height;  //modulus operator because forhead might be taller than regions A,B,C

		if (firstJ < middleInd)   // if first column less than middle column, ignore left side of face
		{
			uchar firstL = firstCol_L.at<uchar>(regionRow);  //always interpolate from first column of A to middle column of B
			uchar lastL = brigthestCol_L.at<uchar>(regionRow);
			int currfirstJ = firstJ;
			int currlastJ = min(middleInd, lastJ);  //handles when last column is less than middle column and won't go to second part

			if (i > face.getTopEye())    //row is below eyebrows, don't update all columns
			{
				currlastJ = min(face.getLeftEdgeEye(), currlastJ);
			}

			int numberOfPoints = currlastJ - currfirstJ + 1;
			updateForeheadPixels(i, firstL, lastL, currfirstJ, currlastJ, numberOfPoints, avgSkinColor, facePixels_Lab);
		}

		if (middleInd < lastJ)   // if last column less than middle column, ignore right side of face
		{
			uchar firstL = brigthestCol_L.at<uchar>(regionRow);    //always interpolate from middle column of B to last column of A;
			uchar lastL = lastCol_L.at<uchar>(regionRow);
			int currfirstJ = max(middleInd, firstJ); // handles when first column is greater than middle column
			int currlastJ = lastJ;

			if (i > face.getTopEye())    //row is below eyebrows, don't update all columns
			{
				currfirstJ = max(face.getRightEdgeEye(), currfirstJ);
			}

			int numberOfPoints = currlastJ - currfirstJ + 1;
			updateForeheadPixels(i, firstL, lastL, currfirstJ, currlastJ, numberOfPoints, avgSkinColor, facePixels_Lab);
		}

		cvtColor(facePixels_Lab, facePixels, cv::COLOR_Lab2BGR);
	}

	/*cv::imshow("hairMaskNegative", hairMaskNegative);
	cv::waitKey();

	hairMaskNegative.copyTo(facePixels, hairMask);*/

	//cv::imshow("facePixels", facePixels);
	//cv::waitKey();
	
	//face.setFacePixels(facePixels);

	return facePixels;

}

void updateForeheadPixels(int i, uchar firstL, uchar lastL, int firstJ, int lastJ, int numberOfPoints, Scalar avgSkinColor, Mat facePixels_Lab)
{
	double stepL = (double)(lastL - firstL) / numberOfPoints;

	double currL = firstL;

	for (int j = firstJ; j <= lastJ;j++)
	{
		facePixels_Lab.at<Vec3b>(i, j)[0] = (uchar)cvRound(currL);
		facePixels_Lab.at<Vec3b>(i, j)[1] = avgSkinColor[0];
		facePixels_Lab.at<Vec3b>(i, j)[2] = avgSkinColor[1];
		currL = currL + stepL;
	}
}


Mat findIndForhead(Mat facePixels, Mat faceMask, Mat hairMask, int* firstForeheadRow, int* lastForeheadRow)
{
	Mat foreheadIndFirstCol(facePixels.rows, 1, CV_32SC1, Scalar(10000)); //initialize matrix with first and last column of forehead for each row
	Mat foreheadIndLastCol(facePixels.rows, 1, CV_32SC1, Scalar(0));
	Mat foreheadInd;

	hconcat(foreheadIndFirstCol, foreheadIndLastCol, foreheadInd);

	*firstForeheadRow = 10000;
    *lastForeheadRow  = 0;

	//search for first and last col of each row on the forehead
	for (int i = 0;i < facePixels.rows; i++)
	{

		for (int j = 0;j < facePixels.cols;j++)
		{

			if(*firstForeheadRow != 10000 && faceMask.at<uchar>(i, j) > 0)  //if this is not the first intersecting row, this row needs to be fully updated inside the face mask, even if no hair is found on this row
			{
				if (j < foreheadInd.at<int>(i, firstCol))
				{
					foreheadInd.at<int>(i, firstCol) = j;

				}

				if (j > foreheadInd.at<int>(i, lastCol))
				{
					foreheadInd.at<int>(i, lastCol) = j;

				}
			}

			if (faceMask.at<uchar>(i, j) > 0 && hairMask.at<uchar>(i, j) > 0)
			{
				
				if (j < foreheadInd.at<int>(i, firstCol))
				{
					foreheadInd.at<int>(i, firstCol) = j;
					
				}

				if (j > foreheadInd.at<int>(i, lastCol))
				{
					foreheadInd.at<int>(i, lastCol) = j;
					
				}

				if (i > *lastForeheadRow)
				{
					*lastForeheadRow = i;
				}

				if (i < *firstForeheadRow)
				{
					*firstForeheadRow = i;
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

Scalar getAvgSkinColor(Mat A_Lab, Mat C_Lab)
{	
	Scalar A_mean = mean(A_Lab);
	Scalar C_mean = mean(C_Lab);

	uchar A_a = A_mean[1];
	uchar A_b = A_mean[2];
	uchar C_a = C_mean[2];
	uchar C_b = C_mean[2];

	int sumA = (int)A_a + (int)C_a;
	int sumB = (int)A_b + (int)C_b;

	uchar avg_a = (uchar) (sumA / 2);
	uchar avg_b = (uchar)(sumB / 2);

	return Scalar(avg_a, avg_b);
}

