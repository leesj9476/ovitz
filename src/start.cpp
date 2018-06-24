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

