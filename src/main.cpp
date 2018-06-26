#include <iostream>
#include <fstream>
#include <unistd.h>
#include <csignal>
#include <thread>
#include <chrono>

#include "ssd1306_i2c.h"

#include "capture.h"
#include "image.h"
#include "types.h"
#include "util.h"

using namespace std;
using namespace cv;

double *lens_mat[5][5];

int main () {
	ifstream dup_lock("./lock/dup_lock");
	if (dup_lock.is_open())
		return -1;
	else
		system("touch ./lock/dup_lock");

	// options setting information
	Options opt = parseSettingFile();
	makeLensMatrix();

	// image mode
	if (opt.option[INPUT_IMAGE_FILE]) {
		Image image(opt);
		image.init();
		cout << image.findAllPoints() << endl;
	}
	// cam mode
	else {
		Capture cam(opt);

		// Initiate i2c led
		ssd1306_begin(SSD1306_SWITCHCAPVCC, SSD1306_I2C_ADDRESS);
		ssd1306_setTextSize(1);
		ssd1306_display();
		this_thread::sleep_for(chrono::milliseconds(1000));

		// Ctrl-c interrupt(kill -9 <pid>)
		signal(SIGINT, int_handler);

		if (!cam.isValid()) {
			cerr << "<error> camera open failed" << endl;
			return -1;
		}

		cam.shot();
	}

	return 0;
}
