#ifndef __VIDEO_H__
#define __VIDEO_H__

#include <string>

#include <opencv2/highgui/highgui.hpp>

class Video {
public:
	Video(const std::string &);
	~Video();

	void showVideo(const std::string & = "window", int = CV_WINDOW_AUTOSIZE);
	void exitVideo(const std::string & = "window");

	double getFps();
	bool isValid();

private:
	std::string video_filename;
	cv::VideoCapture video;
};

#endif
