#ifndef PAL_TYPES_HPP
#define PAL_TYPES_HPP

namespace PAL
{
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
	
	using SintPlatform=Sint64;
	using UintPlatform=Uint64;
	
	using SintSize=SintPlatform;
	using UintSize=UintPlatform;
	
	using SintPtr=SintPlatform;
	using UintPtr=UintPlatform;
	using PtrInt=UintPtr;//The UintXX of void*
	
//	using ErrorType=Sint32;
};

#endif
