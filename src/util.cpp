#include <iostream>
#include <vector>
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

vector<double> calcVecDistance(vector<Point_t *> &v1, vector<Point_t *> v2) {
	if (v1.size() != v2.size())
		return {};

	double x_dist;
	double y_dist;

	vector<double> dist;
	for (uint i = 0; i < v1.size(); i++) {
		x_dist = (v1[i]->x - v2[i]->x);
		y_dist = (v1[i]->y - v2[i]->y);

		dist.push_back(sqrt(x_dist * x_dist + y_dist * y_dist));
	}

	return dist;
}
