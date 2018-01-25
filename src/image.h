#ifndef __IMAGE_H__
#define __IMAGE_H__

#include <iostream>
#include <string>
#include <vector>

#include <opencv2/highgui/highgui.hpp>

#define COL_POINT_NUM	7
#define ROW_POINT_NUM	7

#define REDUCE_RATIO	0.7

#define COL_BASIC_DISTANCE	110
#define ROW_BASIC_DISTANCE	110

#define SEARCH_GAP	2

#define Variance_t	Point_t

typedef struct Point_t {
	Point_t(int = 0, int = 0);
	Point_t operator=(const Point_t &);
	bool operator==(const Point_t &);

	int x;
	int y;
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

	void calcVariance();

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
