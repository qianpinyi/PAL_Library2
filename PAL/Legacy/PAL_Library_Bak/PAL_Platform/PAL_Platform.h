#ifndef PAL_PLATFORM_H
#define PAL_PLATFORM_H 1

#include "PAL_PlatformCommon.cpp"

#if PAL_CurrentPlatform == PAL_Platform_Windows
	#include "PAL_Windows.cpp"
#endif
#if PAL_CurrentPlatform == PAL_Platform_Linux
	#include "PAL_Linux.cpp"
#endif
#if PAL_CurrentPlatform == PAL_Platform_Android
	#include "PAL_Android.cpp"
#endif

#endif
