#ifndef __IMAGE_H__
#define __IMAGE_H__

#include <string>

#include <opencv2/highgui/highgui.hpp>

class Image {
public:
	Image(const std::string &);
	Image(const std::string &, int);
	Image(cv::Mat);
	~Image();

	void showImage(const std::string & = "window", int = CV_WINDOW_AUTOSIZE, int = 0);
	void exitImage(const std::string & = "window");

	cv::Size getSize();	
	bool isValid();

private:
	std::string image_filename;
	cv::Mat image;
};

#endif
