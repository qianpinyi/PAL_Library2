#ifndef PAL_DEBUGOUTEXTRA_CPP
#define PAL_DEBUGOUTEXTRA_CPP

#include "../PAL_BuildConfig.h"

#include "PAL_Debug.cpp"
#include "PAL_Time.cpp"
#include "PAL_StringEX.cpp"
#include "../PAL_Thread/PAL_Mutex.cpp"

namespace PAL::Legacy
{
	Debug_Out& operator << (Debug_Out &dd,PAL_Parallel::PAL_Mutex &mu)
	{
		mu.Lock();
		return dd;
	}

	Debug_Out& operator >> (Debug_Out &dd,PAL_Parallel::PAL_Mutex &mu)//??
	{
		mu.Unlock();
		return dd;
	}

	Debug_Out& Time(Debug_Out &dd)
	{
		return dd<<"<"<<GetTime1()<<">";
	}

	Debug_Out& Ticks(Debug_Out &dd)
	{
		return dd<<"<"<<llTOstr(clock())<<">";
	}
}

#endif
