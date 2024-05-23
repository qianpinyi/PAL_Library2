#ifndef PAL_MULTISEMAPHORE_CPP
#define PAL_MULTISEMAPHORE_CPP 1

#include "../PAL_BuildConfig.h"

#include "PAL_Semaphore.cpp"
#include "PAL_Mutex.cpp"

#include <array>

namespace PAL_Parallel
{
	template <unsigned N> class PAL_MultiSemaphore:protected PAL_Mutex
	{
		protected:
			PAL_Semaphore sems[N];
			unsigned values[N];
			
			inline bool CheckI()
			{return 1;}
			
			template <typename ...Ts> inline bool CheckI(unsigned i,Ts ...args)
			{
				if (values[i]>0)
					return CheckI(args...);
				else
				{
					Unlock();
					sems[i].Wait();
					Lock();
					return 0;
				}
			}
			
			inline void FetchI() {}
			
			template <typename ...Ts> inline void FetchI(unsigned i,Ts ...args)
			{
				--values[i];
				FetchI(args...);
			}
			
			inline void _SignalI() {}
			
			template <typename ...Ts> inline void _SignalI(unsigned i,Ts ...args)
			{
				++values[i];
				sems[i].Signal(PAL_Semaphore::SignalAll);
				_SignalI(args...);
			}
			
			template <unsigned m> inline bool CheckN()
			{return 1;}
			
			template <unsigned m,typename ...Ts> inline bool CheckN(unsigned x,Ts ...args)
			{
				if (x<=values[m])
					return CheckN<m+1>(args...);
				else
				{
					Unlock();
					sems[m].Wait();
					Lock();
					return 0;
				}
			}
			
			template <unsigned m> inline void FetchN() {}
			
			template <unsigned m,typename ...Ts> inline void FetchN(unsigned x,Ts ...args)
			{
				values[m]-=x;
				FetchN<m+1>(args...);
			}
			
			template <unsigned m> inline void SignalN() {}
			
			template <unsigned m,typename ...Ts> inline void SignalN(unsigned x,Ts ...args)
			{
				if (x)
				{
					values[m]+=x;
					sems[m].Signal(PAL_Semaphore::SignalAll);
				}
				SignalN<m+1>(args...);
			}
			
			template <unsigned m> inline void Init() {}
			
			template <unsigned m,typename ...Ts> inline void Init(unsigned x,Ts ...args)
			{
				values[m]=x;
				Init<m+1>(args...);
			}
			
		public:
			template <typename ...Ts> inline void WaitI(Ts ...args)//Wait for args indexed semaphores
			{
				Lock();
				while (!CheckI(args...));
				FetchI(args...);
				Unlock();
			}
			
			template <typename ...Ts> inline void SignalI(Ts ...args)//Signal args indexed semaphores
			{
				Lock();
				_SignalI(args...);
				Unlock();
			}
			
			template <typename ...Ts> inline void WaitN(Ts ...args)//Wait for N semaphores with different value...
			{
				Lock();
				while (!CheckN<0>(args...));
				FetchN<0>(args...);
				Unlock();
			}
			
			template <typename ...Ts> inline void SignalN(Ts ...args)//Signal N semaphores with different value...
			{
				Lock();
				SignalN<0>(args...);
				Unlock();
			}
			
			inline void WaitA(const std::array <unsigned,N> &nums)//Similar to WaitN, the only difference is parameter need {}
			{
				Lock();
				while (1)
				{
					int i;
					for (i=0;i<N;++i)
						if  (values[i]<nums[i])
							break;
					if (i==N)
					{
						for (i=0;i<N;++i)
							values[i]-=nums[i];
						break;
					}
					else
					{
						Unlock();
						sems[i].Wait();
						Lock();
					}
				}
				Unlock();
			}
			
			inline void SignalA(const std::array <unsigned,N> &nums)//Similar to WaitN, the only difference is parameter need {}
			{
				Lock();
				for (int i=0;i<N;++i)
					if (nums[i])
					{
						values[i]+=nums[i];
						sems[i].Signal(PAL_Semaphore::SignalAll);
					}
				Unlock();
			}
			
			unsigned ValueI(unsigned i) const
			{return values[i];}
			
			unsigned WaitersI(unsigned i) const
			{return max(0,-sems[i].Value());}
			
			~PAL_MultiSemaphore() {}
			
			PAL_MultiSemaphore()
			{
				MemsetT(values,0,N);
			}
			
			PAL_MultiSemaphore(const std::array <unsigned,N> &initValues)
			{
				for (int i=0;i<N;++i)
					values[i]=initValues[i];
			}
			
			template <typename ...Ts> PAL_MultiSemaphore(Ts ...args)
			{
				Init<0>(args...);
			}
			
			PAL_MultiSemaphore(const PAL_MultiSemaphore&)=delete;
			PAL_MultiSemaphore(const PAL_MultiSemaphore&&)=delete;
			PAL_MultiSemaphore& operator = (const PAL_MultiSemaphore&)=delete;
			PAL_MultiSemaphore& operator = (const PAL_MultiSemaphore&&)=delete;
	};
};

#endif
