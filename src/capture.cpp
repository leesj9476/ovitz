#include <iostream>
#include <string>
#include <cmath>
#include <thread>
#include <mutex>
#include <chrono>

#include <opencv2/core/core.hpp>
#include <raspicam/raspicam_cv.h>

#include "ssd1306_i2c.h"

#include "capture.h"
#include "image.h"
#include "types.h"

using namespace cv;
using namespace raspicam;
using namespace std;

Capture::Capture(Options &options) {
	opt = options;

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

// Receive the frame data from raspicam.
// The raspicam capture the image data continuously using threads,
// so use global mutex through all analysis for current frame.
void Capture::shot() {
	Image *image;
	Mat captured_image;
	mutex m;

	// Initiate exposure time and gain value.
	// If auto control option is on, the values are controled automatically.
	int exposure_time = 20;
	int gain = 50;

	// Notice the first frame for initialization.
	bool init = false;

	string result;

	// Change checker
	bool exposure_time_changed = true;
	bool gain_changed = true;

	// If the analysis function is called, loading value is changed to false
	bool loading = true;

	// average pixel value - polar value of permissible pixel range
	int pixel_diff = 0;

	// Analysis loop
	while (true) {
		m.lock();

		// If exposure time value is changed,
		// change the exposure time setting of raspicam.
		if (exposure_time_changed) {
			cam.set(CV_CAP_PROP_EXPOSURE, exposure_time);
			exposure_time_changed = false;
		}

		// If gain value is changed, change the gain setting of raspicam.
		if (gain_changed) {
			cam.set(CV_CAP_PROP_GAIN, gain);
			gain_changed = false;
		}

		// Get the frame and save to captured_image.
		cam.grab();
		cam.retrieve(captured_image);

		result = "Loading...";
		loading = true;

		// Initiate Image class
		if (!init) {
			image = new Image(captured_image, opt);
			image->init();
			init = true;
		}
		// Just change image structure
		else 
			image->changeImage(captured_image);

		// Pre-processing for image.
		image->gaussianFiltering();
		image->makePixelCDF();

		// Get average pixel value.
		int cur_pixel_avg = image->getValCDF(0.5);
		pixel_diff = 0;

		// If auto control option is off, do not control exposure time
		// and gain value and just call findAllPoints().
		if (opt.option[AUTO_CONTROL_OFF]) {
			result = image->findAllPoints();
			loading = false;
		}
		// If current average pixel value is in the permissible range, 
		// call findAllPoints() directly.
		else if (opt.pixel_min <= cur_pixel_avg && cur_pixel_avg <= opt.pixel_max) {
			result = image->findAllPoints();

			loading = false;
		}
		// When the current average pixel value is bigger than
		// maximum pixel value of permissible range.
		else if (cur_pixel_avg > opt.pixel_max) {

			// No more control exposure time and gain value.
			if (exposure_time == 1 && gain == 0) {
				result = image->findAllPoints();

				pixel_diff = cur_pixel_avg - opt.pixel_max;
				loading = false;
			}
			// Control exposure time and gain value.
			else {
				if (exposure_time != 1) {
					if (exposure_time < 10)
						exposure_time--;
					else
						exposure_time = ceil(exposure_time * 0.9);

					exposure_time_changed = true;
				}
				
				if (gain != 0) {
					if (gain < 10)
						gain--;
					else
						gain = ceil(gain * 0.8);

					gain_changed = true;
				}
			}
		}
		// When the current average pixel value is smaller than
		// minimum pixel value of permissible range.
		else if (cur_pixel_avg < opt.pixel_min) {

			// No more control exposure time and gain value.
			if (exposure_time == 100 && gain == 100) {
				result = image->findAllPoints();

				pixel_diff = cur_pixel_avg - opt.pixel_min;
				loading = false;
			}
			// Control exposure time and gain value.
			else {
				if (exposure_time != 100) {
					exposure_time = ceil(exposure_time * 1.1);
					exposure_time_changed = true;
				}
	
				if (gain != 100) {
					if (gain == 0)
						gain = 1;
					else
						gain = ceil(gain * 1.2);

					gain_changed = true;
				}
			}
		}

		// The maximum value of exposure time and gain is 100.
		if (exposure_time > 100)
			exposure_time = 100;
		
		if (gain > 100)
			gain = 100;

		// Add average pixel value.
		// If average pixel value is not in the range, show H or L additionally.
		result += "\n  " + to_string(cur_pixel_avg);
		if (pixel_diff != 0) {
			if (cur_pixel_avg < 10)
				result += "  ";
			else if (cur_pixel_avg < 100)
				result += " ";

			if (pixel_diff > 0) {
				result += " H";
			}
			else if (pixel_diff < 0) {
				result += " L";
			}
		}

		// Terminal mode
		if (opt.option[TERMINAL]) {
			if (loading)
				cout << "exposure time: " << exposure_time << " gain: " << gain << endl;
			else
				cout << "========" << endl << result << endl << "========" << endl;
		}
		// Show to led
		else {
			ssd1306_clearDisplay();
			ssd1306_drawString(const_cast<char *>(result.c_str()));
			ssd1306_display();;
		}

		// Delay interval between analysis
		this_thread::sleep_for(chrono::milliseconds(opt.delay_ms));

		m.unlock();
	}

	delete image;
}
