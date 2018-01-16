#include <opencv2/highgui/highgui.hpp>
#include <string>

#include "video.h"
#include "image.h"

using namespace std;
using namespace cv;

Video::Video(const std::string &filename) {
	video_filename = filename;
	video = VideoCapture(filename);
}

Video::~Video() {}

bool Video::isValid() {
	return video.isOpened();
}

double Video::getFps() {
	return video.get(CV_CAP_PROP_FPS);
}

void Video::showVideo(const string &window_name, int flags) {
	namedWindow(window_name, flags);
	while (true) {
		Mat frame;
		if (!video.read(frame)) {
			break;
		}
		
		Image read_frame(frame);
		read_frame.showImage();
	}
}

void Video::exitVideo(const string &window_name) {
	destroyWindow(window_name);
}
