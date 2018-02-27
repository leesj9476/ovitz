#include <wiringPi.h>
#include <opencv2/highgui/highgui.hpp>

#include <iostream>
#include <fstream>
#include <cstdlib>
#include <unistd.h>
#include <string>
#include <thread>
#include <chrono>

#include "oled.h"

#define A_PIN	5
#define B_PIN	6

using namespace std;

bool a_pressed = false;
bool b_pressed = false;

void aPress() {
	a_pressed = true;
}

void bPress() {
	b_pressed = true;
}

bool isUInt(const char *);
bool isUFloat(const char *);
string parseSettingFile(Oled &);

int main () {
	Oled oled;
	string menu = "A: start\nB: end";
	bool menu_print = false;

	wiringPiSetupGpio();
	pinMode(A_PIN, INPUT);
	pinMode(B_PIN, INPUT);
	
	wiringPiISR(A_PIN, INT_EDGE_FALLING, aPress);
	wiringPiISR(B_PIN, INT_EDGE_FALLING, bPress);

	string cmd = parseSettingFile(oled);
	cout << cmd << endl;
	while (true) {
		if (!menu_print) {
			oled.showString(menu);
			menu_print = true;
		}

		if (a_pressed) {
			system(cmd.c_str());

			if (b_pressed) {
				b_pressed = false;
			}

			a_pressed = false;
			menu_print = false;
		}

		if (b_pressed) {
			break;
		}
	}
	oled.clear();

	return 0;
}

bool isUInt(const char *str) {
	while (*str) {
		if (!isdigit(*str))
			return false;

		str++;
	}

	return true;
}

bool isUFloat(const char *str) {
	int point = 0;
	if (*str == '.')
		return false;

	while (*str) {
		if (*str == '.') {
			if (point == 0)
				point++;
			else
				return false;
		}
		else if (!isdigit(*str))
			return false;

		str++;
	}

	return true;
}

string parseSettingFile(Oled &oled) {
	string cmd = "./ovitz";

	ifstream f("./conf/setting.conf");
	if (!f.is_open()) {
		oled.showString("Setting file is not exist!!");
		this_thread::sleep_for(chrono::seconds(2));
		return "";
	}

	string line;
	while (getline(f, line)) {
		if (line[0] == '#' || line[0] == ' ' || line[0] == '\n')
			continue;

		string key;
		string val;
		size_t pos = line.find('=');
		if (pos == string::npos || pos == line.size() - 1)
			continue;

		key = line.substr(0, pos);
		val = line.substr(pos + 1);
		const char *val_str = val.c_str();

		if (key == "basic_distance" && isUInt(val_str)) {
			cmd += " -d " + to_string(atoi(val_str));
		}
		else if (key == "real_pixel_size" && isUFloat(val_str)) {
			cmd += " --real_pixel_size " + to_string(stof(val));
		}
		else if (key == "pixel_min" && isUInt(val_str)) {
			cmd += " --pixel_min " + to_string(atoi(val_str));
		}
		else if (key == "pixel_max" && isUInt(val_str)) {
			cmd += " --pixel_max " + to_string(atoi(val_str));
		}
		else if (key == "focal" && isUFloat(val_str)) {
			cmd += " -f " + to_string(stof(val));
		}
		else if (key == "window" && val.find("true") != string::npos) {
			cmd += " -w";
		}
		else if (key == "terminal" && val.find("true") != string::npos) {
			cmd += " -t";
		}
	}

	f.close();

	return cmd;
}
