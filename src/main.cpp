#include <iostream>
#include <unistd.h>
#include <string>
#include <vector>
#include <cmath>
#include <csignal>
#include <getopt.h>

#include <raspicam/raspicam_cv.h>
#include <opencv2/highgui/highgui.hpp>

#include "capture.h"
#include "oled.h"
#include "image.h"
#include "types.h"
#include "util.h"

using namespace std;
using namespace cv;

bool continue_analyze = true;
bool option[MAX_OPTION_NUM];

double *lens_mat[5][5];

struct option long_options[] = {
	{ "filename", required_argument, 0, 'i' },
	{ "lens_distance", required_argument, 0, 'd' },
	{ "real_pixel_size", required_argument, 0, 0 },
	{ "pixel_max", required_argument, 0, 0 },
	{ "pixel_min", required_argument, 0, 0 },
	{ "focal", required_argument, 0, 'f' },
	{ "window", no_argument, 0, 'w' },
	{ "print_terminal", no_argument, 0, 't' },
	{ 0, 0, 0, 0 }
};

void int_handler(int);

int main (int argc, char *argv[]) {
	int opt;
	string image_filename;
	int basic_distance = 22;
	double focal = 7;
	double pixel_size = 1.4;

	int pixel_max = 30;
	int pixel_min = 15;

	// TODO add width, height option
	int opt_idx = 0;
	while ((opt = getopt_long(argc, argv, "i:d:f:wt", long_options, &opt_idx)) != -1) {
		switch (opt) {
		case 0: {
			if (long_options[opt_idx].flag != 0)
				break;

			string opt_name(long_options[opt_idx].name);
			if (opt_name == "real_pixel_size") {
				option[PIXEL_SIZE] = true;
				pixel_size = stod(string(optarg));
				break;
			}
			else if (opt_name == "pixel_max") {
				option[PIXEL_MAX] = true;
				pixel_max = atoi(optarg);
				break;
			}
			else if (opt_name == "pixel_min") {
				option[PIXEL_MIN] = true;
				pixel_min = atoi(optarg);
				break;		
			}
			else {
				cerr << "<error> unknown option" << endl;
				return ARGUMENT_ERROR;
			}

			break;
		}

		case 'i':
			option[IMAGE_FILE] = true;
			image_filename = string(optarg);
			break;

		case 'd':
			option[DISTANCE] = true;
			basic_distance = atoi(optarg);
			break;

		case 'f':
			option[FOCAL] = true;
			focal = stod(string(optarg));
			break;

		case 'w':
			option[SHOW_WINDOW] = true;
			break;

		case 't':
			option[TERMINAL] = true;
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

		Image image(image_filename, basic_distance, focal, pixel_size);
		image.init();
		cout << image.findAllPoints() << endl;
	}
	//////////////////////////////
	//         cam mode         //
	//////////////////////////////
	else {
		while (continue_analyze) {
			Capture cam(pixel_max, pixel_min, basic_distance, focal, pixel_size);

			signal(SIGINT, int_handler);

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

void int_handler(int sig) {
	raspicam::RaspiCam_Cv cam;
	cam.release();

	Oled oled;
	oled.clear();

	exit(0);
}
