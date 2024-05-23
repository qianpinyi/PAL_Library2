#define PAL_AUTOINIT_VTERM
#include <PAL/IO/Xout>
#include <PAL/Legacy/PAL_Library/PAL_BasicFunctions/PAL_System.cpp>
#include <PAL/Single>
using namespace PAL::IO;

int main()
{
	xout[Test]<<"Hello"<<endl;
	return 0;
}
