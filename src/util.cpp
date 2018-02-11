#include <iostream>
#include <cmath>
#include <cctype>

#include "util.h"
#include "image.h"

using namespace std;

bool isInt(const char *str) {
	if (*str == '-')
		str++;

	while (*str) {
		if (!isdigit(*str))
			return false;

		str++;
	}

	return true;
}

int getPointDistance(const Point_t &p1, const Point_t &p2) {
	int d = ceil(sqrt(pow(p1.x - p2.x, 2) + pow(p1.y - p2.y, 2)));

	return d;
}
