#ifndef PAL_MUTEX_CPP
#define PAL_MUTEX_CPP 1

#include "../PAL_BuildConfig.h"

//Config area
#define PAL_Mutex_MutexType_CPP11 1
#define PAL_Mutex_MutexType_SDL 2
#define PAL_Mutex_MutexType_WINAPI 3

#define PAL_Mutex_MutexType PAL_Mutex_MutexType_SDL

//End of config

#if PAL_Mutex_MutexType==PAL_Mutex_MutexType_SDL
	#include <SDL2/SDL_mutex.h>
#else
	#error Uncompleted code
#endif

namespace PAL_Parallel
{
	class PAL_Mutex
	{
		protected:
		#if PAL_Mutex_MutexType==PAL_Mutex_MutexType_SDL
			SDL_mutex *mu=nullptr;
		#else
			#error Uncompleted code
		#endif
		
		public:
			void Lock();
			void Unlock();
			bool TryLock();
			
			~PAL_Mutex();
			PAL_Mutex();
			
			PAL_Mutex(const PAL_Mutex&)=delete;
			PAL_Mutex(const PAL_Mutex&&)=delete;
			PAL_Mutex& operator = (const PAL_Mutex&)=delete;
			PAL_Mutex& operator = (const PAL_Mutex&&)=delete;
	};

#if PAL_Mutex_MutexType==PAL_Mutex_MutexType_SDL
	void PAL_Mutex::Lock()
	{
		if (SDL_LockMutex(mu)!=0)
			throw "PAL_Mutex:Failed in SDL_LockMutex.";
	}
	
	void PAL_Mutex::Unlock()
	{
		if (SDL_UnlockMutex(mu)!=0)
			throw "PAL_Mutex:Failed in SDL_UnlockMutex.";
	}
	
	bool PAL_Mutex::TryLock()
	{
		int re=SDL_TryLockMutex(mu);
		if (re==0)
			return 1;
		else if (re==SDL_MUTEX_TIMEDOUT)
			return 0;
		else throw "PAL_Mutex:Failed in SDL_TryLockMutex.";
			
	}
	
	PAL_Mutex::~PAL_Mutex()//It is not safe if it is locked.
	{SDL_DestroyMutex(mu);}
	
	PAL_Mutex::PAL_Mutex()
	{
		mu=SDL_CreateMutex();
		if (mu==nullptr)
			throw "PAL_Mutex:Failed to create SDL_Mutex";
	}
#else
	#error Uncompleted code
#endif

	class PAL_SpinLock
	{
		
	};
	
	class PAL_TicketLock
	{
		
	};
};

#endif
