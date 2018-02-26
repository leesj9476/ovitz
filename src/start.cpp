#include <wiringPi.h>

#include <iostream>
#include <cstdlib>
#include <unistd.h>

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

int main () {
	wiringPiSetupGpio();
	pinMode(A_PIN, INPUT);
	pinMode(B_PIN, INPUT);
	
	wiringPiISR(A_PIN, INT_EDGE_FALLING, aPress);
	wiringPiISR(B_PIN, INT_EDGE_FALLING, bPress);

	while (true) {
		if (a_pressed) {
			system("./ovitz -d22");
			a_pressed = false;
		}

		if (b_pressed) {
			break;
		}
	}

	return 0;
}
