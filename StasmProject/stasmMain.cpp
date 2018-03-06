#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <fstream>

// OpenCV
#include <opencv2//core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include "opencv/highgui.h"
#include "stasm_lib.h"


using namespace std;
using namespace cv;

int main(int argc, char *argv[])
{

	std::string file;
	std::string dataDir;

	if (argc == 3)
	{
		file = argv[1];
		dataDir = argv[2];
	}

	else
	{
		dataDir = "data/";
		file = dataDir + "10.png";
	}
	
	const char * pathFile = file.c_str();
	const char * dataDirC = dataDir.c_str();

	Mat_<unsigned char> imgGray(imread(pathFile, CV_LOAD_IMAGE_GRAYSCALE));
	if (!imgGray.data)
	{
		printf("Cannot load %s\n", imgGray);
		exit(1);
	}

	int foundface;

	float landmarks[2 * stasm_NLANDMARKS]; // x,y coords (note the 2)

	if (!stasm_search_single(&foundface, landmarks,
		(const char*)imgGray.data, imgGray.cols, imgGray.rows, pathFile, dataDirC))
	{
		printf("Error in stasm_search_single: %s\n", stasm_lasterr());
		exit(1);
	}

	if (!foundface)
	{
		printf("No face found in %s\n", pathFile);
		exit(1);
	}
	else
	{
		stasm_force_points_into_image(landmarks, imgGray.cols, imgGray.rows);

		ofstream fileStream;
		fileStream.open("stasmResults.csv");

		for (int i = 0; i < stasm_NLANDMARKS; i++)
		{
			fileStream << landmarks[2 * i] << "," << landmarks[2 * i+1] << "\n";
		}
		fileStream.close();
		return 0;
	}

	
    return 0;
}
