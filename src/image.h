#ifndef __IMAGE_H__
#define __IMAGE_H__

#include <iostream>
#include <string>

#include <opencv2/highgui/highgui.hpp>

#define SEARCH_GAP	2

#define EXIST		1
#define NONE		0

#define UP		0
#define RIGHT	1
#define DOWN	2
#define LEFT	3
#define NO_FLAG	255

typedef struct Point_t {
	Point_t(int = 0, int = 0, int = NONE);

	Point_t operator=(const Point_t &);
	bool operator==(const Point_t &);

	int x;
	int y;

	// EXIST, NONE
	int avail;
} Point_t;

std::ostream& operator<<(std::ostream &, const Point_t &);

typedef struct Vertex_t {
	Point_t v[4];
} Vertex_t;

class Image {
public:
	Image(const std::string &, int);
	Image(const cv::Mat&, int);
	~Image();

	void saveImage();

	void init();
	void changeImage(cv::Mat &);
	void setZeroUnderThreshold();
	void setAllPointsToNONE();

	void convertRGBtoGRAY(cv::Mat &);
	void makePixelCDF();
	int getValCDF(double);

	void gaussianFiltering();

	void findAllPoints();
	void findAllAxisPoints();
	void makeRefPointsInfo();
	void makeRefPointsInCircle();

	Point_t adjustPoint(const Point_t &, int, int, int = NO_FLAG);
	Point_t findClosestWhitePoint(const Point_t &, int, int);
	Point_t findCenterPoint(const Point_t &, int, int, int);
	Point_t calcCenterOfMass(Point_t &, int, int);

private:	
	std::string filename;
	cv::Mat image;
	cv::Mat original;

	double cdf[256];

	int threshold;
	int basic_distance;

	int point_col, point_row;

	int center_x, center_y;

	// UP, RIGHT, DOWN, LEFT - 0 1
	//                         3 2
	Point_t unit_center_v[4][4];

	int point_num;

	// center of points
	Point_t **points;
	Point_t center_p;

	// ref points
	Point_t **ref;

	// vertex points of unit boxes
	Vertex_t **vertexes;

	// reduce average proportion
	double prop;

	int radius;
	int radius_p;
};

#endif
