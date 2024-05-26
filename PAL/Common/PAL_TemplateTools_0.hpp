#ifndef PAL_TEMPLATETOOLS_0_HPP
#define PAL_TEMPLATETOOLS_0_HPP

namespace PAL
{
	template <typename T> inline void Swap(T &x,T &y)//move version of Swap?
	{
		T t=x;
		x=y;
		y=t;
	}
	
	template <typename T1,typename T2,typename T3> constexpr inline bool InRange(const T1 &x,const T2 &L,const T3 &R)
	{return L<=x&&x<=R;}
	
	template <typename T1,typename T2,typename T3> constexpr inline T1 EnsureInRange(const T1 &x,const T2 &L,const T3 &R)
	{
		if (x<L) return L;
		else if (x>R) return R;
		else return x;
	}
	
	template <typename T0> constexpr inline bool InThisSet(const T0 &x)//useless, just for completeness
	{return false;}
	
	template <typename T0,typename T1> constexpr inline bool InThisSet(const T0 &x,const T1 &a)
	{return x==a;}
	
	template <typename T0,typename T1,typename...Ts> constexpr inline bool InThisSet(const T0 &x,const T1 &a,const Ts &...args)
	{
		if (x==a) return 1;
		else return InThisSet(x,args...);
	}
	
	template <typename T0> constexpr inline bool NotInSet(const T0 &x)
	{return true;}
	
	template <typename T0,typename T1> constexpr inline bool NotInSet(const T0 &x,const T1 &a)
	{return x!=a;}
	
	template <typename T0,typename T1,typename...Ts> constexpr inline bool NotInSet(const T0 &x,const T1 &a,const Ts &...args)
	{
		if (x==a) return 0;
		else return NotInSet(x,args...);
	}
	
	template <typename T> constexpr inline T max3(const T &x,const T &y,const T &z)
	{
		if (x<y)
			if (y<z) return z;
			else return y;
		else 
			if (x<z) return z;
			else return x;
	}
	
	template <typename T> constexpr inline T min3(const T &x,const T &y,const T &z)
	{
		if (x<y)
			if (x<z) return x;
			else return z;
		else
			if (y<z) return y;
			else return z;
	}
	
	template <typename T> constexpr inline T maxN(const T &a)
	{return a;}
	
	template <typename T> constexpr inline T maxN(const T &a,const T &b)
	{return a<b?b:a;}
	
	template <typename T,typename...Ts> constexpr inline T maxN(const T &a,const T &b,const Ts &...args)
	{
		if (a<b) return (T)maxN(b,(T)args...);
		else return (T)maxN(a,(T)args...);
	}
	
	template <typename T> constexpr inline T minN(const T &a)
	{return a;}
	
	template <typename T> constexpr inline T minN(const T &a,const T &b)
	{return b<a?b:a;}
	
	template <typename T,typename...Ts> constexpr inline T minN(const T &a,const T &b,const Ts &...args)
	{
		if (b<a) return (T)minN(b,(T)args...);
		else return (T)minN(a,(T)args...);
	}
	
	template <typename T> inline T*& DeleteToNULL(T *&ptr)
	{
		if (ptr!=nullptr)
		{
			delete ptr;
			ptr=nullptr;
		}
		return ptr;
	}
	
	template <typename T> inline T*& DELETEtoNULL(T *&ptr)
	{
		if (ptr!=nullptr)
		{
			delete[] ptr;
			ptr=nullptr;
		}
		return ptr;
	}
	
	template <typename T> inline T* DumpAsPtr(const T &tar)
	{
		T *re=new T(tar);
		return re;
	}
	
	template <typename T> inline T* DumpAsPtr(const T *tar)
	{
		return DumpAsPtr(*tar);
	}
	
//	template <typename T> T* OperateForAll(T *src,unsigned long long count,void(*func)(T&))
//	{
//		if (src==nullptr) return nullptr;
//		for (unsigned long long i=0;i<count;++i)
//			func(src[i]);
//		return src;
//	}
	
	template <typename T> inline void MemsetT(T *dst,const T &val,unsigned long long count)
	{
		if (dst==nullptr||count==0) return;
		T *end=dst+count;
		while (dst!=end)
			*dst++=val;
	}
	
	template <typename T> inline void MemcpyT(T *dst,const T *src,unsigned long long count)
	{
		if (dst==nullptr||src==nullptr||dst==src||count==0) return;
		T *end=dst+count;
		while (dst!=end)
			*dst++=*src++;
	}
	
	template <typename T> inline void MemmoveT(T *dst,const T *src,unsigned long long count)
	{
		if (dst==nullptr||src==nullptr||dst==src||count==0) return;
		if (dst<src)
		{
			T *end=dst+count;
			while (dst!=end)
				*dst++=*src++;
		}
		else
		{
			T *start=dst;
			dst+=count;
			src+=count;
			do *--dst=*--src;
			while (dst!=start);
		}
	}
	
	template <typename T> inline constexpr bool GetBitMask(T tar,unsigned i)
	{return (tar>>i)&1;}
	
	template <typename T> inline constexpr void SetBitMask1(T &tar,unsigned i)
	{tar|=1ull<<i;}
	
	template <typename T> inline constexpr void SetBitMask0(T &tar,unsigned i)
	{tar&=~(1ull<<i);}
	
	template <typename T,int N=0> inline T EndianSwitch(T x)
	{
		char *b=(char*)&x;
		constexpr int n=N==0?sizeof(x):N;
		for (int i=(n>>1)-1;i>=0;--i)
			Swap(b[i],b[n-i-1]);
		return x;
	}
	
#if PCFG_PlatformIsBigEndian == 0
	template <typename T> inline T ToLittleEndian(T x)
	{return x;}
	
	template <typename T> inline T ToBigEndian(T x)
	{return EndianSwitch(x);}
	
	template <typename T> inline T LittleEndianToThis(T x)
	{return x;}
	
	template <typename T> inline T BigEndianToThis(T x)
	{return EndianSwitch(x);}
#else
	template <typename T> inline T ToLittleEndian(T x)
	{return EndianSwitch(x);}
	
	template <typename T> inline T ToBigEndian(T x)
	{return x;}
	
	template <typename T> inline T LittleEndianToThis(T x)
	{return EndianSwitch(x);}
	
	template <typename T> inline T BigEndianToThis(T x)
	{return x;}
#endif
};

#endif
