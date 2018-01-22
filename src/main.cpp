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

bool option[OPTION_NUM];

//TODO: change c-style exception state to OOP-style(try - catch)
int main (int argc, char *argv[]) {
	int opt;
	string image_filename = "";
	Oled oled;

	int exposure_time = 0;
	int gain = 0;
	bool opt_valid = true;

	// change to getopt_long function
	while ((opt = getopt(argc, argv, "f:e:g:h")) != -1) {
		switch (opt) {
		case 'f':
			if (option[IMAGE_FILE]) {
				cout << "<error> 'f' option is duplicated" << endl;
				return ARGUMENT_ERROR;
			}

			option[IMAGE_FILE] = true;
			image_filename = string(optarg);
			break;

		case 'e':
			if (option[EXPOSURE]) {
				cout << "<error> 'e' option is duplicated" << endl;
				return ARGUMENT_ERROR;
			}

			option[EXPOSURE] = true;
			opt_valid = isInt(optarg);
			if (!opt_valid) {
				cout << "<error> 'e' option's argument is not int" << endl;
				return ARGUMENT_ERROR;
			}

			exposure_time = atoi(optarg);
			break;

		case 'g':
			if (option[GAIN]) {
				cout << "<error> 'g' option is duplicated" << endl;
				return ARGUMENT_ERROR;
			}

			option[GAIN] = true;
			opt_valid = isInt(optarg);
			if (!opt_valid) {
				cout << "<error> 'g' option's argument is not int" << endl;
				return ARGUMENT_ERROR;
			}

			gain = atoi(optarg);
			break;

		case 'h':
			option[HELP] = true;
			break;

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

	if (option[IMAGE_FILE]) {
		Image image(image_filename);
		if (!image.isValid()) {
			cerr << "<error> image open is failed" << endl;
			return IMAGE_ERROR;
		}

		Point_t p = image.getCentreOfMassByWhole();
		cout << "( " << p.x << ", " << p.y << " )" << endl;
		cout << image.getSize() << endl;
	}

	return SUCCESS;
}
