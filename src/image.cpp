#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>

#include <fstream>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <string>
#include <cmath>

#include "types.h"
#include "image.h"
#include "util.h"

using namespace std;
using namespace cv;

extern double *lens_mat[5][5];

// Point_t constructor using double x, y value
Point_t::Point_t(double real_x_, double real_y_, int avail_)
	: real_x(real_x_), real_y(real_y_), avail(avail_) {

	x = static_cast<int>(real_x);
	y = static_cast<int>(real_y);
}

// Point_t constructor using int x, y value
Point_t::Point_t(int x_, int y_, int avail_)
	: x(x_), y(y_), avail(avail_) {

	real_x = static_cast<double>(x);
	real_y = static_cast<double>(y);
}

// Point_t copy operator= overloading
Point_t Point_t::operator=(const Point_t &p) {
	this->x = p.x;
	this->y = p.y;
	this->real_x = p.real_x;
	this->real_y = p.real_y;
	this->avail = p.avail;

	return *this;
}

// Point_t compare operator== overloading
bool Point_t::operator==(const Point_t &p) {
	return (x == p.x && y == p.y);
}

// define Point_t print stream
// form => (x,y)
ostream& operator<<(ostream &os, const Point_t &p) {
	cout << "(" << setw(4) << p.x << "," << setw(4) << p.y << ")";
	return os;
}

// Image constructor using image filename from option
Image::Image(Options &options) {
	opt = options;

	image = imread(opt.image_filename, CV_LOAD_IMAGE_GRAYSCALE);
	original = imread(opt.image_filename);

	int_basic_distance = static_cast<int>(opt.basic_distance);

	ref_point_num = -1;

	points = NULL;
	ref = NULL;
	vertex = NULL;
}

// Image constructor using cv::Mat structure
Image::Image(const Mat &image_, Options &options) {
	opt = options;

	image_.copyTo(original);
	convertRGBtoGRAY(original);

	int_basic_distance = static_cast<int>(opt.basic_distance);

	ref_point_num = -1;

	points = NULL;
	ref = NULL;
	vertex = NULL;
}

// Image destructor, free all dynamic allocated datum
Image::~Image() {
	for (int i = 0; i < point_row; i++) {
		delete[] points[i];
		delete[] vertex[i];
	}

	delete[] points;
	delete[] vertex;
}

// Initialize datum using input(image, options)
void Image::init(){

	// theoretically maximum number of points(column, row)
	point_col = image.cols / int_basic_distance;
	point_row = image.rows / int_basic_distance;

	// make odd number(bigger than current value)
	if ((point_col & 0x01) == 0x00)
		point_col++;
	if ((point_row & 0x01) == 0x00)
		point_row++;

	// center point's index is the half of point number calculated upper lines
	center_x = point_col >> 1;
	center_y = point_row >> 1;

	// if datum are pre-allocated, deallocate them
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

	if (vertex) {
		for (int i = 0; i < point_row; i++) {
			if (vertex[i])
				delete[] vertex[i];
		}

		delete[] vertex;
	}

	// allocate 2d array structures related to search points
	points = new Point_t*[point_row];
	for (int i = 0; i < point_row; i++)
		points[i] = new Point_t[point_col];

	ref = new Point_t*[point_row];
	for (int i = 0; i < point_row; i++)
		ref[i] = new Point_t[point_col];

	vertex = new Vertex_t*[point_row];
	for (int i = 0; i < point_row; i++)
		vertex[i] = new Vertex_t[point_col];
}

// Change old image to new image
void Image::changeImage(Mat &new_image) {
	new_image.copyTo(original);
	convertRGBtoGRAY(original);
}

// Change image to gray scale
void Image::convertRGBtoGRAY(Mat &original) {
	cvtColor(original, image, CV_RGB2GRAY);
}

// Calculate CDF using pixel values
// If THRESHOLD_AREA option is true, it will use only the center box of image
// center box size = (2 x threshold_area + 1) x (2 x threshold_area + 1)
void Image::makePixelCDF() {
	uchar *data = (uchar *)image.data;

	int col_start = 0;
	int col_end = image.cols;
	int row_start = 0;
	int row_end = image.rows;

	// Calculate center box size
	if (option[THRESHOLD_AREA]) {
		int cen_p_x = image.cols / 2;
		int cen_p_y = image.rows / 2;

		col_start = cen_p_x - opt.threshold_area;
		col_end = cen_p_x + opt.threshold_area;

		row_start = cen_p_y - opt.threshold_area;
		row_end = cen_p_y + opt.threshold_area;

		if (col_start < 0 || row_start < 0 ||
			col_end >= image.cols || row_end >= image.rows) {
	
			col_start = 0;
			col_end = image.cols;
			row_start = 0;
			row_end = image.rows;
		}
	}

	// Make distribution data using pixel values
	memset(cdf, 0, sizeof(cdf));
	for (int i = row_start; i <= row_end; i++) {
		int idx = i * image.cols + col_start;

		for (int j = col_start; j <= col_end; j++) {
			cdf[data[idx]]++;
			idx++;
		}
	}

	// Make data values to percent
	int total_pixel = row_end * col_end;
	for (int i = 0; i < 256; i++) {
		cdf[i] /= total_pixel;
	}

	// Make data to CDF(cumulative distribution function)
	for (int i = 1; i < 256; i++) {
		cdf[i] += cdf[i - 1];
	}
}

// Find index(cdf[i] > percent p => return smallest i)
int Image::getValCDF(double p) {
	int i = 0;
	for (i = 0; i < 256; i++) {
		if (cdf[i] > p)
			break;
	}

	return i;
}

// To remove noise, use gaussian filter(size => 3 x 3)
void Image::gaussianFiltering() {
	GaussianBlur(image, image, Size(3, 3), 0);
}

// find all points
string Image::findAllPoints() {
	if (filename != "") {
		gaussianFiltering();
		makePixelCDF();
	}

	// calcuate threshold value
	if (option[THRESHOLD_TOP_P])
		threshold_val = getValCDF(opt.threshold_top_p / 100.0);
	else
		threshold_val = getValCDF(0.50) * (opt.threshold_p / 100.0);

	if (threshold_val > 255)
		threshold_val = 255;

	// If pixel value is under threshold value, set to 0
	threshold(image, image, threshold_val, 255, 3);

	// Set point structure's avail value to NONE(unfounded)
	setAllPointsToNONE();
	
	// Find x-axis and y-axis points
	findAllAxisPoints();

	// Calculate the spot of point(the case of perfect wave front)
	// and calculate the point that we need to find in the circle
	// (standard = the shortest axis(x or y, + or -)
	int new_ref_points_num = makeRefPointsInfo();

	// Find all points
	// Predicted point spot is calculated from axis point(projected to axis)
	for (int y = 1; y <= radius_p; y++) {
		for (int x = 1; x <= radius_p; x++) {
			if (ref[center_x + x][center_y + y].avail == NONE)
				break;

			points[center_x + x][center_y + y] = 
				adjustPoint(Point_t(points[center_x + x][center_y].x, 
							        points[center_x][center_y + y].y), 
						    x, y);

			points[center_x + x][center_y - y] = 
				adjustPoint(Point_t(points[center_x + x][center_y].x, 
							        points[center_x][center_y - y].y), 
						    x, -y);

			points[center_x - x][center_y + y] = 
				adjustPoint(Point_t(points[center_x - x][center_y].x, 
							        points[center_x][center_y + y].y),
						    -x, y);

			points[center_x - x][center_y - y] = 
				adjustPoint(Point_t(points[center_x - x][center_y].x, 
							        points[center_x][center_y - y].y),
						    -x, -y);
		}
	}

	// Make analysis result picture
	for (int i = 0; i < point_row; i++) {
		for (int j = 0; j < point_col; j++) {

			// In case of founded point, draw searched box and the center point
			if (ref[i][j].avail == EXIST && points[i][j].avail == EXIST) {
				line(original, Point(vertex[i][j].v[0].x, vertex[i][j].v[0].y),
						       Point(vertex[i][j].v[1].x, vertex[i][j].v[1].y),
							   Scalar(255, 0, 0));

				line(original, Point(vertex[i][j].v[1].x, vertex[i][j].v[1].y),
						       Point(vertex[i][j].v[2].x, vertex[i][j].v[2].y),
							   Scalar(255, 0, 0));

				line(original, Point(vertex[i][j].v[2].x, vertex[i][j].v[2].y),
						       Point(vertex[i][j].v[3].x, vertex[i][j].v[3].y),
							   Scalar(255, 0, 0));

				line(original, Point(vertex[i][j].v[3].x, vertex[i][j].v[3].y),
						       Point(vertex[i][j].v[0].x, vertex[i][j].v[0].y),
							   Scalar(255, 0, 0));

				original.at<Vec3b>(points[i][j].y, points[i][j].x) = { 255, 255, 255 };
			}
		}
	}

	// Draw the circle
	circle(original, Point(center_p.x, center_p.y), radius, Scalar(0, 255, 0));

	// If SHOW_WINDOW option is true, show the result to window
	if (option[SHOW_WINDOW]) {
		imshow("result", original);
		waitKey(20);
	}

	// the calculated maximum radius point number is 5
	int p = radius_p > 5 ? 5 : radius_p;

	// If ref point number is changed(number of point in circle is changed),
	// delete pre-allocated diff array and newly allocate
	if (ref_point_num != new_ref_point_num) {
		ref_point_num = new_ref_point_num;

		if (diff != NULL)
			delete[] diff;

		diff = new double[ref_points_num * 2];
	}

	// calculate diff data about x and y
	// form => diff(p0 x) diff(p1 x) ... diff(p0 y) diff(p1 y) ...
	int diff_i = 0;
	for (int y = center_y - p; y <= center_y + p; y++) {
		for (int x = center_x - p; x <= center_x + p; x++) {

			// In case of existing point theoretically
			if (ref[x][y].avail == EXIST) {
				diff[diff_i] = points[x][y].real_x - ref[x][y].real_x; // diff x
				diff[diff_i + ref_num] = -(points[x][y].real_y - ref[x][y].real_y); // diff y

				diff_i++;
			}
		}
	}

	// ccd_pixel = max_row(1944) / cur_row(image's row) * real_pixel_size
	double w = 1944 / image.rows * pixel_size / focal;
	for (int i = 0; i < ref_points_num * 2; i++) {
		diff[i] *= w;
	}
	
	// calculate result values
	string result_str = "";
	int mat_num = p - 1;
	double result[5] = { 0,  };
	for (int i = 0; i < 5; i++) {
		for (int j = 0; j < ref_num * 2; j++) {
			result[i] += lens_mat[mat_num][i][j] * diff[j];
		}
	}
	
	for (int i = 0; i < 5; i++) {
		result[i] *= (-1);
	}

	// Make the result's precision to 3 under point
	stringstream result_precision[3];
	result_precision[0] << fixed << setprecision(3) << result[2];
	result_precision[1] << fixed << setprecision(3) << result[3];
	result_precision[2] << fixed << setprecision(3) << result[4];

	// Make result string that will be printed to led screen
	result_str = "3: " + result_precision[0].str() + "\n" +
		         "4: " + result_precision[1].str() + "\n" +
				 "5: " + result_precision[2].str();

	return result_str;
}

// Set point structure's avail value to NONE(unfounded)
void Image::setAllPointsToNONE() {
	for (int i = 0; i < point_row; i++) {
		for (int j = 0; j < point_col; j++) 
			points[i][j].avail = NONE;
	}
}

// Find all points on x axis and y axis and set the standard
// radius point number(the smallest point number x or y, + or -)
void Image::findAllAxisPoints() {

	// Calculate center point
	Point_t zero_p;
	Point_t expected_center_p = calcCenterOfMass(zero_p, image.cols, image.rows);
	center_p = adjustPoint(expected_center_p, 0, 0);
	points[center_x][center_y] = center_p;

	// Find 1st axis point using basic distance
	points[center_x + 1][center_y] =
		adjustPoint(Point_t(center_p.x + int_basic_distance, center_p.y), 1, 0, RIGHT);

	points[center_x - 1][center_y] =
		adjustPoint(Point_t(center_p.x - int_basic_distance, center_p.y), -1, 0, LEFT);

	points[center_x][center_y + 1] = 
		adjustPoint(Point_t(center_p.x, center_p.y + int_basic_distance), 0, 1, DOWN);

	points[center_x][center_y - 1] = 
		adjustPoint(Point_t(center_p.x, center_p.y - int_basic_distance), 0, -1, UP);

	// Find 2nd axis point using linear search from 1st point through axis
	uchar *data = (uchar *)image.data;
	bool searching[4] = { true, true, true, true };

	// if picture's row or column value is very small(< 5),
	// cannot find more points
	if (point_row < 5) {
		searching[UP] = false;
		searching[DOWN] = false;
	}

	if (point_col < 5) {
		searching[RIGHT] = false;
		searching[LEFT] = false;
	}

	int gap = 0;
	int idx = 0;
	while (searching[UP] || searching[RIGHT] || searching[DOWN] || searching[LEFT]) {
		gap++;

		// if searching gap >= (int)basic_distance,
		// set the spot using previous points
		if (gap >= int_basic_distance) {
			// UP
			if (points[center_x][center_y - 2].avail == NONE) {
				points[center_x][center_y - 2].x = points[center_x][center_y - 1].x;
				points[center_x][center_y - 2].y = points[center_x][center_y - 1].y - int_basic_distance;
			}
			
			// RIGHT
			if (points[center_x + 2][center_y].avail == NONE) {
				points[center_x + 2][center_y].x = points[center_x + 1][center_y].x + int_basic_distance;
				points[center_x + 2][center_y].y = points[center_x + 1][center_y].y;
			}
			
			// DOWN
			if (points[center_x][center_y + 2].avail == NONE) {
				points[center_x][center_y + 2].x = points[center_x][center_y + 1].x;
				points[center_x][center_y + 2].y = points[center_x][center_y + 1].y + int_basic_distance;
			}
			
			// LEFT
			if (points[center_x - 2][center_y].avail == NONE) {
				points[center_x - 2][center_y].x = points[center_x - 1][center_y].x;
				points[center_x - 2][center_y].y = points[center_x - 1][center_y].y - int_basic_distance;
			}
			
			break;
		}
		
		// Check boundary and if point is needed to be searched
		// The unit_center_v value is set, when the 1st axis point is searched
		// .       .   .   .   .   .   .
		//
		//     p          == search >>
		//
		// .       .   .   .   .   .   .
		
		// UP
		if (search[UP]) {
			if (unit_center_v[UP][0].y - gap >= 0) {
				idx = (unit_center_v[UP][0].y - gap) * image.cols + unit_center_v[UP][0].x;
				for (int i = unit_center_v[UP][0].x; i <= unit_center_v[UP][1].x; i++) {

					// The white point is founded
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
		}

		// RIGHT
		if (search[RIGHT]) {
			if (unit_center_v[RIGHT][1].x + gap < image.cols) {
				idx = unit_center_v[RIGHT][1].y * image.cols + (unit_center_v[RIGHT][1].x + gap);
				for (int i = unit_center_v[RIGHT][1].y; i <= unit_center_v[RIGHT][2].y; i++) {

					// The white point is founded
					if (data[idx] > threshold_val) {
						points[center_x + 2][center_y] = adjustPoint(Point_t(unit_center_v[RIGHT][1].x + gap, i), 2, 0);
						search[RIGHT] = false;	

						break;
					}

					idx += image.cols;
		
				}
			}
			else
				search[RIGHT] = false;
		}

		// DOWN
		if (search[DOWN] && unit_center_v[DOWN][2].y + gap < image.rows) {
			if (unit_center_v[DOWN][2].y + gap < image.rows) {
				idx = (unit_center_v[DOWN][2].y + gap) * image.cols + unit_center_v[DOWN][2].x;
				for (int i = unit_center_v[DOWN][2].x; i >= unit_center_v[DOWN][3].x; i--) {

					// The white point is founded
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
		}

		// LEFT
		if (search[LEFT] && unit_center_v[LEFT][3].x - gap >= 0) {
			if (unit_center_v[LEFT][3].x - gap >= 0) {
				idx = unit_center_v[LEFT][3].y * image.cols + (unit_center_v[LEFT][3].x - gap);
				for (int i = unit_center_v[LEFT][3].y; i >= unit_center_v[LEFT][0].y; i--) {

					// The white point is founded
					if (data[idx] > threshold_val) {
						points[center_x - 2][center_y] = adjustPoint(Point_t(unit_center_v[LEFT][3].x - gap, i), -2, 0);
						search[LEFT] = false;

						break;
					}

					idx -= image.cols;
				}
			}
			else
				search[LEFT] = false;
		}
	}

	// If at least one direction's 2nd axis point is not founded,
	// the effective point is 1st axis points.
	int min_p = (center_x < center_y) ? center_x : center_y;
	if (points[center_x + 2][center_y].avail == NONE ||
		points[center_x - 2][center_y].avail == NONE ||
		points[center_x][center_y + 2].avail == NONE ||
		points[center_x][center_y - 2].avail == NONE)

		min_p = 1;
	

	// If the all 2nd axis point is founded(effective)
	if (min_p != 1) {

		// Calculate reduction proportion
		// The distance from previous point to searching point decrease
		// for the fixed reduction proportion.
		double reduce_p[4] = { 0, 0, 0, 0 };

		// in the above case(min_p != 1), all the 2nd axis points are EXIST
		reduce_p[UP] = (points[center_x][center_y - 1].y - points[center_x][center_y - 2].y) /
			            static_cast<double>(int_basic_distance);
		reduce_p[DOWN] = (points[center_x][center_y + 2].y - points[center_x][center_y + 1].y) /
			              static_cast<double>(int_basic_distance);
		reduce_p[RIGHT] = (points[center_x + 2][center_y].x - points[center_x + 1][center_y].x) /
			               static_cast<double>(int_basic_distance);
		reduce_p[LEFT] = (points[center_x - 1][center_y].x - points[center_x - 2][center_y].x) /
			              static_cast<double>(int_basic_distance);

		reduction_prop = (reduce_p[UP] + reduce_p[RIGHT] + reduce_p[DOWN] + reduce_p[LEFT]) / 4;
	
		// Find all axis points
		int d[4];

		// UP, DOWN
		for (int i = 3; i <= min_p; i++) {
			// Find up, down axis points
			d[UP] = ceil((points[center_x][center_y - i + 2].y - points[center_x][center_y - i + 1].y) * reduction_prop);
			d[DOWN] = ceil((points[center_x][center_y + i - 1].y - points[center_x][center_y + i - 2].y) * reduction_prop);
	
			// UP
			points[center_x][center_y - i] = adjustPoint(Point_t(points[center_x][center_y - i + 1].x,
						                                         points[center_x][center_y - i + 1].y - d[UP]),
					                                     0, -i);
			
			// DOWN
			points[center_x][center_y + i] = adjustPoint(Point_t(points[center_x][center_y + i - 1].x,
						                                         points[center_x][center_y + i - 1].y + d[DOWN]),
					                                     0, i);
	
			if (points[center_x][center_y - i].avail == NONE ||
				points[center_x][center_y + i].avail == NONE)
				
				min_p = i - 1;
		}
	
		// RIGHT, LEFT
		for (int i = 3; i <= min_p; i++) {
			// Find right, left axis points
			d[RIGHT] = ceil((points[center_x + i - 1][center_y].x - points[center_x + i - 2][center_y].x) * reduction_prop);
			d[LEFT] = ceil((points[center_x - i + 2][center_y].x - points[center_x - i + 1][center_y].x) * reduction_prop);
	
			// RIGHT
			points[center_x + i][center_y] = adjustPoint(Point_t(points[center_x + i - 1][center_y].x + d[RIGHT],
						                                         points[center_x + i - 1][center_y].y),
					                                     i, 0);
			
			// LEFT
			points[center_x - i][center_y] = adjustPoint(Point_t(points[center_x - i + 1][center_y].x - d[LEFT],
						                                         points[center_x - i + 1][center_y].y),
					                                     -i, 0);
	
			if (points[center_x + i][center_y].avail == NONE ||
				points[center_x - i][center_y].avail == NONE)
				
				min_p = i - 1;
		}
	
		// If the axis point is founded over index min_p from center point, 
		// set avail value to NONE from EXIST (EXIST -> NONE)

		// x-axis
		for (int i = min_p + 1; i <= center_x; i++) {
			if (points[center_x + i][center_y].avail == EXIST)
				points[center_x + i][center_y].avail = NONE;
	
			if (points[center_x - i][center_y].avail == EXIST)
				points[center_x - i][center_y].avail = NONE;
		}
	
		// y-axis
		for (int i = min_p + 1; i <= center_y; i++) {
			if (points[center_x][center_y + i].avail == EXIST)
				points[center_x][center_y + i].avail = NONE;
	
			if (points[center_x][center_y - i].avail == EXIST)
				points[center_x][center_y - i].avail = NONE;
		}
	}

	// Set radius Point number
	radius_p = min_p;
}

// The predicted point is needed to be adjusted
//
// If pixel value is under threshold, find the closest point over threshold
// and find the center of mass in unit box spreading from founded point. Else 
// pixel value is over theshold, just find the center of mass.
Point_t Image::adjustPoint(const Point_t &p, int diff_x, int diff_y, int flag) {

	// Check boundary
	if (p.x < 0 || p.x >= image.cols || p.y < 0 || p.y >= image.rows)
		return p;

	// Find closest white point from point p
	uchar *data = (uchar *)image.data;
	int idx = p.y*image.cols + p.x;
	Point_t adjust_p = p;

	// If pixel value is under threshold, find the closest point over threshold
	if (data[idx] <= threshold_val) {
		adjust_p = findClosestWhitePoint(p, diff_x, diff_y);

		// If searching is failed, just return point p
		if (adjust_p.avail == NONE)
			return p;
	}

	// find the center of mass in unit box spreading from founded point
	return findCenterPoint(adjust_p, diff_x, diff_y, flag);
}

//
Point_t Image::findClosestWhitePoint(const Point_t &p, int diff_x, int diff_y) {
	uchar *data = (uchar *)image.data;
	int idx = p.y * image.cols + p.x;;

	// Set maximum unit box size
	int unit_y_up, unit_y_down, unit_x_right, unit_x_left;

	// Searching point we want to find is point[center_x + diff_x][center_y+ diff_x]
	if (diff_x == 1 || diff_x == 0 || diff_x == -1)
		unit_x_left = unit_x_right = ceil(int_basic_distance / 2.0);
	else if (diff_x > 1) {
		unit_x_left = ceil((p.x - points[center_x + diff_x - 1][center_y].x) / 2.0);
		unit_x_right = ceil(unit_x_left * reduction_prop);
	}
	else if (diff_x < -1) {
		unit_x_right = ceil((points[center_x + diff_x + 1][center_y].x - p.x) / 2.0);
		unit_x_left = ceil(unit_x_right * reduction_prop);
	}

	if (diff_y == 1 || diff_y == 0 || diff_y == -1)
		unit_y_up = unit_y_down = ceil(int_basic_distance / 2.0);
	else if (diff_y > 1) {
		unit_y_up = ceil((p.y - points[center_x][center_y + diff_y - 1].y) / 2.0);
		unit_y_down = ceil(unit_y_up * reducion_prop);
	}
	else if (diff_y < -1) {
		unit_y_down = ceil((points[center_x][center_y + diff_y + 1].y - p.y) / 2);
		unit_y_up = ceil(unit_y_down * reduction_prop);
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
		unit_x_left = unit_x_right = ceil(int_basic_distance / 2.0);
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
		unit_y_up = unit_y_down = ceil(int_basic_distance / 2.0);
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
		vertex[center_x + diff_x][center_y + diff_y].v[i] = v[i];

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

int Image::makeRefPointsInfo() {
	int d;
	if (int_basic_distance & 0x01)
		d = (int_basic_distance + 1) >> 1;
	else
		d = int_basic_distance >> 1;

	Point_t outer_p(ref[center_x + radius_p][center_y].x + d,
			        ref[center_x + radius_p][center_y].y + d);

	radius = getPointDistance(center_p, outer_p);

	int ref_num = 0;

	ref[center_x][center_y] = center_p;
	ref[center_x][center_y].avail = EXIST;
	ref_num += 1;

	for (int i = 1; i <= radius_p; i++) {
		ref[center_x + i][center_y] = ref[center_x + (i - 1)][center_y];
		ref[center_x + i][center_y].x += opt.basic_distance;
		ref[center_x + i][center_y].real_x += opt.basic_distance;

		ref[center_x - i][center_y] = ref[center_x - (i - 1)][center_y];
		ref[center_x - i][center_y].x -= opt.basic_distance;
		ref[center_x - i][center_y].real_x -= opt.basic_distance;

		ref[center_x][center_y + i] = ref[center_x][center_y + (y - 1)];
		ref[center_x][center_y + i].y += opt.basic_distance;
		ref[center_x][center_y + i].real_y += opt.basic_distance;

		ref[center_x][center_y - i] = ref[center_x][center_y - (i - 1)];
		ref[center_x][center_y - i].y -= opt.basic_distance;
		ref[center_x][center_y - i].real_y -= opt.basic_distance;

		ref[center_x + i][center_y].avail = EXIST;
		ref[center_x - i][center_y].avail = EXIST;
		ref[center_x][center_y + i].avail = EXIST;
		ref[center_x][center_y - i].avail = EXIST;

		ref_num += 4;
	}

	for (int y = 1; y <= radius_p; y++) {
		int p = sqrt(pow(static_cast<double>(radius) / int_basic_distance, 2) - pow((i + 0.5), 2)) - 0.5;

		for (int x = 1; x <= p; x++) {
			ref[center_x + x][center_y + y] = Point_t(ref[center_x + x][center_y].real_x,
					                                  ref[center_x][center_y + y].real_y);

			ref[center_x + x][center_y - y] = Point_t(ref[center_x + x][center_y].real_x,
					                                  ref[center_x][center_y - y].real_y);

			ref[center_x - x][center_y + y] = Point_t(ref[center_x - x][center_y].real_x,
					                                  ref[center_x][center_y + y].real_y);

			ref[center_x - x][center_y - y] = Point_t(ref[center_x - x][center_y].real_x,
					                                  ref[center_x][center_y - y].real_y);

			ref[center_x + x][center_y + y].avail = EXIST;
			ref[center_x + x][center_y - y].avail = EXIST;
			ref[center_x - x][center_y + y].avail = EXIST;
			ref[center_x - x][center_y - y].avail = EXIST;

			ref_num += 4;
		}
	}

	return ref_num;
}
