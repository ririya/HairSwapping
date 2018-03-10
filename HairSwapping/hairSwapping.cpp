#include <stdio.h>
#include <stdlib.h>
#include <list>

// OpenCV
#include <opencv2//core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/photo.hpp>"
#include "FaceRecognition.h"
#include "HairExtraction.h"
#include "SkinSynthesis.h"
#include "HairEditing.h"
#include "Hair.h"
#include "Face.h"

using namespace cv;

void createMosaic(std::string dir, std::string ext, int nRows, int nCols, Scalar textColor)
{
	Scalar backgroundColor = Scalar(255, 255, 255);
	Mat mosaic(2 * nRows, 9 * nCols, CV_8UC3, backgroundColor);

	for (int fileInd = 1; fileInd <= 18; fileInd++)
	{
		string pathStringModel = dir + std::to_string(fileInd) + ext;

			const char * path = pathStringModel.c_str();

			Mat img(imread(path, CV_LOAD_IMAGE_COLOR));
			if (!img.data)
			{	
				img = Mat(nRows, nCols, CV_8UC3, backgroundColor);
			}

			int xStart, yStart;

			if (fileInd <= 9)
			{
				yStart = 0;
				xStart = (fileInd - 1)*nCols;
			

			}
			else
			{
				yStart = nRows;
				xStart = (fileInd - 10)*nCols;
			}
			

			Rect rect(xStart, yStart, nCols, nRows);

			Mat roi(mosaic(rect));

			putText(img, std::to_string(fileInd), cvPoint(0, 35), FONT_HERSHEY_PLAIN, 3, textColor, 3);
			img.copyTo(roi);

		/*	cv::imshow("img", img);
			cv::waitKey();*/
	}

	double scale = 0.5;
	Mat resized;
	Mat scalingMatrix = (Mat_<double>(2, 3) << scale, 0, 0, 0, scale, 0);

	int newHeight = mosaic.rows*scale;
	int newWidth = mosaic.cols*scale;

	warpAffine(mosaic, resized, scalingMatrix, Size(newWidth, newHeight), 1, 0, backgroundColor);

	/*cv::imshow("mosaic", resized);
	cv::waitKey();*/

	cv::imwrite(dir + "mosaic.bmp", resized);
	
}

Mat generateResultImage(Mat imgTarget, Mat imgModel, Mat hairSwap)
{
	int nCols = imgTarget.cols;
	int nRows = imgTarget.rows;

	Mat resultImage(nRows, 3 * nCols, CV_8UC3);

	Rect rectModel(0, 0, nCols, nRows);

	Mat roiModel(resultImage(rectModel));

	putText(imgModel, "Model", cvPoint(0, 30), FONT_HERSHEY_PLAIN, 2, Scalar(255,0,0), 3);
	imgModel.copyTo(roiModel);


	Rect rectTarget(nCols, 0, nCols, nRows);

	Mat roiTarget(resultImage(rectTarget));

	putText(imgTarget, "Target", cvPoint(0, 30), FONT_HERSHEY_PLAIN, 2, Scalar(255, 0, 0), 3);
	imgTarget.copyTo(roiTarget);

	Rect rectSwap(2*nCols, 0, nCols, nRows);

	Mat roiSwap(resultImage(rectSwap));

	putText(hairSwap, "Result", cvPoint(0, 30), FONT_HERSHEY_PLAIN, 2, Scalar(255, 0, 0), 3);
	hairSwap.copyTo(roiSwap);

	return resultImage;

}

std::string removeExtension(const std::string& filename) {
	size_t lastdot = filename.find_last_of(".");
	if (lastdot == std::string::npos) return filename;
	return filename.substr(0, lastdot);
}

void createMosaicImages()
{
	if (createMosaicImages)
	{
		string dataDir = "data/";
		string faceDir = "faces/";
		string modelsDir = "hairmodels/";
		string segmentationDir = "segmentation/";
		string resultsDir = "results/";

		for (int fileInd = 1; fileInd <= 18; fileInd++)
		{

			printf("fileInd = %d \n", fileInd);
			string pathString = dataDir + std::to_string(fileInd) + ".png";

			const char * path = pathString.c_str();
			Mat_<unsigned char> imgGray(imread(path, CV_LOAD_IMAGE_GRAYSCALE));
			Mat imgRGB(imread(path, CV_LOAD_IMAGE_COLOR));
			if (!imgRGB.data)
			{
				printf("Cannot load %s\n", path);
				continue;
				//exit(1);
			}

			Face face = detectFace(imgGray, path, dataDir.c_str());

			Mat segmentationLabels;
			Hair hair;

			int retCode = extractHair(imgRGB, face, &segmentationLabels, &hair);

			if (retCode == -1)
			{
				printf("Could not extract hair\n");
				continue;
			}

			cv::imwrite(segmentationDir + std::to_string(fileInd) + ".bmp", segmentationLabels);

			Mat hairPixels(imgRGB.rows, imgRGB.cols, CV_8UC3, Scalar(255, 255, 255));
			imgRGB.copyTo(hairPixels, hair.getHairMask());

			cv::imwrite(modelsDir + std::to_string(fileInd) + ".bmp", hairPixels);

			Mat synthesizedFace = synthesizeSkin(imgRGB, face, hair);

			cv::imwrite(faceDir + std::to_string(fileInd) + ".bmp", synthesizedFace);
		}

		//createMosaic(dataDir, ".png", 320, 240, Scalar(255, 0, 0));
		createMosaic(faceDir, ".bmp", 320, 240, Scalar(255, 0, 0));
		createMosaic(modelsDir, ".bmp", 320, 240, Scalar(255, 0, 0));
		createMosaic(segmentationDir, ".bmp", 320, 240, Scalar(255, 255, 255));
	}
}

void testAll()
{
	string dataDir = "data/";
	string faceDir = "faces/";
	string modelsDir = "hairmodels/";
	string segmentationDir = "segmentation/";
	string resultsDir = "results/";

	vector<Face> faces;
	vector<Hair> hairModels;
	vector<Mat> synthesizedFaces;

	printf("Generating models\n");
	for (int fileInd = 1; fileInd <= 18; fileInd++)
	{
		printf("fileInd = %d \n", fileInd);
		string pathString = dataDir + std::to_string(fileInd) + ".png";

		const char * path = pathString.c_str();
		Mat_<unsigned char> imgGray(imread(path, CV_LOAD_IMAGE_GRAYSCALE));
		Mat imgRGB(imread(path, CV_LOAD_IMAGE_COLOR));
		if (!imgRGB.data)
		{
			printf("Cannot load %s\n", path);
			continue;
			//exit(1);
		}

		Face face = detectFace(imgGray, path, dataDir.c_str());

		Mat segmentationLabels;
		Hair hair;

		int retCode = extractHair(imgRGB, face, &segmentationLabels, &hair);

		if (retCode == -1)
		{
			printf("Could not extract hair\n");
			continue;
		}
				
		Mat hairPixels(imgRGB.rows, imgRGB.cols, CV_8UC3, Scalar(255, 255, 255));
		imgRGB.copyTo(hairPixels, hair.getHairMask());

		cv::imwrite(modelsDir + std::to_string(fileInd) + ".bmp", hairPixels);

		Mat synthesizedFace = synthesizeSkin(imgRGB, face, hair);

		cv::imwrite(faceDir + std::to_string(fileInd) + ".bmp", synthesizedFace);

		faces.push_back(face);
		hairModels.push_back(hair);
		synthesizedFaces.push_back(synthesizedFace);

	}

	for (int model = 1; model <= 18; model++)
	{
		for (int target = 1; target <= 18; target++)
		{
			
			if (model == target)
			{
				continue;
			}

			printf("Swapping Model %d and Target %d \n", model, target);

			string pathStringModel = dataDir + std::to_string(model) + ".png";
			string pathStringTarget = dataDir + std::to_string(target) + ".png";
			const char * pathTarget = pathStringTarget.c_str();
			const char * pathModel = pathStringModel.c_str();

			Mat imgRGBModel(imread(pathModel, CV_LOAD_IMAGE_COLOR));
			Mat imgRGBTarget(imread(pathTarget, CV_LOAD_IMAGE_COLOR));

			Hair hairModel = hairModels.at(model - 1);
			Face faceModel = faces.at(model - 1);
			Face faceTarget = faces.at(target - 1);
			Mat synthesizedface = synthesizedFaces.at(target - 1);

			Mat hairSwap = swapHair(hairModel, faceTarget, faceModel.getHeadSize(), synthesizedface);

			Mat resultImage = generateResultImage(imgRGBTarget, imgRGBModel, hairSwap);

			cv::imwrite(resultsDir + "Hair" + std::to_string(model) + "xFace" + std::to_string(target) + ".bmp", resultImage);
		}
	}
}

void swapHairMain(int argc, char *argv[])
{
	string dataDir = "data/";
	string faceDir = "faces/";
	string modelsDir = "hairModels/";
	string segmentationDir = "segmentation/";
	string resultsDir = "results/";

	string model;
	string target;

	if (argc == 3)
	{
		model = argv[1];
		target = argv[2];
	}

	else
	{
		model = "3.png";
		target = "7.png";
	}

	string pathStringModel = dataDir + model;
	string pathStringTarget = dataDir + target;

	const char * pathTarget = pathStringTarget.c_str();
	const char * pathModel = pathStringModel.c_str();
	const char * dataDirC = dataDir.c_str();

	int retCode;
	printf("Model image: %s\n", pathModel);
	printf("Target image: %s\n", pathTarget);
	Mat_<unsigned char> imgGrayModel(imread(pathModel, CV_LOAD_IMAGE_GRAYSCALE));
	Mat imgRGBModel(imread(pathModel, CV_LOAD_IMAGE_COLOR));
	if (!imgGrayModel.data)
	{
		printf("Cannot load %s\n", imgGrayModel);
		exit(1);
	}

	Mat_<unsigned char> imgGrayTarget(imread(pathTarget, CV_LOAD_IMAGE_GRAYSCALE));
	Mat imgRGBTarget(imread(pathTarget, CV_LOAD_IMAGE_COLOR));
	if (!imgGrayTarget.data)
	{
		printf("Cannot load %s\n", imgGrayTarget);
		exit(1);
	}

	printf("Detecting face from model... \n");
	Face faceModel = detectFace(imgGrayModel, pathModel, dataDirC);
	printf("Face detected. \n");

	Mat segmentationLabelsModel;
	Hair hairModel;

	printf("Extracting hair from model... \n");
	retCode = extractHair(imgRGBModel, faceModel, &segmentationLabelsModel, &hairModel);
	if (retCode == -1)
	{
	printf("Cannot extract hair. \n");
	}
	printf("Hair extracted. \n");

	printf("Detecting face from target... \n");
	Face faceTarget = detectFace(imgGrayTarget, pathTarget, dataDirC);
	Mat segmentationLabelsTarget;
	Hair hairTarget;
	printf("Face detected. \n");

	printf("Extracting hair from target... \n");
	retCode = extractHair(imgRGBTarget, faceTarget, &segmentationLabelsTarget, &hairTarget);

	if (retCode == -1)
	{
		printf("Cannot extract hair.\n");
	}
	printf("Hair extracted. \n");

	printf("Synthesizing face... \n");
	Mat synthesizedface = synthesizeSkin(imgRGBTarget, faceTarget, hairTarget);
	printf("Face synthesized. \n");	

	Mat hairSwap = swapHair(hairModel, faceTarget, faceModel.getHeadSize(), synthesizedface);

	Mat resultImage = generateResultImage(imgRGBTarget, imgRGBModel, hairSwap);

	cv::imwrite(resultsDir + "Hair" + removeExtension(model) + "xFace" + removeExtension(target) + ".bmp", resultImage);	
}


int main(int argc, char *argv[])
{
	//createMosaicImages();

	swapHairMain(argc, argv);

	//testAll();

	destroyAllWindows();

    return 0;
}
