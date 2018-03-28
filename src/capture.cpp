#include <opencv2/core/core.hpp>
#include <raspicam/raspicam_cv.h>
#include <wiringPi.h>

#include <iostream>
#include <string>
#include <cmath>
#include <thread>
#include <mutex>
#include <chrono>

#include "capture.h"
#include "image.h"
#include "oled.h"
#include "types.h"

using namespace cv;
using namespace raspicam;
using namespace std;

extern bool continue_analyze;
extern bool option[MAX_OPTION_NUM];

#define A_PIN	5
#define B_PIN	6

bool a_pressed = false;
bool b_pressed = false;

void aPress() {
	a_pressed = true;
}

void bPress() {
	b_pressed = true;
}

Capture::Capture(int pixel_max_, int pixel_min_, int basic_distance_,
				 double focal_, double pixel_size_, double threshold_p_)
	: pixel_max(pixel_max_), pixel_min(pixel_min_), basic_distance(basic_distance_),
	  focal(focal_), pixel_size(pixel_size_), threshold_p(threshold_p_) {

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
	Oled oled;
	mutex m;

	if (!option[TERMINAL] && !oled.isValid())
		return FAIL;

	int exposure_time = 20;
	int gain = 50;

	wiringPiSetupGpio();
	pinMode(A_PIN, INPUT);
	pinMode(B_PIN, INPUT);

	wiringPiISR(A_PIN, INT_EDGE_FALLING, aPress);
	wiringPiISR(B_PIN, INT_EDGE_FALLING, bPress);

	a_pressed = false;
	b_pressed = false;

	bool init = false;
	int succ = SUCCESS;
	string result = "Loading...";
	int pixel_val_arrow = 0;
	while (true) {
		m.lock();

		cam.set(CV_CAP_PROP_EXPOSURE, exposure_time);
		cam.set(CV_CAP_PROP_GAIN, gain);

		cam.grab();
		cam.retrieve(captured_image);

		result = "Loading...";

		if (!init) {
			image = new Image(captured_image, basic_distance, focal, pixel_size, threshold_p);
			image->init();
			init = true;
		}
		else 
			image->changeImage(captured_image);

		if (!option[TERMINAL]) {
			if (a_pressed) {
				continue_analyze = false;
				result = "Exiting...";
				m.unlock();
				break;
			}

			if (b_pressed) {
				result = "Refreshing...";
				m.unlock();
				break;
			}
		}
		else {
			char input;
			input = getchar();
			if (input == 'c') {
				result = "Exiting...";
				continue_analyze = false;
				break;
			}
			else if (input == 'r') {
				result = "Refreshing...";
				break;
			}
		}

		image->gaussianFiltering();
		image->makePixelCDF();
		int cur_pixel_avg = image->getValCDF(0.5);
		pixel_val_arrow = 0;

		if (option[AUTO_CONTROL_OFF]) {
			result = image->findAllPoints();
		}
		else if (pixel_min <= cur_pixel_avg && cur_pixel_avg <= pixel_max) {
			result = image->findAllPoints();
		}
		else if (cur_pixel_avg > pixel_max) {
			if (exposure_time == 1 && gain == 0) {
				result = image->findAllPoints();
				pixel_val_arrow = cur_pixel_avg - pixel_max;
			}
			else {
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
		}
		else if (cur_pixel_avg < pixel_min) {
			if (exposure_time == 100 && gain == 100) {
				result = image->findAllPoints();
				pixel_val_arrow = cur_pixel_avg - pixel_min;
			}
			else {
				if (exposure_time != 100)
					exposure_time = ceil(exposure_time * 1.1);
	
				if (gain != 100) {
					if (gain == 0)
						gain = 1;
					else
						gain = ceil(gain * 1.2);
				}
			}
		}

		if (exposure_time > 100)
			exposure_time = 100;
		
		if (gain > 100)
			gain = 100;

		if (pixel_val_arrow > 0) {
			result += "";
		}
		else if (pixel_val_arrow < 0) {
			result += "";
		}

		if (option[TERMINAL]) {
			if (result != "Loading...")
				cout << "========" << endl << result << endl << "========" << endl;
			else
				cout << "exposure time: " << exposure_time << " gain: " << gain << endl;
		}
		else
			oled.showString(result);

		m.unlock();
	}

	if (option[TERMINAL])
		cout << result << endl;
	else
		oled.showString(result);

	this_thread::sleep_for(chrono::seconds(1));

	delete image;
	return succ;
}
