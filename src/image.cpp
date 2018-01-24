#include <opencv2/highgui/highgui.hpp>
#include <iostream>
#include <string>
#include <cmath>

#include "types.h"
#include "image.h"
#include "util.h"

using namespace std;
using namespace cv;

Point_t::Point_t(double x, double y)
	: x(x), y(y) {}

Point_t Point_t::operator=(const Point_t &p) {
	this->x = p.x;
	this->y = p.y;

	return *this;
}

ostream& operator<<(ostream &os, const Point_t &p) {
	cout << "( " << p.x << ", " << p.y << " ) ";
	return os;
}

Image::Image(const string &filename, int flags) 
	: image_filename(filename), flag(flags),
	  unit_centre_points(NULL), unit_ref_points(NULL) {

	image = imread(filename, flags);
	
	// TODO hardcoded standard center point
	///////////////////////////////////////////////////////////////////////////////////////////////////////////
	unit_ref_points = new Point_t[COL_POINT_NUM * ROW_POINT_NUM];
	Point_t (*p)[ROW_POINT_NUM] = (Point_t(*)[COL_POINT_NUM])unit_ref_points;

	// hard coded center point dot
	int row_magic_point[ROW_POINT_NUM] = { 52, 147, 252, 357, 452 }; 
	int col_magic_point[COL_POINT_NUM] = { 52, 147, 252, 357, 452 };
	for (int row_i = 0; row_i < ROW_POINT_NUM; row_i++) {
		for (int col_j = 0; col_j < COL_POINT_NUM; col_j++) {
			p[row_i][col_j].x = col_magic_point[col_j];
			p[row_i][col_j].y = row_magic_point[row_i];
		}
	}

	// hard coded distance
	row_distance = new int[(ROW_POINT_NUM + 1) / 2];
	for (int i = 0; i < (ROW_POINT_NUM + 1) / 2; i++) {
		if (i == 0)
			row_distance[0] = ROW_BASIC_DISTANCE;
		else
			row_distance[i] = static_cast<double>(row_distance[i - 1]) * REDUCE_RATIO;
	}

	col_distance = new int[(COL_POINT_NUM + 1) / 2];
	for (int i = 0; i < (COL_POINT_NUM + 1) / 2; i++) {
		if (i == 0)
			col_distance[0] = COL_BASIC_DISTANCE;
		else
			col_distance[i] = static_cast<double>(col_distance[i - 1]) * REDUCE_RATIO;
	}
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////
}

Image::~Image() {
	delete unit_centre_points;
	delete unit_ref_points;
	delete row_distance;
	delete col_distance;
}

// @return	size of image(<width> x <height>)
Size Image::getSize() {
	return image.size();
}

// @return	whether image is valid(opened)
bool Image::isValid() {
	return !image.empty();
}

// Calculate centre of mass point in unit image.
//
//        unit_width
//  -----------------------
// |                       |
// |          ...          | unit_height
// |          ... mass     |
// |          ...          |
// |                       |
//  -----------------------
//
// @arg		Point_t p		starting point
// @arg		int unit_width	unit image width
// @arg		int unit_height	unit image height
//
// @return	centre of mass point by unit
Point_t Image::getCentreOfMass(const Point_t &p, int unit_width, int unit_height) {
	// Get pixel data to 1-st array.
	uchar *data = (uchar *)image.data;

	// Sum weight(pixel value), x, y for calculation.
	int weight = 0;
	double x = 0;
	double y = 0;

	int start_y = p.y;
	if (start_y < 0)
		start_y = 0;

	int end_y = p.y + unit_height;
	if (end_y > image.cols)
		end_y = image.cols;

	int start_x = p.x;
	if (start_x < 0)
		start_x = 0;

	int end_x = p.x + unit_width;
	if (end_x > image.rows)
		end_x = image.rows;

	int idx = start_y * image.cols + start_x;
	for (int row_i = start_y; row_i < end_y; row_i++) {
		for (int col_j = start_x; col_j < end_x; col_j++) {
			if (data[idx]) {
				int w = data[idx];

				weight += w;
				x += col_j * w;
				y += row_i * w;
			}

			idx++;
		}

		idx += (image.cols - (end_x - start_x));
	}

	// Calculate x, y coordinate value.
	x /= weight;
	y /= weight;

	return Point_t(x, y);
}

// Calculate center point of whole image by using centre of mass.
// If points of image is biased, need to be adjusted slightly.
//
//         image.cols
//  -----------------------
// |    .      .      .    |
// |                       | image.rows
// |    .      .      .    |
// |                       |
// |    .      .      .    |
//  -----------------------
//
// @return	center point though adjustment
Point_t Image::getCenterPoint() {
	Point_t p(0, 0);
	Point_t centre_of_mass_p = getCentreOfMass(p, image.cols, image.rows);

	return adjustCenterPoint(centre_of_mass_p);
}

// If the result of centre of mass by whole image is black pixel
// find the closest point(not balck pixel) to calculate real center point.
//
// @arg		Point_t &p	the result of centre of mass from whole image
//
// @return	the closest white point(pixel value > 0)
//
// TODO 1-pass-1 search -> n-pass-n search
// TODO check avilability coordinate values
Point_t Image::getClosestWhitePoint(const Point_t &p) {
	uchar *data = (uchar *)image.data;

	int d = 0;
	int idx = p.y * image.cols + p.x;
	while (true) {
		d += 2;
		idx -= (image.cols + 1);

		// UP
		for (int i = 0; i < d; i++) {
			if (data[idx])
				goto FINDWHITEPOINT;

			idx += 1;
		}

		// RIGHT
		for (int i = 0; i < d; i++) {
			if (data[idx])
				goto FINDWHITEPOINT;

			idx += image.cols;
		}

		// DOWN
		for (int i = 0; i < d; i++) {
			if (data[idx])
				goto FINDWHITEPOINT;

			idx -= 1;
		}

		// LEFT
		for (int i = 0; i < d; i++) {
			if (data[idx])
				goto FINDWHITEPOINT;

			idx -= image.cols;
		}
	}

FINDWHITEPOINT:
	int x = idx % image.cols;
	int y = idx / image.cols;

	return Point_t(x, y);
}

// Adjust center point(result of upper function, getCentreOfMassByWhole).
// If lazer image is biased, the result of centre of mass can be different
// with the real center point slightly.
//
//         image.cols
//  -----------------------
// | .   .   .             |
// | .   .   .             | image.rows
// | .   .   .             |
// |                       |
// |                       |
//  -----------------------
//
// @arg		Point_t &p	the result of centre of mass or
// 						the closest white point from the result
//
// @return	the result of adjustment of centre of mass
Point_t Image::adjustCenterPoint(const Point_t &p) {
	uchar *data = (uchar *)image.data;
	int idx = p.y * image.cols + p.x;

	// [UP, RIGHT, DOWN, LEFT] direction -> expanding
	bool search[4] = { true, true, true, true };

	// find closest white point from the calculated centre of mass point p
	Point_t basic_p = p;
	if (data[idx] == BLACK_PIXEL) {
		basic_p = getClosestWhitePoint(p);
	}

	// 0       1
	//   -----
	//  |     |
	//  |     |
	//   -----
	// 3       2
	//
	// TODO need to add availability about coordinate values///////////////////////////////////////////////////////////////
	Point_t std_p[4] = { {basic_p.x - 1, basic_p.y - 1}, {basic_p.x + 1, basic_p.y - 1},
						 {basic_p.x + 1, basic_p.y + 1}, {basic_p.x - 1, basic_p.y + 1} };

	int white = 0;
	while (search[UP] || search[RIGHT] || search[DOWN] || search[LEFT]) {
		// search lines, excluding vertex
		// UP
		white = 0;
		if (search[UP]) {
			idx = std_p[0].y * image.cols + std_p[0].x + 1;
			for (int i = std_p[0].x + 1; i < std_p[1].x; i++) {
				if (data[idx])
					white++;

				idx += 1;
			}

			if (white == 0)
				search[UP] = false;
		}

		// RIGHT
		white = 0;
		if (search[RIGHT]) {
			idx = (std_p[1].y + 1) * image.cols + std_p[1].x;
			for (int i = std_p[1].y + 1; i < std_p[2].y; i++) {
				if (data[idx])
					white++;

				idx += image.cols;
			}
				
			if (white == 0)
				search[RIGHT] = false;
		}

		// DOWN
		white = 0;
		if (search[DOWN]) {
			idx = std_p[3].y * image.cols + std_p[3].x + 1;
			for (int i = std_p[3].x + 1; i < std_p[2].x; i++) {
				if (data[idx])
					white++;

				idx += 1;
			}

			if (white == 0)
				search[DOWN] = false;
		}

		// LEFT
		white = 0;
		if (search[LEFT]) {
			idx = (std_p[0].y + 1) * image.cols + std_p[0].x;
			for (int i = std_p[0].y + 1; i < std_p[3].y; i++) {
				if (data[idx])
					white++;

				idx += image.cols;
			}

			if (white == 0)
				search[LEFT] = false;
		}

		// vertex check
		// point 0
		idx = std_p[0].y * image.cols + std_p[0].x;
		if (data[idx]) {
			search[LEFT] = true; search[UP] = true;
		}
		// point 1
		idx = std_p[1].y * image.cols + std_p[1].x;
		if (data[idx]) {
			search[UP] = true; search[RIGHT] = true;
		}
		// point 2
		idx = std_p[2].y * image.cols + std_p[2].x;
		if (data[idx]) {
			search[RIGHT] = true; search[DOWN] = true;
		}
		// point 3
		idx = std_p[3].y * image.cols + std_p[3].x;
		if (data[idx]) {
			search[DOWN] = true; search[LEFT] = true;
		}

		// adjust point x, y
		if (search[UP]) {
			std_p[0].y--; std_p[1].y--;
		}
		if (search[RIGHT]) {
			std_p[1].x++; std_p[2].x++;
		}
		if (search[DOWN]) {
			std_p[2].y++; std_p[3].y++;
		}
		if (search[LEFT]) {
			std_p[3].x--; std_p[0].x--;
		}
	}

	return getCentreOfMass(std_p[0], std_p[2].x - std_p[0].x + 1, std_p[2].y - std_p[0].y + 1);
}

// Calculate all centre of mass points
//
// @return	only when not initiated unit_centre_points return false
// 
// @lemma	point-to-point distance
// = BASIC_DISTANCE*(REDUCE_RATIO^0 + ... + REDUCE_RATIO^(abs_deviation - 1)) + POINT_SIZE*ABS(DEVIATION)
// = (BASIC_DISTANCE - BASIC_DISTANCE*(REDUCE_RATIO^abs_deviation)) / (1 - REDUCE_RATIO) + POINT_SIZE*ABS(DEVIATION)
//
// @warning	need to clean unit_centre_points(Point_t *) before calculate centre point
bool Image::calcCentrePoints() {
	// Point_t *unit_centre_points is needed to be cleaned
	if (unit_centre_points != NULL)
		return false;

	// Dynamically allocate array and make 2nd-access-method
	unit_centre_points = new Point_t[ROW_POINT_NUM * COL_POINT_NUM];
	Point_t (*p)[ROW_POINT_NUM] = (Point_t(*)[COL_POINT_NUM])unit_centre_points;

	// Calculate center point
	int center_x = COL_POINT_NUM / 2;
	int center_y = ROW_POINT_NUM / 2;
	p[center_x][center_y] = getCenterPoint();

	// column -> y
	for (int row_i = 0; row_i < ROW_POINT_NUM; row_i++) {
		// The n-th point from center point
		int deviation_y = row_i - center_y;
		int abs_deviation_y = abs(deviation_y);

		// Calculate point-to-point distance.
		int distance_deviation_y = (ROW_BASIC_DISTANCE - row_distance[abs_deviation_y]) / (1 - REDUCE_RATIO) + POINT_SIZE * abs_deviation_y;
		if (deviation_y < 0)
			distance_deviation_y *= -1;

		// Calculate search height for centre of mass.
		int expected_y = p[center_x][center_y].y + distance_deviation_y;
		int search_y_up, search_y_down;
		if (deviation_y < 0) {
			search_y_up = row_distance[abs_deviation_y] / 2;
			search_y_down = row_distance[abs_deviation_y - 1] / 2;
		}
		else if (deviation_y > 0) {
			search_y_up = row_distance[abs_deviation_y - 1] / 2;
			search_y_down = row_distance[abs_deviation_y] / 2;
		}
		else {
			search_y_up = search_y_down = row_distance[0] / 2;
		}

		// Result argument of y(required for calculating centre of mass)
		int start_calc_y = expected_y - search_y_up - POINT_SIZE / 2;
		int calc_height = search_y_up + search_y_down + POINT_SIZE;

		// column -> x
		for (int col_j = 0; col_j < COL_POINT_NUM; col_j++) {
			// The center point is already calculated
			if (row_i == center_y && col_j == center_x){
				continue;
			}

			// The n-th point from center point
			int deviation_x = col_j - center_x;
			int abs_deviation_x = abs(deviation_x);

			
			// Calculate point-to-point distance
			int distance_deviation_x = (COL_BASIC_DISTANCE - col_distance[abs_deviation_x]) / (1 - REDUCE_RATIO) + POINT_SIZE * abs_deviation_x;
			if (deviation_x < 0)
				distance_deviation_x *= -1;

			// Calculate search width for centre of mass
			int expected_x = p[center_x][center_y].x + distance_deviation_x;
			int search_x_left, search_x_right;
			if (deviation_x < 0) {
				search_x_left = col_distance[abs_deviation_x] / 2;
				search_x_right = col_distance[abs_deviation_x - 1] / 2;
			}
			else if (deviation_x > 0) {
				search_x_left = col_distance[abs_deviation_x - 1] / 2;
				search_x_right = col_distance[abs_deviation_x] / 2;
			}
			else {
				search_x_left = search_x_right = col_distance[0] / 2;
			}

			// Result argument of x(required for calculating centre of mass)
			int start_calc_x = expected_x - search_x_left - POINT_SIZE / 2;
			int calc_width = search_x_left + search_x_right + POINT_SIZE;

			// Calculate centre of mass -> our short-term goal is finished!!///
			Point_t start_p(start_calc_x, start_calc_y);
			p[row_i][col_j] = getCentreOfMass(start_p, calc_width, calc_height);
		}
	}

	return true;
}

// Calculate distance between basic and centre points
//
// @return	distance array
double* Image::calcCentrePointsDistance() {
	// Calculate distance 1-by-1
	return calcVecDistance(unit_centre_points, unit_ref_points);
}
