#ifndef __PICTURE_H__
#define __PICTURE_H__

#include <string>

#include <opencv2/highgui/highgui.hpp>

class Image {
public:
	Image(const std::string &);
	Image(const std::string &, int);
	~Image();

	cv::Size size();	
	bool isEmpty();
	bool showImage(const std::string & = "window", int = CV_WINDOW_AUTOSIZE);
	void exitImage(const std::string & = "window");

private:
	std::string image_filename;
	cv::Mat img;
};

#endif
