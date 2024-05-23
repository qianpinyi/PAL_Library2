#ifndef PAL_THREAD_CPP
#define PAL_THREAD_CPP 1

#include "../PAL_BuildConfig.h"

#define PAL_Thread_ThreadType_CPP11 1
#define PAL_Thread_ThreadType_SDL 2
#define PAL_Thread_ThreadType_WINAPI 3

#define PAL_Thread_ThreadType PAL_Thread_ThreadType_SDL

#if PAL_Thread_ThreadType==PAL_Thread_ThreadType_SDL
	#include <SDL2/SDL_thread.h>
#elif PAL_Thread_ThreadType==PAL_Thread_ThreadType_CPP11
	#include <thread>
#else
	#error Uncompleted code
#endif

#include <atomic>

namespace PAL_Parallel
{
	template <class R,class A> class PAL_Thread
	{
		protected:
			struct ThreadData
			{
				R(*func)(A&);
				A funcdata;
				R ret;
				std::atomic_flag Flag;
				bool Detached;
				
				bool Remove()
				{
					bool re=0;
					while (Flag.test_and_set());
					if (!Detached)
						Detached=1;
					else re=1;
					Flag.clear();
					return re;
				}
				
				ThreadData(R(*_func)(A&),const A &_funcdata)
				:func(_func),funcdata(_funcdata),Flag(ATOMIC_FLAG_INIT),Detached(0)
				{
					if (func==nullptr)
						throw "PAL_Thread: ThreadData func is nullptr.";
				}
			}*data=nullptr;
			
		#if PAL_Thread_ThreadType==PAL_Thread_ThreadType_SDL
			SDL_Thread *Th=nullptr;
			
			static int ThreadFunc(void *userdata)
			{
				ThreadData *data=(ThreadData*)userdata;
				data->ret=data->func(data->funcdata);
				if (data->Remove())
					delete data;
				return 0;
			}
			
		#else
			#error Uncompleted code
		#endif
		
		public:
			R Join();
			void Detach();
			bool Valid() const;
			bool Run(R(*_func)(A&),const A &_funcdata,const char *name="");
			
			~PAL_Thread();//if uncomplete, it will wait until complete(join)
			PAL_Thread() {}
			template <typename ...Ts> PAL_Thread(const Ts &...args) {Run(args...);}
			
			PAL_Thread(const PAL_Thread&)=delete;
			PAL_Thread(const PAL_Thread&&)=delete;
			PAL_Thread& operator = (const PAL_Thread&)=delete;
			PAL_Thread& operator = (const PAL_Thread&&)=delete;
	};
	
#if PAL_Thread_ThreadType==PAL_Thread_ThreadType_SDL
	template <class R,class A> R PAL_Thread<R,A>::Join()
	{
		if (!Valid())
			throw "PAL_Thread:Join twice.";
		SDL_WaitThread(Th,NULL);
		Th=nullptr;
		return data->ret;
	}
	
	template <class R,class A> void PAL_Thread<R,A>::Detach()
	{
		if (!Valid()) return;
		SDL_DetachThread(Th);
		Th=nullptr;
		if (data->Remove())
			delete data;
		else data=nullptr;
	}
	
	template <class R,class A> bool PAL_Thread<R,A>::Valid() const
	{return Th!=nullptr;}
	
	template <class R,class A> bool PAL_Thread<R,A>::Run(R(*_func)(A&),const A &_funcdata,const char *name)
	{
		if (Valid()) return 0;
		if (data!=nullptr)
			delete data;
		data=new ThreadData(_func,_funcdata);
		Th=SDL_CreateThread(ThreadFunc,name,data);
		if (Th==nullptr)
		{
			delete data;
			data=nullptr;
			return 0;
		}
		else return 1;
	}
	
	template <class R,class A> PAL_Thread<R,A>::~PAL_Thread()
	{
		if (Valid())
			Join();
		if (data!=nullptr)
			delete data;
	}
#else
	#error Uncompleted code
#endif
};

#endif
