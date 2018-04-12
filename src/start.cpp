#include <wiringPi.h>

#include <iostream>
#include <fstream>
#include <cstdlib>
#include <unistd.h>
#include <string>
#include <thread>
#include <chrono>
#include <csignal>

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

string parseSettingFile();
void int_handler(int sig);

bool window_avail = false;

int main () {
	signal(SIGINT, int_handler);
	
	ifstream dup_lock("./lock/dup_lock");
	if (dup_lock.is_open())
		return -1;
	else
		system("touch ./lock/dup_lock");
	
	dup_lock.close();

	Oled oled;
	string menu = "A: start\nB: end";
	bool menu_print = false;

	ifstream window_lock("./lock/window_avail");
	if (window_lock.is_open())
		window_avail = true;
	else
		system("touch ./lock/window_avail");

	window_lock.close();

	wiringPiSetupGpio();
	pinMode(A_PIN, INPUT);
	pinMode(B_PIN, INPUT);
	
	wiringPiISR(A_PIN, INT_EDGE_FALLING, aPress);
	wiringPiISR(B_PIN, INT_EDGE_FALLING, bPress);

	string cmd = parseSettingFile();
	while (true) {
		char input;
		if (!menu_print) {
			if (cmd.find("-t") == string::npos) {
				oled.showString(menu);
				menu_print = true;
			}
			else {
				cout << "a: start" << endl << "b: end" << endl;
				cin >> input;
			}
		}

		if (a_pressed || input == 'a') {
			system(cmd.c_str());

			if (b_pressed) {
				b_pressed = false;
			}

			a_pressed = false;
			menu_print = false;
		}

		if (b_pressed || input == 'b') {
			break;
		}
	}
	oled.clear();

	system("rm ./lock/dup_lock");

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

string parseSettingFile() {
	string cmd = "./ovitz";
	string threshold_cmd = "";

	ifstream f("./conf/setting.conf");
	Oled oled;
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

		if (key == "basic_distance" && isUFloat(val_str)) {
			cmd += " -d " + to_string(stod(val));
		}
		else if (key == "real_pixel_size" && isUFloat(val_str)) {
			cmd += " --real_pixel_size " + to_string(stod(val));
		}
		else if (key == "pixel_min" && isUInt(val_str)) {
			cmd += " --pixel_min " + to_string(atoi(val_str));
		}
		else if (key == "pixel_max" && isUInt(val_str)) {
			cmd += " --pixel_max " + to_string(atoi(val_str));
		}
		else if (key == "focal" && isUFloat(val_str)) {
			cmd += " -f " + to_string(stod(val));
		}
		else if (key == "window" && val.find("true") != string::npos && window_avail) {
			cmd += " -w";
		}
		else if (key == "terminal" && val.find("true") != string::npos) {
			cmd += " -t";
		}
		else if (key == "auto_control" && val.find("off") != string::npos) {
			cmd += " --auto_control_off";
		}
		else if (key == "threshold_percent" && isUFloat(val_str)) {
			threshold_cmd = " -p " + to_string(stod(val));
		}
		else if (key == "threshold_top_percent" && isUFloat(val_str)) {
			threshold_cmd = " --tp " + to_string(stod(val));
		}
	}

	f.close();

	cmd += threshold_cmd;
	return cmd;
}

void int_handler(int sig) {
	Oled oled;
	oled.clear();
	system("rm ./lock/dup_lock");
	exit(0);
}
