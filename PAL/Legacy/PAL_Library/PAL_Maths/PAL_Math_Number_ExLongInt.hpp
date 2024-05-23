#ifndef PAL_MATH_NUMBER_EXLONGINT_HPP
#define PAL_MATH_NUMBER_EXLONGINT_HPP 1

#include <stdint.h>

namespace PAL::Legacy::PAL_Maths
{
	template <unsigned char P> class ExLongUint
	{
		protected:
			ExLongUint <P-1> datal,datah;
			
		public:
			
			
	};
	
	template <> class ExLongUint <3>
	{
		protected:
			uint64_t data;
			
		public:
			inline operator uint64_t () const
			{return data;}
			
			inline ExLongUint operator + (const ExLongUint &tar) const
			{return data+tar.data;}
			
			inline ExLongUint operator - (const ExLongUint &tar) const
			{return data-tar.data;}
			
			inline ExLongUint operator * (const ExLongUint &tar) const
			{return data*tar.data;}
			
			inline ExLongUint operator / (const ExLongUint &tar) const
			{return data/tar.data;}
			
			
			
			ExLongUint(const uint64_t &src):data(src) {}
			
			ExLongUint():data(0) {}
	};
	
	template <> class ExLongUint <2>
	{
		
	};
	
	template <> class ExLongUint <1>
	{
		
	};
	
	template <> class ExLongUint <0>
	{

	};

	template <unsigned char P> class ExLongInt
	{
			
	};
	
	template <> class ExLongInt <3>
	{
			
	};
	
	template <> class ExLongInt <2>
	{
			
	};
	
	template <> class ExLongInt <1>
	{
			
	};
	
	template <> class ExLongInt <0>
	{
			
	};
	
};

#endif
