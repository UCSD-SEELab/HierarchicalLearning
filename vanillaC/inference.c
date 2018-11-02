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
			break;
		default:
			interval = binSearch(partitions, length, val);

	}
	return interval;
}

int binSearch(float *partitions, int length, float val) {
	int low, high, mid, i1, i2, interval;
	
	low = 0;
	high = length - 1;

	while (low <= high) {
		mid = (low + high) >> 1;
		i1 = partitions[mid];
		i2 = partitions[mid + 1];

		if (i1 <= val && val < i2)
			return (mid + 1);
		else if (val < i1)
			high = mid - 1;
		else
			low = mid + 1;
	}

	if (high < 0)
		return 0;
	else
		return length;
}
