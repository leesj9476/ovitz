#include <iostream>
#include <unistd.h>
#include <string>

#include <opencv2/highgui/highgui.hpp>

#include "image.h"
#include "video.h"
#include "oled.h"
#include "types.h"

using namespace std;
using namespace cv;

//TODO: change c-style exception state to OOP-style(try - catch)
int main (int argc, char *argv[]) {
	int opt;
	string image_filename = "";
	string video_filename = "";
	Oled oled;

	int image_opt = 0;
	int video_opt = 0;
	int help_opt  = 0;

	while ((opt = getopt(argc, argv, "hi:v:")) != -1) {
		switch (opt) {
		case 'i':
			image_filename = string(optarg);
			image_opt = 1;
			break;

		case 'v':
			video_filename = string(optarg);
			video_opt = 1;
			break;

		case 'h':
			help_opt = 1;
			break;

		default:
			cerr << "argument error" << endl;
			return ARGUMENT_ERROR;
		}
	}

	if (!oled.isValid()) {
		cerr << "oled is invalid" << endl;
		return OLED_ERROR;
	}

	if (image_opt && video_opt) {
		cerr << "argument error(both image and video on)" << endl;
		return ARGUMENT_ERROR;
	}
	else if (image_opt) {
		Image image(image_filename);
		if (!image.isValid()) {
			cerr << "image open error" << endl;
			return IMAGE_ERROR;
		}

		image.showImage();
		image.exitImage();
	}
	else if (video_opt) {
		Video video(video_filename);
		if (!video.isValid()) {
			cerr << "video open error" << endl;
			return VIDEO_ERROR;
		}

		video.showVideo();
		video.exitVideo();
	}
	else {
		cerr << "argument err(both image and video off)" << endl;
		return ARGUMENT_ERROR;
	}

	return SUCCESS;
}
