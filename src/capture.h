#ifndef __CAPTURE_H__
#define __CAPTURE_H__

#include <raspicam/raspicam_cv.h>

#include "util.h"
#include "types.h"

class Capture {
public:
	Capture(Options &);
	~Capture();

	bool isValid();
	void shot();

private:
	raspicam::RaspiCam_Cv cam;

	Options opt;
};

#endif
