#include <iostream>
#include <fstream>
#include <unistd.h>
#include <csignal>

#include "ssd1306_i2c.h"

#include "capture.h"
#include "image.h"
#include "types.h"
#include "util.h"

using namespace std;
using namespace cv;

double *lens_mat[5][5];

void int_handler(int);

int main () {
	ifstream dup_lock("./lock/dup_lock");
	if (dup_lock.is_open())
		return -1;
	else
		system("touch ./lock/dup_lock");

	// options setting information
	Options options = parseSettingFile();
	makeLensMatrix();

	// Initiate i2c led
	ssd1306_begin(SSD1306_SWITCHCAPVCC, SSD1306_I2C_ADDRESS);
	ssd1306_setTextSize(1);
	ssd1306_clearDisplay();
	ssd1306_display();

	// Ctrl-c interrupt(kill -9 <pid>)
	signal(SIGINT, int_handler);

	// image mode
	if (options.option[INPUT_IMAGE_FILE]) {
		Image image(options);
		image.init();
		cout << image.findAllPoints() << endl;
	}
	// cam mode
	else {
		Capture cam(options);

		if (!cam.isValid()) {
			cerr << "<error> camera open failed" << endl;
			return -1;
		}

		cam.shot();
	}

	return 0;
}
