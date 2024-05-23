#ifndef PAL_ENUMS_SGR_NAME_HPP
#define PAL_ENUMS_SGR_NAME_HPP

namespace PAL::Enums::SGR_Namespace
{
	enum SGR_Name
	{
		UnknownSGR		=-1,
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

#endif
