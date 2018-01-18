#ifndef __IMAGE_H__
#define __IMAGE_H__

#include <string>
#include <vector>

#include <opencv2/highgui/highgui.hpp>

// TODO 5 is hard-coded -> change it to automallycally select number.
#define UNIT_WIDTH_NUM 5
#define UNIT_HEIGHT_NUM 5

typedef struct Point_t {
	Point_t(double, double);

	double x;
	double y;
} Point_t;

class Image {
public:
	Image(const std::string &, int = CV_LOAD_IMAGE_GRAYSCALE);
	Image(cv::Mat);
	~Image();

	void showImage(const std::string & = "window", int = CV_WINDOW_AUTOSIZE, int = 0);
	void exitImage(const std::string & = "window");

	cv::Size getSize();	
	bool isValid();

	Point_t* getCentreOfMass(Point_t &);
	void calcCentrePoints();
	std::vector<Point_t *> getCentrePoints();

	std::vector<double> calcCentrePointsDistance();

private:
	std::string image_filename;
	cv::Mat image;
	int flag;

	int unit_width;
	int unit_height;

	std::vector<Point_t *> unit_centre_points;
	std::vector<Point_t *> unit_std_points;
};

#endif
