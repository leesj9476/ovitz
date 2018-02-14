#ifndef __OLED_H__
#define __OLED_H__

#include <string>

#include "OledI2C.h"

class Oled {
public:
	Oled();
	Oled(const std::string &, uint8_t);
	~Oled();

	void showString(const std::string &, int = 0);
	void clear();

	bool isValid();

private:
	SSD1306::OledI2C *oled;
};

#endif
