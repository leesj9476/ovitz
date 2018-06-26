#include <iostream>
#include <fstream>
#include <iomanip>
#include <cmath>
#include <cctype>
#include <string>

#include <raspicam/raspicam_cv.h>

#include "ssd1306_i2c.h"

#include "util.h"
#include "image.h"

using namespace std;

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

Options::Options() {
	basic_distance = 50.17284;
	focal = 7;
	pixel_size = 1.4;
	threshold_percent = 100;
	threshold_top_percent = 95;
	threshold_area = 0;

	pixel_max = 30;
	pixel_min = 15;

	delay_ms = 0;

	// Init the option checker array to false
	for (int i = 0; i < MAX_OPTION_NUM; i++)
		option[i] = false;

	// Default thresholding method is threshold using histogram CDF
	option[THRESHOLD_TOP_P] = true;
}

// Calculate 2-d euclidean distance
int getPointDistance(const Point_t &p1, const Point_t &p2) {
	int d = ceil(sqrt(pow(p1.x - p2.x, 2) + pow(p1.y - p2.y, 2)));

	return d;
}

// Read matrix_lens files and make matrix for calcuation
void makeLensMatrix() {
	int points[5] = { 10, 26, 58, 98, 146 };
	for (int i = 0; i < 5; i++) {
		for (int j = 0; j < 5; j++) {
			lens_mat[i][j] = new double[points[i]];
		}
	}

	for (int i = 0; i < 5; i++) {
		string filename = "./data/matrix_lens" + to_string(2*i + 3) + ".txt";
		ifstream f(filename);

		for (int row = 0; row < 5; row++) {
			for (int col = 0; col < points[i]; col++) {
				f >> lens_mat[i][row][col];
			}
		}

		f.close();
	}
}

// Check the str is integer
// sign(true) -> +/-
bool isInt(const char *str, bool sign) {
	if (*str == '-' && sign)
		str++;

	while (*str) {
		if (!isdigit(*str))
			return false;

		str++;
	}

	return true;
}

// Check the str is float
// sign(true) -> +/-
bool isFloat(const char *str, bool sign) {
	if (*str == '-' && sign)
		str++;

	int point = 0;
	if (*str == '.')
		return false;

	while (*str) {
		if (*str == '.') {
			if (point == 0)
				point++;
			else
				return false;
		}
		else if (!isdigit(*str))
			return false;

		str++;
	}

	return true;
}

// Parse setting file(conf/setting.conf file)
Options parseSettingFile() {
	Options opt;

	// setting file
	ifstream f("./conf/setting.conf");

	// execute with default setting
	if (!f.is_open())
		return opt;

	// Check if window it available
	// If default execution(auto starting), there is no window_avail file
	// so window is unavailable
	bool window_avail = false;
	ifstream window_lock("./lock/window_avail");
	if (window_lock.is_open())
		window_avail = true;
	else
		system("touch ./lock/window_avail");

	window_lock.close();

	// Read setting file line by line
	string line;
	while (getline(f, line)) {

		// Ignore blank line
		if (line[0] == '#' || line[0] == ' ' || line[0] == '\n')
			continue;

		// Separate key and val
		string key;
		string val;

		// remove all space from the line
		uint i = 0;
		while (i < line.size()) {
			while (line[i] == ' ')
				line.erase(i, 1);

			i++;
		}

		// Find separater
		size_t pos = line.find('=');

		// Check strange form
		if (pos == string::npos || pos == line.size() - 1)
			continue;

		// Get key and val info
		key = line.substr(0, pos);
		val = line.substr(pos + 1);

		// Set the values of option structure
		if (key == "basic_distance" && isFloat(val.c_str(), false)) {
			opt.option[DISTANCE] = true;
			opt.basic_distance = stod(val);
		}
		else if (key == "real_pixel_size" && isFloat(val.c_str(), false)) {
			opt.option[PIXEL_SIZE] = true;
			opt.pixel_size = stod(val);
		}
		else if (key == "pixel_min" && isInt(val.c_str(), false)) {
			opt.option[PIXEL_MIN] = true;
			opt.pixel_min = atoi(val.c_str());
		}
		else if (key == "pixel_max" && isInt(val.c_str(), false)) {
			opt.option[PIXEL_MAX] = true;
			opt.pixel_max = atoi(val.c_str());
		}
		else if (key == "focal" && isFloat(val.c_str(), false)) {
			opt.option[FOCAL] = true;
			opt.focal = stod(val);
		}
		else if (key == "window" && val.find("true") != string::npos) {
			if (window_avail)
				opt.option[SHOW_WINDOW] = true;
		}
		else if (key == "terminal" && val.find("true") != string::npos) {
			opt.option[TERMINAL] = true;
		}
		else if (key == "auto_control" && val.find("off") != string::npos) {
			opt.option[AUTO_CONTROL_OFF] = true;
		}
		else if (key == "threshold_percent" && isFloat(val.c_str(), false)) {
			opt.option[THRESHOLD_P] = true;
			opt.option[THRESHOLD_TOP_P] = false;
			opt.threshold_percent = stod(val);
		}
		else if (key == "threshold_top_percent" && isFloat(val.c_str(), false)) {
			opt.option[THRESHOLD_P] = false;
			opt.option[THRESHOLD_TOP_P] = true;
			opt.threshold_top_percent = stod(val);
		}
		else if (key == "threshold_area" && isInt(val.c_str(), false)) {
			opt.option[THRESHOLD_AREA] = true;
			opt.threshold_area = atoi(val.c_str());
		}
		else if (key == "input_image_filename") {
			opt.option[INPUT_IMAGE_FILE] = true;
			opt.input_image_file = val;
		}
		else if (key == "output_image_filename") {
			opt.option[OUTPUT_IMAGE_FILE] = true;
			opt.output_image_file = val;
		}
		else if (key == "delay" && isInt(val.c_str(), false)) {
			opt.option[DELAY] = true;
			opt.delay_ms = atoi(val.c_str());
		}
	}

	f.close();

	return opt;
}

void int_handler(int sig) {
	raspicam::RaspiCam_Cv cam;
	cam.release();

	ssd1306_clearDisplay();
	ssd1306_display();

	system("rm -f ./lock/dup_lock");

	exit(0);	
}
