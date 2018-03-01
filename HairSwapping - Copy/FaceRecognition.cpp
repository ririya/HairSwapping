#include <stdio.h>
#include <stdlib.h>
#include <list>

// OpenCV
#include <opencv2//core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include "opencv/highgui.h"
#include "stasm_lib.h"
#include "faceRecognition.h"

using namespace cv;

Face detectFace(Mat_<unsigned char> img, const char * path, const char* dataDir) {
	
	Mat faceMask;
	Mat skinMask;

	int upperPointX;
	int upperPointY;

	int leftEdgeX;
	int leftEdgeY;

	int rightEdgeX;
	int rightEdgeY;

	Rect RectA;
	Rect RectB;
	Rect RectC;

	int foundface;

	float landmarks[2 * stasm_NLANDMARKS]; // x,y coords (note the 2)

	if (!stasm_search_single(&foundface, landmarks,
		(const char*)img.data, img.cols, img.rows, path, dataDir))
	{
		printf("Error in stasm_search_single: %s\n", stasm_lasterr());
		exit(1);
	}

	if (!foundface)
		printf("No face found in %s\n", path);
	else
	{
		stasm_force_points_into_image(landmarks, img.cols, img.rows);		

		faceMask = detectUpperBoundaries(img, landmarks, &upperPointX, &upperPointY);

		skinMask = detectSkinMask(img, landmarks);

		leftEdgeX = landmarks[2 * LEFT_EDGE_IND];
		leftEdgeY = landmarks[2 * LEFT_EDGE_IND+1];

		rightEdgeX = landmarks[2 * RIGHT_EDGE_IND];
		rightEdgeY = landmarks[2 * RIGHT_EDGE_IND + 1];

		//find regions A,B,C using landmarks
		int x1A = landmarks[2 * LEFT_EDGE_OF_LEFT_EYE_IND];
		int x2A = landmarks[2 * LEFT_EDGE_OF_NOSE_IND];
		int top = min(landmarks[2 * BASE_OF_LEFT_EYE_IND + 1] + 5, landmarks[2 * BASE_OF_RIGHT_EYE_IND + 1] + 5);	
		int bottom = landmarks[2 * BASE_OF_NOSE_IND + 1];		
		int widthA = x2A - x1A;
		int height = bottom - top;
		RectA = Rect(x1A, top, widthA, height);
		rectangle(img, RectA, 255);
		//regionA = img(RectA);

		int x1B = landmarks[2 * LEFT_EDGE_OF_NOSE_IND];
		int x2B = landmarks[2 * RIGHT_EDGE_OF_NOSE_IND];		
		int widthB = x2B- x1B;
		RectB = Rect(x1B, top, widthB, height);
		rectangle(img, RectB, 255);
		//regionB = img(RectB);

		int x1C = landmarks[2 * RIGHT_EDGE_OF_NOSE_IND];
		int x2C = landmarks[2 * RIGHT_EDGE_OF_RIGHT_EYE_IND];
		int widthC = x2C - x1C;
		RectC = Rect(x1C, top, widthC, height);
		rectangle(img, RectC, 255);
		//regionC = img(RectC);

	/*	cv::imshow("regionA", regionA);
		cv::waitKey();
		cv::imshow("regionB", regionB);
		cv::waitKey();
		cv::imshow("regionC", regionC);
		cv::waitKey();*/		
	}

	int leftEyeEdgeX = landmarks[2 * LEFT_EDGE_OF_LEFT_EYE_IND] - 5;
	int leftEyeEdgeY = landmarks[2 * LEFT_EDGE_OF_LEFT_EYE_IND +1];
	int rightEyeEdgex = landmarks[2 * RIGHT_EDGE_OF_RIGHT_EYE_IND] + 5;
	int rightEyeEdgeY = landmarks[2 * RIGHT_EDGE_OF_RIGHT_EYE_IND +1];
	int topEyeY = min(landmarks[2 * TOP_LEFT_EYEBROW_IND+1], landmarks[2 * TOP_RIGHT_EYEBROW_IND + 1]) - 2;
	int topEyeX = landmarks[2 * TOP_LEFT_EYEBROW_IND];
	int bottomEyeY = max(landmarks[2 * BASE_OF_LEFT_EYE_IND + 1], landmarks[2 * BASE_OF_RIGHT_EYE_IND + 1]) + 5;
	int bottomEyeX = landmarks[2 * BASE_OF_LEFT_EYE_IND + 1];
	int bottomNoseY = landmarks[2 * BASE_OF_NOSE_IND + 1];
	int bottomNoseX = landmarks[2 * BASE_OF_NOSE_IND];
	int topOfMouthIndY = landmarks[2 * TOP_OF_MOUTH_IND + 1];
	int topOfMouthIndX = landmarks[2 * TOP_OF_MOUTH_IND];
	int hairTypicalBottom = (bottomNoseY + topOfMouthIndY) / 2;

	//img(cvRound(leftEyeEdgeY), cvRound(leftEyeEdgeX)) = 255;
	//img(cvRound(rightEyeEdgeY), cvRound(rightEyeEdgex)) = 255;
	//img(cvRound(topEyeY), cvRound(topEyeX)) = 255;
	//img(cvRound(bottomEyeY), cvRound(bottomEyeX)) = 255;
	//img(cvRound(bottomNoseY), cvRound(bottomNoseX)) = 255;
	//img(cvRound(topOfMouthIndY), cvRound(topOfMouthIndX)) = 255;
	//

	/*img(cvRound(leftEdgeY), cvRound(leftEdgeX)) = 255;
	img(cvRound(rightEdgeY), cvRound(rightEdgeX)) = 255;

	cv::imshow("face", img);
	cv::waitKey();*/

	Face face(faceMask, skinMask, leftEdgeX, rightEdgeX, upperPointX, upperPointY, leftEyeEdgeX, rightEyeEdgex, bottomEyeY, topEyeY, hairTypicalBottom, RectA, RectB, RectC);
	return face;

}

Mat  detectSkinMask(Mat img, float landmarks[])
{
	Mat skinMask(img.rows, img.cols, CV_8UC1, Scalar(0));

	cv::line(skinMask, Point(landmarks[0], landmarks[1]), Point(landmarks[2* (IND_LOWER_BOUND_MIDDLE*2)], landmarks[2 *(IND_LOWER_BOUND_MIDDLE * 2)+1]), Scalar(255, 255, 255), 1, 8); // segment from M to I
	
	int nSkinPoints = sizeof(noseIndSequence) / sizeof(int);

	for (int i = 0; i < IND_LOWER_BOUND_MIDDLE * 2; i++)
	{
		int point1_X = landmarks[2 * i];
		int point1_Y = landmarks[2 * i + 1];

		int point2_X = landmarks[2 * i + 2];
		int point2_Y = landmarks[2 * i + 3];

		cv::line(skinMask, Point(point1_X, point1_Y), Point(point2_X, point2_Y), Scalar(255, 255, 255), 1, 8); // segment from M to I
	}
	

	Mat im_floodfill = skinMask.clone();
	floodFill(im_floodfill, cv::Point(0, 0), Scalar(255));

	// Invert floodfilled image
	bitwise_not(im_floodfill, skinMask);

	return skinMask;
}


Mat detectUpperBoundaries(Mat_<unsigned char> img, float landmarks[], int *upperPoint_X, int *upperPoint_Y)
{
	Mat faceMask(img.rows, img.cols, CV_8UC1, Scalar(0));

	int lowerBoundMiddleX = landmarks[IND_LOWER_BOUND_MIDDLE * 2];
	int lowerBoundMiddleY = landmarks[IND_LOWER_BOUND_MIDDLE * 2 + 1];
	
	std::map<int, int> mapKeyPoints;

	int baseNoseX = landmarks[BASE_OF_NOSE_IND * 2];
	int baseNoseY = landmarks[BASE_OF_NOSE_IND * 2 + 1];

	int J_X = lowerBoundMiddleX;
	int J_Y = lowerBoundMiddleY + 3 * (baseNoseY - lowerBoundMiddleY);  // face is divided into 3 parts

	*upperPoint_X = J_X;
	*upperPoint_Y = J_Y;

	int M_X = landmarks[LEFT_EDGE_IND * 2];
	int M_Y = landmarks[LEFT_EDGE_IND * 2+1];

	int N_X = landmarks[RIGHT_EDGE_IND * 2];
	int N_Y = landmarks[RIGHT_EDGE_IND * 2 + 1];

	int I_X = M_X + (N_X - M_X) / 4;
	int I_Y = J_Y + 3;

	int K_X = M_X + 3 * (N_X - M_X) / 4;
	int K_Y = J_Y + 3;

	img(cvRound(lowerBoundMiddleY), cvRound(lowerBoundMiddleX)) = 255;
	img(cvRound(landmarks[BASE_OF_NOSE_IND * 2 + 1]), cvRound(landmarks[BASE_OF_NOSE_IND * 2])) = 255;
	img(cvRound(M_Y), cvRound(M_X)) = 255;
	img(cvRound(N_Y), cvRound(N_X)) = 255;
	img(cvRound(J_Y), cvRound(J_X)) = 255;
	img(cvRound(I_Y), cvRound(I_X)) = 255;
	img(cvRound(K_Y), cvRound(K_X)) = 255;
	
	//draw boundaries for upper face
	cv::ellipse(faceMask, Point(I_X, M_Y), Size(I_X - M_X, M_Y - I_Y), 0, 180, 270 , Scalar(255,255,255), 1, 8, 0); // segment from M to I
	cv::ellipse(faceMask, Point(J_X, I_Y), Size(J_X - I_X, I_Y - J_Y), 0, 180, 270, Scalar(255, 255, 255), 1, 8, 0); //segment from I to J
	cv::ellipse(faceMask, Point(J_X, K_Y), Size(K_X - J_X, K_Y - J_Y), 0, 270, 360, Scalar(255, 255, 255), 1, 8, 0); //segment from J to K
	cv::ellipse(faceMask, Point(K_X, N_Y), Size(N_X - K_X, N_Y - K_Y), 0, 270, 360, Scalar(255, 255, 255), 1, 8, 0); //segment from K to N
	
	//draw boundaries for lower face																												 																										 //draw boundaries for upper face//draw boundaries for upper face
	for (int i = 0; i < IND_LOWER_BOUND_MIDDLE * 2; i++)
	{
		int point1_X = landmarks[2*i];
		int point1_Y = landmarks[2 *i +1];

		int point2_X = landmarks[2 * i + 2];
		int point2_Y = landmarks[2 * i + 3];

		cv::line(faceMask, Point(point1_X, point1_Y), Point(point2_X, point2_Y), Scalar(255, 255, 255), 1, 8);
	}

	Mat im_floodfill = faceMask.clone();
	floodFill(im_floodfill, cv::Point(0, 0), Scalar(255));

	// Invert floodfilled image to get the faceMask
	bitwise_not(im_floodfill, faceMask);

	return faceMask;
}




