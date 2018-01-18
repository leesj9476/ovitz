#include <iostream>
#include <unistd.h>
#include <string>
#include <vector>

#include <opencv2/highgui/highgui.hpp>

#include "image.h"
#include "oled.h"
#include "types.h"
#include "util.h"

using namespace std;
using namespace cv;

//TODO: change c-style exception state to OOP-style(try - catch)
int main (int argc, char *argv[]) {
	int opt;
	string image_filename = "";
	Oled oled;

	int image_opt = 0;
	int help_opt  = 0;

	while ((opt = getopt(argc, argv, "i:ph")) != -1) {
		switch (opt) {
		case 'i':
			image_filename = string(optarg);
			image_opt = 1;
			break;

		case 'h':
			help_opt = 1;
			break;

		default:
			cerr << "argument error" << endl;
			return ARGUMENT_ERROR;
		}
	}

	if (help_opt) {
		help();
		return SUCCESS;
	}

	if (!oled.isValid()) {
		cerr << "oled is invalid" << endl;
		return OLED_ERROR;
	}

	if (image_opt) {
		Image image(image_filename);
		if (!image.isValid()) {
			cerr << "image open error" << endl;
			return IMAGE_ERROR;
		}

		if (!image.calcCentrePoints()) {
			cerr << "calculate centre point error" << endl;
			return CALC_ERROR;
		}

		double *d = image.calcCentrePointsDistance();
		for (int i = 0; i < UNIT_WIDTH_NUM * UNIT_HEIGHT_NUM; i++) {
			cout << d[i] << endl;
		}
	}

	return SUCCESS;
}
