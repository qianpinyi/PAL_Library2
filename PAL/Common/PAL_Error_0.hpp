#ifndef PAL_ERROR_HPP
#define PAL_ERROR_HPP

namespace PAL
{
	enum class ErrorCode:Uint64
	{
		ERR_None=0
	};
	
	inline void Abort()
	{
		
	}
	
	inline void PAL_Assert(bool flags)
	{
		
	}
	
	inline void Assert()
	{
		TODO;
	}
	
	inline void AlwaysAssert()
	{
		
	}
	
	//#define AssertEX()
};

#endif
