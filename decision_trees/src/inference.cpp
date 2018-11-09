#include "inference.h"

const float splits[][2] = {{2.963}, {-4.885, 4.806}, {-0.115, 0.568}, {0.268}, {5443.253},\
	{1989.434}, {25264.602, 59657.531}, {-2.084, -2.205}, {5.657}};
const int splitLengths[9] = {1, 2, 2, 1, 1, 1, 2, 2, 1};
int prod = 1;

void calcProd() {
	int iter;

	for (iter = 1; iter < numsplits; iter++)
		prod *= splitLengths[iter];
	
}

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
	int low, high, mid, interval;
	float i1, i2;
	
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

int accInterval(float *feats) {
	int iter, retVal, intrvl, prevProd, val;

	retVal = 0;

	val = prod;
	for (iter = 0; iter < (numsplits - 1); iter++) {
		intrvl = whichInterval((float *)splits[iter], splitLengths[iter], feats[iter]);
		prevProd = val;
		val = prevProd/splitLengths[iter];
		retVal += (intrvl * val);
	}
	
	retVal += whichInterval((float *)splits[iter], splitLengths[iter], feats[iter]);
	return retVal;
}

size_t readField(File* file, char* str, size_t size, const char* delim) {
  char ch;
  size_t n = 0;
  while ((n + 1) < size && file->read(&ch, 1) == 1) {
    // Delete CR.
    if (ch == '\r') {
      continue;
    }
    str[n++] = ch;
    if (strchr(delim, ch)) {
        break;
    }
  }
  str[n] = '\0';
  return n;
} 

void processData(File file)
{
	long i; 
	int j, result;
	size_t n;
	char str[15], *ptr;
	float featarray[9];
	unsigned long start, end, tot;

	tot = 0;

	for (i = 0; i < 174640; i++) {
		for (j = 0; j < 9; j++) {
			n = readField(&file, str, sizeof(str), ",\n");
			
			if (n == 0) errorHalt("too less lines");
			featarray[j] = strtof(str, &ptr);

			while (*ptr == ' ')
				ptr++;

			if (*ptr != ',' && *ptr != '\n' && *ptr != '\0')
				errorHalt("extra characters in field");
      		
			/*Serial.print(featarray[j]);
			Serial.print(",");*/
		}
		/*Serial.println();
		Serial.println(accInterval(featarray));*/
		start = micros();
		result = accInterval(featarray);
		end = micros();
		tot += (end - start);

	}
	Serial.print("Time elapsed (micro seconds):");
	Serial.println(tot);
}
