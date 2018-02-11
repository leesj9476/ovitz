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

bool continue_analyze = true;

bool option[OPTION_NUM];

//TODO: change c-style exception state to OOP-style(try - catch)
int main (int argc, char *argv[]) {
	int opt;
	string image_filename;
	string output_filename;
	Oled oled;

	int basic_distance;

	bool opt_valid = true;

	// TODO add width, height option
	while ((opt = getopt(argc, argv, "f:d:")) != -1) {
		switch (opt) {
		case 'f':
			if (option[IMAGE_FILE]) {
				cerr << "<error> 'f' option is duplicated" << endl;
				return ARGUMENT_ERROR;
			}

			option[IMAGE_FILE] = true;
			image_filename = string(optarg);
			
			break;
		case 'd':
			if (option[DISTANCE]) {
				cerr << "<error> 'd' options is duplicated" << endl;
				return ARGUMENT_ERROR;
			}

			option[DISTANCE] = true;
			opt_valid = isInt(optarg);
			if (!opt_valid) {
				cerr << "<error> 'd' option's argjment is not int" <<endl;
				return ARGUMENT_ERROR;
			}

			basic_distance = atoi(optarg);
			
			break;
		default:
			cerr << "<error> argument error" << endl;
			return ARGUMENT_ERROR;
		}
	}

	if (!oled.isValid()) {
		cerr << "<error> oled is invalid" << endl;
		return OLED_ERROR;
	}

	//////////////////////////////////
	//          image mode          //
	//////////////////////////////////
	if (option[IMAGE_FILE]) {
		if (!option[DISTANCE]) {
			cerr << "<error> image mode - need distance argument" << endl;
			return ARGUMENT_ERROR;
		}

		cout << "image mode" << endl;

		Image image(image_filename, basic_distance);
		image.init();
		image.findAllPoints();
	}
	//////////////////////////////
	//         cam mode         //
	//////////////////////////////
	else {
		cout << "cam mode" << endl;

		if (!option[DISTANCE]) {
			cerr << "<error> cam mode - need distance argument" << endl;
			return ARGUMENT_ERROR;
		}

		while (continue_analyze) {
			Capture cam(basic_distance);
			if (!cam.isValid()) {
				cerr << "<error> camera open failed" << endl;
				return CAM_ERROR;
			}

			if (!cam.shot()) {
				cerr << "<error> camera shot failed" << endl;
				return CAM_ERROR;
			}
		}
	}

	return 0;
}
