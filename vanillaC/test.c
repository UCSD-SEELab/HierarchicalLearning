#include <stdio.h>
#include "inference.h"

int main()
{
	float a[4] = {-2.33, 4.65, 8.77, 23};
	printf("4: %d\n", whichInterval(a, 4, 7));
	printf("3: %d\n", whichInterval(a, 4, 3));
	printf("11: %d\n", whichInterval(a, 4, 11));
	printf("70: %d\n", whichInterval(a, 4, 70));
	printf("-7: %d\n", whichInterval(a, 4, -7));
}
