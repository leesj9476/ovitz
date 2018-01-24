#include <opencv2/core/core.hpp>
#include <raspicam/raspicam_cv.h>
#include <string>
#include <chrono>
#include <thread>

#include "capture.h"

using namespace cv;
using namespace raspicam;
using namespace std;

Capture::Capture() {
	cam.open();
	output_file = "output.png";
	duration = chrono::seconds(3);
}

Capture::~Capture() {
	cam.release();
}

bool Capture::isValid() {
	return cam.isOpened();
}

bool Capture::setOption(int propID, double val) {
	switch (propID) {
	case CV_CAP_PROP_EXPOSURE:
		if (val < 1 || val > 100)
			return false;
		
		break;
	case CV_CAP_PROP_GAIN:
		if (val < 0 || val > 100)
			return false;
		
		break;
	case CV_CAP_PROP_FORMAT:
		if (val != CV_8UC1 && val != CV_8UC3)
			return false;

		break;
	case CV_CAP_PROP_FRAME_WIDTH:
	case CV_CAP_PROP_FRAME_HEIGHT:
		break;

	default:
		return false;
	}

	return cam.set(propID, val);
}

void Capture::setOutputFilename(const string &filename) {
	output_file = filename;
}

void Capture::setWaitSec(int sec) {
	duration = chrono::seconds(sec);
}

bool Capture::shot() {
	Mat image;
	
	this_thread::sleep_for(duration);
	if (!cam.grab())
		return false;

	cam.retrieve(image);
	imwrite(output_file, image);

	return true;
}
