#include <opencv2/highgui/highgui.hpp>
#include <iostream>
#include <string>
#include <vector>

#include "image.h"
#include "util.h"

using namespace std;
using namespace cv;

Point_t::Point_t(double x, double y)
	: x(x), y(y) {}

Image::Image(const string &filename, int flags) 
	: image_filename(filename), flag(flags) {

	image = imread(filename, flags);
	
	unit_width = image.cols / UNIT_WIDTH_NUM;
	unit_height = image.rows / UNIT_HEIGHT_NUM;

	double x = 0;
	double y = 0;
	for (int row_i = 0; row_i < UNIT_HEIGHT_NUM; row_i++) {
		y = (2 * row_i + 1) * unit_height / 2;

		for (int col_j = 0; col_j < UNIT_WIDTH_NUM; col_j++) {
			x = (2 * col_j + 1) * unit_width / 2;
			Point_t *p = new Point_t(x, y);
			unit_std_points.push_back(p);
		}
	}		
}

Image::~Image() {}

void Image::showImage(const string &window_name, int flags, int wait_key) {
	namedWindow(window_name, flags);
	imshow(window_name, image);
	waitKey(wait_key);
}

void Image::exitImage(const string &window_name) {
	destroyWindow(window_name);
}

Size Image::getSize() {
	return image.size();
}

bool Image::isValid() {
	return !image.empty();
}

// TODO need to check availability of array
Point_t* Image::getCentreOfMass(Point_t &p) {
	uchar *data = (uchar *)image.data;
	int idx = p.y * image.cols + p.x;

	int weight = 0;
	double x = 0;
	double y = 0;

	for (int row_i = 0; row_i < unit_height; row_i++) {
		for (int col_j = 0; col_j < unit_width; col_j++) {
			int w = data[idx];

			weight += w;
			x += col_j * w;
			y += row_i * w;

			idx++;
		}

		idx += (image.cols - unit_width);
	}
	x /= weight;
	y /= weight;

	Point_t *centre_p = new Point_t(p.x + x, p.y + y);

	return centre_p;
}

void Image::calcCentrePoints() {
	int x = 0;
	int y = 0;

	unit_centre_points.clear();
	for (int row_i = 0; row_i < UNIT_HEIGHT_NUM; row_i++) {
		for (int col_j = 0; col_j < UNIT_WIDTH_NUM; col_j++) {
			Point_t cur_p(x + col_j * unit_width, y + row_i * unit_height);
			Point_t *centre_p = getCentreOfMass(cur_p);
			unit_centre_points.push_back(centre_p);
		}
	}
}

vector<Point_t *> Image::getCentrePoints() {
	return unit_centre_points;
}

vector<double> Image::calcCentrePointsDistance() {
	return calcVecDistance(unit_centre_points, unit_std_points);
}
