#ifndef PAL_TEMP_VT100CtlSeq_hpp
#define PAL_TEMP_VT100CtlSeq_hpp

#include "../PAL_DataStructure/PAL_Tuple.cpp"

#include <string>
#include <vector>

namespace PAL::VT100CtlSeq
{
	namespace ControlSequenceName
	{
		enum ControlSequenceType
		{
			ERR=-100,
			PARSE_FAILED,
			PARSE_UNKNOWN,
			PARSE_NOTTARGET,
			PARSE_REACHEND,
			PARSE_REACHESC,
			PARSE_REACHTERM,
			PARSE_MULTITEXTERR,
			PARSE_TEXTBADST,
			
			NONE=0,
			
			//EasyCursorPosition
			RI,				//ESC M
			DECSC,			//ESC 7
			DECSR,			//ESC 8
			
			//CursorPosition
			CUU,			//ESC[<n>A
			CUD,			//ESC[<n>B
			CUF,			//ESC[<n>C
			CUB,			//ESC[<n>D
			CNL,			//ESC[<n>E
			CPL,			//ESC[<n>F
			CHA,			//ESC[<n>G
			VPA,			//ESC[<n>d
			CUP,			//ESC[<y>;<x>H
			HVP,			//ESC[<y>;<x>f//??
			ANSISYSSC,		//ESC[s
			ANSISYSRC,		//ESC[u
			
			//CursorVisibility//??
			ATT160_SET,		//ESC[?12h
			ATT160_RST,		//ESC[?12l
			DECTCEM_SET,	//ESC[?25h
			DECTCEM_RST,	//ESC[?25l
			
			//CursorShape
			DESCCUSR,		//ESC[<n> SP q
			
			//ScreenScroll
			SU,				//ESC[<n>S
			SD,				//ESC[<n>T
			
			//TextModification
			ICH,			//ESC[<n>@
			DCH,			//ESC[<n>P
			ECH,			//ESC[<n>X
			IL,				//ESC[<n>L
			DL,				//ESC[<n>M
			ED,				//ESC[<n>J
			EL,				//ESC[<n>K
			
			//TextFormat
			SGR,			//ESC[<n>m
			
			//ExtendColor
			//...
			
			//ScreenColor
			//...
			
			//ChangeMode
			DECKPAM,		//ESC=
			DECKPNM,		//ESC>
			DECCKM_SET,		//ESC[?1h
			DECCKM_RST,		//ESC[?1l
			
			//QueryStatus
			DECXCPR,		//ESC[6n
			DA,				//ESC[0c
			
			//Table
			HTS,			//ESC H
			CHT,			//ESC[<n>I
			CBT,			//ESC[<n>Z
			TBC_Current,	//ESC[0g
			TBC_All,		//ESC[3g
			
			//SpecifyCharset
			//...
			
			//ScrollSideWidth
			DECSTBM,		//ESC[<t>;<b> r
			
			//WindowTitle
			TITLESET,		//ESC]0;<string>ST
							//ESC]2;<string>ST
			
			//SecondBuffer
			//...
			
			//Others
			DECSET,			//ESC[?Pm h
			DECRST,			//ESC[?Pm l
		};
		
		enum SGR_Name
		{
			Reset			=0,
			Bold			=1,
			Dim				=2,
			Underlined		=4,
			Blink			=5,
			Reverse			=7,
			Hidden			=8,
			ResetBold		=21,
			ResetDim		=22,
			ResetUnderlined	=24,
			ResetBlink		=25,
			ResetReverse	=27,
			ResetHidden		=28,
			
			ResetFore		=39,
			Black			=30,
			Red				=31,
			Green			=32,
			Yellow			=33,
			Blue			=34,
			Magenta			=35,
			Cyan			=36,
			LightGray		=37,
			DarkGray		=90,
			LightRed		=91,
			LightGreen		=92,
			LightYellow		=93,
			LightBlue		=94,
			LightMagenta	=95,
			LightCyan		=96,
			White			=97,
			
			ResetBG			=49,
			BlackBG			=40,
			RedBG			=41,
			GreenBG			=42,
			YellowBG		=43,
			BlueBG			=44,
			MagentaBG		=45,
			CyanBG			=46,
			LightGrayBG		=47,
			DarkGrayBG		=100,
			LightRedBG		=101,
			LightGreenBG	=102,
			LigthYellowBG	=103,
			LigthBlueBG		=104,
			LightMagentaBG	=105,
			LigthCyanBG		=106,
			WhiteBG			=107
		};
	};
	
	struct ControlSequenceCode
	{
		const char *ns=nullptr;
		ControlSequenceName::ControlSequenceType type;
		std::vector <int> params;
		std::string text;
		
		void Append(int x)
		{params.push_back(x);}
		
		int Back() const
		{return params.back();}
		
		int BackOr0() const
		{
			if (params.empty())
				return 0;
			else return params.back();
		}
		
		int BackOrN1() const
		{
			if (params.empty())
				return -1;
			else return params.back();
		}
		
		ControlSequenceName::ControlSequenceType operator () () const
		{return type;}
		
		int operator [] (int pos) const
		{
			if (pos<0)
				return params[(int)params.size()+pos];
			else return params[pos];
		}
		
		ControlSequenceCode& R(const char *_ns,ControlSequenceName::ControlSequenceType _type)
		{
			ns=_ns;
			type=_type;
			return *this;
		}
		
		ControlSequenceCode(const char *_ns,ControlSequenceName::ControlSequenceType _type):ns(_ns),type(_type) {}
		ControlSequenceCode():type(ControlSequenceName::NONE){}
	};
	
	ControlSequenceCode ParseControlSequenceWithoutESC(const char *s,const char *e=nullptr)
	{
		using namespace ControlSequenceName;
		ControlSequenceCode re;
		
		auto NumberGoahead=[&re,&s,e]()->int//return howmany numbers solved, or error code(currently no error, if have, error solve should exist).
		{
			int cnt=0,x=0;
			while (1)
				if (e&&s>=e)
					return cnt;
				else if (InRange(*s,'0','9'))
					x=x*10+*s++-'0';
				else//*s==0 included in this case.
				{
					re.Append(x);
					x=0;
					++cnt;
					if (*s!=';')
						return cnt;
					else ++s;
				}
		};
		
		auto StepinGetString=[&re,&s,e](bool strictcharset=true)->int//No error return string length, else return error code.
		{
			if (!re.text.empty())
				return PARSE_MULTITEXTERR;
			while (1)
				if (e&&s>=e||*s=='\0')
					return PARSE_REACHEND;
				else if (InThisSet(*s,'\a'))
					return ++s,re.text.length();
				else if (InThisSet(*s,'\e',0x18,0x1A)||strictcharset&&InRange(*s,0,31))
					return PARSE_TEXTBADST;
				else re.text+=*s++;
		};
		
		#define CommonCases								\
			case '\e':	return re.R(s,PARSE_REACHESC);	\
			case 0x18:	return re.R(s,PARSE_REACHTERM);	\
			case 0x1A:	return re.R(s,PARSE_REACHTERM);	\
			default:	return re.R(s,PARSE_UNKNOWN);
			//Complete cases when default found unknown.
		
		#define ReachEndReturn					\
			if (e&&s>=e||*s==0)					\
				return re.R(s,PARSE_REACHEND);	\
			else;
			
		if (s==nullptr)
			return re.R(s,PARSE_FAILED);
		ReachEndReturn;
		switch (*s++)
		{
			case 'M':	return re.R(s,RI);
			case '7':	return re.R(s,DECSC);
			case '8':	return re.R(s,DECSR);
			case '=':	return re.R(s,DECKPAM);
			case '>':	return re.R(s,DECKPNM);
			case 'H':	return re.R(s,HTS);
			case '[':
			{
				int n=NumberGoahead();
				ReachEndReturn;
				switch (*s++)
				{
					case 'A':	return re.R(s,CUU);
					case 'B':	return re.R(s,CUD);
					case 'C':	return re.R(s,CUF);
					case 'D':	return re.R(s,CUB);
					case 'E':	return re.R(s,CNL);
					case 'F':	return re.R(s,CPL);
					case 'G':	return re.R(s,CHA);
					case 'd':	return re.R(s,VPA);
					case 'H':	return re.R(s,CUP);
					case 'f':	return re.R(s,HVP);
					case 's':	return re.R(s,ANSISYSSC);
					case 'u':	return re.R(s,ANSISYSRC);
					
					case 'S':	return re.R(s,SU);
					case 'T':	return re.R(s,SD);
					
					case '@':	return re.R(s,ICH);
					case 'P':	return re.R(s,DCH);
					case 'X':	return re.R(s,ECH);
					case 'L':	return re.R(s,IL);
					case 'M':	return re.R(s,DL);
					case 'J':	return re.R(s,ED);
					case 'K':	return re.R(s,EL);
					
					case 'm':	return re.R(s,SGR);
					
					case '=':	return re.R(s,DECKPAM);
					case '>':	return re.R(s,DECKPNM);
					
					case 'n':
						switch (n==0?-1:re.Back())
						{
							case 6:	return re.R(s,DECXCPR);
							default:return re.R(s,PARSE_UNKNOWN);
						}
					case 'c':
						switch (n==0?-1:re.Back())
						{
							case 0:	return re.R(s,DA);
							default:return re.R(s,PARSE_UNKNOWN);
						}

					case 'I':	return re.R(s,CHT);
					case 'Z':	return re.R(s,CBT);
					case 'g':
						switch (n==0?-1:re.Back())
						{
							case 0:	return re.R(s,TBC_Current);
							case 3:	return re.R(s,TBC_All);
							default:return re.R(s,PARSE_UNKNOWN);
						}
					
					case 'r':	return re.R(s,DECSTBM);
					
					case ' ':
						ReachEndReturn
						switch (*s++)
						{
							case 'q':	return re.R(s,DESCCUSR);
							default:	return re.R(s,PARSE_UNKNOWN);
						}
			
					case '?':
					{
						int m=NumberGoahead();
						ReachEndReturn;
						switch (*s++)
						{
							case 'h':
								switch (m==0?-1:re.Back())
								{
									case 1:		return re.R(s,DECCKM_SET);
									case 12:	return re.R(s,ATT160_SET);
									case 25:	return re.R(s,DECTCEM_SET);
									default:	return re.R(s,DECSET);
								}
							case 'l':	
								switch (m==0?-1:re.Back())
								{
									case 1:		return re.R(s,DECCKM_RST);
									case 12:	return re.R(s,ATT160_RST);
									case 25:	return re.R(s,DECTCEM_RST);
									default:	return re.R(s,DECRST);
								}
							CommonCases;
						}
					}
					CommonCases;
				}
			}
			case ']':
			{
				ReachEndReturn;
				switch (*s++)
				{
					case '0':
					case '2':
					{
						ReachEndReturn;
						switch (*s++)
						{
							case ';':
							{
								ReachEndReturn;
								int l=StepinGetString();
								if (l>=0)
									return re.R(s,TITLESET);
								else return re.R(s,(ControlSequenceType)l);
							}
							CommonCases;
						}
					}
					CommonCases;
				}
			}
			CommonCases;
		}
		#undef ReachEndReturn
		#undef CommonCases
		//Cannot reach here!
	}
	
	inline ControlSequenceCode ParseControlSequence(const char *s,const char *e=nullptr)
	{
		using namespace ControlSequenceName;
		if (s==nullptr||(e==nullptr||s<e)&&*s!='\e')
			return ControlSequenceCode(s,PARSE_NOTTARGET);
		else return ParseControlSequenceWithoutESC(s+1,e);
	}
	
	std::string GenerateControlSequence(const ControlSequenceCode &code,bool withPrefixESC=true)
	{
		using namespace ControlSequenceName;
		
	}
	
	RGBA SGR_ToRGBA(int sgr)
	{
		using namespace ControlSequenceName;
		switch (sgr)
		{
			case Black			:
			case BlackBG		:
				return RGBA_Black;
			case Red			:
			case RedBG			:
				return RGBA_DarkRed;
			case Green			:
			case GreenBG		:
				return RGBA_DarkGreen;
			case Yellow			:
			case YellowBG		:
				return RGBA_DarkYellow;
			case Blue			:
			case BlueBG			:
				return RGBA_DarkBlue;
			case Magenta		:
			case MagentaBG		:
				return RGBA_DarkMagenta;
			case Cyan			:
			case CyanBG			:
				return RGBA_DarkCyan;
			case LightGray		:
			case LightGrayBG	:
				return RGBA_LightGray;
			case DarkGray		:
			case DarkGrayBG		:
				return RGBA_DarkGray;
			case LightRed		:
			case LightRedBG		:
				return RGBA_LightRed;
			case LightGreen		:
			case LightGreenBG	:
				return RGBA_LightGreen;
			case LightYellow	:
			case LigthYellowBG	:
				return RGBA_LightYellow;
			case LightBlue		:
			case LigthBlueBG	:
				return RGBA_LightBlue;
			case LightMagenta	:
			case LightMagentaBG	:
				return RGBA_LightMagenta;
			case LightCyan		:
			case LigthCyanBG	:
				return RGBA_LightCyan;
			case White			:
			case WhiteBG		:
				return RGBA_White;
			default:	return RGBA_NONE;
		};
	}
};

#endif
