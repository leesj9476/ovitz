#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>

#include <fstream>
#include <iostream>
#include <iomanip>
#include <string>
#include <cmath>

#include "types.h"
#include "image.h"
#include "util.h"

using namespace std;
using namespace cv;

extern double *lens_mat[5][5];

int num = 0;

Point_t::Point_t(double real_x_, double real_y_, int avail)
	: real_x(real_x_), real_y(real_y_), avail(avail) {

	x = static_cast<int>(real_x);
	y = static_cast<int>(real_y);
}

Point_t::Point_t(int x_, int y_, int avail)
	: x(x_), y(y_), avail(avail) {

	real_x = static_cast<double>(x);
	real_y = static_cast<double>(y);
}

Point_t Point_t::operator=(const Point_t &p) {
	this->x = p.x;
	this->y = p.y;
	this->real_x = p.real_x;
	this->real_y = p.real_y;
	this->avail = p.avail;

	return *this;
}

bool Point_t::operator==(const Point_t &p) {
	return (x == p.x && y == p.y);
}

ostream& operator<<(ostream &os, const Point_t &p) {
	cout << "(" << setw(4) << p.x << "," << setw(4) << p.y << ")";
	return os;
}

Image::Image(const string &filename_, int basic_distance_) 
	: filename(filename_), basic_distance(basic_distance_) {
	
	image = imread(filename, CV_LOAD_IMAGE_GRAYSCALE);
	original = imread(filename);

	points = NULL;
	ref = NULL;
	vertexes = NULL;
}

Image::Image(const Mat &image_, int basic_distance_) 
	: basic_distance(basic_distance_) {

	filename = "";

	image_.copyTo(original);
	convertRGBtoGRAY(original);

	points = NULL;
	ref = NULL;
	vertexes = NULL;
}

Image::~Image() {
	for (int i = 0; i < point_row; i++) {
		delete[] points[i];
		delete[] vertexes[i];
	}

	delete[] points;
	delete[] vertexes;
}

void Image::init(){
	point_col = image.cols / basic_distance;
	point_row = image.rows / basic_distance;

	if ((point_col & 0x01) == 0x00)
		point_col++;
	if ((point_row & 0x01) == 0x00)
		point_row++;

	center_x = point_col >> 1;
	center_y = point_row >> 1;

	if (points) {
		for (int i = 0; i < point_row; i++) {
			if (points[i])
				delete[] points[i];
		}

		delete[] points;
	}

	if (ref) {
		for (int i = 0; i < point_row; i++) {
			if (ref[i])
				delete[] ref[i];
		}

	delete[] ref;
	}

	if (vertexes) {
		for (int i = 0; i < point_row; i++) {
			if (vertexes[i])
				delete[] vertexes[i];
		}

		delete[] vertexes;
	}

	points = new Point_t*[point_row];
	for (int i = 0; i < point_row; i++)
		points[i] = new Point_t[point_col];

	ref = new Point_t*[point_row];
	for (int i = 0; i < point_row; i++)
		ref[i] = new Point_t[point_col];

	vertexes = new Vertex_t*[point_row];
	for (int i = 0; i < point_row; i++)
		vertexes[i] = new Vertex_t[point_col];
}

void Image::changeImage(Mat &new_image) {
	new_image.copyTo(original);
	convertRGBtoGRAY(original);
}

void Image::convertRGBtoGRAY(Mat &original) {
	cvtColor(original, image, CV_RGB2GRAY);
}

void Image::makePixelCDF() {
	uchar *data = (uchar *)image.data;
	memset(cdf, 0, sizeof(cdf));

	for (uint i = 0; i < image.total(); i++) {
		cdf[data[i]]++;
	}

	for (int i = 0; i < 256; i++) {
		cdf[i] /= image.total();
	}

	for (int i = 1; i < 256; i++) {
		cdf[i] += cdf[i - 1];
	}
}

int Image::getValCDF(double p) {
	int i = 0;
	for (i = 0; i < 256; i++) {
		if (cdf[i] > p)
			break;
	}

	return i;
}

void Image::gaussianFiltering() {
	GaussianBlur(image, image, Size(3, 3), 0);
}

// find all points
string Image::findAllPoints() {
	if (filename != "") {
		gaussianFiltering();
		makePixelCDF();
	}

	threshold_val = getValCDF(0.95);
	threshold(image, image, threshold_val, 255, 3);
	setAllPointsToNONE();
	
	findAllAxisPoints();
	makeRefPointsInfo();
	makeRefPointsInCircle();

	// find all points
	for (int y = 1; y <= radius_p; y++) {
		for (int x = 1; x <= radius_p; x++) {
			if (ref[center_x + x][center_y + y].avail == NONE)
				break;

			points[center_x + x][center_y + y] = adjustPoint(Point_t(points[center_x + x][center_y].x, points[center_x][center_y + y].y), x, y);
			points[center_x + x][center_y - y] = adjustPoint(Point_t(points[center_x + x][center_y].x, points[center_x][center_y - y].y), x, -y);
			points[center_x - x][center_y + y] = adjustPoint(Point_t(points[center_x - x][center_y].x, points[center_x][center_y + y].y), -x, y);
			points[center_x - x][center_y - y] = adjustPoint(Point_t(points[center_x - x][center_y].x, points[center_x][center_y - y].y), -x, -y);
		}
	}

	// make analysis result picture
	int d = (basic_distance >> 1);
	for (int i = 0; i < point_row; i++) {
		for (int j = 0; j < point_col; j++) {
			if (ref[i][j].avail == EXIST) {
				line(original, Point(ref[i][j].x - d, ref[i][j].y - d), Point(ref[i][j].x + d, ref[i][j].y - d), Scalar(255, 0, 0));
				line(original, Point(ref[i][j].x + d, ref[i][j].y - d), Point(ref[i][j].x + d, ref[i][j].y + d), Scalar(255, 0, 0));
				line(original, Point(ref[i][j].x + d, ref[i][j].y + d), Point(ref[i][j].x - d, ref[i][j].y + d), Scalar(255, 0, 0));
				line(original, Point(ref[i][j].x - d, ref[i][j].y + d), Point(ref[i][j].x - d, ref[i][j].y - d), Scalar(255, 0, 0));

				if (points[i][j].avail == EXIST)
					original.at<Vec3b>(points[i][j].y, points[i][j].x) = { 0, 0, 255 };
			}
		}
	}
	circle(original, Point(center_p.x, center_p.y), radius, Scalar(0, 255, 0));
	imshow("result", original);
	imwrite("result.png", original);

	/*
	cout << "|     real point    |    ref point    | x-var | y-var |" << endl;
	cout << " ----------------------------------------------------- " << endl;
	for (int y = 0; y < point_row; y++) {
		for (int x = 0; x < point_col; x++) {
			if (points[x][y].avail == EXIST) {
				cout << "|    " << points[x][y] << "    |   " << ref[x][y] << "   | " << setw(5) << (points[x][y].real_x - ref[x][y].real_x) << " | " << setw(5) << (points[x][y].real_y - ref[x][y].real_y) << " |" << endl;
			}
		}
	}*/

	int ref_num = 0;
	for (int i = 0; i < point_row; i++) {
		for (int j = 0; j < point_col; j++) {
			if (ref[i][j].avail == EXIST) { 
				ref_num++;
			}
		}
	}

	// TODO reallocate only when ref_num is changed.
	// TODO hard-coded 512 -> get info from cam class
	slope = new double[ref_num * 2];
	double w = 1944 / 512 * (0.2); // ccd_pixel = (maximum height pixel = 1944) / (current height) * (real pixel distance = 1.4)
	                               // focal = 7mm
	int slope_i = 0;
	for (int y = center_y - radius_p; y <= center_y + radius_p; y++) {
		for (int x = center_x - radius_p; x <= center_x + radius_p; x++) {
			if (ref[x][y].avail == EXIST) {
				// slope x
				slope[slope_i] = points[x][y].real_x - ref[x][y].real_x;

				// slope y
				slope[slope_i + ref_num] = -(points[x][y].real_y - ref[x][y].real_y);

				slope_i++;
			}
		}
	}

	for (int i = 0; i < ref_num * 2; i++) {
		slope[i] *= w;
	}
	
	string result_str = "";
	if (radius_p <= 5) {
		double result[5] = { 0,  };
		for (int i = 0; i < 5; i++) {
			for (int j = 0; j < ref_num * 2; j++) {
				result[i] += lens_mat[radius_p - 1][i][j] * slope[j];
			}
		}
	
		for (int i = 0; i < 5; i++) {
			result[i] *= (-1);
		}

		result_str = to_string(result[2]) + "\n" + to_string(result[3]) + "\n" + to_string(result[4]);
	}

	delete[] slope;
	return result_str;
}

void Image::setAllPointsToNONE() {
	for (int i = 0; i < point_row; i++) {
		for (int j = 0; j < point_col; j++) 
			points[i][j].avail = NONE;
	}
}

void Image::findAllAxisPoints() {
	// find center point
	Point_t zero_p;
	Point_t expected_center_p = calcCenterOfMass(zero_p, image.cols, image.rows);
	center_p = adjustPoint(expected_center_p, 0, 0);
	points[center_x][center_y] = center_p;

	// find 1st axis point -> use basic distance
	points[center_x + 1][center_y] = adjustPoint(Point_t(center_p.x + basic_distance, center_p.y), 1, 0, RIGHT);
	points[center_x - 1][center_y] = adjustPoint(Point_t(center_p.x - basic_distance, center_p.y), -1, 0, LEFT);
	points[center_x][center_y + 1] = adjustPoint(Point_t(center_p.x, center_p.y + basic_distance), 0, 1, DOWN);
	points[center_x][center_y - 1] = adjustPoint(Point_t(center_p.x, center_p.y - basic_distance), 0, -1, UP);

	// find 2nd axis point
	uchar *data = (uchar *)image.data;
	bool search[4] = { true, true, true, true };
	if (point_row < 5) {
		search[UP] = false; search[DOWN] = false;
	}
	if (point_col < 5) {
		search[RIGHT] = false; search[LEFT] = false;
	}

	int gap = 0;
	int idx = 0;
	while (search[UP] || search[RIGHT] || search[DOWN] || search[LEFT]) {
		gap++;
		if (gap >= basic_distance) {
			// UP
			if (points[center_x][center_y - 2].avail == NONE) {
				points[center_x][center_y - 2].x = points[center_x][center_y - 1].x;
				points[center_x][center_y - 2].y = points[center_x][center_y - 1].y - basic_distance;
			}
			
			// RIGHT
			if (points[center_x + 2][center_y].avail == NONE) {
				points[center_x + 2][center_y].x = points[center_x + 1][center_y].x + basic_distance;
				points[center_x + 2][center_y].y = points[center_x + 1][center_y].y;
			}
			
			// DOWN
			if (points[center_x][center_y + 2].avail == NONE) {
				points[center_x][center_y + 2].x = points[center_x][center_y + 1].x;
				points[center_x][center_y + 2].y = points[center_x][center_y + 1].y + basic_distance;
			}
			
			// LEFT
			if (points[center_x - 2][center_y].avail == NONE) {
				points[center_x - 2][center_y].x = points[center_x - 1][center_y].x;
				points[center_x - 2][center_y].y = points[center_x - 1][center_y].y - basic_distance;
			}
			
			break;
		}
		
		if (search[UP] && unit_center_v[UP][0].y - gap >= 0) {
			idx = (unit_center_v[UP][0].y - gap) * image.cols + unit_center_v[UP][0].x;
			for (int i = unit_center_v[UP][0].x; i <= unit_center_v[UP][1].x; i++) {
				if (data[idx] > threshold_val) {
					points[center_x][center_y - 2] = adjustPoint(Point_t(i, unit_center_v[UP][0].y - gap, i), 0, -2);
					search[UP] = false;

					break;
				}

				idx++;
			}
		}
		else
			search[UP] = false;

		if (search[RIGHT] && unit_center_v[RIGHT][1].x + gap < image.cols) {
			idx = unit_center_v[RIGHT][1].y * image.cols + (unit_center_v[RIGHT][1].x + gap);
			for (int i = unit_center_v[RIGHT][1].y; i <= unit_center_v[RIGHT][2].y; i++) {
				if (data[idx] > threshold_val) {
					points[center_x + 2][center_y] = adjustPoint(Point_t(unit_center_v[RIGHT][1].x + gap, i), 2, 0);
					search[RIGHT] = false;	

					break;
				}

				idx+=image.cols;
		
			}
		}
		else
			search[RIGHT] = false;

		if (search[DOWN] && unit_center_v[DOWN][2].y + gap < image.rows) {
			idx = (unit_center_v[DOWN][2].y + gap) * image.cols + unit_center_v[DOWN][2].x;
			for (int i = unit_center_v[DOWN][2].x; i >= unit_center_v[DOWN][3].x; i--) {
				if (data[idx] > threshold_val) {
					points[center_x][center_y + 2] = adjustPoint(Point_t(i, unit_center_v[DOWN][2].y + gap), 0, 2);
					search[DOWN] = false;

					break;
				}
	
				idx--;
			}
		}
		else
			search[DOWN] = false;

		if (search[LEFT] && unit_center_v[LEFT][3].x - gap >= 0) {
			idx = unit_center_v[LEFT][3].y * image.cols + (unit_center_v[LEFT][3].x - gap);
			for (int i = unit_center_v[LEFT][3].y; i >= unit_center_v[LEFT][0].y; i--) {
				if (data[idx] > threshold_val) {
					points[center_x - 2][center_y] = adjustPoint(Point_t(unit_center_v[LEFT][3].x - gap, i), -2, 0);
					search[LEFT] = false;

					break;
				}

				idx-=image.cols;
			}
		}
		else
			search[LEFT] = false;
	}

	int min_p = (center_x < center_y) ? center_x : center_y;
	if (points[center_x + 2][center_y].avail == NONE ||
		points[center_x - 2][center_y].avail == NONE ||
		points[center_x][center_y + 2].avail == NONE ||
		points[center_x][center_y - 2].avail == NONE)

		min_p = 1;
	
	if (min_p != 1) {
		//calc reduction proportion
		double reduce_p[4] = { 0, };
		if (point_row >= 5) {
			if (points[center_x][center_y - 2].avail == EXIST)
				reduce_p[UP] = (points[center_x][center_y - 1].y - points[center_x][center_y - 2].y) / static_cast<double>(basic_distance);
			
			if (points[center_x][center_y + 2].avail == EXIST)
				reduce_p[DOWN] = (points[center_x][center_y + 2].y - points[center_x][center_y + 1].y) / static_cast<double>(basic_distance);
		}
	
		if (point_col >= 5) { 
			if (points[center_x + 2][center_y].avail == EXIST)
				reduce_p[RIGHT] = (points[center_x + 2][center_y].x - points[center_x + 1][center_y].x) / static_cast<double>(basic_distance);
			
			if (points[center_x - 2][center_y].avail == EXIST)
				reduce_p[LEFT] = (points[center_x - 1][center_y].x - points[center_x - 2][center_y].x) / static_cast<double>(basic_distance);
		}

		prop = (reduce_p[UP] + reduce_p[RIGHT] + reduce_p[DOWN] + reduce_p[LEFT]) / 4;
	
		// find all axis points
		int d[4];
		for (int i = 3; i <= min_p; i++) {
			// find up, down axis points
			d[UP] = ceil((points[center_x][center_y - i + 2].y - points[center_x][center_y - i + 1].y) * prop);
			d[DOWN] = ceil((points[center_x][center_y + i - 1].y - points[center_x][center_y + i - 2].y) * prop);
	
			// UP
			points[center_x][center_y - i] = adjustPoint(Point_t(points[center_x][center_y - i + 1].x, points[center_x][center_y - i + 1].y - d[UP]), 0, -i);
			
			// DOWN
			points[center_x][center_y + i] = adjustPoint(Point_t(points[center_x][center_y + i - 1].x, points[center_x][center_y + i - 1].y + d[DOWN]), 0, i);
	
			if (points[center_x][center_y - i].avail == NONE || points[center_x][center_y + i].avail == NONE)
				min_p = i - 1;
		}
	
		for (int i = 3; i <= min_p; i++) {
			// find right, left axis points
			d[RIGHT] = ceil((points[center_x + i - 1][center_y].x - points[center_x + i - 2][center_y].x) * prop);
			d[LEFT] = ceil((points[center_x - i + 2][center_y].x - points[center_x - i + 1][center_y].x) * prop);
	
			// RIGHT
			points[center_x + i][center_y] = adjustPoint(Point_t(points[center_x + i - 1][center_y].x + d[RIGHT], points[center_x + i - 1][center_y].y), i, 0);
			
			// LEFT
			points[center_x - i][center_y] = adjustPoint(Point_t(points[center_x - i + 1][center_y].x - d[LEFT], points[center_x - i + 1][center_y].y), -i, 0);
	
			if (points[center_x + i][center_y].avail == NONE || points[center_x - i][center_y].avail == NONE)
				min_p = i - 1;
		}
	
		for (int i = min_p + 1; i <= center_x; i++) {
			if (points[center_x + i][center_y].avail == EXIST)
				points[center_x + i][center_y].avail = NONE;
	
			if (points[center_x - i][center_y].avail == EXIST)
				points[center_x - i][center_y].avail = NONE;
		}
	
		for (int i = min_p + 1; i <= center_x; i++) {
			if (points[center_x][center_y + i].avail == EXIST)
				points[center_x][center_y + i].avail = NONE;
	
			if (points[center_x][center_y - i].avail == EXIST)
				points[center_x][center_y - i].avail = NONE;
		}
	}

	// radius
	radius_p = min_p;
	if (radius > 5)
		radius_p = 5;
}

Point_t Image::adjustPoint(const Point_t &p, int diff_x, int diff_y, int flag) {
	if (p.x < 0 || p.x >= image.cols || p.y < 0 || p.y >= image.rows)
		return p;

	uchar *data = (uchar *)image.data;
	int idx = p.y*image.cols + p.x;
	Point_t adjust_p = p;
	if (data[idx] <= threshold_val) {
		adjust_p = findClosestWhitePoint(p, diff_x, diff_y);
		if (adjust_p.avail == NONE)
			return p;
	}

	return findCenterPoint(adjust_p, diff_x, diff_y, flag);
}

Point_t Image::findClosestWhitePoint(const Point_t &p, int diff_x, int diff_y) {
	uchar *data = (uchar *)image.data;
	int idx = p.y * image.cols + p.x;;

	// set unit box size
	int unit_y_up, unit_y_down, unit_x_right, unit_x_left;

	if (diff_x == 1 || diff_x == 0 || diff_x == -1)
		unit_x_left = unit_x_right = ceil(basic_distance / 2.0);
	else if (diff_x > 0) {
		unit_x_left = ceil((p.x - points[center_x + diff_x - 1][center_y].x) / 2.0);
		unit_x_right = ceil(unit_x_left * prop);
	}
	else if (diff_x < 0) {
		unit_x_right = ceil((points[center_x + diff_x + 1][center_y].x - p.x) / 2.0);
		unit_x_left = ceil(unit_x_right * prop);
	}

	if (diff_y == 1 || diff_y == 0 || diff_y == -1)
		unit_y_up = unit_y_down = ceil(basic_distance / 2.0);
	else if (diff_y > 0) {
		unit_y_up = ceil((p.y - points[center_x][center_y + diff_y - 1].y) / 2.0);
		unit_y_down = ceil(unit_y_up * prop);
	}
	else if (diff_y < 0) {
		unit_y_down = ceil((points[center_x][center_y + diff_y + 1].y - p.y) / 2);
		unit_y_up = ceil(unit_y_down * prop);
	}

	// 0       1
	//   -----
	//  |     |
	//  |     |
	//   -----
	// 3       2
	//
	Point_t search_p[4] = { { p.x, p.y }, { p.x, p.y }, 
							{ p.x, p.y }, { p.x, p.y } };

	// UP, RIGHT, DOWN, LEFT
	bool search_expand[4] = { true, true, true, true };
	int expand[4] = { 0, };

	while (true) {
		// set searching vertex point
		//
		// calculate searching range
		if (search_expand[UP]) {
			if (search_p[0].y >= SEARCH_GAP && expand[UP] <= unit_y_up) {
				search_p[0].y -= SEARCH_GAP;
				search_p[1].y -= SEARCH_GAP;

				idx -= SEARCH_GAP * image.cols;
				expand[UP] += SEARCH_GAP;
			}
			else
				search_expand[UP] = false;
		}

		if (search_expand[RIGHT]) {
			if (search_p[1].x + SEARCH_GAP < image.cols && expand[RIGHT] <= unit_x_right) {
				search_p[1].x += SEARCH_GAP;
				search_p[2].x += SEARCH_GAP;

				expand[RIGHT] += SEARCH_GAP;
			}
			else
				search_expand[RIGHT] = false;
		}

		if (search_expand[DOWN]) {
			if (search_p[2].y + SEARCH_GAP < image.cols && expand[DOWN] <= unit_y_down) {
				search_p[2].y += SEARCH_GAP;
				search_p[3].y += SEARCH_GAP;

				expand[DOWN] += SEARCH_GAP;
			}
			else
				search_expand[DOWN] = false;
		}

		if (search_expand[LEFT]) {
			if (search_p[3].x >= SEARCH_GAP && expand[LEFT] <= unit_x_left) {
				search_p[3].x -= SEARCH_GAP;
				search_p[0].x -= SEARCH_GAP;

				idx -= SEARCH_GAP;
				expand[LEFT] += SEARCH_GAP;
			}
			else
				search_expand[LEFT] = false;
		}

		if (!search_expand[UP] && !search_expand[RIGHT] && !search_expand[DOWN] && !search_expand[LEFT])
			return Point_t(p.x, p.y, NONE);

		// search white point
		// if the boundary point -> skip searching
		
		// UP
		//
		if (search_expand[UP]) {
			for (int i = search_p[0].x; i < search_p[1].x; i++) {
				if (data[idx] > threshold_val)
					goto FINDWHITEPOINT;

				idx++;
			}
		}
		else
			idx += (search_p[1].x - search_p[0].x);

		// RIGHT
		//
		if (search_expand[RIGHT]) {
			for (int i = search_p[1].y; i < search_p[2].y; i++) {
				if (data[idx] > threshold_val)
					goto FINDWHITEPOINT;

				idx += image.cols;
			}
		}
		else
			idx += image.cols*(search_p[2].y - search_p[1].y);

		// DOWN
		//
		if (search_expand[DOWN]) {
			for (int i = search_p[2].x; i > search_p[3].x; i--) {
				if (data[idx] > threshold_val)
					goto FINDWHITEPOINT;

				idx--;
			}
		}
		else
			idx -= (search_p[2].x - search_p[3].x);

		// LEFT
		//
		if (search_expand[LEFT]) {
			for (int i = search_p[3].y; i > search_p[0].y; i--) {
				if (data[idx] > threshold_val)
					goto FINDWHITEPOINT;

				idx -= image.cols;
			}
		}
		else
			idx -= image.cols*(search_p[3].y - search_p[0].y);
	}

FINDWHITEPOINT:
	int x = idx % image.cols;
	int y = idx / image.cols;

	return Point_t(x, y, EXIST);
}

//
//  v0     v1
//    . . .
//    . . .
//    . . .
//  v3     v2
//
// find unit rectangle -> calc center point
Point_t Image::findCenterPoint(const Point_t &p, int diff_x, int diff_y, int flag) {
	// UP, RIGHT, DOWN, LEFT -> search expand direction
	bool search[4] = {true, true, true, true};
	int expand[4] = { 0, };
	Point_t v[4] = { { p.x, p.y }, { p.x, p.y },
					 { p.x, p.y }, { p.x, p.y } };
	
	// set unit box size
	int unit_y_up, unit_y_down, unit_x_right, unit_x_left;

	if (diff_x == 1 || diff_x == 0 || diff_x == -1)
		unit_x_left = unit_x_right = ceil(basic_distance / 2.0);
	else if (diff_x > 0) {
		unit_x_left = ceil((p.x - points[center_x + diff_x - 1][center_y].x) / 2.0);
	
		if (diff_x == 2) {
			unit_x_right = unit_x_left;
		}
		else {
			unit_x_right = ceil(unit_x_left * prop);
		}
	}
	else if (diff_x < 0) {
		unit_x_right = ceil((points[center_x + diff_x + 1][center_y].x - p.x) / 2.0);
		
		if (diff_x == -2) {
			unit_x_left = unit_x_right;
		}
		else {
			unit_x_left = ceil(unit_x_right * prop);
		}
	}

	if (diff_y == 1 || diff_y == 0 || diff_y == -1)
		unit_y_up = unit_y_down = ceil(basic_distance / 2.0);
	else if (diff_y > 0) {
		unit_y_up = ceil((p.y - points[center_x][center_y + diff_y - 1].y) / 2.0);
		
		if (diff_y == 2) {
			unit_y_down = unit_y_up;
		}
		else {
			unit_y_down = ceil(unit_y_up * prop);
		}
	}
	else if (diff_y < 0) {
		unit_y_down = ceil((points[center_x][center_y + diff_y + 1].y - p.y) / 2);
		
		if (diff_y == -2) {
			unit_y_up = unit_y_down;
		}
		else {
			unit_y_up = ceil(unit_y_down * prop);
		}
	}

	bool white = false;
	uchar *data = (uchar *)image.data;
	int idx = (v[0].y * image.cols) + v[0].x;
	while (search[UP] || search[RIGHT] || search[DOWN] || search[LEFT]) {
		///////////////////////////////////////////////////////////
		// check availability and expand unit image vertex point //
		///////////////////////////////////////////////////////////

		// UP
		if (search[UP] && v[0].y - SEARCH_GAP >= 0 && expand[UP] <= unit_y_up) {
			v[0].y -= SEARCH_GAP;
			v[1].y -= SEARCH_GAP;

			expand[UP] += SEARCH_GAP;
		}
		else
			search[UP] = false;

		// RIGHT
		if (search[RIGHT] && v[1].x + SEARCH_GAP < image.cols && expand[RIGHT] <= unit_x_right) {
			v[1].x += SEARCH_GAP;
			v[2].x += SEARCH_GAP;

			expand[RIGHT] += SEARCH_GAP;
		}
		else
			search[RIGHT] = false;

		// DOWN
		if (search[DOWN] && v[2].y + SEARCH_GAP < image.rows && expand[DOWN] <= unit_y_down) {
			v[2].y += SEARCH_GAP;
			v[3].y += SEARCH_GAP;

			expand[DOWN] += SEARCH_GAP;
		}
		else
			search[DOWN] = false;

		// LEFT
		if (search[LEFT] && v[3].x - SEARCH_GAP >= 0 && expand[LEFT] <= unit_x_left) {
			v[3].x -= SEARCH_GAP;
			v[0].x -= SEARCH_GAP;

			expand[LEFT] += SEARCH_GAP;
		}
		else
			search[LEFT] = false;

		//////////////////////////////////////////////////////////////////
		// check points of lines about up, right, down, left directions //
		//////////////////////////////////////////////////////////////////
		
		// UP
		if (search[UP]) {
			white = false;
			idx = v[0].y * image.cols + (v[0].x + 1);
			for (int i = v[0].x+1; i < v[1].x; i++) {
				if (data[idx] > threshold_val) {
					white = true;
					break;
				}

				idx++;
			}

			if (white == false)
				search[UP] = false;
		}

		// RIGHT
		if (search[RIGHT]) {
			white = false;
			idx = (v[1].y + 1) * image.cols + v[1].x;
			for (int i = v[1].y+1; i < v[2].y; i++) {
				if (data[idx] > threshold_val) {
					white = true;
					break;
				}

				idx += image.cols;
			}

			if (white == false)
				search[RIGHT] = false;
		}

		// DOWN
		if (search[DOWN]) {
			white = false;
			idx = v[2].y * image.cols + (v[2].x - 1);
			for (int i = v[2].x-1; i > v[3].x; i--) {
				if (data[idx] > threshold_val) {
					white = true;
					break;
				}

				idx--;
			}

			if (white == false)
				search[DOWN] = false;
		}

		// LEFT
		if (search[LEFT]) {
			white = false;
			idx = (v[3].y - 1) * image.cols + v[3].x;
			for (int i = v[3].y-1; i > v[0].y; i--) {
				if (data[idx] > threshold_val) {
					white = true;
					break;
				}

				idx -= image.cols;
			}

			if (white == false)
				search[LEFT] = false;
		}

		////////////////////////
		// check vertex point //
		////////////////////////
		
		// v0
		idx = v[0].y * image.cols + v[0].x;
		if (data[idx] > threshold_val && v[0].x >= SEARCH_GAP && v[0].y >= SEARCH_GAP && expand[LEFT] <= unit_x_left && expand[UP] <= unit_y_up) {
			search[LEFT] = true; search[UP] = true;
		}

		// v1
		idx = v[1].y * image.cols + v[1].x;
		if (data[idx] > threshold_val && v[1].x + SEARCH_GAP < image.cols && v[1].y >= SEARCH_GAP && expand[UP] <= unit_y_up && expand[RIGHT] <= unit_x_right) {
			search[UP] = true; search[RIGHT] = true;
		}

		// v2
		idx = v[2].y * image.cols + v[2].x;
		if (data[idx] > threshold_val && v[2].x + SEARCH_GAP < image.cols && v[2].y + SEARCH_GAP < image.cols && expand[RIGHT] <= unit_x_right && expand[DOWN] <= unit_y_down) {
			search[RIGHT] = true; search[DOWN] = true;
		}

		// v3
		idx = v[3].y * image.cols + v[3].x;
		if (data[idx] > threshold_val && v[3].x >= SEARCH_GAP && v[3].y + SEARCH_GAP < image.cols && expand[DOWN] <= unit_y_down && expand[LEFT] <= unit_x_left) {
			search[DOWN] = true; search[LEFT] = true;
		}
	}

	if (flag != NO_FLAG) {
		unit_center_v[flag][0] = v[0];
		unit_center_v[flag][1] = v[1];
		unit_center_v[flag][2] = v[2];
		unit_center_v[flag][3] = v[3];
	}

	for (int i = 0; i < 4; i++)
		vertexes[center_x + diff_x][center_y + diff_y].v[i] = v[i];

	int width = v[2].x - v[0].x + 1;
	int height = v[2].y - v[0].y + 1;
	return calcCenterOfMass(v[0], width, height);
}

Point_t Image::calcCenterOfMass(Point_t &p, int width, int height) {
	long long int w = 0;
	long long int x = 0;
	long long int y = 0;

	uchar *data = (uchar *)image.data;
	int idx = p.y * image.cols + p.x;
	for (int row_i = 0; row_i < height; row_i++) {
		for (int col_j = 0; col_j < width; col_j++) {
			if (data[idx] > threshold_val) {
				w += data[idx];
				x += data[idx] * (p.x + col_j);
				y += data[idx] * (p.y + row_i);
			}

			idx++;
		}

		idx += (image.cols - width);
	}

	return Point_t(x/static_cast<double>(w), y/static_cast<double>(w), EXIST);
}

void Image::makeRefPointsInfo() {
	ref[center_x][center_y] = center_p;
	ref[center_x][center_y].avail = NONE;

	for (int x = 1; x <= center_x; x++) {
		ref[center_x + x][center_y] = ref[center_x + (x - 1)][center_y];
		ref[center_x + x][center_y].x += basic_distance;
		ref[center_x + x][center_y].real_x += basic_distance;
		ref[center_x + x][center_y].avail = NONE;

		ref[center_x - x][center_y] = ref[center_x - (x - 1)][center_y];
		ref[center_x - x][center_y].x -= basic_distance;
		ref[center_x - x][center_y].real_x -= basic_distance;
		ref[center_x - x][center_y].avail = NONE;
	}

	for (int y = 1; y <= center_y; y++) {
		ref[center_x][center_y + y] = ref[center_x][center_y + (y - 1)];
		ref[center_x][center_y + y].y += basic_distance;
		ref[center_x][center_y + y].real_y += basic_distance;
		ref[center_x][center_y + y].avail = NONE;

		ref[center_x][center_y - y] = ref[center_x][center_y - (y - 1)];
		ref[center_x][center_y - y].y -= basic_distance;
		ref[center_x][center_y - y].real_y -= basic_distance;
		ref[center_x][center_y - y].avail = NONE;
	}

	for (int y = 1; y <= center_y; y++) {
		for (int x = 1; x <= center_x; x++) {
			ref[center_x + x][center_y + y] = Point_t(ref[center_x + x][center_y].real_x, ref[center_x][center_y + y].real_y);
			ref[center_x + x][center_y + y].avail = NONE;

			ref[center_x + x][center_y - y] = Point_t(ref[center_x + x][center_y].real_x, ref[center_x][center_y - y].real_y);
			ref[center_x + x][center_y - y].avail = NONE;

			ref[center_x - x][center_y + y] = Point_t(ref[center_x - x][center_y].real_x, ref[center_x][center_y + y].real_y);
			ref[center_x - x][center_y + y].avail = NONE;

			ref[center_x - x][center_y - y] = Point_t(ref[center_x - x][center_y].real_x, ref[center_x][center_y - y].real_y);
			ref[center_x - x][center_y - y].avail = NONE;
		}
	}
}

void Image::makeRefPointsInCircle() {
	int d;
	if (basic_distance & 0x01)
		d = (basic_distance + 1) >> 1;
	else
		d = basic_distance >> 1;

	Point_t outer_p(ref[center_x + radius_p][center_y].x + d, ref[center_x + radius_p][center_y].y + d);
	radius = getPointDistance(center_p, outer_p);

	ref[center_x][center_y].avail = EXIST;
	for (int i = 1; i <= radius_p; i++) {
		ref[center_x + i][center_y].avail = EXIST;
		ref[center_x - i][center_y].avail = EXIST;
		ref[center_x][center_y + i].avail = EXIST;
		ref[center_x][center_y - i].avail = EXIST;

		int p = sqrt(pow(static_cast<double>(radius) / basic_distance, 2) - pow((i + 0.5), 2)) - 0.5;
		for (int j = 1; j <= p; j++) {
			ref[center_x + j][center_y + i].avail = EXIST;
			ref[center_x + j][center_y - i].avail = EXIST;
			ref[center_x - j][center_y + i].avail = EXIST;
			ref[center_x - j][center_y - i].avail = EXIST;
		}
	}
}
