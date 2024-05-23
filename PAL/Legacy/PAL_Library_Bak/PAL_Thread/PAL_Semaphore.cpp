#ifndef PAL_SEMAPHORE_CPP
#define PAL_SEMAPHORE_CPP 1

#include "../PAL_BuildConfig.h"

#include "../PAL_BasicFunctions/PAL_BasicFunctions_0.cpp"

#define PAL_Semaphore_SemType_SDL 1//User should init SDL first!
#define PAL_Semaphore_SemType_WINAPI 2

#define PAL_Semaphore_SemType PAL_Semaphore_SemType_SDL

#if PAL_Semaphore_SemType==PAL_Semaphore_SemType_SDL
	#include <SDL2/SDL_mutex.h>
#else
	#error Uncompleted code
#endif

#include <atomic>

namespace PAL_Parallel
{
	class PAL_Semaphore
	{
		public:
			enum:unsigned
			{
				TryWait=0,
				KeepWait=(unsigned)-1
			};
			enum:unsigned
			{
				SignalAll=0,
				SignalOne=1
			};
			
		protected:
			std::atomic_int count;//??
		#if PAL_Semaphore_SemType==PAL_Semaphore_SemType_SDL
			SDL_sem *sem=nullptr;
		#else
			#error Uncompleted code
		#endif
			
		public:
			bool Wait(unsigned timeOut=KeepWait);
			void Signal(unsigned n=SignalOne);
			int Value() const;
			
			~PAL_Semaphore();
			PAL_Semaphore(unsigned initValue);
			
			PAL_Semaphore(const PAL_Semaphore&)=delete;
			PAL_Semaphore(const PAL_Semaphore&&)=delete;
			PAL_Semaphore& operator = (const PAL_Semaphore&)=delete;
			PAL_Semaphore& operator = (const PAL_Semaphore&&)=delete;
	};
	
	template <unsigned N> class PAL_Barrier:public PAL_Semaphore
	{
		protected:
			std::atomic_uint an;
			
		public:
			inline void Barrier()
			{
				if (--an==0)
				{
					an=N;
					Signal(N-1);
				}
				else Wait();
			}
			
			inline void operator () ()
			{Barrier();}
			
			inline unsigned Remainder()
			{return an;}
			
			~PAL_Barrier() {}
			PAL_Barrier():PAL_Semaphore(0),an(N) {}
			
			PAL_Barrier(const PAL_Barrier&)=delete;
			PAL_Barrier(const PAL_Barrier&&)=delete;
			PAL_Barrier& operator = (const PAL_Barrier&)=delete;
			PAL_Barrier& operator = (const PAL_Barrier&&)=delete;
	};
	
#if PAL_Semaphore_SemType==PAL_Semaphore_SemType_SDL
	bool PAL_Semaphore::Wait(unsigned timeOut)
	{
		int re;
		--count;
		if (timeOut==TryWait)
			re=SDL_SemTryWait(sem);
		else if (timeOut==KeepWait)
			re=SDL_SemWait(sem);
		else re=SDL_SemWaitTimeout(sem,timeOut);
		++count;
		if (re==0)
			return true;
		else if (re==SDL_MUTEX_TIMEDOUT)
			return false;
		else throw "PAL_Semaphore:Failed in SDL_SemWaitXX.";
	}
	
	void PAL_Semaphore::Signal(unsigned n)
	{
		if (n==SignalOne)
		{
			if (SDL_SemPost(sem)!=0)
				throw "PAL_Semaphore:Failed in SDL_SemPost.";
		}
		else if (n!=SignalAll)
			while (n--)
				Signal();
		else while (Value()<0)
			Signal();
	}
	
	int PAL_Semaphore::Value() const
	{
		int re=SDL_SemValue(sem);
		return re==0?(int)count:re;
	}
	
	PAL_Semaphore::~PAL_Semaphore()//It is not safe if exist thread waiting on it.
	{SDL_DestroySemaphore(sem);}
	
	PAL_Semaphore::PAL_Semaphore(unsigned initValue=0):count(initValue)
	{
		sem=SDL_CreateSemaphore(initValue);
		if (sem==nullptr)
			throw "PAL_Semaphore:Failed to create SDL_semaphore.";
	}
#else
	#error Uncompleted code
#endif
};

#endif
