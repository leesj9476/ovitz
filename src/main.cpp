#include <iostream>
#include <unistd.h>
#include <string>

#include <opencv2/highgui/highgui.hpp>

#include "picture.h"
#include "oled.h"
#include "types.h"

using namespace std;
using namespace cv;

int main (int argc, char *argv[]) {
	int opt;
	string *filename = NULL;
	Oled oled;

	while ((opt = getopt(argc, argv, "hf:")) != -1) {
		switch (opt) {
		case 'f':
			filename = new string(optarg);
			break;

		case 'h':
			break;

		default:
			cerr << "argument error" << endl;
			return ARGUMENT_ERROR;
		}
	}

	if (filename == NULL) {
		cerr << "no filename" << endl;
		return FILENAME_ERROR;
	}

	if (oled.isValid())
		oled.showString("image show");

	Image image(*filename);
	if (!image.showImage()) {
		cerr << "image show error" << endl;
		oled.clear();
		oled.update();
		return IMAGE_ERROR;
	}

	oled.showString("good image");
	image.exitImage();

	return SUCCESS;
}
