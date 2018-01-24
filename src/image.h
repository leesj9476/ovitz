#ifndef __IMAGE_H__
#define __IMAGE_H__

#include <iostream>
#include <string>
#include <vector>

#include <opencv2/highgui/highgui.hpp>

#define COL_POINT_NUM	13
#define ROW_POINT_NUM	13

#define REDUCE_RATIO	0.7
#define POINT_SIZE		5

#define COL_BASIC_DISTANCE	500
#define ROW_BASIC_DISTANCE	500

#define Variance_t	Point_t

typedef struct Point_t {
	Point_t(double = 0, double = 0);
	Point_t operator=(const Point_t &);

	double x;
	double y;
} Point_t;

std::ostream& operator<<(std::ostream &, const Point_t &);

class Image {
public:
	Image(const std::string &, int = CV_LOAD_IMAGE_GRAYSCALE);
	Image(cv::Mat);
	~Image();

	cv::Size getSize();	
	bool isValid();

	Point_t getCentreOfMass(const Point_t &, int, int);
	Point_t getCenterPoint();
	Point_t getClosestWhitePoint(const Point_t &);
	Point_t adjustCenterPoint(const Point_t &);
	bool calcCentrePoints();

	Variance_t* calcVariance();

private:
	std::string image_filename;
	cv::Mat image;
	int flag;

	Point_t center_point;

	Point_t *unit_centre_points;
	Point_t *unit_ref_points;

	Variance_t *variance;
};

#endif
