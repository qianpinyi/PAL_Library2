#ifndef PAL_THREADWORKER_CPP
#define PAL_THREADWORKER_CPP 1

#include "../PAL_DataStructure/PAL_Tuple.cpp"

#include "PAL_Thread.cpp"
#include "PAL_Mutex.cpp"
#include "PAL_Semaphore.cpp"

#include "../PAL_DataStructure/PAL_LeastRecentlyUsed.cpp"

#include <list>
#include <atomic>

namespace PAL::Legacy::PAL_Parallel
{
	
	template <class T,int N> class PAL_ThreadWorker
	{
		protected:
			std::atomic_int SemWaitTime,//In ms
							ActiveWorkers,
							QuitFlag;//used for user to check if it need stop or pause; 1:Quit 2:Pause
			struct WorkerData
			{
				T CurrentData;
				unsigned ID;
				bool Active=0;
				PAL_ThreadWorker *PTW=nullptr;
				
				void SetActive(bool active)
				{
					if (Active==active)
						return;
					Active=active;
					if (Active)
						++PTW->ActiveWorkers;
					else --PTW->ActiveWorkers;
				}
			}WorkerDatas[N];
			PAL_Thread <int,WorkerData*> Th[N];
			PAL_Semaphore Sem;
			PAL_Mutex Mu;
			void (*func)(T&,const std::atomic_int&,int);
			bool Started=0;
			
			virtual bool Get(T&)=0;

			inline void Lock()//A prepared mutex, if needed
			{Mu.Lock();}
			
			inline void Unlock()
			{Mu.Unlock();}
			
			inline void WakeWork()
			{Sem.Signal();}
			
			static int ThreadFunc(WorkerData *&WD)
			{
				PAL_ThreadWorker *This=WD->PTW;
				while (This->QuitFlag!=1)
				{
					This->Sem.Wait(This->SemWaitTime);
					while (This->QuitFlag==0)
					{
						if (!This->Get(WD->CurrentData))
							break;
						WD->SetActive(1);
						This->func(WD->CurrentData,This->QuitFlag,WD->ID);
						WD->SetActive(0);
					}
				}
				return 0;
			}
			
		public:
			enum
			{
				NotstartWhenCreated=0,
				StartWhenCreated=1
			};
			
			inline void Pause(bool pause=1)
			{
				if (QuitFlag==0)
					QuitFlag=2;
			}
			
			inline void Continue()
			{Pause(0);}
			
			inline bool IsFree()
			{return ActiveWorkers==0;}
			
			inline int ActiveWorkerCounts()
			{return ActiveWorkers;}
			
//			void AsyncQuit()
//			{
//				if (!Started) return;
//
//			}
			
			void Quit()
			{
				if (!Started) return;
				QuitFlag=1;
				for (int i=0;i<N;++i)
					Sem.Signal();
				for (int i=0;i<N;++i)
					Th[i].Join();
				Started=0;
			}
			
			void Start()
			{
				if (Started) return;
				QuitFlag=0;
				for (int i=0;i<N;++i)
					if (WorkerDatas[i].PTW==nullptr)
					{
						WorkerDatas[i].ID=i;
						WorkerDatas[i].PTW=this;
						Th[i].Run(ThreadFunc,&WorkerDatas[i]);
						Sem.Signal();
					}
				Started=1;
			}
			
			inline void SetFunc(void (*_func)(T&,const std::atomic_int&,int))
			{func=_func;}
			
			virtual ~PAL_ThreadWorker()
			{
				Quit();
			}
			
			PAL_ThreadWorker(unsigned _semwaittime=1000):SemWaitTime(_semwaittime),ActiveWorkers(0),QuitFlag(0) {}
			
			PAL_ThreadWorker(void (*_func)(T&,const std::atomic_int&,int),bool startWhenCreated=0,unsigned _semwaittime=1000)
			:PAL_ThreadWorker(_semwaittime)
			{
				func=_func;
				if (startWhenCreated)
					Start();
			}
	};
	
	template <class T,int N=1> class PTW_List:public PAL_ThreadWorker <T,N>
	{
		protected:
			std::list <T> li;
		
			using PAL_ThreadWorker<T,N>::Lock;
			using PAL_ThreadWorker<T,N>::Unlock;
			using PAL_ThreadWorker<T,N>::WakeWork;
			
			virtual bool Get(T &data)
			{
				Lock();
				if (li.empty())
				{
					Unlock();
					return 0;
				}
				else
				{
					data=li.front();
					li.pop_front();
					Unlock();
					return 1;
				}
			}
			
		public:
			inline void Pushfront(const T &data)
			{
				Lock();
				li.push_front(data);
				Unlock();
				WakeWork();
			}
			
			inline void Pushback(const T &data)
			{
				Lock();
				li.push_back(data);
				Unlock();
				WakeWork();
			}
			
			inline void Clear()//Current running task won't be stoped.
			{
				Lock();
				li.clear();
				Unlock();
			}
			
			inline std::list <T> MoveAndClear()
			{
				Lock();
				auto re=li;
				li.clear();
				Unlock();
				return re;
			}
			
			inline size_t Size()
			{
				Lock();
				size_t re=li.size();
				Unlock();
				return re;
			}
			
			template <typename ...Ts> PTW_List(const Ts & ...args):PAL_ThreadWorker<T,N>(args...) {}
	};
	
	template <class T_Key,class T,int N=1> class PTW_LRU:public PAL_ThreadWorker <T,N>
	{
		protected:
			PAL_DS::LRU_LinkHashTable <T_Key,T> LRU;
		
			using PAL_ThreadWorker<T,N>::Lock;
			using PAL_ThreadWorker<T,N>::Unlock;
			using PAL_ThreadWorker<T,N>::WakeWork;
			
			virtual bool Get(T &data)
			{
				Lock();
				bool re=LRU.PopHead(data);
				Unlock();
				return re;
			}
			
		public:
			inline void Insert(const T_Key &key,const T &data)
			{
				Lock();
				if (LRU.Get(key)==nullptr)
					LRU.Insert(key,data);
				Unlock();
				WakeWork();
			}
			
			inline void Erase(const T_Key &key)
			{
				Lock();
				LRU.Erase(key);
				Unlock();
			}
			
			inline void Clear()
			{
				Lock();
				LRU.Clear();
				Unlock();
			}
			
			inline unsigned long long Size()
			{
				Lock();
				auto re=LRU.Count();
				Unlock();
				return re;
			}
			
			inline void SetSizeLimit(unsigned long long sizeLimit)
			{
				Lock();
				LRU.SetSizeLimit(sizeLimit);
				Unlock();
			}
			
			template <typename ...Ts> PTW_LRU(const Ts & ...args)
			:PAL_ThreadWorker<T,N>(args...),LRU(1000000000) {}
	};
	
	template <class T,int N=1> class PTW_Heap:public PAL_ThreadWorker <T,N>
	{
			
	};
	
	template <class T_Key,class T_Data,int N=1> class PTW_SplayMap:public PAL_ThreadWorker <T_Data,N>
	{
		protected:
			
			
		public:
			
			void Cancel(const T_Key &key)
			{
				
			}
	};
};

#endif
