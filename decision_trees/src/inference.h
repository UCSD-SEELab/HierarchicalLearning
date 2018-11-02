#define numsplits 9

extern const float splits[][2];
extern const int splitLengths[9];

void calcProd();
int whichInterval(float *, int, float);
int binSearch(float *, int, float);
int accInterval(float *);
