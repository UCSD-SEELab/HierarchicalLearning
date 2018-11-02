#include "iostream"
#include "inference.h"

using namespace std;

int main()
{
	calcProd();
	float a[9] = {6.4983678,  0.88696843, 0.59811729, -0.45599312, 62240.84, 13285.165, 53057.996, 8.9070396, 8.7032156};
	/*cout << "4: " << whichInterval(a, 4, 4) << "\n";
	cout << "7: " << whichInterval(a, 4, 7) << "\n";
	cout << "11: " << whichInterval(a, 4, 11) << "\n";
	cout << "70: " << whichInterval(a, 4, 70) << "\n";
	cout << "-7: " << whichInterval(a, 4, -7) << "\n"; */

	cout<<accInterval(a);
}
