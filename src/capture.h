#ifndef __CAPTURE_H__
#define __CAPTURE_H__

#include <raspicam/raspicam_cv.h>

class Capture {
public:
	Capture(int, int, int, double, double);
	~Capture();

	bool isValid();
	int shot();

private:
	raspicam::RaspiCam_Cv cam;

	int pixel_max;
	int pixel_min;

	int basic_distance;

	double focal;
	double pixel_size;
};

#endif
