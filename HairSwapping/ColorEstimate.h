#ifndef COLOR_ESTIMATE_H
#define COLOR_ESTIMATE_H

#include <opencv2//core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>

using namespace cv;

class ColorEstimate
{
private:

	Scalar meanA;
	Scalar stddevA;
	Scalar meanC;
	Scalar stddevC;
	Scalar meanB;
	Scalar stddevB;
	Scalar avgSkinColor;

	void estimateColors(Mat A, Mat B, Mat C);

public:

	ColorEstimate(Mat A, Mat B, Mat C);
	Scalar getMeanA();
	Scalar getMeanB();
	Scalar getMeanC();
	Scalar getStdA();
	Scalar getStdC();
	Scalar getStdB();
	Scalar getAvgSkinColor();
};

#endif