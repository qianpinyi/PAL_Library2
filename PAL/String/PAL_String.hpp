#ifndef PAL_STRING_HPP
#define PAL_STRING_HPP

#include "../Common/PAL_Types.hpp"

/*
	String: similar to c++/java/python string
	Chars: similar to char[]
	ConstString: similar to const char*
	StringT: templated string with char as any char type
	Text: 2D string ?
	LiteralString: string saved in .rodata that there is no need to save alternatively
	
*/

namespace PAL
{
	template <class T> class LiteralStringT
	{
		public:
			
			
	};
	
	template <class T> class CharsT
	{
		protected:
			
			
		public:
			
			
	};
	using Chars=CharsT <char>;
	using Wchars=CharsT <wchar_t>;
	
	template <class T> class CppStringT
	{
		
	};
	using CppString		=CppStringT <char>;
	using CppWstring	=CppStringT <wchar_t>;
//	using CppU8string	=CppStringT <char8_t>;
	using CppU16string	=CppStringT <char16_t>;
	using CppU32string	=CppStringT <char32_t>;
	
	template <class T> class StringT
	{
		protected:
			UintSize size;
			T *str;
			
		public:
			
			~StringT()
			{
				
			}
			
			StringT()
			{
				
			}
	};
	using String=StringT <char>;
};

#endif
