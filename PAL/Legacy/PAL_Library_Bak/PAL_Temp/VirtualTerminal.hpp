#ifndef PAL_TEMP_VIRTUALTERMINAL_HPP
#define PAL_TEMP_VIRTUALTERMINAL_HPP

namespace PAL
{
	class VirtualTerminal
	{
		protected:
			struct Char
			{
				char32_t ch;
				Uint32 flags;//is wide, color, effect, etc...
				int extra;//extra is an index to extra attribute list with ref count.
				/*
					flag bits:
						Reset			=0,
						Bold			=1,
						Dim				=2,
						Underlined		=4,
						Blink			=5,
						Reverse			=7,
						Hidden			=8,
				*/
			};
			
			enum
			{
				Mode_Main,
				Mode_Sub
			}mode;
			
			List <Char> MainBuffer;
			Array2D <Char> ScreenBuffer;
			
			BasicIO *bio=nullptr;
			
		public:
			
			
			
			VirtualTermial(...)
			{
				
			}
	};
};

#endif
