#ifndef PAL_NET_0_CPP
#define PAL_NET_0_CPP 1

#include <string>
#include "../PAL_BasicFunctions/PAL_StringEX.cpp"

namespace PAL::Legacy
{
	std::string IPv4ToStr(unsigned int x,bool ShowFullZero=0)
	{return llTOstr(x%256,ShowFullZero?3:1)+"."+llTOstr(x/256%256,ShowFullZero?3:1)+"."+llTOstr(x/256/256%256,ShowFullZero?3:1)+"."+llTOstr(x/256/256/256,ShowFullZero?3:1);}
}
	
#endif
