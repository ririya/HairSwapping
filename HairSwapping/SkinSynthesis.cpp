#include <stdio.h>
#include <stdlib.h>
#include <list>
#include <random>

// OpenCV
#include <opencv2//core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/photo.hpp>"
#include "Hair.h"
#include "Face.h"
#include "skinSynthesis.h"
#include "ColorEstimate.h"


using namespace std;
using namespace cv;

Mat synthesizeSkin(Mat imgRGB, Face face, Hair hair)
{
	Mat hairMask = hair.getHairMask();
	Mat hairPixels = hair.getHairPixels();

	Mat A = imgRGB(face.getRegionA());
	Mat B = imgRGB(face.getRegionB());
	Mat C = imgRGB(face.getRegionC());

	Mat facePixels_Lab;
	Mat A_Lab;
	Mat B_Lab;
	Mat C_Lab;

	cv::cvtColor(A, A_Lab, cv::COLOR_BGR2Lab);
	cv::cvtColor(B, B_Lab, cv::COLOR_BGR2Lab);
	cv::cvtColor(C, C_Lab, cv::COLOR_BGR2Lab);

	ColorEstimate colorEst(A_Lab, B_Lab, C_Lab); //calculate average values in each region
	
	Mat facePixels(imgRGB.rows, imgRGB.cols, imgRGB.type(), Scalar(BACKGROUND_SKIN_B, BACKGROUND_SKIN_G, BACKGROUND_SKIN_R));

	Mat faceMask = face.getFaceMask();

	imgRGB.copyTo(facePixels, faceMask);
	
	cvtColor(facePixels, facePixels_Lab, cv::COLOR_BGR2Lab);

	//define brightest col in B
	Mat B_Lab_Vec[3];
	split(B_Lab, B_Lab_Vec);
	Mat B_L = B_Lab_Vec[0];
	int brightestColInd = findbrightestColumnB(B_L);
	Mat brightestCol_L = B_L.col(brightestColInd);
	int middleInd = brightestColInd + face.getRegionB().x;

	//define first col in A
	Mat A_Lab_Vec[3];
	split(A_Lab, A_Lab_Vec);
	Mat A_L = A_Lab_Vec[0];	
	Mat firstCol_L = A_L.col(A_L.cols - 1); //rightmost column of A

	//define last col in C
	Mat C_Lab_Vec[3];
	split(C_Lab, C_Lab_Vec);
	Mat C_L = C_Lab_Vec[0];
	Mat lastCol_L = C_L.col(0); //leftmost column of C

	int firstForeheadRow;
	int lastForeheadRow;
	int firstForeheadCol;
	int lastForeheadCol;
	

	Mat replaceMask = Mat::zeros(facePixels.size(), CV_8UC1);

	Mat foreheadInd = findIndForhead(replaceMask, facePixels, faceMask, hair, &firstForeheadRow, &lastForeheadRow, &firstForeheadCol, &lastForeheadCol, colorEst, face); //find beginning and ending of hair pixels in each row

	obtainReplaceMask(replaceMask, facePixels, faceMask, hair, colorEst, face, firstForeheadRow, lastForeheadRow, firstForeheadCol, lastForeheadCol); //replace hair pixels with interpolated patch


	Mat maskForTexture;

	Mat foreheadPixels_Lab = Mat(facePixels.rows, facePixels.cols, facePixels.type(), Scalar(0, 0, 0));
	
	//handling dark pixels that cause weird transition: if too dark, set to average 
	for (int i = 0; i < firstCol_L.rows; i++)
	{	
		//if (firstCol_L.at<uchar>(i,0) < colorEst.getMeanA()[0] || lastCol_L.at<uchar>(i, 0) > WEIGHT_COLOR_ESTIMATE*colorEst.getMeanA()[0] + colorEst.getStdA()[0]
		if (firstCol_L.at<uchar>(i, 0) < colorEst.getMeanA()[0] - WEIGHT_COLOR_ESTIMATE*colorEst.getMeanA()[0])
		{
			firstCol_L.at<uchar>(i, 0) = (uchar)colorEst.getMeanA()[0];
		}
	}

	for (int i = 0; i < lastCol_L.rows; i++)
	{
		//if (firstCol_L.at<uchar>(i,0) < colorEst.getMeanA()[0] || lastCol_L.at<uchar>(i, 0) > WEIGHT_COLOR_ESTIMATE*colorEst.getMeanA()[0] + colorEst.getStdA()[0])if (lastCol_L.at<uchar>(i, 0) < colorEst.getMeanC()[0] || lastCol_L.at<uchar>(i, 0) > WEIGHT_COLOR_ESTIMATE*(colorEst.getMeanC()[0] + colorEst.getStdC()[0]))
		if (lastCol_L.at<uchar>(i, 0) < colorEst.getMeanC()[0] - WEIGHT_COLOR_ESTIMATE*colorEst.getMeanC()[0])
		{
			lastCol_L.at<uchar>(i, 0) = (uchar)colorEst.getMeanC()[0];
		}
	}

	for (int i = 0; i < brightestCol_L.rows; i++)
	{
		//if (brightestCol_L.at<uchar>(i, 0) < colorEst.getMeanB()[0] || lastCol_L.at<uchar>(i, 0) > WEIGHT_COLOR_ESTIMATE*colorEst.getMeanB()[0] + colorEst.getStdB()[0])
		if (brightestCol_L.at<uchar>(i, 0) < colorEst.getMeanB()[0] - WEIGHT_COLOR_ESTIMATE*colorEst.getMeanB()[0])
		{
			brightestCol_L.at<uchar>(i, 0) = (uchar)colorEst.getMeanB()[0];
		}
	}

	Mat foreHeadMask = Mat(facePixels.rows, facePixels.cols, CV_8UC1, Scalar(0, 0, 0));
		
	int regionRow = face.getRegionA().height - 1;
	int updateRow = -1;	
	for (int i = lastForeheadRow - 1; i > 0; i--)
	{		
		int firstJ = foreheadInd.at<int>(i, FIRST_COL);
		int lastJ = foreheadInd.at<int>(i, LAST_COL);

		if (firstJ == INT_MAX)  //no hair on this row;
		{
			continue;
		}

		//int regionRow = (i - firstForeheadRow) % face.getRegionA().height;  //modulus operator because forhead might be taller than regions A,B,C

		if (firstJ < middleInd)   // left side of face
		{
			uchar firstL = firstCol_L.at<uchar>(regionRow);  //always interpolate from last column of A to middle column of B
			uchar lastL = brightestCol_L.at<uchar>(regionRow);

			int currfirstJ = firstJ;
			int currlastJ = min(middleInd, lastJ);  //handles when last column is less than middle column 

			if (i > face.getTopEye())    //row is below eyebrows, don't update all columns
			{
				//currlastJ = min(face.getLeftEdgeEye(), currlastJ);
				currlastJ = face.getLeftEdgeEye();	
			}

			int numberOfInterpolationPoints = middleInd - firstForeheadCol + 1;
			updateForeheadPixels(i, firstL, lastL, currfirstJ, currlastJ, numberOfInterpolationPoints, firstForeheadCol, middleInd, colorEst.getAvgSkinColor(), foreheadPixels_Lab, foreHeadMask);		
		}

		if (middleInd < lastJ)   // right side of face
		{
			uchar firstL = brightestCol_L.at<uchar>(regionRow);    //always interpolate from middle column of B to first column of C;
			uchar lastL = lastCol_L.at<uchar>(regionRow);

			int currfirstJ = max(middleInd, firstJ); // handles when first column is greater than middle column
			int currlastJ = lastJ;

			if (i > face.getTopEye())    //row is below eyebrows, don't update all columns
			{
				//currfirstJ = max(face.getRightEdgeEye(), currfirstJ);
				currfirstJ = face.getRightEdgeEye();
			}

			int numberOfInterpolationPoints = lastForeheadCol - middleInd + 1;
			updateForeheadPixels(i, firstL, lastL, currfirstJ, currlastJ, numberOfInterpolationPoints, middleInd, lastForeheadCol, colorEst.getAvgSkinColor(), foreheadPixels_Lab, foreHeadMask);		
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

 	Mat foreheadPixels;

	cv::cvtColor(foreheadPixels_Lab, foreheadPixels, cv::COLOR_Lab2BGR);
	
	int nRowsAboveReferencePoint = N_ROWS_ABOVE_REFERENCE_POINT;
	int referencePoint = face.getTopEye();
	int nRowsBelowReferencePoint = lastForeheadRow - referencePoint + 1;
	Rect foreheadRoi = Rect(firstForeheadCol, referencePoint - nRowsAboveReferencePoint, lastForeheadCol - firstForeheadCol + 1, nRowsAboveReferencePoint + nRowsBelowReferencePoint + 1);
	Mat facePixelsRect = facePixels(foreheadRoi);
	Mat foreheadPixelsRect = foreheadPixels(foreheadRoi);
	Mat foreheadMaskRect = foreHeadMask(foreheadRoi);
	Point center(firstForeheadCol + foreheadMaskRect.cols / 2, face.getTopEye());

	int nRowsForehead = lastForeheadRow - firstForeheadRow + 1;
	int nColsForehead = lastForeheadCol - firstForeheadCol + 1;

	int textureBlockSize = TEXTURE_BLOCK_SIZE;

	int nBlocksVertical = ceil((double)nRowsForehead / textureBlockSize);  //rounded up so there are more blocks than the effective forehead region
	int nBlocksHorizontal = ceil((double)nColsForehead / textureBlockSize);
	Rect textureRect = Rect(firstForeheadCol, firstForeheadRow, nBlocksHorizontal*textureBlockSize, nBlocksVertical*textureBlockSize);
		
	Mat textureReference = A.cols > C.cols ? A : C;

	Mat texture = synthesizeTexture(textureReference, textureBlockSize, nRowsForehead, nColsForehead, 0.5);

	Mat facePixelsNoHair = facePixels.clone();

	foreheadPixels.copyTo(facePixelsNoHair, replaceMask); // getting rid of hair pixels which will cause problems on seamless 
	//texture.copyTo(facePixelsNoHair(textureRect), replaceMask(textureRect)); // getting rid of hair pixels which will cause problems on seamless clone
	
	Mat facePixelsOriginal = facePixels.clone();

	Mat seamLessCloneOutput;

	foreheadPixels.copyTo(facePixels, foreHeadMask);
	
	//Mat interpolationOnly = facePixels;
	
	Mat foreheadMaskOriginal = foreHeadMask.clone(); //seamlessClone modifies the mask so store it to use again later
	seamlessClone(foreheadPixelsRect, facePixelsNoHair, foreheadMaskRect, center, seamLessCloneOutput, MIXED_CLONE);

	Mat mixSeamLessCloneFacePixels = facePixels.clone();	

	seamLessCloneOutput(foreheadRoi).copyTo(mixSeamLessCloneFacePixels(foreheadRoi));
	
	Mat seamLessCloneOutputTexture;	

	Mat cloneMaskTexture = foreheadMaskOriginal(textureRect);
	center = Point(textureRect.x + textureRect.width/2, textureRect.y + textureRect.height / 2);

	seamlessClone(texture, mixSeamLessCloneFacePixels, cloneMaskTexture, center, seamLessCloneOutputTexture, NORMAL_CLONE);

	/*cv::imwrite("texture.png", texture);
	cv::imwrite("replaceMask.png", replaceMask);
	cv::imwrite("facePixels.png", facePixelsOriginal);
	cv::imwrite("facePixelsNoHair.png", facePixelsNoHair);
	cv::imwrite("linearInterpolation.png", facePixels);
	cv::imwrite("seamLessCloneOutput.png", seamLessCloneOutput);
	cv::imwrite("mixSeamLessCloneFacePixels.png", mixSeamLessCloneFacePixels);
	cv::imwrite("seamlesscloneoutputtexture.png", seamLessCloneOutputTexture);*/
	

	//cv:namedWindow("x", CV_WINDOW_AUTOSIZE);
	//cv::imshow("facepixelsnohair", facePixelsNoHair);
	//cv::imshow("seamless cloning", seamLessCloneOutput);
	//cv::imshow("mixSeamLessCloneFacePixels", mixSeamLessCloneFacePixels);
	//cv::imshow("seamless cloning3", seamLessCloneOutputTexture);
	//cv::imshow("interpolation only", interpolationOnly);
	//cv::waitKey();
	

	return seamLessCloneOutputTexture;

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


void updateForeheadPixels(int i, uchar firstL, uchar lastL, int firstUpdateJ, int lastUpdateJ, int numberOfPoints, int firstForeheadCol, int lastForeheadCol, Scalar avgSkinColor, Mat facePixels_Lab, Mat foreHeadMask)
{
	double stepL = (double)(lastL - firstL) / numberOfPoints; //linear interpolation
	
	for (int j = firstUpdateJ; j <= lastUpdateJ;j++)
	{

		double currL = firstL + (j - firstForeheadCol)*stepL;
		
		facePixels_Lab.at<Vec3b>(i, j)[0] = (uchar)cvRound(currL);
		facePixels_Lab.at<Vec3b>(i, j)[1] = avgSkinColor[0];
		facePixels_Lab.at<Vec3b>(i, j)[2] = avgSkinColor[1];

		foreHeadMask.at<uchar>(i, j) = 255;
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

void findBestMatchingNeighborBlock(int currV, int currH, int blockSize, int nBlocksVerticalRef, int nBlocksHorRef, Mat leftBlock_LastCol, Mat upperBlock_LastRow, Mat textureReference, double*chosenBlockV_ptr, double*chosenBlockH_ptr, double blockStep)
{
	int minCost = INT_MAX;
	for (double v = 0; v <= nBlocksVerticalRef-1; v += blockStep)
	{
		for (double h = 0; h <= nBlocksHorRef-1; h += blockStep)
		{

			//printf("vref = %f, href = %f \n", v, h);

			if (v == currV && h == currH)
			{
				continue; //do not pick same block
			}

			int currCost = 0;

			Rect currRect(int(h*blockSize), int(v*blockSize), blockSize, blockSize);

			Mat currBlock = textureReference(currRect);

			Mat currBlock_firstRow = currBlock.row(0);
			Mat currBlock_firstCol = currBlock.col(0);

			if (upperBlock_LastRow.cols > 0) // there is an upper neighbor
			{
				currCost = currCost + norm(upperBlock_LastRow - currBlock_firstRow);
			}

			if (leftBlock_LastCol.cols > 0) // there is a left neighbor
			{
				currCost = currCost + norm(leftBlock_LastCol - currBlock_firstCol);
			}

			if (currCost < minCost)
			{
				minCost = currCost;
				*chosenBlockV_ptr = v;
				*chosenBlockH_ptr = h;
			}

		}
	}
}

void findBlockClosestToAverage(Mat textureReference, int blockSize, int nBlocksVerticalRef, int nBlocksHorRef, double*chosenBlockV_ptr, double*chosenBlockH_ptr)
{
	double minDiff = INT_MAX;

	Scalar avgReference = mean(textureReference);

	for (double v = 0; v <= nBlocksVerticalRef - 1; v += 0.2)
	{
		for (double h = 0; h <= nBlocksHorRef - 1; h += 0.2)
		{
			Rect currRect(int(h*blockSize), int(v*blockSize), blockSize, blockSize);

			Mat currBlock = textureReference(currRect);
			
			Scalar avgCurrBlock = mean(currBlock);

			double currDiff = norm(avgReference - avgCurrBlock);

			if (currDiff < minDiff)
			{
				minDiff = currDiff;
				*chosenBlockV_ptr = v;
				*chosenBlockH_ptr = h;
			}
			
		}
	}
}

void findBestMatchingBlock(int v, int h, int blockSize, int nBlocksVerticalRef, int nBlocksHorRef,  Mat textureReference, Mat texture, double*chosenBlockV_ptr,double*chosenBlockH_ptr, double blockStep)
{	
	if (v == 0 && h == 0)
	{
		//*chosenBlockV_ptr = rand() % nBlocksVerticalRef;
		//*chosenBlockH_ptr = rand() % nBlocksHorRef;

		findBlockClosestToAverage(textureReference, blockSize, nBlocksVerticalRef, nBlocksHorRef, chosenBlockV_ptr, chosenBlockH_ptr);

		return;
	}

	Mat leftBlock_LastCol;
	Mat upperBlock_LastRow;

	if (h > 0)
	{
		Rect leftBlockRect((h - 1)*blockSize, v*blockSize, blockSize, blockSize);

		Mat leftBlock = texture(leftBlockRect);

		leftBlock_LastCol = leftBlock.col(blockSize - 1);		
	}

	if (v > 0)
	{
		Rect upperBlockRect(h*blockSize, (v-1)*blockSize, blockSize, blockSize);

		Mat upperBlock = texture(upperBlockRect);

		upperBlock_LastRow = upperBlock.row(blockSize - 1);
	}

	findBestMatchingNeighborBlock(v,h,blockSize, nBlocksVerticalRef, nBlocksHorRef, leftBlock_LastCol, upperBlock_LastRow, textureReference, chosenBlockV_ptr, chosenBlockH_ptr, blockStep);
}

Mat synthesizeTexture(Mat textureReference, int blockSize, int nRowsForehead, int nColsForehead, double blockStep)
{
	int nBlocksVertical = ceil((double) nRowsForehead / blockSize);  //rounded up so there are more blocks than the effective forehead region
	int nBlocksHorizontal = ceil((double) nColsForehead / blockSize);

	Mat texture(nBlocksVertical*blockSize, nBlocksHorizontal*blockSize, CV_8UC3);

	int nBlocksVerticalRef = textureReference.rows / blockSize; //rounded down so that blocks are always inside the reference texture
	int nBlocksHorRef = textureReference.cols / blockSize;
	
	for (int v = 0; v < nBlocksVertical; v++)
	{
		for (int h = 0; h < nBlocksHorizontal; h++)
		{
			//printf("v = %d, h = %d \n", v, h);
					
			Rect rectTexture(h*blockSize, v*blockSize, blockSize, blockSize);	
			Mat currBlock = texture(rectTexture);   //to be updated

			double chosenBlockV;
			double chosenBlockH;

			findBestMatchingBlock(v, h, blockSize, nBlocksVerticalRef, nBlocksHorRef, textureReference, texture, &chosenBlockV, &chosenBlockH, blockStep);

			Rect rectRef((int)(chosenBlockH*blockSize), (int)(chosenBlockV*blockSize), blockSize, blockSize);
			Mat refBlock = textureReference(rectRef);

			refBlock.copyTo(currBlock);  //copy chosen block from reference to block in synthesized texture
		}
	}

	
	//Mat gBlur;
	Mat medBlur;
	//Mat bilBlur;
	//GaussianBlur(texture, gBlur, Size(blurSize, blurSize), 0, 0);
	medianBlur(texture, medBlur, BLUR_SIZE);
	//bilateralFilter(texture, bilBlur, blurSize, blurSize * 2, blurSize / 2);

	/*imwrite("Blur_gaussian.png", gBlur);
	imwrite("Blur_median.png", medBlur);
	imwrite("Blur_bilateral.png", bilBlur);*/

	//cv:namedWindow("x", CV_WINDOW_AUTOSIZE);
	//cv::imshow("textureReference", textureReference);
	//cv::imshow("texture", texture);
	//cv::imshow("medBlur", medBlur);
	//cv::waitKey();

	return medBlur;
}

Mat synthesizeTextureRandom(Mat textureReference, int blockSize, int nRowsForehead, int nColsForehead)
{
	int nBlocksVertical = ceil((double)nRowsForehead / blockSize);  //rounded up so there are more blocks than the effective forehead region
	int nBlocksHorizontal = ceil((double)nColsForehead / blockSize);

	Mat texture(nBlocksVertical*blockSize, nBlocksHorizontal*blockSize, CV_8UC3);

	int nBlocksVerticalRef = textureReference.rows / blockSize; //rounded down so that blocks are always inside the reference texture
	int nBlocksHorRef = textureReference.cols / blockSize;

	for (int v = 0; v < nBlocksVertical; v++)
	{
		for (int h = 0; h < nBlocksHorizontal; h++)
		{
			//printf("v = %d, h = %d \n", v, h);

			Rect rectTexture(h*blockSize, v*blockSize, blockSize, blockSize);
			Mat currBlock = texture(rectTexture);   //to be updated

			double chosenBlockV;
			double chosenBlockH;

			chosenBlockV = rand() % nBlocksVerticalRef;
			chosenBlockH = rand() % nBlocksHorRef;

			Rect rectRef((int)chosenBlockH*blockSize, (int)chosenBlockV*blockSize, blockSize, blockSize);
			Mat refBlock = textureReference(rectRef);

			refBlock.copyTo(currBlock);  //copy random block from reference to block in synthesized texture
		}
	}

	return texture;
}

Vec3b findClosestSkinPixel(int i, int j, int maxNeighboordSize, Mat facePixels_lab, Mat facePixels, ColorEstimate cEst)
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

				uchar L = facePixels_lab.at <Vec3b>(i + ni, j + nj)[0];
				uchar a = facePixels_lab.at <Vec3b>(i + ni, j + nj)[1];
				uchar b = facePixels_lab.at <Vec3b>(i + ni, j + nj)[2];


				double stddev_L = cEst.getStdA()[0];
				double mu_L = cEst.getMeanA()[0];
				double stddev_a = cEst.getStdA()[1];
				double mu_a = cEst.getMeanA()[1];
				double stddev_b = cEst.getStdA()[2];
				double mu_b = cEst.getMeanA()[2];

				bool closeColor = abs(a - mu_a) < stddev_a && abs(b - mu_b) < stddev_b; //check if color is close to the mean of region A

				if (closeColor)
				{
					return facePixels.at<Vec3b>(i + ni, j + nj);
				}

			}
		
		}

		neighboorHoodSize++;
	}

	return Vec3b(0, 0, 0);
}

void updateReplaceMask(int i, int j, Mat replaceMask, Mat facePixels, Hair hair, ColorEstimate cEst, Face face)
{
	uchar b = facePixels.at<Vec3b>(i, j)[0];
	uchar g = facePixels.at<Vec3b>(i, j)[1];
	uchar r = facePixels.at<Vec3b>(i, j)[2];

	double stddev_b = hair.getHairStd()[0];
	double mu_b = hair.getHairMean()[0];
	double stddev_g = hair.getHairStd()[1];
	double mu_g = hair.getHairMean()[1];
	double stddev_r = hair.getHairStd()[2];
	double mu_r = hair.getHairMean()[2];

	double beta = 1;

	//bool closeColorToHair = abs(r - mu_r) < beta*stddev_r && abs(b - mu_b) < beta*stddev_b && abs(g - mu_g) < beta*stddev_g;
	//bool closeColorToHair = hairMaskNoMatting.at<uchar>(i, j) > 0;
	Vec3b skinColorVec = Vec3b((uchar)cvRound(cEst.getMeanA()[0]), (uchar)cvRound(cEst.getMeanA()[1]), (uchar)cvRound(cEst.getMeanA()[2]));
	Mat skinColorMat_Lab = Mat(1, 1, CV_8UC3, Scalar(skinColorVec[0], skinColorVec[1], skinColorVec[2]));
	Mat skinColorMat;
	cv::cvtColor(skinColorMat_Lab, skinColorMat, cv::COLOR_Lab2BGR);

	Scalar avgColorSkin = mean(skinColorMat);

	double distHair = sqrt(pow(b - mu_b, 2) + pow(g - mu_g, 2) + pow(r - mu_r, 2));
	double distSkin = sqrt(pow(b - avgColorSkin[0], 2) + pow(g - avgColorSkin[1], 2) + pow(r - avgColorSkin[2], 2));

	bool closeColorToHair = distHair < beta*distSkin;
	
	if (closeColorToHair)
	{
		replaceMask.at<uchar>(i, j) = 255;
	}

	//double beta = 3;

	//Mat facePixelMat = Mat(1, 1, CV_8UC3, Scalar(b, g, r));
	//Mat facePixel_Lab;
	//cv::cvtColor(facePixelMat, facePixel_Lab, cv::COLOR_BGR2Lab);
	//uchar L = facePixel_Lab.at<Vec3b>(0, 0)[0];
	//uchar a = facePixel_Lab.at<Vec3b>(0, 0)[1];
	//uchar b2 = facePixel_Lab.at<Vec3b>(0, 0)[2];

	//cv::cvtColor(facePixel_Lab, facePixelMat, cv::COLOR_Lab2BGR);
	//uchar bb = facePixelMat.at<Vec3b>(0, 0)[0];
	//uchar gg= facePixelMat.at<Vec3b>(0, 0)[1];
	//uchar rr = facePixelMat.at<Vec3b>(0, 0)[2];

	////bool closeColorSkin = abs(L - cEst.getMeanA()[0]) < beta*cEst.getStdA()[0] && abs(a - cEst.getMeanA()[1]) < beta*cEst.getStdA()[1] && abs(bb - cEst.getMeanA()[2]) < beta*cEst.getStdA()[2]
	//bool closeColorSkin = abs(a - cEst.getMeanA()[1]) < beta*cEst.getStdA()[1] && abs(b2 - cEst.getMeanA()[2]) < beta*cEst.getStdA()[2];
	//
	//if (!closeColorSkin)
	//{
	//	replaceMask.at<uchar>(i, j) = 255;
	//}
}

void obtainReplaceMask(Mat replaceMask, Mat facePixels, Mat faceMask, Hair hair, ColorEstimate cEst, Face face, int firstForeheadRow, int lastForeheadRow, int firstForeheadCol, int lastForeheadCol)
{	
	// replaceMask needs to be different from forehead mask, otherwise seamless cloning will not be able to capture the color from the original forehead

	int firstOffset = 30;
	int finalOffset = -5;
	int numberRowsRamp = 20;
	double step = double (firstOffset - finalOffset) / numberRowsRamp;	
	//step = 0;
	int lastRowRamp = face.getTopEye();
	int firstRowRamp = lastRowRamp - numberRowsRamp;	

	for (int i = firstForeheadRow;i < lastForeheadRow; i++)
	{
		for (int j = 0;j < faceMask.cols;j++)
		{						
			if (faceMask.at<uchar>(i, j) > 0)
			{
				if (i >= lastRowRamp)
				{
					if (j > face.getLeftEdgeEye() - (-finalOffset) && j < face.getRightEdgeEye() + (-finalOffset))   //don't update eye region and leave a little blank between side hair and the forehead mask for seamless cloning
					{
						continue;
					}
				}

				//if (i >= lastRowRamp)
				//{
				//	if (j > face.getLeftEdgeEye() && j < face.getRightEdgeEye())   //don't update eye region and leave a little blank between side hair and the forehead mask for seamless cloning
				//	{
				//		continue;
				//	}
				//}

				if (i >= firstRowRamp && i < lastRowRamp)
				{
					double currOffset = firstOffset - (i- firstRowRamp)*step;

					if (j > face.getLeftEdgeEye() + cvRound(currOffset) && j < face.getRightEdgeEye() - cvRound(currOffset)) //leave a blank in the part where seamless cloning will be used
					{
						continue;
					}
				}

				replaceMask.at<uchar>(i, j) = 255;
				//updateReplaceMask(i, j, replaceMask, facePixels, hair, cEst, face);				
				
			}
		}
	}
}

Mat findIndForhead(Mat replaceMask, Mat facePixels, Mat faceMask, Hair hair, int* firstForeheadRow, int* lastForeheadRow, int* firstForeheadCol, int* lastForeheadCol, ColorEstimate cEst, Face face)
{
	Mat hairMask = hair.getHairMask();
	Mat hairMaskNoMatting = hair.getHairMaskNoMatting();

	Mat foreheadIndFirstCol(faceMask.rows, 1, CV_32SC1, Scalar(INT_MAX)); //initialize matrix with first and last column of forehead for each row
	Mat foreheadIndLastCol(faceMask.rows, 1, CV_32SC1, Scalar(0));
	Mat foreheadInd;

	hconcat(foreheadIndFirstCol, foreheadIndLastCol, foreheadInd);

	*firstForeheadRow = INT_MAX;
    *lastForeheadRow  = 0;

	*firstForeheadCol = INT_MAX;
	*lastForeheadCol = 0;

	//search for first and last col of each row on the forehead
	for (int i = 0;i < faceMask.rows; i++)
	{

		for (int j = 0;j < faceMask.cols;j++)
		{	
		
			if (faceMask.at<uchar>(i, j) > 0 && hairMask.at<uchar>(i, j) > 0)
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

	int previousLastForeheadRow = *lastForeheadRow;

	//update all pixels in the forehead
	for (int i = 0;i < previousLastForeheadRow; i++)
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

int findbrightestColumnB(Mat B_L)
{	
	uchar brightestValue = 0;

	int brightestColInd = 0;

	for (int i = 0;i < B_L.rows; i++)
	{
		for (int j = 0;j < B_L.cols;j++)
		{
			if (B_L.at<uchar>(i, j) > brightestValue)
			{
				brightestValue = B_L.at<uchar>(i, j);
				brightestColInd = j;
			}
		}
	}

	return brightestColInd;
}





