#include <iostream>
#include <cmath>

#include "util.h"
#include "image.h"

using namespace std;

void help() {
	cout << "Usage:" << endl << endl;
	cout << "    ./ovitz [options]" << endl << endl;
	cout << "options" << endl;
	cout << "    -i         asdf" << endl;
	cout << "    -v         asdf" << endl;
	cout << "    -h         asdf" << endl;
}

double* calcVecDistance(Point_t *p1, Point_t *p2) {
	double x_dist;
	double y_dist;

	double *dist = new double[UNIT_WIDTH_NUM * UNIT_HEIGHT_NUM];
	for (uint i = 0; i < UNIT_WIDTH_NUM * UNIT_HEIGHT_NUM; i++) {
		x_dist = (p1[i].x - p2[i].x);
		y_dist = (p1[i].y - p2[i].y);
		dist[i] = sqrt(x_dist * x_dist + y_dist * y_dist);
	}

	return dist;
}
