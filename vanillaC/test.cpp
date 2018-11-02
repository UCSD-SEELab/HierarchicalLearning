#include "iostream"
#include "inference.h"

using namespace std;

int main()
{
	float a[4] = {-2.33, 4.65, 8.77, 23};
	cout << "4: " << whichInterval(a, 4, 4) << "\n";
	cout << "7: " << whichInterval(a, 4, 7) << "\n";
	cout << "11: " << whichInterval(a, 4, 11) << "\n";
	cout << "70: " << whichInterval(a, 4, 70) << "\n";
	cout << "-7: " << whichInterval(a, 4, -7) << "\n";
}
