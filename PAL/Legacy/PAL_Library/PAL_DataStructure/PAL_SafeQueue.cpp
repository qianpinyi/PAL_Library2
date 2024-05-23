#ifndef PAL_SafeQUEUE_CPP
#define PAL_SafeQUEUE_CPP 1

#include <mutex>

namespace PAL::Legacy
{
	template <class T> class SafeQueue
	{
		public:
			class SafeQueueData
			{
				protected:
					mutable T *data;
				
				public:
					inline bool operator ! ()
					{return data==NULL;}
					
					inline bool Empty()
					{return data==NULL;}
					
					inline T& operator () ()
					{return *data;}
					
					inline T& AccessData()
					{return *data;}
					
					inline T* operator & ()
					{
						T *re=data;
						data=NULL;
						return re;
					}
					
					inline T* HandOverData()
					{
						T *re=data;
						data=NULL;
						return re;
					}
					
					inline void operator = (const SafeQueueData &tar)
					{
						if (data)
							delete data;
						data=tar.data;
						tar.data=NULL;
					}
					
					SafeQueueData(const SafeQueueData &tar)
					{
						data=tar.data;
						tar.data=NULL;
					}
					
					~SafeQueueData()
					{
						if (data)
							delete data;
					}
					
					explicit SafeQueueData(const T &da):data(new T(da)) {}
					
					explicit SafeQueueData():data(NULL) {}
			};
			
		protected:
			class QueueNode:public SafeQueueData
			{
				public:
					QueueNode *nxt=NULL;
					QueueNode(const T &da):SafeQueueData(da) {}
			};
			QueueNode *frontNode=NULL,
					  *backNode=NULL;
			long long size=0;
			std::mutex mu;
			
			void CopyFrom(SafeQueue &tar)
			{
				mu.lock();
				tar.mu.lock();
				QueueNode *p=tar.frontNode;
				QueueNode **q=&frontNode;
				while (p)
				{
					backNode=*q=new QueueNode(p());
					q=&(*q)->nxt;//??
					p=p->nxt;
				}
				size=tar.size;
				tar.mu.lock();
				mu.unlock();
			}
			
			void MoveFrom(SafeQueue &tar)
			{
				mu.lock();
				tar.mu.lock();
				frontNode=tar.frontNode;
				backNode=tar.backNode;
				tar.frontNode=tar.backNode=NULL;
				size=tar.size;
				tar.size=0;
				tar.mu.unlock();
				mu.unlock();
			}
			
		public:
			SafeQueueData Back()
			{
				SafeQueueData re;
				mu.lock();
				if (size>0)
					re=SafeQueueData(backNode->AccessData());
				mu.unlock();
				return re;
			}
			
			SafeQueueData Front()
			{
				SafeQueueData re;
				mu.lock();
				if (size>0)
					re=SafeQueueData(frontNode->AccessData());
				mu.unlock();
				return re;
			}
			
			void Pop()
			{
				mu.lock();
				if (size>0)
				{
					QueueNode *p=frontNode;
					if (size==1)
						frontNode=backNode=NULL;
					else frontNode=frontNode->nxt;
					--size;
					delete p;
				}
				mu.unlock();
			}
			
			void Push(const T &da)
			{
				mu.lock();
				if (size==0)
					frontNode=backNode=new QueueNode(da);
				else
				{
					backNode->nxt=new QueueNode(da);
					backNode=backNode->nxt;
				}
				++size;
				mu.unlock();
			}
			
			SafeQueueData PopFront()
			{
				SafeQueueData re;
				mu.lock();
				if (size>0)
				{
					re=*frontNode;
					QueueNode *p=frontNode;
					if (size==1)
						frontNode=backNode=NULL;
					else frontNode=frontNode->nxt;
					--size;
					delete p;
				}
				mu.unlock();
				return re;
			}
			
			bool Empty()//Be careful because of multithread!
			{
				mu.lock();
				bool re=size==0;
				mu.unlock();
				return re;
			}
			
			long long Size()
			{
				mu.lock();
				long long re=size;
				mu.unlock();
				return re;
			}

			SafeQueue& operator = (SafeQueue &tar)
			{
				CopyFrom(tar);
				return *this;
			}

			SafeQueue& operator = (SafeQueue &&tar)
			{
				MoveFrom(tar);
				return *this;
			}		

			SafeQueue(SafeQueue &tar)
			{CopyFrom(tar);}
			
			SafeQueue(SafeQueue &&tar)
			{MoveFrom(tar);}
			
			~SafeQueue()
			{
				mu.lock();
				while (frontNode)
				{
					QueueNode *p=frontNode;
					frontNode=frontNode->nxt;
					delete p;
				}
				mu.unlock();
			}
			
			SafeQueue() {}
	};
}

#endif
