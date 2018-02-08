#ifndef __CAPTURE_H__
#define __CAPTURE_H__

#include <raspicam/raspicam_cv.h>
#include <string>
#include <chrono>

class Capture {
public:
	Capture(int);
	~Capture();

	bool isValid();
	bool shot();

private:
	raspicam::RaspiCam_Cv cam;

	int basic_distance;
};

#endif
