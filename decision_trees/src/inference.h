#include "application.h"
#include "SdFat.h"

#define errorHalt(msg) {Serial.println(F(msg)); SysCall::halt();}
#define numsplits 9

extern const float splits[][2];
extern const int splitLengths[9];

void calcProd();
int whichInterval(float *, int, float);
int binSearch(float *, int, float);
int accInterval(float *);
size_t readField(File* file, char* str, size_t size, const char* delim); 
void processData(File file);
