#include <string>

#include "oled.h"

#include "OledI2C.h"
#include "OledFont8x16.h"

using namespace std;

Oled::Oled() {
	oled = new SSD1306::OledI2C("/dev/i2c-1", 0x3C);
}

Oled::Oled(const std::string &device, uint8_t address) {
	oled = new SSD1306::OledI2C(device, address);
}

Oled::~Oled() {
	clear();
	oled->displayUpdate();
	delete oled;
}

void Oled::showString(const string &str, int row) {
	clear();
	drawString8x16(SSD1306::OledPoint(0, row * 16), str, SSD1306::PixelStyle::Set, *oled);
	oled->displayUpdate();
}

void Oled::clear() {
	oled->clear();
	oled->displayUpdate();
}

bool Oled::isValid() {
	return oled != NULL;
}
