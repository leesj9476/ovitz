#include <iostream>
#include <unistd.h>
#include <string>
#include <vector>

#include <opencv2/highgui/highgui.hpp>

#include "capture.h"
#include "image.h"
#include "oled.h"
#include "types.h"
#include "util.h"

using namespace std;
using namespace cv;

bool option[OPTION_NUM];

//TODO: change c-style exception state to OOP-style(try - catch)
int main (int argc, char *argv[]) {
	int opt;
	string image_filename;
	string output_filename;
	Oled oled;

	int exposure_time = 0;
	int gain = 0;
	int wait_sec = 0;
	bool opt_valid = true;

	// TODO change to getopt_long function
	// TODO add width, height option
	while ((opt = getopt(argc, argv, "f:e:g:co:s:w:h:")) != -1) {
		switch (opt) {
		case 'f':
			if (option[IMAGE_FILE]) {
				cerr << "<error> 'f' option is duplicated" << endl;
				return ARGUMENT_ERROR;
			}

			option[IMAGE_FILE] = true;
			image_filename = string(optarg);
			break;

		case 'e':
			if (option[EXPOSURE]) {
				cerr << "<error> 'e' option is duplicated" << endl;
				return ARGUMENT_ERROR;
			}

			option[EXPOSURE] = true;
			opt_valid = isInt(optarg);
			if (!opt_valid) {
				cerr << "<error> 'e' option's argument is not int" << endl;
				return ARGUMENT_ERROR;
			}

			exposure_time = atoi(optarg);
			break;

		case 'g':
			if (option[GAIN]) {
				cerr << "<error> 'g' option is duplicated" << endl;
				return ARGUMENT_ERROR;
			}

			option[GAIN] = true;
			opt_valid = isInt(optarg);
			if (!opt_valid) {
				cerr << "<error> 'g' option's argument is not int" << endl;
				return ARGUMENT_ERROR;
			}

			gain = atoi(optarg);
			break;

		case 'c':
			if (option[COLOR]) {
				cerr << "<error> 'c' option is duplicated" << endl;
				return ARGUMENT_ERROR;
			}

			option[COLOR] = true;
			break;

		case 'o':
			if (option[OUTPUT_FILE]) {
				cerr << "<error> 'o' option is duplicated" << endl;
				return ARGUMENT_ERROR;
			}

			option[OUTPUT_FILE] = true;
			output_filename = string(optarg);
			break;

		case 's':
			if (option[WAIT_SEC]) {
				cerr << "<error> 's' option is duplicated" << endl;
				return ARGUMENT_ERROR;
			}

			option[WAIT_SEC] = true;
			opt_valid = isInt(optarg);
			if (!opt_valid) {
				cerr << "<error> 's' option's argument is not int" << endl;
				return ARGUMENT_ERROR;
			}

			wait_sec = atoi(optarg);
			break;

		/*
		case 'h':
			option[HELP] = true;
			break;
		*/

		default:
			cerr << "<error> argument error" << endl;
			return ARGUMENT_ERROR;
		}
	}

	if (option[HELP]) {
		help();
		return SUCCESS;
	}

	if (!oled.isValid()) {
		cerr << "<error> oled is invalid" << endl;
		return OLED_ERROR;
	}

	//////////////////////////////////
	//          image mode          //
	//////////////////////////////////
	if (option[IMAGE_FILE]) {
		cout << "image mode" << endl;

		Image image(image_filename);
		if (!image.isValid()) {
			cerr << "<error> image open is failed" << endl;
			return IMAGE_ERROR;
		}

		Point_t p = image.getCenterPoint();
		cout << "center point: " << p << endl;

		if (!image.calcCentrePoints()) {
			cerr << "<error> calculation centre point is failed" << endl;
			return CALC_ERROR;
		}

		image.calcVariance();

		cout << "1000" << endl;
		cout << "image size: " << image.getSize() << endl;
	}
	//////////////////////////////////
	//         capture mode         //
	//////////////////////////////////
	else {
		cout << "capture mode" << endl;

		Capture cam;
		if (!cam.isValid()) {
			cerr << "<error> camera open is failed" << endl;
			return CAM_ERROR;
		}

		if (option[EXPOSURE]) {
			if (!cam.setOption(CV_CAP_PROP_EXPOSURE, exposure_time)) {
				cerr << "<error> exposure time option error([1, 100], shutter speed 0 to 33ms)" << endl;
				return OPTION_ERROR;
			}
		}

		if (option[GAIN]) {
			if (!cam.setOption(CV_CAP_PROP_GAIN, gain)) {
				cerr << "<error> gain option error([0, 100])" << endl;
				return OPTION_ERROR;
			}
		}

		if (option[COLOR]) {
			if (!cam.setOption(CV_CAP_PROP_FORMAT, CV_8UC3)) {
				cerr << "<error> color option error" << endl;
				return OPTION_ERROR;
			}
		}

		if (option[OUTPUT_FILE]) {
			if (output_filename.length() == 0) {
				cerr << "<error> output filename argument error" << endl;
				return OPTION_ERROR;
			}

			cam.setOutputFilename(output_filename);
		}

		if (option[WAIT_SEC]) {
			if (wait_sec < 1 || wait_sec > 10) {
				cerr << "<error> wait second option error(3sec default. [1, 10])" << endl;
				return OPTION_ERROR;
			}
		
			cam.setWaitSec(wait_sec);
		}

		cam.shot();
	}

	return SUCCESS;
}
