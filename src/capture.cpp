#include <opencv2/core/core.hpp>
#include <raspicam/raspicam_cv.h>

#include <iostream>
#include <string>
#include <cmath>
#include <thread>

#include "capture.h"
#include "image.h"
#include "types.h"

using namespace cv;
using namespace raspicam;
using namespace std;

extern bool continue_analyze;

Capture::Capture(int basic_distance_)
	: basic_distance(basic_distance_) {

	cam.set(CV_CAP_PROP_FRAME_WIDTH, 512);
	cam.set(CV_CAP_PROP_FRAME_HEIGHT, 512);
	cam.open();
}

Capture::~Capture() {
	cam.release();
}

bool Capture::isValid() {
	return cam.isOpened();
}

int Capture::shot() {
	Image *image;
	Mat captured_image;

	int exposure_time = 20;
	int gain = 50;
	
	// average pixel min-max setting
	int min_pixel = 20;
	int max_pixel = 40;
	if (min_pixel < 0)
		min_pixel = 0;

	if (max_pixel > 255)
		max_pixel = 255;

	bool init = false;
	int result = SUCCESS;
	char pressed;
	while (true) {
		if (!cam.set(CV_CAP_PROP_EXPOSURE, exposure_time)) {
			cerr << "exposure time error" << endl;
			result = FAIL;

			break;
		}

		if (!cam.set(CV_CAP_PROP_GAIN, gain)) {
			cerr << "gain error" << endl;
			result = FAIL;

			break;
		}

		cam.grab();
		cam.retrieve(captured_image);

		// maybe need cut black square
		if (!init) {
			image = new Image(captured_image, basic_distance);
			image->init();
			init = true;
		}
		else 
			image->changeImage(captured_image);

		imshow("original", captured_image);
		pressed = waitKey(20);
		if (pressed == 27) {
			imwrite("output.png", captured_image);
			destroyWindow("original");
			destroyWindow("result");
			continue_analyze = false;
			break;
		}
		else if (pressed == 32) {
			destroyWindow("original");
			destroyWindow("result");
			break;
		}

		image->gaussianFiltering();
		image->makePixelCDF();
		int cur_pixel_avg = image->getValCDF(0.5);

		if (min_pixel <= cur_pixel_avg && cur_pixel_avg <= max_pixel) {
			image->findAllPoints();
		}
		else if (cur_pixel_avg > max_pixel) {
			if (exposure_time != 1) {
				if (exposure_time < 10)
					exposure_time--;
				else
					exposure_time = ceil(exposure_time * 0.9);
			}
			
			if (gain != 0) {
				if (gain < 10)
					gain--;
				else
					gain = ceil(gain * 0.8);
			}
		}
		else if (cur_pixel_avg < min_pixel) {
			if (exposure_time != 100)
				exposure_time = ceil(exposure_time * 1.1);

			if (gain != 100)
				gain = ceil(gain * 1.2);
		}

		if (exposure_time > 100)
			exposure_time = 100;
		
		if (gain > 100)
			gain = 100;
	}

	delete image;
	return result;
}
