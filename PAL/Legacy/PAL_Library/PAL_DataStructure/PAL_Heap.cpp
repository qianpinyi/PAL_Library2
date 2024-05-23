#ifndef PAL_HEAP_CPP
#define PAL_HEAP_CPP 1

#include <vector>
#include <iostream>

namespace PAL::Legacy::PAL_DS
{
	template <class T> class PAL_Heap//Small root heap
	{
		protected:
			unsigned long long HeapSize=0;
			bool (*cmp)(const T&,const T&)=NULL;
			
			virtual ~PAL_Heap() {}
		public:
			virtual void Push(const T &tar)=0;
			virtual void Pop()=0;
			virtual const T& Top()=0;
			virtual T PopTop()=0;
			virtual void Clear()=0;
			
			unsigned long long Size()
			{return HeapSize;}
			
			bool Empty()
			{return HeapSize==0;}
	};
	
	template <class T> class BinaryHeap:public PAL_Heap<T>
	{
		protected:
			std::vector <T> heap;
			
		public:
			virtual void Push(const T &tar)
			{
				auto p=++this->HeapSize;
				heap.push_back(tar);
				if (this->cmp==NULL)
					while (p>>1>=1&&heap[p]<heap[p>>1])
						std::swap(heap[p],heap[p>>1]),p>>=1;
				else
					while (p>>1>=1&&this->cmp(heap[p],heap[p>>1]))
						std::swap(heap[p],heap[p>>1]),p>>=1;
			}
			
			virtual void Pop()
			{
				if (this->HeapSize==0) return;
				int p=1;
				heap[0]=heap[1];
				heap[1]=heap[this->HeapSize--];
				if (this->cmp==NULL)
					while (p<<1<=this->HeapSize&&(heap[p<<1]<heap[p]||heap[p<<1|1]<heap[p]))
						if (heap[p<<1]<heap[p<<1|1]) std::swap(heap[p],heap[p<<1]),p<<=1;
						else std::swap(heap[p],heap[p<<1|1]),p=p<<1|1;
				else
					while (p<<1<=this->HeapSize&&(this->cmp(heap[p<<1],heap[p])||this->cmp(heap[p<<1|1],heap[p])))
						if (this->cmp(heap[p<<1],heap[p<<1|1])) std::swap(heap[p],heap[p<<1]),p<<=1;
						else std::swap(heap[p],heap[p<<1|1]),p=p<<1|1;
				heap.pop_back();
			}
			
			virtual const T& Top()
			{return heap[1];}
			
			virtual T PopTop()
			{
				Pop();
				return heap[0];
			}
			
			T& LastPop()
			{return heap[0];}
			
			virtual void Clear()
			{
				T lastpop=heap[0];
				heap.clear();
				heap.push_back(lastpop);
				this->HeapSize=0;
			}
			
			BinaryHeap(bool (*_cmp)(const T&,const T&)=NULL)
			{
				this->cmp=_cmp;
				heap.resize(1);
			}
	};
	
	template <class T> class MergeableHeap:public PAL_Heap<T>
	{
		protected:
			
		public:
			
			
	};
	
	template <class T> class LeftistTree:public MergeableHeap<T>
	{
		protected:
			
		public:
			
			
	};
	
	template <class T> class FibonacciHeap:public MergeableHeap<T>
	{
		protected:
			
		public:
			
			
	};
}
#endif
