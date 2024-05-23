#ifndef PAL_BUILDCONFIG_H
#define PAL_BUILDCONFIG_H
#ifndef PAL_BUILDCONFIG_USER_H

#define PAL_Library_Version_Main  2
#define PAL_Library_Version_Sub   0
#define PAL_Library_Version_Third 2403250
#define PAL_Library_Author "qianpinyi"
#define PAL_Library_AuthorMail "qianpinyi@outlook.com"

//Marcos:
#define PAL_Platform_Unknown	0x00
#define PAL_Platform_Windows	0x01
#define PAL_Platform_Linux		0x02
#define PAL_Platform_Android	0x04

//Config Area:
#ifndef PAL_CurrentPlatform
	#if defined(_WIN32) || defined(_WIN64)
		#define PAL_CurrentPlatform PAL_Platform_Windows
	#elif defined(__linux__)
		#define PAL_CurrentPlatform PAL_Platform_Linux
	#elif defined(__ANDROID__)
		#define PAL_CurrentPlatform PAL_Platform_Android
	#else
		#define PAL_CurrentPlatform PAL_Platform_Unknown
	#endif
#endif

#ifndef PAL_CFG_EnableOldFunctions
	#define PAL_CFG_EnableOldFunctions 1//0,1
#endif
#ifndef PAL_SDL_IncludeSyswmHeader
	#define PAL_SDL_IncludeSyswmHeader 0//0,1
#endif

#ifndef PAL_Library2UseSDL
	#define PAL_LibraryUseSDL 0//0,1
#else
	#define PAL_LibraryUseSDL 1
#endif
#ifndef PAL_SDL_HeaderPath
	#define PAL_SDL_HeaderPath <SDL2/SDL.h>
#endif
#ifndef PAL_SDL_syswmHeaderPath
	#define PAL_SDL_syswmHeaderPath <SDL2/SDL_syswm.h>
#endif
#ifndef PAL_SDL_ttfHeaderPath
	#define PAL_SDL_ttfHeaderPath <SDL2/SDL_ttf.h>
#endif
#ifndef PAL_SDL_netHeaderPath
	#define PAL_SDL_netHeaderPath <SDL2/SDL_net.h>
#endif
#ifndef PAL_SDL_imageHeaderPath
	#define PAL_SDL_imageHeaderPath <SDL2/SDL_image.h>
#endif
#ifndef PAL_OpenCV2_HeaderPath
	#define PAL_OpenCV2_HeaderPath <opencv2/opencv.hpp>
#endif
#ifndef PAL_OpenCV2_matHeaderPath
	#define PAL_OpenCV2_matHeaderPath <opencv2/core/mat.hpp>
#endif

#ifndef PAL_GUI_UseOriginalMain
	#define PAL_GUI_UseOriginalMain 1
#endif
#ifndef PAL_GUI_UseWindowsIME
	#define PAL_GUI_UseWindowsIME 0
#endif

#ifndef PAL_GUI_UseSystemFontFile
	#define PAL_GUI_UseSystemFontFile 1
#endif

#ifndef PAL_GUI_EnableTransparentBackground
	#define PAL_GUI_EnableTransparentBackground 0
#endif

#ifndef PAL_GUI_EnablePenEvent
	#define PAL_GUI_EnablePenEvent 0
#endif

#ifndef InitialDefaultDebugOutChannel
	#define InitialDefaultDebugOutChannel DebugOut_CERR
#endif

#ifndef EnableBasicfunciton0PredefineUSintX
	#define EnableBasicfunciton0PredefineUSintX 1
#endif

//#ifndef PAL_LibraryWithVersion2
//	#define PAL_LibraryWithVersion2 0
//#endif
#define PAL_LibraryWithVersion2 1

////End of Config
//
#if PAL_GUI_EnableTransparentBackground == 1
	#define PAL_SDL_IncludeSyswmHeader 1
#endif

#if PAL_CurrentPlatform==PAL_Platform_Windows
	#define _WIN32_WINNT 0x0602
	#define PLATFORM_PATHSEPERATOR '\\'
	#define PLATFORM_PATHSEPERATORW L'\\'
	#define INITGUID
#elif PAL_CurrentPlatform==PAL_Platform_Linux
	#define PLATFORM_PATHSEPERATOR '/'
	#define PLATFORM_PATHSEPERATORW L'/'
#else
	#error
#endif

#if PAL_GUI_EnablePenEvent == 1
	#define PAL_SDL_IncludeSyswmHeader 1
#endif

//#endif
//#endif

#endif
#endif
