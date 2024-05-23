#ifndef PAL_DEBUG_PAL_DEBUG_0_HPP
#define PAL_DEBUG_PAL_DEBUG_0_HPP

#include "../IO/PAL_Xout_0.hpp"
#include "../IO/PAL_Xout_1.hpp"

namespace PAL::DEBUG//This namespace is special with DEBUG, because there is also XoutType Debug that would collition with it.
{
	using namespace PAL::IO;
	extern Xout DD;
	
	class Debug_FuncCallIndicator
	{
		protected:
			const char *s=nullptr;
			
		public:
			Debug_FuncCallIndicator& operator = (const Debug_FuncCallIndicator&)=delete;
			Debug_FuncCallIndicator&& operator = (Debug_FuncCallIndicator&&)=delete;
			Debug_FuncCallIndicator(const Debug_FuncCallIndicator&)=delete;
			Debug_FuncCallIndicator(Debug_FuncCallIndicator&&)=delete;
			
			~Debug_FuncCallIndicator()
			{
				DD[Debug]<<"~"<<s<<endl;
			}
			
			Debug_FuncCallIndicator(const char *_s):s(_s)
			{
				DD[Debug]<<s<<endl;
			}
	};
};

#define DD_FUNC \
	Debug_FuncCallIndicator __Debug_FuncCallIndicator(__PRETTY_FUNCTION__);

/*
	
	TraceClass
	
	TraceBack
	
	LifeTimeCount

	ObjectEmu//Work as raw object, but traced
	
	MemoryManager
*/


#endif
