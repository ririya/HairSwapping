#include <stdio.h>
#include <stdlib.h>
#include <list>

// OpenCV
#include <opencv2//core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include "opencv/highgui.h"
#include "stasm_lib.h"
#include "HairExtraction.h"
#include "faceRecognition.h"
#include "globalMatting.h"
#include "guidedfilter.h"

using namespace std;
using namespace cv;

int extractHair(Mat img, Face face, Mat *labels, Hair *hair)
{
	Mat centers(N_CENTERS, 3, CV_8UC1);
    
	getBackgroundCenter(img, centers);

	getClothesCenter(img, centers);

	getSkinCenter(img, face.getSkinMask(), centers);

	getHairCenter(img, face.getUpperPointX(),face.getUpperPointY(), centers);

	//viewCenters(centers);

	Mat pixelSequence = prepareImageForKmeans(img);

	Mat labelsSequence = segmentImage(pixelSequence, centers, img.rows, img.cols);

	*labels = getLabelsImage(labelsSequence, img.rows, img.cols);
	
	Mat hairImageMask = findHairPixels(pixelSequence, labelsSequence, img.rows, img.cols, face.getUpperPointX(), face.getUpperPointY());
	
	Scalar checkSum = sum(hairImageMask);

	if (checkSum[0] == 0 && checkSum[1] == 0 && checkSum[2] == 0)
	{
		return -1;
	}

	//Mat hairPixels1(img.rows, img.cols, CV_8UC3, Scalar(255, 255, 255));

	//Mat hairPixels2 = img.clone();

	//for (int i = 0; i < img.rows; i++)
	//{
	//	for (int j = 0; j < img.cols; j++)
	//	{
	//		if (hairImageMask.at<uchar>(i,j) == 0)
	//		
	//		{
	//			hairPixels2.at<Vec3b>(i, j)[0] = 0;
	//			hairPixels2.at<Vec3b>(i, j)[1] = 255;
	//			hairPixels2.at<Vec3b>(i, j)[2] = 0;
	//		}
	//	}
	//}

	//	namedWindow("hairPixels2", CV_WINDOW_FREERATIO);
	//cv::imshow("hairPixels2", hairPixels2);
	//cv::waitKey();

	//for (int i = 0; i < img.rows; i++)
	//{
	//	for (int j = 0; j < img.cols; j++)
	//	{
	//		if (hairImageMask.at<uchar>(i, j) == 0)
	//			
	//			{
	//				hairPixels2.at<Vec3b>(i, j)[0] = img.at<Vec3b>(i, j)[0];
	//				hairPixels2.at<Vec3b>(i, j)[1] = img.at<Vec3b>(i, j)[1];
	//				hairPixels2.at<Vec3b>(i, j)[2] = img.at<Vec3b>(i, j)[2];
	//			}
	//	}
	//}

	//img.copyTo(hairPixels1, hairImageMask);

	//namedWindow("hairPixels2", CV_WINDOW_FREERATIO);
	//cv::imshow("hairPixels2", hairPixels2);
	//cv::waitKey();

	/*namedWindow("hairPixels1", CV_WINDOW_FREERATIO);
	cv::imshow("hairPixels1", hairPixels1);*/
	//cv::waitKey();

	Mat hairPixels;

	if (USE_MATTING)
	{
		hairPixels = performMatting(&hairImageMask, img);
	}

	/*Mat hairPixels(img.rows, img.cols, CV_8UC3, Scalar(255, 255, 255));
	img.copyTo(hairPixels, hairImageMask);*/
	

	cv::imwrite("hair.png", hairPixels);

	/*cv::imshow("hairPixelsAfterMatting", hairPixels);
	cv::waitKey();*/

	*hair = findConnectionPoint(hairImageMask, hairPixels, face);

	//int upperPointX = face.getUpperPointX();
	//int upperPointY = face.getUpperPointY();

	//img.at<Vec3b>(upperPointY, upperPointX)[0] = 0;
	//img.at<Vec3b>(upperPointY, upperPointX)[1] = 255;
	//img.at<Vec3b>(upperPointY, upperPointX)[2] = 0;

	//int connX = hair.getHairConnectionPointLocationX();
	//int connY = hair.getHairConnectionPointLocationY();

	//int ConnDistX = hair.getHairConnectionPointDistanceToJ_X();
	//int ConnDistY = hair.getHairConnectionPointDistanceToJ_Y();

	/*img.at<Vec3b>(connY, connX)[0] = 0;
	img.at<Vec3b>(connY, connX)[1] = 0;
	img.at<Vec3b>(connY, connX)[2] = 255;

	namedWindow("img", WINDOW_AUTOSIZE);
	imshow("img", img);	
	waitKey();*/

	return 0;

}

void FindBlobs(Mat &binary, vector < vector<Point2i> > &blobs)
{
	blobs.clear();

	// Fill the label_image with blobs
	// 0  - background
	// 1  - unlabelled foreground
	// 2+ - labelled foreground

	Mat label_image;
	binary.convertTo(label_image, CV_32SC1);

	int label_count = 2; // starts at 2 because 0,1 are used already

	for (int y = 0; y < label_image.rows; y++) {
		int *row = (int*)label_image.ptr(y);
		for (int x = 0; x < label_image.cols; x++) {
			if (row[x] != 1) {
				continue;
			}

			Rect rect;
			floodFill(label_image, Point(x, y), label_count, &rect, 0, 0, 4);

			vector <Point2i> blob;

			for (int i = rect.y; i < (rect.y + rect.height); i++) {
				int *row2 = (int*)label_image.ptr(i);
				for (int j = rect.x; j < (rect.x + rect.width); j++) {
					if (row2[j] != label_count) {
						continue;
					}

					blob.push_back(Point2i(j, i));
				}
			}

			blobs.push_back(blob);

			label_count++;
		}
	}
}

Mat findHairPixels(Mat pixelSequence, Mat labels, int nRows, int nCols, int upperPointX, int upperPointY)
{
	Mat hairSequenceMask(pixelSequence.rows, pixelSequence.cols, CV_8UC1);

	hairSequenceMask = (labels == HAIR_CENTER_INDEX);
	
	Mat hairSequencePixels = pixelSequence.clone();
	
	Mat hairImageMaskInitial = reconstructImage1D(hairSequenceMask, nRows, nCols);

	/*cv::imshow("hairImageMask", hairImageMaskInitial);
	cv::waitKey();*/
	
	threshold(hairImageMaskInitial, hairImageMaskInitial, 0.0, 1.0, cv::THRESH_BINARY);

	vector<vector<Point2i>> blobs;

	FindBlobs(hairImageMaskInitial, blobs);
		
	int hairBlobInd  = findHairBlob(blobs, upperPointX, upperPointY);

	Mat hairImageMask(hairImageMaskInitial.rows, hairImageMaskInitial.cols, CV_8UC1, Scalar(0));


	if (hairBlobInd != -1)
	{
		for (size_t j = 0; j < blobs[hairBlobInd].size(); j++)
		{
			int x = blobs[hairBlobInd][j].x;
			int y = blobs[hairBlobInd][j].y;

			hairImageMask.at<uchar>(y, x) = 1;
		}
	}
/*
	cv::imshow("hairImageMaskErosion", hairImageMaskWhite);
	cv::waitKey();

	cv::imshow("hairImageMaskDilation", hairImageMaskGray);
	cv::waitKey();

	cv::imshow("hairImageMaskSubtraction", hairImageMaskSubtraction);
	cv::waitKey();*/


	//cv::imshow("hairImageMask2", hairImageMask2);
	//cv::waitKey();

	return hairImageMask;
}

Hair findConnectionPoint(Mat hairMask, Mat hairPixels, Face face)
{

	int hairConnectionPointLocationX;
	int hairConnectionPointLocationY;

	int hairConnectionPointDistanceToJ_X;
	int hairConnectionPointDistanceToJ_Y;

	double minDist = 10000;	

	for (int i = 0; i < hairMask.rows; i++)
	{
		for (int j = 0;j < hairMask.cols;j++)
		{
		
			if (hairMask.at<uchar>(i, j) > 0)  //it's a pixel in the hair
			{	
				double currDist = sqrt(pow(j - face.getUpperPointX(),2) + pow(i - face.getUpperPointY(),2));
				
				if (currDist < minDist)
				{
					minDist = currDist;
					hairConnectionPointLocationX = j;
					hairConnectionPointLocationY = i;

					hairConnectionPointDistanceToJ_X = hairConnectionPointLocationX - face.getUpperPointX();
					hairConnectionPointDistanceToJ_Y = hairConnectionPointLocationY - face.getUpperPointY();

				}
			}
		}
	}

	/*cv::namedWindow("hairPixels", CV_WINDOW_AUTOSIZE);
	cv::imshow("hairPixels", hairPixels);
	cv::waitKey();*/

	return Hair(hairMask, hairPixels, hairConnectionPointLocationX, hairConnectionPointLocationY, hairConnectionPointDistanceToJ_X, hairConnectionPointDistanceToJ_Y);
}

Mat performMatting(Mat *hairImageMask, Mat Image)
{
	//Refer to the paper 'Shared Sampling for Real-Time Alpha Matting'

	Mat element = getStructuringElement(MORPH_RECT, Size(EROSION_SIZE, EROSION_SIZE));

	Mat hairImageMaskWhite;
	Mat hairImageMaskGray;

	erode(*hairImageMask, hairImageMaskWhite, element);
	dilate(*hairImageMask, hairImageMaskGray, element);

	Mat hairImageMaskSubtraction = hairImageMaskGray - hairImageMaskWhite;

	Mat trimap;

	trimap = hairImageMaskWhite.clone() * 255;
	hairImageMaskSubtraction = hairImageMaskSubtraction * 128;

	trimap = trimap + hairImageMaskSubtraction;

	//cv::imshow("trimap", trimap);
	//cv::waitKey();


	cv::imwrite("trimap.png", trimap);

	expansionOfKnownRegions(Image, trimap, 9);

	cv::Mat foreground, alpha;
	globalMatting(Image, trimap, foreground, alpha);

	// filter the result with fast guided filter
	alpha = guidedFilter(Image, alpha, 10, 1e-5);
	for (int x = 0; x < trimap.cols; ++x)
		for (int y = 0; y < trimap.rows; ++y)
		{
			if (trimap.at<uchar>(y, x) == 0)
				alpha.at<uchar>(y, x) = 0;
			else if (trimap.at<uchar>(y, x) == 255)
				alpha.at<uchar>(y, x) = 255;
		}

	//cv::imwrite("foreground.png", foreground);
	cv::imwrite("alpha.png", alpha);

	cv::imwrite("image.png", Image);

	threshold(alpha, *hairImageMask, 0.0, 1.0, cv::THRESH_BINARY);

	std::vector<cv::Mat> matChannels;
	cv::split(Image, matChannels);

	matChannels.push_back(alpha);

	Mat alphaImage;

	cv::merge(matChannels, alphaImage);

	cv::imwrite("alphaImage.png", alphaImage);

	return alphaImage;
}

int findHairBlob(vector<vector<Point2i>> blobs, int upperPointX, int upperPointY)
{
	for (size_t i = 0; i < blobs.size(); i++) {

		for (size_t j = 0; j < blobs[i].size(); j++) {

			int x = blobs[i][j].x;
			int y = blobs[i][j].y;

			if ( abs(x - upperPointX) < 3 && abs(y - (upperPointY - OFFSET_HAIR)) < 3) // this blob intersects with the upper point
			{
				return i;				
			}

		}
	}

	return -1;
}

Mat segmentImage(Mat pixelSequence, Mat centers, int nRows, int nCols)
{

	Mat labels = performKmeans(pixelSequence, centers, 10, nRows, nCols);	

	return labels;
}

Mat performKmeans(Mat pixelSequence, Mat centers, int maxIterations, int nRows, int nCols)
{
	Mat labels(pixelSequence.rows, 1, CV_8UC1);

	for (int it = 0; it < maxIterations;it++)
	{
		int numberPointsInCluster[N_CENTERS];

		for (int c = 0; c < N_CENTERS; c++)
		{
			numberPointsInCluster[c] = 0;
		}

		Mat	currItCenters(N_CENTERS, 3, CV_64FC1, Scalar(0));
		Mat currItCentersUchar;

		for (int j = 0; j < pixelSequence.rows; j++)
		{
			Mat pixel = pixelSequence.row(j);

			uchar b = pixel.at<uchar>(0, 0);
			uchar g = pixel.at<uchar>(0, 1);
			uchar r = pixel.at<uchar>(0, 2);

			double minDist = 10000;  //initialize with large number

			double bestB;
			double bestG;
			double bestR;

			
			for (int c = 0; c < N_CENTERS;c++)  // look for center to assign this pixel, based on distance
			{
				Mat center = centers.row(c);

				uchar bc = center.at<uchar>(0, 0);
				uchar gc = center.at<uchar>(0, 1);
				uchar rc = center.at<uchar>(0, 2);

				//double dist = norm(pixel - center, NORM_L2SQR); 

				double dist = sqrt(pow(bc - b, 2) + pow(gc - g, 2) + pow(rc - r, 2));

				if (dist < minDist)
				{
					minDist = dist;
					labels.at<uchar>(j, 0) = (uchar)c;
					bestB = b;
					bestG = g;
					bestR = r;
				}
			}

			uchar chosenCluser = labels.at<uchar>(j, 0);

			currItCenters.at<double>(chosenCluser, 0) += bestB;
			currItCenters.at<double>(chosenCluser, 1) += bestG;
			currItCenters.at<double>(chosenCluser, 2) += bestR;

			numberPointsInCluster[chosenCluser]++;

		}

		int numberOfTotalPoints = numberPointsInCluster[0] + numberPointsInCluster[1] + numberPointsInCluster[2] + numberPointsInCluster[3];

	/*	getLabelsImage(labels, nRows, nCols);

		viewCenters(centers);*/

		bool isDifferentFromLastIteration = false;

		for (int c = 0; c < N_CENTERS; c++)
		{

			if (numberPointsInCluster[c] > 0)
			{
				currItCenters.at<double>(c, 0) /= numberPointsInCluster[c];
				currItCenters.at<double>(c, 1) /= numberPointsInCluster[c];
				currItCenters.at<double>(c, 2) /= numberPointsInCluster[c];
			}

			
			currItCenters.convertTo(currItCentersUchar, CV_8UC1);

			//since values are integers, early stopping criterion can be achived with hard equality

			uchar currItCentersB = currItCentersUchar.at<uchar>(c, 0);
			uchar currItCentersG = currItCentersUchar.at<uchar>(c, 1);
			uchar currItCentersR = currItCentersUchar.at<uchar>(c, 2);

			uchar centersB = centers.at<uchar>(c, 0);
			uchar centersG = centers.at<uchar>(c, 1);
			uchar centersR = centers.at<uchar>(c, 2);

			if (currItCentersB != centersB || currItCentersG != centersG || currItCentersR != centersR)
			{
				isDifferentFromLastIteration = true;
			}
		}

		if (!isDifferentFromLastIteration) 
		{
			return labels;
		}

		centers = currItCentersUchar;
	}

	return labels;
}

Mat getLabelsImage(Mat labels, int nRows, int nCols)
{
	
	Mat labelsImage(nRows, nCols, CV_8UC3, Scalar(0, 0, 0));
	
	for (int i = 0; i < nRows; i++)
	{
		for (int j = 0; j < nCols; j++)
		{
			int labelInd = i*nCols + j;
			int currLabel = labels.at<uchar>(labelInd, 0);
			
			if (currLabel == BACKGROUND_CENTER_INDEX)
			{
				labelsImage.at<Vec3b>(i, j)[0] = 255; // background is blue
			}
			if (currLabel == HAIR_CENTER_INDEX)
			{
				labelsImage.at<Vec3b>(i, j)[1] = 255; //hair is green
			}
			if (currLabel == SKIN_CENTER_INDEX)    
			{
				labelsImage.at<Vec3b>(i, j)[2] = 255; // skin is red
			}

			// clothes are black
			
		}
	}


	//cv::imshow("labelsImage", labelsImage);
	//cv::waitKey();

	return labelsImage;
}



void viewCenters(Mat centers)
{

	vector<Mat> centersMat;

	string centerNames[4] = { "background", "hair", "skin","clothes"};

	for (int c = 0; c < N_CENTERS; c++)
	{
		uchar b = centers.at<uchar>(c, 0);
		uchar g = centers.at<uchar>(c, 1);
		uchar r = centers.at<uchar>(c, 2);

		Mat currCenter(100, 100, CV_8UC3, Scalar(b,g,r));

		centersMat.push_back(currCenter);

		namedWindow(centerNames[c]);
		cv::imshow(centerNames[c], currCenter);

	}

	cv::waitKey();

}

Mat reconstructImage1D(Mat pixelSequence, int nRows, int nCols)
{
	Mat img(nRows, nCols, CV_8UC1, Scalar(0));

	for (int i = 0; i < nRows; i++)
	{
		for (int j = 0; j < nCols; j++)
		{
			int pixelInd = i*nCols + j;

			img.at<uchar>(i, j) = pixelSequence.at<uchar>(pixelInd, 0);
		}
	}

	//cv::imshow("reconstructedImage", img);
	//cv::waitKey();

	return img;
}

Mat reconstructImage3D(Mat pixelSequence, int nRows, int nCols)
{
	Mat img(nRows, nCols, CV_8UC3, Scalar(0, 0, 0));

	for (int i = 0; i < nRows; i++)
	{
		for (int j = 0; j < nCols; j++)
		{
			int pixelInd = i*nCols + j;

			img.at<Vec3b>(i, j)[0] = pixelSequence.at<uchar>(pixelInd, 0);
			img.at<Vec3b>(i, j)[1] = pixelSequence.at<uchar>(pixelInd, 1);
			img.at<Vec3b>(i, j)[2] = pixelSequence.at<uchar>(pixelInd, 2);

		}
	}

	cv::imshow("reconstructedImage", img);
	cv::waitKey();

	return img;
}

Mat prepareImageForKmeans(Mat img)
{
	// turn image into 3xnPixels sequence for K-Means
	int nPixels = img.cols*img.rows;

	Mat pixelSequence(nPixels, 3 , CV_8UC1);

	uint8_t* pixelPtr = (uint8_t*)img.data;

	int cn = img.channels();

	for (int i = 0; i < img.rows; i++)
	{
		for (int j = 0; j < img.cols; j++)
		{

			int pixelInd = i*img.cols + j;

			pixelSequence.at<uchar>(pixelInd, 0) = pixelPtr[i*img.cols*cn + j*cn + 0];
			pixelSequence.at<uchar>(pixelInd, 1) = pixelPtr[i*img.cols*cn + j*cn + 1];
			pixelSequence.at<uchar>(pixelInd, 2) = pixelPtr[i*img.cols*cn + j*cn + 2];
		}
	}


	return pixelSequence;

}


void getHairCenter(Mat img, int upperPointX, int upperPointY, Mat centers)
{

	int topPoint = max(upperPointY - OFFSET_HAIR, 0);
	int height = upperPointY - topPoint;

	/*Rect roi(upperPointX, topPoint, 1, height);

	Mat roiMat(img, roi);

	Scalar avgIntensity = mean(roiMat);

	uchar b = avgIntensity[0];
	uchar g = avgIntensity[1];
	uchar r = avgIntensity[2];*/

	Vec3b intensity = img.at<Vec3b>(topPoint, upperPointX);
	uchar b = intensity.val[0];
	uchar g = intensity.val[1];
	uchar r = intensity.val[2];

	//img.at<Vec3b>(topPoint, upperPointX)[2] = 255;

	/*cv::imshow("img", img);
	cv::waitKey();*/


	centers.at<uchar>(HAIR_CENTER_INDEX, 0) = b;
	centers.at<uchar>(HAIR_CENTER_INDEX, 1) = g;
	centers.at<uchar>(HAIR_CENTER_INDEX, 2) = r;
}


void getSkinCenter(Mat img, Mat skinMask, Mat centers)
{	
	uchar currB, currG, currR;

	//double bAvg = 0;
	//double gAvg = 0;
	//double rAvg = 0;

	uint8_t* pixelPtr = (uint8_t*)img.data;
	int cn = img.channels();

	vector<uchar> r;
	vector<uchar> b;
	vector<uchar> g;

	uint8_t* pixelPtrSkinMask = (uint8_t*)skinMask.data;

	int numPixels = 0;

 for(int i = 0; i < img.rows; i++)
	for (int j = 0; j < img.cols; j++)
	{
		uchar skinMaskPixelValue = pixelPtrSkinMask[i*skinMask.cols + j];

		if (skinMaskPixelValue > 0)
		{
			currB = pixelPtr[i*img.cols*cn + j*cn + 0]; // B
			currG = pixelPtr[i*img.cols*cn + j*cn + 1]; // G
			currR = pixelPtr[i*img.cols*cn + j*cn + 2]; // R

			b.push_back(currB);
			g.push_back(currG);
			r.push_back(currR);

			numPixels++;

		/*	bAvg = bAvg + (double)currB;
			gAvg = gAvg + (double)currG;
			rAvg = rAvg + (double)currR;*/
			
		}
	}

	sort(b.begin(), b.end());
	sort(g.begin(), g.end());
	sort(r.begin(), r.end());
	
	//get medians as centers
	uchar medianB = b[cvRound(b.size() / 2)];
	uchar medianG = g[cvRound(g.size() / 2)];
	uchar medianR = r[cvRound(r.size() / 2)];

	/*centers.at<uchar>(0, 1) = );
	centers.at<uchar>(1, 1) = cvRound(g.size()/2);
	centers.at<uchar>(2, 1) = cvRound(r.size()/2); */

	/*bAvg = bAvg / numPixels;
	gAvg = gAvg / numPixels;
	rAvg = rAvg / numPixels;*/

	centers.at<uchar>(SKIN_CENTER_INDEX, 0) = cvRound(medianB);
	centers.at<uchar>(SKIN_CENTER_INDEX, 1) = cvRound(medianG);
	centers.at<uchar>(SKIN_CENTER_INDEX, 2) = cvRound(medianR);
}

void getBackgroundCenter(Mat img, Mat centers)
{
	double b = 0, r = 0, g = 0;
	double currB, currG, currR;

	uint8_t* pixelPtr = (uint8_t*)img.data;
	int cn = img.channels();

	int i = 0;

	Mat topRow(img, Rect(0, 0, img.cols, 1));

	Scalar avg = mean(topRow);

	centers.at<uchar>(BACKGROUND_CENTER_INDEX, 0) = avg[0];
	centers.at<uchar>(BACKGROUND_CENTER_INDEX, 1) = avg[1];
	centers.at<uchar>(BACKGROUND_CENTER_INDEX, 2) = avg[2];

}

void getClothesCenter(Mat img, Mat centers)
{
	double currB, currG, currR;

	int i = img.rows-1; // clothes center is obtained from bottom row

	Mat bottomRow(img, Rect(0, i, img.cols, 1));
	
	Scalar avg = mean(bottomRow);

	centers.at<uchar>(CLOTHES_CENTER_INDEX, 0) = avg[0];
	centers.at<uchar>(CLOTHES_CENTER_INDEX, 1) = avg[1];
	centers.at<uchar>(CLOTHES_CENTER_INDEX, 2) = avg[2];
	
}




