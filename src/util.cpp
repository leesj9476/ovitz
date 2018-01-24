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

void help() {
	cout << "Usage:" << endl << endl;
	cout << "    ./ovitz [options]" << endl << endl;
	cout << "options" << endl;
	cout << "    -f <image filename>   Open image file(.png, .jpg, ...)" << endl;
	cout << "    -e <exposure time>    -1 auto. [1, 100] shutter speed from 0 to 33ms" << endl;
	cout << "    -g <gain>             (iso) [1, 100]" << endl;
	cout << "    -h                    print help messages" << endl;
}

Variance_t* calcVecVariance(Point_t *p1, Point_t *p2) {
	Variance_t *variance = new Variance_t[ROW_POINT_NUM * COL_POINT_NUM];
	for (uint i = 0; i < ROW_POINT_NUM * COL_POINT_NUM; i++) {
		variance[i].x = (p1[i].x - p2[i].x);
		variance[i].y = (p1[i].y - p2[i].y);
	}

	return variance;
}
