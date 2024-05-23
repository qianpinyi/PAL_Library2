#ifndef PAL_BASICFUNCTIONS_0_CPP
#define PAL_BASICFUNCTIONS_0_CPP 1

#include "../PAL_BuildConfig.h"

#include "../../../Common/PAL_TemplateTools_0.hpp"

namespace PAL::Legacy
{
	#if PAL_CFG_EnableOldFunctions == 1
	//#define null 0
	//#define NULLptr nullptr

	void * const CONST_THIS=new int(0);
	void * const CONST_TRUE=new int(0);
	void * const CONST_FALSE=new int(0);
	void * const CONST_Ptr_0=new int(0);
	void * const CONST_Ptr_1=new int(1);
	void * const CONST_Ptr_2=new int(2);
	void * const CONST_Ptr_3=new int(3);
	#endif

	#if EnableBasicfunciton0PredefineUSintX == 1
	using Sint8=signed char;
	using Sint16=signed short;
	using Sint32=signed int;
	using Sint64=signed long long;
	//using Sint128=__int128_t;
	using Uint8=unsigned char; 
	using Uint16=unsigned short;
	using Uint32=unsigned int;
	using Uint64=unsigned long long;
	//using Uint128=__uint128_t;
	#endif

//	template <typename T1,typename T2,typename T3> T1 EnsureInRange(const T1 &x,const T2 &L,const T3 &R)
//	{
//		if (x<L) return L;
//		else if (x>R) return R;
//		else return x;
//	}
//
//	template <typename T1,typename T2,typename T3>  bool InRange(const T1 &x,const T2 &L,const T3 &R)
//	{return L<=x&&x<=R;}
//
//	template <typename T0,typename T1> bool InThisSet(const T0 &x,const T1 &a)
//	{return x==a;}
//
//	template <typename T0,typename T1,typename...Ts> bool InThisSet(const T0 &x,const T1 &a,const Ts &...args)
//	{
//		if (x==a) return 1;
//		else return InThisSet(x,args...);
//	}
//
//	template <typename T0,typename T1> bool NotInSet(const T0 &x,const T1 &a)
//	{return x!=a;}
//
//	template <typename T0,typename T1,typename...Ts> bool NotInSet(const T0 &x,const T1 &a,const Ts &...args)
//	{
//		if (x==a) return 0;
//		else return NotInSet(x,args...);
//	}
//
//	template <typename T> T max3(const T &x,const T &y,const T &z)
//	{
//		if (x<y)
//			if (y<z) return z;
//			else return y;
//		else 
//			if (x<z) return z;
//			else return x;
//	}
//
//	template <typename T> T min3(const T &x,const T &y,const T &z)
//	{
//		if (x<y)
//			if (x<z) return x;
//			else return z;
//		else
//			if (y<z) return y;
//			else return z;
//	}
//
//	template <typename T> T maxN(const T &a,const T &b)
//	{return a<b?b:a;}
//
//	template <typename T,typename...Ts> T maxN(const T &a,const T &b,const Ts &...args)
//	{
//		if (a<b) return (T)maxN(b,(T)args...);
//		else return (T)maxN(a,(T)args...);
//	}
//
//	template <typename T> T minN(const T &a,const T &b)
//	{return b<a?b:a;}
//
//	template <typename T,typename...Ts> T minN(const T &a,const T &b,const Ts &...args)
//	{
//		if (b<a) return (T)minN(b,(T)args...);
//		else return (T)minN(a,(T)args...);
//	}
//
//	template <typename T> T*& DeleteToNULL(T *&ptr)
//	{
//		if (ptr!=nullptr)
//		{
//			delete ptr;
//			ptr=nullptr;
//		}
//		return ptr;
//	}
//
//	template <typename T> T*& DELETEtoNULL(T *&ptr)
//	{
//		if (ptr!=nullptr)
//		{
//			delete[] ptr;
//			ptr=nullptr;
//		}
//		return ptr;
//	}
//
//	template <typename T> void MemsetT(T *dst,const T &val,unsigned long long count)
//	{
//		if (dst==nullptr||count==0) return;
//		T *end=dst+count;
//		while (dst!=end)
//			*dst=val,++dst;
//	}

	template <typename T> T* OperateForAll(T *src,unsigned long long count,void(*func)(T&))
	{
		if (src==nullptr) return nullptr;
		for (unsigned long long i=0;i<count;++i)
			func(src[i]);
		return src;
	}

	#define CheckErrorAndDDReturn(x,y)		 								\
	{																		\
		int ErrorCode=(x);													\
		if (ErrorCode)														\
		{																	\
			DD[2]<<"Run <"<<(#x)<<"> failed! ErrorCode:"<<ErrorCode<<endl;	\
			return y;														\
		}																	\
	}

	#define DoNothing 0

	#define ReturnError(expr) 										\
		if (auto _errorreturnvalue=(expr);(int)_errorreturnvalue)	\
			return _errorreturnvalue;								\
		else DoNothing;

	class BaseTypeFuncAndData
	{
		public:
			virtual int CallFunc(int usercode)=0;
			virtual ~BaseTypeFuncAndData() {};
	};

	template <class T> class TypeFuncAndData:public BaseTypeFuncAndData
	{
		protected:
			int (*func)(T&,int)=nullptr;
			T funcdata;
		public:
			virtual int CallFunc(int usercode)
			{
				if (func!=nullptr)
					return func(funcdata,usercode);
				else return 0;
			}
			
			virtual ~TypeFuncAndData() {}
			
			TypeFuncAndData(int (*_func)(T&,int),const T &_funcdata):func(_func),funcdata(_funcdata) {}
	};

	template <class T> class TypeFuncAndDataV:public BaseTypeFuncAndData
	{
		protected:
			void (*func)(T&)=nullptr;
			T funcdata;
		public:
			virtual int CallFunc(int usercode)
			{
				if (func!=nullptr)
					func(funcdata);
				return 0;
			}
			
			virtual ~TypeFuncAndDataV() {}
			
			TypeFuncAndDataV(void (*_func)(T&),const T &_funcdata):func(_func),funcdata(_funcdata) {}
	};

	class VoidFuncAndData:public BaseTypeFuncAndData
	{
		protected:
			void (*func)(void*)=nullptr;
			void *funcdata=nullptr;
		public:
			virtual int CallFunc(int)
			{
				if (func!=nullptr)
					func(funcdata);
				return 0;
			}
			
			VoidFuncAndData(void (*_func)(void*),void *_funcdata=nullptr):func(_func),funcdata(_funcdata) {}
	};

	//template <class T> FunctorFuncAndData:public BaseTypeFuncAndData
	//{
	//	protected:
	//		T f;
	//		
	//	public:
	//		
	//};

	template <class T> class LocalPtr
	{
		protected:
			T *p=nullptr;
			
		public:
			inline operator T* ()
			{return p;}
			
			inline T* operator -> ()
			{return p;}
			
			inline T& operator * ()
			{return *p;}
			
			inline bool operator ! () const
			{return p==nullptr;}
			
			inline T* Target() const
			{return p;}
			
			~LocalPtr()
			{
				if (p)
					delete p;
			}
			
			explicit LocalPtr(T *tar)
			{p=tar;}
			
			LocalPtr(const LocalPtr &)=delete;
			LocalPtr(const LocalPtr &&)=delete;
			LocalPtr& operator = (const LocalPtr &)=delete;
			LocalPtr& operator = (const LocalPtr &&)=delete;
	};

	template <class T> class SharedPtr
	{
		protected:
			T *p=nullptr;
			unsigned int *count=nullptr;
			
			inline void CopyFrom(const SharedPtr &tar)
			{
				if (!tar)
					return;
				p=tar.p;
				count=tar.count;
				++*count;
			}
			
			inline void Deconstruct()
			{
				--*count;
				if (*count==0)
				{
					delete p;
					delete count;
				}
				p=nullptr;
				count=nullptr;
			}
		
		public:
			inline operator T* ()
			{return p;}
			
			inline T* operator -> ()
			{return p;}
			
			inline T& operator * ()
			{return *p;}
			
			inline bool operator ! () const
			{return p==nullptr||count==nullptr;}
			
			inline T* Target() const
			{return p;}
			
			inline unsigned Count() const
			{return count==nullptr?0:*count;}
			
			SharedPtr& operator = (const SharedPtr &tar)
			{
				if (&tar==this)
					return *this;
				if (p!=nullptr)
					Deconstruct();
				CopyFrom(tar);
				return *this;
			}
			
			~SharedPtr()
			{
				if (p!=nullptr)
					Deconstruct();
			}
			
			SharedPtr(const SharedPtr &tar)
			{CopyFrom(tar);}
			
			explicit SharedPtr(T *tar)
			{
				if (tar!=nullptr)
				{
					p=tar;
					count=new unsigned int(1);
				}
			}
			
			explicit SharedPtr() {}
			
	//		template <typename...Ts> SharedPtr(const Ts &...args):SharedPtr(new T(args...)) {}//??
	};

	template <class T> class GroupPtr
	{
		protected:
			T **p=nullptr;
			unsigned int *count=nullptr;
			
			inline void CopyFrom(const GroupPtr &tar)
			{
				if (!tar)
					return;
				p=tar.p;
				count=tar.count;
				++*count;
			}
			
			inline void Deconstruct()
			{
				--*count;
				if (*count==0)
				{
					delete p;
					delete count;
				}
				p=nullptr;
				count=nullptr;
			}
			
		public:
			inline void SetTarget(T *tar)//if NULL,the group will break down; if not NULL, the group changes their target(Use this carefully!).
			{
				if (tar!=nullptr)
				{
					if (p!=nullptr)
					{
						if (*p!=nullptr)
						{
							*p=tar;
							return;
						}
						else Deconstruct();
					}
					p=new T*(tar);
					count=new unsigned(1);
				}
				else if (p!=nullptr)
				{
					*p=nullptr;
					Deconstruct();
				}
			}
			
			inline operator T* ()
			{return p==nullptr?nullptr:*p;}
			
			inline T* operator -> ()
			{return *p;}
			
			inline bool operator ! () const
			{return p==nullptr||*p==nullptr;}
			
			inline bool Valid() const
			{return p!=nullptr&&*p!=nullptr;}
			
			inline T* Target() const
			{return p==nullptr?nullptr:*p;}
			
			inline unsigned Count() const
			{return p==nullptr?0:*count;}
			
			GroupPtr& operator = (const GroupPtr &tar)
			{
				if (&tar==this)
					return *this;
				if (p!=nullptr)
					Deconstruct();
				CopyFrom(tar);
				return *this;
			}
			
			~GroupPtr()
			{
				if (p!=nullptr)
					Deconstruct();
			}
			
			GroupPtr(const GroupPtr &tar)
			{CopyFrom(tar);}
			
			GroupPtr() {}
	};

	class DataWithSize
	{
		public:
			void *data;
			Uint64 size;
			
			DataWithSize(void *_data,Uint64 _size):data(_data),size(_size) {}
	};

	class SizedBuffer
	{
		public:
			char *data;
			Uint64 size;
			
			~SizedBuffer()
			{
				if (data!=nullptr)
					delete[] data;
			}
			
			SizedBuffer(Uint64 _size):size(_size)
			{
				if (size==0)
					data=nullptr;
				else data=new char[size];
			}
			
			SizedBuffer(const SizedBuffer &)=delete;
			SizedBuffer(const SizedBuffer &&)=delete;
			SizedBuffer& operator = (const SizedBuffer &)=delete;
			SizedBuffer& operator = (const SizedBuffer &&)=delete;
	};
}
	
#endif
