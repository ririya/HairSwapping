// minimal.cpp: Display the landmarks of a face in an image.
//              This demonstrates stasm_search_single.



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
#include "FaceRecognition.h"
#include "HairExtraction.h"
#include "SkinSynthesis.h"
#include "HairEditing.h"

using namespace cv;

int main()
{
	//string dataDir = "data/";
	//string faceDir = "faces/";
	//string modelsDir = "hairModels/";
	//string segmentationDir = "segmentation/";
	//string resultsDir = "results/";

	//for (int fileInd = 1; fileInd <= 18; fileInd++)
	//{
	//	string pathStringModel = dataDir + std::to_string(fileInd) + ".png";

	//	const char * pathModel = pathStringModel.c_str();
	//	Mat_<unsigned char> imgGrayModel(imread(pathModel, CV_LOAD_IMAGE_GRAYSCALE));
	//	Mat imgRGBModel(imread(pathModel, CV_LOAD_IMAGE_COLOR));
	//	if (!imgGrayModel.data)
	//	{
	//		printf("Cannot load %s\n", imgGrayModel);
	//		continue;
	//		//exit(1);
	//	}

	//	Face faceModel = detectFace(imgGrayModel, pathModel, dataDir.c_str());

	//	Mat segmentationLabels;
	//	Hair hairModel;

	//	int retCode = extractHair(imgRGBModel, faceModel, &segmentationLabels, &hairModel);

	//	if (retCode == -1)
	//	{
	//		printf("Could not extract hair\n");
	//		continue;
	//	}

	//	cv::imwrite(segmentationDir + std::to_string(fileInd) + "_segmentation.bmp", segmentationLabels);

	//	Mat hairPixels(imgRGBModel.rows, imgRGBModel.cols, CV_8UC3, Scalar(255, 255, 255));
	//	imgRGBModel.copyTo(hairPixels, hairModel.getHairMask());

	//	cv::imwrite(modelsDir + std::to_string(fileInd) + "_hairModel.bmp", hairPixels);

	//	Mat synthesizedFace = synthesizeSkin(imgRGBModel, faceModel, hairModel.getHairMask());

	//	cv::imwrite(faceDir + std::to_string(fileInd) + "_face.bmp", synthesizedFace);
	//}
	
	string dataDir = "data/";
	string faceDir = "faces/";
	string modelsDir = "hairModels/";
	string segmentationDir = "segmentation/";
	string resultsDir = "results/";
	
	int targetInd = 7;
	int modelInd = 6;

	string pathStringTarget = dataDir + std::to_string(targetInd) + ".png";
	string pathStringModel = dataDir + std::to_string(modelInd) + ".png";

	const char * pathTarget = pathStringTarget.c_str();
	const char * pathModel = pathStringModel.c_str();
	const char * dataDirC = dataDir.c_str();

	Mat_<unsigned char> imgGrayModel(imread(pathModel, CV_LOAD_IMAGE_GRAYSCALE));
	Mat imgRGBModel(imread(pathModel, CV_LOAD_IMAGE_COLOR));
	if (!imgGrayModel.data)
	{
		printf("Cannot load %s\n", imgGrayModel);
		exit(1);
	}

	printf("Detecting face from model... \n");
	Face faceModel = detectFace(imgGrayModel, pathModel, dataDirC);
	printf("Face detected. \n");

	Mat segmentationLabelsModel;
	Hair hairModel;

	printf("Extracting hair from model... \n");
	int retCode = extractHair(imgRGBModel, faceModel, &segmentationLabelsModel, &hairModel);
	if (retCode == -1)
	{
		printf("Cannot extract hair. \n");
	}
	printf("Hair extracted. \n");

	Mat_<unsigned char> imgGrayTarget(imread(pathTarget, CV_LOAD_IMAGE_GRAYSCALE));
	Mat imgRGBTarget(imread(pathTarget, CV_LOAD_IMAGE_COLOR));
	if (!imgGrayTarget.data)
	{
		printf("Cannot load %s\n", imgGrayTarget);
		exit(1);	}	
	
	printf("Detecting face from target... \n");
	Face faceTarget = detectFace(imgGrayTarget, pathTarget, dataDirC);
	Mat segmentationLabelsTarget;
	Hair hairTarget;
	printf("Face detected. \n");

	printf("Extracting hair from target... \n");
	retCode = extractHair(imgRGBTarget, faceTarget,&segmentationLabelsTarget,&hairTarget);

	if (retCode == -1)
	{
		printf("Cannot extract hair.\n");
	}
	printf("Hair extracted. \n");

	printf("Synthesizing face... \n");
	Mat synthesizedface = synthesizeSkin(imgRGBTarget, faceTarget, hairTarget.getHairMask());
	printf("Face synthesized. \n");

	Mat hairSwap = swapHair(hairModel, faceTarget, imgRGBModel, imgRGBTarget, synthesizedface);

	cv::imwrite(resultsDir + "Hair" + std::to_string(modelInd) + "xFace" + std::to_string(targetInd) + ".bmp", hairSwap);
	
    return 0;
}
