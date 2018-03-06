#include "ColorEstimate.h"

// OpenCV
#include <opencv2//core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include "HairExtraction.h"
#include "faceRecognition.h"
#include "skinSynthesis.h"


//void plotHistogram(Mat src)
//{
//	vector<Mat> planes;
//	split(src, planes);
//
//	/// Establish the number of bins
//	int histSize = 256;
//
//	/// Set the ranges ( for B,G,R) )
//	float range[] = { 0, 256 };
//	const float* histRange = { range };
//
//	bool uniform = true; bool accumulate = false;
//
//	Mat L_hist, a_hist, b_hist;
//
//	/// Compute the histograms:
//	calcHist(&planes[0], 1, 0, Mat(), L_hist, 1, &histSize, &histRange, uniform, accumulate);
//	calcHist(&planes[1], 1, 0, Mat(), a_hist, 1, &histSize, &histRange, uniform, accumulate);
//	calcHist(&planes[2], 1, 0, Mat(), b_hist, 1, &histSize, &histRange, uniform, accumulate);
//
//	// Draw the histograms for B, G and R
//	int hist_w = 512; int hist_h = 400;
//	int bin_w = cvRound((double)hist_w / histSize);
//
//	Mat histImage(hist_h, hist_w, CV_8UC3, Scalar(0, 0, 0));
//
//	/// Normalize the result to [ 0, histImage.rows ]
//	normalize(L_hist, L_hist, 0, histImage.rows, NORM_MINMAX, -1, Mat());
//	normalize(a_hist, a_hist, 0, histImage.rows, NORM_MINMAX, -1, Mat());
//	normalize(b_hist, b_hist, 0, histImage.rows, NORM_MINMAX, -1, Mat());
//
//	/// Draw for each channel
//	for (int i = 1; i < histSize; i++)
//	{
//		line(histImage, Point(bin_w*(i - 1), hist_h - cvRound(L_hist.at<float>(i - 1))),
//			Point(bin_w*(i), hist_h - cvRound(L_hist.at<float>(i))),
//			Scalar(255, 0, 0), 2, 8, 0);
//		line(histImage, Point(bin_w*(i - 1), hist_h - cvRound(a_hist.at<float>(i - 1))),
//			Point(bin_w*(i), hist_h - cvRound(a_hist.at<float>(i))),
//			Scalar(0, 255, 0), 2, 8, 0);
//		line(histImage, Point(bin_w*(i - 1), hist_h - cvRound(b_hist.at<float>(i - 1))),
//			Point(bin_w*(i), hist_h - cvRound(b_hist.at<float>(i))),
//			Scalar(0, 0, 255), 2, 8, 0);
//	}
//
//	/// Display
//	namedWindow("calcHist", CV_WINDOW_AUTOSIZE);
//	imshow("calcHist", histImage);
//	waitKey();
//}

void ColorEstimate::estimateColors(Mat A, Mat B, Mat C)
{	
	meanStdDev(A, meanA, stddevA);
	meanStdDev(C, meanC, stddevC);
	meanStdDev(B, meanB, stddevB);

	//Scalar A_mean = mean(A_Lab);
	//Scalar C_mean = mean(C_Lab);

	uchar A_a = meanA[1];
	uchar A_b = meanA[2];
	uchar C_a = meanC[1];
	uchar C_b = meanC[2];

	int sumA = (int)A_a + (int)C_a;
	int sumB = (int)A_b + (int)C_b;

	uchar avg_a = (uchar)(sumA / 2);
	uchar avg_b = (uchar)(sumB / 2);

	avgSkinColor = Scalar(avg_a, avg_b);
	
}

ColorEstimate::ColorEstimate(Mat A, Mat B, Mat C)
{
	estimateColors(A, B, C);
}

Scalar ColorEstimate::getMeanA()
{
	return meanA;
}

Scalar ColorEstimate::getMeanB()
{
	return meanB;
}

Scalar ColorEstimate::getMeanC()
{
	return meanC;
}


Scalar ColorEstimate::getStdA()
{
	return stddevA;
}

Scalar ColorEstimate::getStdC()
{
	return stddevC;
}

Scalar ColorEstimate::getStdB()
{
	return stddevB;
}

Scalar ColorEstimate::getAvgSkinColor()
{
	return avgSkinColor;
}

