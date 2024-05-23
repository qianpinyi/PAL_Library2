#ifndef PAL_SDL_EX_0_CPP
#define PAL_SDL_EX_0_CPP 1

#include "../PAL_BuildConfig.h"
#include PAL_SDL_HeaderPath

namespace PAL::Legacy
{
	void SendSDLUserEventMsg(Uint32 eventtype,unsigned int code=0,void *data1=NULL,void *data2=NULL)
	{
		if (eventtype!=(Uint32)-1)
		{
			SDL_Event event;
			SDL_memset(&event,0,sizeof(event));
			event.type=eventtype;
			event.user.code=code;
			event.user.data1=data1;
			event.user.data2=data2;
			SDL_PushEvent(&event);
		}
	}
}

#endif
