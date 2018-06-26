#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>

#include <fstream>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <string>
#include <cmath>

#include "types.h"
#include "image.h"
#include "util.h"

using namespace std;
using namespace cv;

extern double *lens_mat[5][5];

// Image constructor using image filename from option
Image::Image(Options &options) {
	opt = options;

	image = imread(opt.input_image_file, CV_LOAD_IMAGE_GRAYSCALE);
	original = imread(opt.input_image_file);

	int_basic_distance = static_cast<int>(opt.basic_distance);

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

	points = NULL;
	ref = NULL;
	vertex = NULL;
}

// Image destructor, free all dynamic allocated datum
Image::~Image() {
	for (int i = 0; i < point_row; i++) {
		delete[] points[i];
		delete[] vertex[i];

		points[i] = NULL;
		vertex[i] = NULL;
	}

	delete[] points;
	delete[] vertex;

	points = NULL;
	vertex = NULL;
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
			if (points[i]) {
				delete[] points[i];
				
				points[i] = NULL;
			}
		}

		delete[] points;

		points = NULL;
	}

	if (ref) {
		for (int i = 0; i < point_row; i++) {
			if (ref[i]) {
				delete[] ref[i];

				ref[i] = NULL;
			}
		}

		delete[] ref;

		ref = NULL;
	}

	if (vertex) {
		for (int i = 0; i < point_row; i++) {
			if (vertex[i]) {
				delete[] vertex[i];

				vertex[i] = NULL;
			}
		}

		delete[] vertex;

		vertex = NULL;
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
	if (opt.option[THRESHOLD_AREA]) {
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

// To remove noise, use gaussian filter(size => 3 x 3).
void Image::gaussianFiltering() {
	GaussianBlur(image, image, Size(3, 3), 0);
}

// Find all points.
string Image::findAllPoints() {

	// If the program is running on the image mode,
	// execute pre-processing(blur, pixel histogram CDF).
	//
	// Otherwise, that pre-processing is executed
	// before this functions is called.
	if (opt.option[INPUT_IMAGE_FILE]) {
		gaussianFiltering();
		makePixelCDF();
	}

	// calcuate threshold value
	if (opt.option[THRESHOLD_FROM_TOP_PERCENT])
		threshold_val = getValCDF(opt.threshold_from_top_percent / 100.0);
	else
		threshold_val = getValCDF(0.50) * (opt.threshold_from_avg_percent / 100.0);

	if (threshold_val > 255)
		threshold_val = 255;

	// If pixel value is under threshold value, set to 0
	threshold(image, image, threshold_val, 255, 3);

	// Set point structure's avail value to NONEXIST(unfounded)
	setAllPointsToNONE();
	
	// Find x-axis and y-axis points
	findAllAxisPoints();

	// Calculate the spot of point(the case of perfect wave front)
	// and calculate the point that we need to find in the circle
	// (standard = the shortest axis(x or y, + or -)
	ref_points_num = makeRefPointsInfo();

	// Find all points
	// Predicted point spot is calculated from axis point(projected to axis)
	for (int y = 1; y <= radius_p; y++) {
		for (int x = 1; x <= radius_p; x++) {
			if (ref[center_x + x][center_y + y].avail == NONEXIST)
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
	if (opt.option[SHOW_WINDOW]) {
		imshow("result", original);
		waitKey(20);
	}

	if (opt.option[OUTPUT_IMAGE_FILE]) {
		imwrite(opt.output_image_file, original);
	}

	// the calculated maximum radius point number is 5
	int p = radius_p > 5 ? 5 : radius_p;

	// calculate diff data about x and y
	// form => diff(p0 x) diff(p1 x) ... diff(p0 y) diff(p1 y) ...
	int diff_i = 0;
	for (int y = center_y - p; y <= center_y + p; y++) {
		for (int x = center_x - p; x <= center_x + p; x++) {

			// In case of existing point theoretically
			if (ref[x][y].avail == EXIST) {
				diff[diff_i] = points[x][y].real_x - ref[x][y].real_x; // diff x
				diff[diff_i + ref_points_num] = -(points[x][y].real_y - ref[x][y].real_y); // diff y

				diff_i++;
			}
		}
	}

	// ccd_pixel = max_row(1944) / cur_row(image's row) * real_pixel_size
	double w = 1944 / image.rows * opt.pixel_size / opt.focal;
	for (int i = 0; i < ref_points_num * 2; i++) {
		diff[i] *= w;
	}
	
	// calculate result values
	string result_str = "";
	int mat_num = p - 1;
	double result[5] = { 0,  };
	for (int i = 0; i < 5; i++) {
		for (int j = 0; j < ref_points_num * 2; j++) {
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

// Set point structure's avail value to NONEXIST(unfounded)
void Image::setAllPointsToNONE() {
	for (int i = 0; i < point_row; i++) {
		for (int j = 0; j < point_col; j++) 
			points[i][j].avail = NONEXIST;
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
	bool search[4] = { true, true, true, true };

	// if picture's row or column value is very small(< 5),
	// cannot find more points
	if (point_row < 5) {
		search[UP] = false;
		search[DOWN] = false;
	}

	if (point_col < 5) {
		search[RIGHT] = false;
		search[LEFT] = false;
	}

	int gap = 0;
	int idx = 0;
	while (search[UP] || search[RIGHT] || search[DOWN] || search[LEFT]) {
		gap++;

		// if searching gap >= (int)basic_distance,
		// set the spot using previous points
		if (gap >= int_basic_distance) {
			// UP
			if (points[center_x][center_y - 2].avail == NONEXIST) {
				points[center_x][center_y - 2].x = points[center_x][center_y - 1].x;
				points[center_x][center_y - 2].y = points[center_x][center_y - 1].y - int_basic_distance;
			}
			
			// RIGHT
			if (points[center_x + 2][center_y].avail == NONEXIST) {
				points[center_x + 2][center_y].x = points[center_x + 1][center_y].x + int_basic_distance;
				points[center_x + 2][center_y].y = points[center_x + 1][center_y].y;
			}
			
			// DOWN
			if (points[center_x][center_y + 2].avail == NONEXIST) {
				points[center_x][center_y + 2].x = points[center_x][center_y + 1].x;
				points[center_x][center_y + 2].y = points[center_x][center_y + 1].y + int_basic_distance;
			}
			
			// LEFT
			if (points[center_x - 2][center_y].avail == NONEXIST) {
				points[center_x - 2][center_y].x = points[center_x - 1][center_y].x;
				points[center_x - 2][center_y].y = points[center_x - 1][center_y].y - int_basic_distance;
			}
			
			break;
		}
		
		// Check boundary and if point is needed to be searched
		// The unit_v value is set, when the 1st axis point is searched
		// .       .   .   .   .   .   .
		//
		//     p          == search >>
		//
		// .       .   .   .   .   .   .
		
		// UP
		if (search[UP]) {
			if (unit_v[UP][0].y - gap >= 0) {
				idx = (unit_v[UP][0].y - gap) * image.cols + unit_v[UP][0].x;
				for (int i = unit_v[UP][0].x; i <= unit_v[UP][1].x; i++) {

					// The white point is founded
					if (data[idx] > threshold_val) {
						points[center_x][center_y - 2] = adjustPoint(Point_t(i, unit_v[UP][0].y - gap, i), 0, -2);
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
			if (unit_v[RIGHT][1].x + gap < image.cols) {
				idx = unit_v[RIGHT][1].y * image.cols + (unit_v[RIGHT][1].x + gap);
				for (int i = unit_v[RIGHT][1].y; i <= unit_v[RIGHT][2].y; i++) {

					// The white point is founded
					if (data[idx] > threshold_val) {
						points[center_x + 2][center_y] = adjustPoint(Point_t(unit_v[RIGHT][1].x + gap, i), 2, 0);
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
		if (search[DOWN] && unit_v[DOWN][2].y + gap < image.rows) {
			if (unit_v[DOWN][2].y + gap < image.rows) {
				idx = (unit_v[DOWN][2].y + gap) * image.cols + unit_v[DOWN][2].x;
				for (int i = unit_v[DOWN][2].x; i >= unit_v[DOWN][3].x; i--) {

					// The white point is founded
					if (data[idx] > threshold_val) {
						points[center_x][center_y + 2] = adjustPoint(Point_t(i, unit_v[DOWN][2].y + gap), 0, 2);
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
		if (search[LEFT] && unit_v[LEFT][3].x - gap >= 0) {
			if (unit_v[LEFT][3].x - gap >= 0) {
				idx = unit_v[LEFT][3].y * image.cols + (unit_v[LEFT][3].x - gap);
				for (int i = unit_v[LEFT][3].y; i >= unit_v[LEFT][0].y; i--) {

					// The white point is founded
					if (data[idx] > threshold_val) {
						points[center_x - 2][center_y] = adjustPoint(Point_t(unit_v[LEFT][3].x - gap, i), -2, 0);
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
	if (points[center_x + 2][center_y].avail == NONEXIST ||
		points[center_x - 2][center_y].avail == NONEXIST ||
		points[center_x][center_y + 2].avail == NONEXIST ||
		points[center_x][center_y - 2].avail == NONEXIST)

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

		prop = (reduce_p[UP] + reduce_p[RIGHT] + reduce_p[DOWN] + reduce_p[LEFT]) / 4;
	
		// Find all axis points
		int d[4];

		// UP, DOWN
		for (int i = 3; i <= min_p; i++) {
			// Find up, down axis points
			d[UP] = ceil((points[center_x][center_y - i + 2].y - points[center_x][center_y - i + 1].y) * prop);
			d[DOWN] = ceil((points[center_x][center_y + i - 1].y - points[center_x][center_y + i - 2].y) * prop);
	
			// UP
			points[center_x][center_y - i] = adjustPoint(Point_t(points[center_x][center_y - i + 1].x,
						                                         points[center_x][center_y - i + 1].y - d[UP]),
					                                     0, -i);
			
			// DOWN
			points[center_x][center_y + i] = adjustPoint(Point_t(points[center_x][center_y + i - 1].x,
						                                         points[center_x][center_y + i - 1].y + d[DOWN]),
					                                     0, i);
	
			if (points[center_x][center_y - i].avail == NONEXIST ||
				points[center_x][center_y + i].avail == NONEXIST)
				
				min_p = i - 1;
		}
	
		// RIGHT, LEFT
		for (int i = 3; i <= min_p; i++) {
			// Find right, left axis points
			d[RIGHT] = ceil((points[center_x + i - 1][center_y].x - points[center_x + i - 2][center_y].x) * prop);
			d[LEFT] = ceil((points[center_x - i + 2][center_y].x - points[center_x - i + 1][center_y].x) * prop);
	
			// RIGHT
			points[center_x + i][center_y] = adjustPoint(Point_t(points[center_x + i - 1][center_y].x + d[RIGHT],
						                                         points[center_x + i - 1][center_y].y),
					                                     i, 0);
			
			// LEFT
			points[center_x - i][center_y] = adjustPoint(Point_t(points[center_x - i + 1][center_y].x - d[LEFT],
						                                         points[center_x - i + 1][center_y].y),
					                                     -i, 0);
	
			if (points[center_x + i][center_y].avail == NONEXIST ||
				points[center_x - i][center_y].avail == NONEXIST)
				
				min_p = i - 1;
		}
	
		// If the axis point is founded over index min_p from center point, 
		// set avail value to NONEXIST from EXIST (EXIST -> NONEXIST)

		// x-axis
		for (int i = min_p + 1; i <= center_x; i++) {
			if (points[center_x + i][center_y].avail == EXIST)
				points[center_x + i][center_y].avail = NONEXIST;
	
			if (points[center_x - i][center_y].avail == EXIST)
				points[center_x - i][center_y].avail = NONEXIST;
		}
	
		// y-axis
		for (int i = min_p + 1; i <= center_y; i++) {
			if (points[center_x][center_y + i].avail == EXIST)
				points[center_x][center_y + i].avail = NONEXIST;
	
			if (points[center_x][center_y - i].avail == EXIST)
				points[center_x][center_y - i].avail = NONEXIST;
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
		if (adjust_p.avail == NONEXIST)
			return p;
	}

	// find the center of mass in unit box spreading from founded point
	return findCenterPoint(adjust_p, diff_x, diff_y, flag);
}

// Find closest white point(over threshold pixel value)
// diff_x, diff_y => the index difference from center point index
//   => pointer[center_x + diff_x][center_y + diff_y]
Point_t Image::findClosestWhitePoint(const Point_t &p, int diff_x, int diff_y) {
	uchar *data = (uchar *)image.data;
	int idx = p.y * image.cols + p.x;;

	// Set maximum unit box size
	int unit_y_up, unit_y_down, unit_x_right, unit_x_left;

	// Searching point we want to find is point[center_x + diff_x][center_y+ diff_x]
	// Set searching box size(x dir, y dir)
	if (diff_x == 1 || diff_x == 0 || diff_x == -1)
		unit_x_left = unit_x_right = ceil(int_basic_distance / 2.0);
	else if (diff_x > 1) {
		unit_x_left = ceil((p.x - points[center_x + diff_x - 1][center_y].x) / 2.0);
		unit_x_right = ceil(unit_x_left * prop);
	}
	else if (diff_x < -1) {
		unit_x_right = ceil((points[center_x + diff_x + 1][center_y].x - p.x) / 2.0);
		unit_x_left = ceil(unit_x_right * prop);
	}

	if (diff_y == 1 || diff_y == 0 || diff_y == -1)
		unit_y_up = unit_y_down = ceil(int_basic_distance / 2.0);
	else if (diff_y > 1) {
		unit_y_up = ceil((p.y - points[center_x][center_y + diff_y - 1].y) / 2.0);
		unit_y_down = ceil(unit_y_up * prop);
	}
	else if (diff_y < -1) {
		unit_y_down = ceil((points[center_x][center_y + diff_y + 1].y - p.y) / 2.0);
		unit_y_up = ceil(unit_y_down * prop);
	}

	// 0       1
	//   -----
	//  |     |
	//  |     |
	//   -----
	// 3       2
	//
	// Start from the center point
	Point_t search_p[4] = { { p.x, p.y }, { p.x, p.y }, 
							{ p.x, p.y }, { p.x, p.y } };

	// UP, RIGHT, DOWN, LEFT
	bool search_expand[4] = { true, true, true, true };
	int expand[4] = { 0, 0, 0, 0 };


	// Find the closest over threshold point while expanding searching box size
	while (true) {

		// expand searching box size
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

		// If cannot find over threshold point, just return the starting point(set avail value to NONEXIST)
		if (!search_expand[UP] && !search_expand[RIGHT] && !search_expand[DOWN] && !search_expand[LEFT])
			return Point_t(p.x, p.y, NONEXIST);

		// Search the over threshold point at the boundary points of searching box
		vector<int> found_points;
		if (search_expand[UP]) {
			for (int i = search_p[0].x; i < search_p[1].x; i++) {
				if (data[idx] > threshold_val) {
					found_points.push_back(idx);
					break;
				}

				idx++;
			}
		}
		else
			idx += (search_p[1].x - search_p[0].x);

		if (search_expand[RIGHT]) {
			for (int i = search_p[1].y; i < search_p[2].y; i++) {
				if (data[idx] > threshold_val) {
					found_points.push_back(idx);
					break;
				}

				idx += image.cols;
			}
		}
		else
			idx += image.cols*(search_p[2].y - search_p[1].y);

		if (search_expand[DOWN]) {
			for (int i = search_p[2].x; i > search_p[3].x; i--) {
				if (data[idx] > threshold_val) {
					found_points.push_back(idx);
					break;
				}

				idx--;
			}
		}
		else
			idx -= (search_p[2].x - search_p[3].x);

		if (search_expand[LEFT]) {
			for (int i = search_p[3].y; i > search_p[0].y; i--) {
				if (data[idx] > threshold_val) {
					found_points.push_back(idx);
					break;
				}

				idx -= image.cols;
			}
		}
		else
			idx -= image.cols*(search_p[3].y - search_p[0].y);

		// Select the index that has the biggest pixel value among found indexes
		if (found_points.size() > 0) {
			idx = found_points[0];

			for (uint i = 1; i < found_points.size(); i++) {
				if (data[idx] < data[found_points[i]])
					idx = found_points[i];
			}

			break;
		}
	}

	// Return the selected point
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
// Find unit rectangle and then calculate the center point(center of mass)
Point_t Image::findCenterPoint(const Point_t &p, int diff_x, int diff_y, int flag) {

	// UP, RIGHT, DOWN, LEFT -> search expand direction
	bool search[4] = {true, true, true, true};
	int expand[4] = { 0, 0, 0, 0 };
	Point_t v[4] = { { p.x, p.y }, { p.x, p.y },
					 { p.x, p.y }, { p.x, p.y } };
	
	// Set unit box size
	int unit_y_up, unit_y_down, unit_x_right, unit_x_left;

	// points[center_x + diff_x][center_y + diff_y]
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
		unit_y_down = ceil((points[center_x][center_y + diff_y + 1].y - p.y) / 2.0);
		
		if (diff_y == -2) {
			unit_y_up = unit_y_down;
		}
		else {
			unit_y_up = ceil(unit_y_down * prop);
		}
	}

	//
	bool white = false;
	uchar *data = (uchar *)image.data;
	int idx = (v[0].y * image.cols) + v[0].x;
	while (search[UP] || search[RIGHT] || search[DOWN] || search[LEFT]) {

		// Check availability and expand unit vertex point(searching box)
		if (search[UP]) {
			if (v[0].y - SEARCH_GAP >= 0 && expand[UP] <= unit_y_up) {
				v[0].y -= SEARCH_GAP;
				v[1].y -= SEARCH_GAP;

				expand[UP] += SEARCH_GAP;
			}
			else
				search[UP] = false;
		}

		if (search[RIGHT]) {
			if (v[1].x + SEARCH_GAP < image.cols && expand[RIGHT] <= unit_x_right) {
				v[1].x += SEARCH_GAP;
				v[2].x += SEARCH_GAP;

				expand[RIGHT] += SEARCH_GAP;
			}
			else
				search[RIGHT] = false;
		}

		if (search[DOWN]) {
			if (v[2].y + SEARCH_GAP < image.rows && expand[DOWN] <= unit_y_down) {
				v[2].y += SEARCH_GAP;
				v[3].y += SEARCH_GAP;

				expand[DOWN] += SEARCH_GAP;
			}
			else
				search[DOWN] = false;
		}

		if (search[LEFT]) { 
			if(v[3].x - SEARCH_GAP >= 0 && expand[LEFT] <= unit_x_left) {
				v[3].x -= SEARCH_GAP;
				v[0].x -= SEARCH_GAP;

				expand[LEFT] += SEARCH_GAP;
			}
			else
				search[LEFT] = false;
		}

		// Check points of lines for up, right, down, left directions
		if (search[UP]) {
			white = false;
			idx = v[0].y * image.cols + (v[0].x + 1);
			for (int i = v[0].x + 1; i < v[1].x; i++) {
				if (data[idx] > threshold_val) {
					white = true;
					break;
				}

				idx++;
			}

			if (white == false)
				search[UP] = false;
		}

		if (search[RIGHT]) {
			white = false;
			idx = (v[1].y + 1) * image.cols + v[1].x;
			for (int i = v[1].y + 1; i < v[2].y; i++) {
				if (data[idx] > threshold_val) {
					white = true;
					break;
				}

				idx += image.cols;
			}

			if (white == false)
				search[RIGHT] = false;
		}

		if (search[DOWN]) {
			white = false;
			idx = v[2].y * image.cols + (v[2].x - 1);
			for (int i = v[2].x - 1; i > v[3].x; i--) {
				if (data[idx] > threshold_val) {
					white = true;
					break;
				}

				idx--;
			}

			if (white == false)
				search[DOWN] = false;
		}

		if (search[LEFT]) {
			white = false;
			idx = (v[3].y - 1) * image.cols + v[3].x;
			for (int i = v[3].y - 1; i > v[0].y; i--) {
				if (data[idx] > threshold_val) {
					white = true;
					break;
				}

				idx -= image.cols;
			}

			if (white == false)
				search[LEFT] = false;
		}

		// Check vertex point
		// Check => pixel value && vertex point index && searching box size
		idx = v[0].y * image.cols + v[0].x;
		if (data[idx] > threshold_val &&
			v[0].x >= SEARCH_GAP && v[0].y >= SEARCH_GAP &&
			expand[LEFT] <= unit_x_left && expand[UP] <= unit_y_up) {
			
			search[LEFT] = true; search[UP] = true;
		}

		idx = v[1].y * image.cols + v[1].x;
		if (data[idx] > threshold_val &&
			v[1].x + SEARCH_GAP < image.cols && v[1].y >= SEARCH_GAP &&
			expand[UP] <= unit_y_up && expand[RIGHT] <= unit_x_right) {
			
			search[UP] = true; search[RIGHT] = true;
		}

		idx = v[2].y * image.cols + v[2].x;
		if (data[idx] > threshold_val &&
			v[2].x + SEARCH_GAP < image.cols && v[2].y + SEARCH_GAP < image.cols &&
			expand[RIGHT] <= unit_x_right && expand[DOWN] <= unit_y_down) {
			
			search[RIGHT] = true; search[DOWN] = true;
		}

		idx = v[3].y * image.cols + v[3].x;
		if (data[idx] > threshold_val &&
			v[3].x >= SEARCH_GAP && v[3].y + SEARCH_GAP < image.cols &&
			expand[DOWN] <= unit_y_down && expand[LEFT] <= unit_x_left) {
			
			search[DOWN] = true; search[LEFT] = true;
		}
	}

	// Save the vertex point result of 1st axis point(just near center point)
	// for convenience and other uses
	if (flag != NO_FLAG) {
		unit_v[flag][0] = v[0];
		unit_v[flag][1] = v[1];
		unit_v[flag][2] = v[2];
		unit_v[flag][3] = v[3];
	}

	// Save the vertex point result(searching box)
	for (int i = 0; i < 4; i++)
		vertex[center_x + diff_x][center_y + diff_y].v[i] = v[i];

	// Return result is the center point of searching box by mass
	int width = v[2].x - v[0].x + 1;
	int height = v[2].y - v[0].y + 1;
	return calcCenterOfMass(v[0], width, height);
}

// Calculate the cneter of mass point about starting point p and searching box width x height
Point_t Image::calcCenterOfMass(Point_t &p, int width, int height) {
	long long int w = 0;
	long long int x = 0;
	long long int y = 0;

	// Calculate weight sum about the pixel value
	uchar *data = (uchar *)image.data;
	int idx = p.y * image.cols + p.x;
	for (int i = 0; i < height; i++) {
		for (int j = 0; j < width; j++) {
			if (data[idx] > threshold_val) {
				w += data[idx];
				x += data[idx] * (p.x + j);
				y += data[idx] * (p.y + i);
			}

			idx++;
		}

		idx += (image.cols - width);
	}

	// Return the center of mass point(the point x, y has real value)
	int center_of_mass_x = x / static_cast<double>(w);
	int center_of_mass_y = y / static_cast<double>(w);
	return Point_t(center_of_mass_x, center_of_mass_y, EXIST);
}

// Make Reference point information map
// Calculate the radius of circle including most exterior point box on the axis and
// set the avail value to EXIST if the reference point is in the circle
int Image::makeRefPointsInfo() {
	int d;
	if (int_basic_distance & 0x01)
		d = (int_basic_distance + 1) >> 1;
	else
		d = int_basic_distance >> 1;

	// Set the all reference point x, y value and avail valut to EXIST, if the point is
	// in the circle and return the reference points number
	
	// For the points on the x, y axis
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

		ref[center_x][center_y + i] = ref[center_x][center_y + (i - 1)];
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

	// Get the most exterior point and calculate radius of circle
	Point_t outer_p(ref[center_x + radius_p][center_y].x + d,
			        ref[center_x + radius_p][center_y].y + d);

	radius = getPointDistance(center_p, outer_p);

	// For the points not on the x, y axis
	for (int y = 1; y <= radius_p; y++) {

		// the number of point at the one side of x direction
		int p = sqrt(pow(static_cast<double>(radius) / int_basic_distance, 2) - pow((y + 0.5), 2)) - 0.5;

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

	// return the number of reference points
	return ref_num;
}
