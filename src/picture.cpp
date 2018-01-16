#include <opencv2/highgui/highgui.hpp>
#include <string>

#include "picture.h"

using namespace std;
using namespace cv;

Image::Image(const string &filename) {
	image_filename = filename;
	img = imread(filename);
}

Image::Image(const string &filename, int flags) {
	image_filename = filename;
	img = imread(filename, flags);
}

Image::~Image() {}

Size Image::size() {
	return img.size();
}

bool Image::isEmpty() {
	return img.empty();
}

bool Image::showImage(const string &window_name, int flags) {
	if (isEmpty())
		return false;

	namedWindow(window_name, flags);
	imshow(window_name, img);
	waitKey(0);

	return true;
}


void Image::exitImage(const string &window_name) {
	destroyWindow(window_name);
}
