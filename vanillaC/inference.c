#include "inference.h"
#include <stdio.h>

int whichInterval(float *partitions, int length, float val) {
	int interval;

	switch (length) {
		case 1:
			interval = (val > partitions[0]) ? 1 : 0;
			break;
		case 2:
			if (val < partitions[0])
				interval = 0;
			else if (val > partitions[1])
				interval = 2;
			else
				interval = 1;
		default:
			break;
	}
	return interval;
}

int binSearch(float *partitions, int length, float val) {
	return 0;
}
