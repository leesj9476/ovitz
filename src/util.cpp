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

double* calcVecDistance(Point_t *p1, Point_t *p2) {
	double x_dist;
	double y_dist;

	double *dist = new double[ROW_POINT_NUM * COL_POINT_NUM];
	for (uint i = 0; i < ROW_POINT_NUM * COL_POINT_NUM; i++) {
		x_dist = (p1[i].x - p2[i].x);
		y_dist = (p1[i].y - p2[i].y);
		dist[i] = sqrt(x_dist * x_dist + y_dist * y_dist);
	}

	return dist;
}
