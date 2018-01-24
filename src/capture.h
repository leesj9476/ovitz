#ifndef __CAPTURE_H__
#define __CAPTURE_H__

#include <raspicam/raspicam_cv.h>
#include <string>
#include <chrono>

class Capture {
public:
	Capture();
	~Capture();

	bool isValid();
	bool setOption(int, double);
	void setOutputFilename(const std::string &);
	void setWaitSec(int);

	bool shot();

private:
	raspicam::RaspiCam_Cv cam;
	std::string output_file;
	std::chrono::seconds duration;

	int width;
	int height;
};

#endif
