#include <opencv2/highgui/highgui.hpp>
#include <string>

#include "image.h"

using namespace std;
using namespace cv;

Image::Image(const string &filename) {
	image_filename = filename;
	image = imread(filename);
}

Image::Image(const string &filename, int flags) {
	image_filename = filename;
	image = imread(filename, flags);
}

Image::Image(Mat frame) {
	image = frame;
}

Image::~Image() {}

Size Image::getSize() {
	return image.size();
}

bool Image::isValid() {
	return !image.empty();
}

void Image::showImage(const string &window_name, int flags, int wait_key) {
	namedWindow(window_name, flags);
	imshow(window_name, image);
	waitKey(wait_key);
}


void Image::exitImage(const string &window_name) {
	destroyWindow(window_name);
}
