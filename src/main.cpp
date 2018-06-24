#include <iostream>
#include <unistd.h>
#include <string>
#include <vector>
#include <cmath>
#include <csignal>
#include <getopt.h>

#include <raspicam/raspicam_cv.h>
#include <opencv2/highgui/highgui.hpp>

/*
#include "capture.h"
#include "oled.h"
#include "image.h"*/
#include "types.h"
#include "util.h"

using namespace std;
using namespace cv;

double *lens_mat[5][5];

//void int_handler(int);

int main (int argc, char *argv[]) {
	// options setting information
	Options options = parseSettingFile();
	makeLensMatrix();

	/*
	//////////////////////////////////
	//          image mode          //
	//////////////////////////////////
	if (option[IMAGE_FILE]) {
		if (!option[DISTANCE]) {
			cerr << "<error> image mode - need distance argument" << endl;
			return ARGUMENT_ERROR;
		}

		Image image(image_filename, options);
		image.init();
		cout << image.findAllPoints() << endl;
	}
	//////////////////////////////
	//         cam mode         //
	//////////////////////////////
	else {
		while (continue_analyze) {
			Capture cam(pixel_max, options);

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
	}*/

	return 0;
}

/*
void int_handler(int sig) {
	raspicam::RaspiCam_Cv cam;
	cam.release();

	Oled oled;
	oled.clear();

	exit(0);
}*/
