#include <iostream>
#include <unistd.h>
#include <string>
#include <vector>

#include <opencv2/highgui/highgui.hpp>

#include "capture.h"
#include "image.h"
#include "types.h"
#include "util.h"

using namespace std;
using namespace cv;

bool continue_analyze = true;
bool option[OPTION_NUM];

double *lens_mat[5][5];

int main (int argc, char *argv[]) {
	int opt;
	string image_filename;

	int basic_distance;
	bool opt_valid = true;

	//namedWindow("original");
	//namedWindow("result");

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

	makeLensMatrix();

	//////////////////////////////////
	//          image mode          //
	//////////////////////////////////
	if (option[IMAGE_FILE]) {
		if (!option[DISTANCE]) {
			cerr << "<error> image mode - need distance argument" << endl;
			return ARGUMENT_ERROR;
		}

		Image image(image_filename, basic_distance);
		image.init();
		cout << image.findAllPoints() << endl;
	}
	//////////////////////////////
	//         cam mode         //
	//////////////////////////////
	else {
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
