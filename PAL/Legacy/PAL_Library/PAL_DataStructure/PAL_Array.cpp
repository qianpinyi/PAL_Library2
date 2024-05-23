#ifndef PAL_ARRAY_CPP
#define PAL_ARRAY_CPP

#include "PAL_Tuple.cpp"

namespace PAL::Legacy::PAL_DS
{
	template <class T,unsigned MM=0> class Array1D2D
	{
		protected:
			T *data=nullptr;
			unsigned Size=0,
					 M=0;
			bool NeedDelete=false;
			
		public:
			template <unsigned m> T& A(int i,int j)
			{return data[i*m+j];}
			
			T& operator () (int i,int j)
			{return data[i*M+j];}
			
			T& operator [] (int i)
			{return data[i];}
			
			template <unsigned m> bool In(int i,int j)
			{return i>=0&&j>=0&&i*m+j<Size;}
			
			bool In(int i,int j)
			{return i>=0&&j>=0&&i*M+j<Size;}
			
			bool In(int i)
			{return 0<=i&&i<Size;}
			
			template <unsigned m> Doublet <int,int> Pos(int i)
			{return {i/m,i%m};}
			
			Doublet <int,int> Pos(int i)
			{return {i/M,i%M};}
			
			template <unsigned m> int Pos(int i,int j)
			{return i*m+j;}
			
			int Pos(int i,int j)
			{return i*M+j;}
			
			void SetM(int m)
			{M=m;}
			
			~Array1D2D()
			{
				if (NeedDelete)
					delete data;
			}
			
			Array1D2D(unsigned size,T *_data,bool needdelete):Size(size),data(_data),NeedDelete(needdelete)
			{
				if constexpr(MM==0)
					M=1;
				else M=MM;
			}
			
			Array1D2D(unsigned n,unsigned m):Size(n*m),M(m),NeedDelete(true)
			{
				data=new T[Size];
			}
			
			Array1D2D(unsigned size):Size(size),NeedDelete(true)
			{
				if constexpr(MM==0)
					M=1;
				else M=MM;
				data=new T[Size];
			}
	};
};

#endif
