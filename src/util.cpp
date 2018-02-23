#include <iostream>
#include <fstream>
#include <cmath>
#include <cctype>
#include <string>

#include "util.h"
#include "image.h"

using namespace std;

extern double *lens_mat[5][5];

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

void makeLensMatrix() {
	int points[5] = { 10, 26, 58, 98, 146 };
	for (int i = 0; i < 5; i++) {
		for (int j = 0; j < 5; j++) {
			lens_mat[i][j] = new double[points[i]];
		}
	}

	for (int i = 0; i < 5; i++) {
		string filename = "./data/matrix_lens" + to_string(2*i + 3) + ".txt";
		ifstream f(filename);

		for (int row = 0; row < 5; row++) {
			for (int col = 0; col < points[i]; col++) {
				f >> lens_mat[i][row][col];
			}
		}

		f.close();
	}
}
