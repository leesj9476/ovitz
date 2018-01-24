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

	// TODO only just for odd number of point
	//       -> if even number of point, maybe error!!!!!!!!
	
	// reference dot data
	// center dot -> (0, 0)
	int col_center = COL_POINT_NUM / 2;
	int row_center = ROW_POINT_NUM / 2;

	unit_ref_points = new Point_t[COL_POINT_NUM * ROW_POINT_NUM];
	Point_t (*p)[ROW_POINT_NUM] = (Point_t(*)[COL_POINT_NUM])unit_ref_points;
	for (int row_i = 0; row_i < ROW_POINT_NUM; row_i++) {
		for (int col_j = 0; col_j < COL_POINT_NUM; col_j++) {
			p[row_i][col_j].x = COL_BASIC_DISTANCE * (col_j - col_center);
			p[row_i][col_j].y = ROW_BASIC_DISTANCE * (row_center - row_i);
		}
	}
}

Image::~Image() {
	delete unit_centre_points;
	delete unit_ref_points;
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
	center_point = adjustCenterPoint(centre_of_mass_p);

	return center_point;
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
	// TODO need to add availability about coordinate values////////////////////
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
// @warning	need to clean unit_centre_points(Point_t *) before calculate centre point
bool Image::calcCentrePoints() {
	// Point_t *unit_centre_points is needed to be cleaned
	if (unit_centre_points != NULL)
		return false;

	// Dynamically allocate array and make 2nd-access-method
	unit_centre_points = new Point_t[ROW_POINT_NUM * COL_POINT_NUM];
	Point_t (*p)[ROW_POINT_NUM] = (Point_t(*)[COL_POINT_NUM])unit_centre_points;

	// Calculate center point
	int col_center = COL_POINT_NUM / 2;
	int row_center = ROW_POINT_NUM / 2;
	p[row_center][col_center] = getCenterPoint();

	// calc x-axis points
	int x_distance = ROW_BASIC_DISTANCE;
	Point_t expected_right_p;
	Point_t expected_left_p;
	for (int i = 0; i < col_center; i++) {
		expected_right_p = p[row_center][col_center + i];
		expected_right_p.x += x_distance;
		p[row_center][col_center + i + 1] = adjustCenterPoint(expected_right_p);

		expected_left_p = p[row_center][col_center - i];
		expected_left_p.x -= x_distance;
		p[row_center][col_center - i - 1] = adjustCenterPoint(expected_left_p);

		x_distance *= REDUCE_RATIO;
	}

	// calc y-axis points
	int y_distance = COL_BASIC_DISTANCE;
	Point_t expected_up_p;
	Point_t expected_down_p;
	for (int i = 0; i < row_center; i++) {
		expected_up_p = p[row_center - i][col_center];
		expected_up_p.y -= y_distance;
		p[row_center - i - 1][col_center] = adjustCenterPoint(expected_up_p);

		expected_down_p = p[row_center + i][col_center];
		expected_down_p.y += y_distance;
		p[row_center + i + 1][col_center] = adjustCenterPoint(expected_down_p);

		y_distance *= REDUCE_RATIO;
	}

	// calc other points
	Point_t expected_p;
	for (int row_i = 0; row_i < ROW_POINT_NUM; row_i++) {
		for (int col_j = 0; col_j < COL_POINT_NUM; col_j++) {
			if (p[row_i][col_j].x != 0 && p[row_i][col_j].y != 0)
				continue;

			expected_p.x = p[row_center][col_j].x;
			expected_p.y = p[row_i][col_center].y;

			p[row_i][col_j] = adjustCenterPoint(expected_p);
		}
	}

	// center point -> (0, 0)
	// x,y-axis coordinate
	for (int i = 0; i < ROW_POINT_NUM * COL_POINT_NUM; i++) {
		unit_centre_points[i].x -= center_point.x;
		unit_centre_points[i].y = center_point.y - unit_centre_points[i].y;

		cout << unit_centre_points[i];
		if (i % COL_POINT_NUM == COL_POINT_NUM - 1)
			cout << endl;
	}

	return true;
}

// Calculate variance of x and y between basic and centre points
void Image::calcVariance() {
	// Calculate variance point-by-point
	variance = new Variance_t[ROW_POINT_NUM][COL_POINT_NUM];
	for (int i = 0; i < ROW_POINT_NUM * COL_POINT_NUM; i++) {
		variance[i].x = unit_centre_points[i].x - unit_ref_points[i].x;
		variance[i].y = unit_centre_points[i].y - unit_ref_points[i].y;
	}
}
