#ifndef __IMAGE_H__
#define __IMAGE_H__

#include <iostream>
#include <string>

#include <opencv2/highgui/highgui.hpp>

#include "util.h"
#include "types.h"

class Image {
public:
	Image(Options &);
	Image(const cv::Mat &, Options &);
	~Image();

	void init();
	void changeImage(cv::Mat &);
	void setAllPointsToNONE();

	void convertRGBtoGRAY(cv::Mat &);
	void makePixelCDF();
	int getValCDF(double);

	void gaussianFiltering();

	std::string findAllPoints();
	void findAllAxisPoints();
	int makeRefPointsInfo();

	Point_t adjustPoint(const Point_t &, int, int, int = NO_FLAG);
	Point_t findClosestWhitePoint(const Point_t &, int, int);
	Point_t findCenterPoint(const Point_t &, int, int, int);
	Point_t calcCenterOfMass(Point_t &, int, int);

private:	
	cv::Mat image;
	cv::Mat original;

	double cdf[256];

	Options opt;

	int int_basic_distance;

	int threshold_val;

	int point_col, point_row;

	int center_x, center_y;

	// UP, RIGHT, DOWN, LEFT - 0 1
	//                         3 2
	Point_t unit_v[4][4];

	// the number of points in circle
	int ref_points_num;
	int point_num;

	// center of points
	Point_t **points;
	Point_t center_p;

	// ref points
	Point_t **ref;

	// diff data
	double diff[146];

	// vertex points of unit boxes
	Vertex_t **vertex;

	// reduce average proportion
	double prop;

	int radius;
	int radius_p;
};

#endif
