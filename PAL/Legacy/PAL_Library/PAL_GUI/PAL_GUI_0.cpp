#ifndef PAL_GUI_0_CPP
#define PAL_GUI_0_CPP 1
/*
	Refactoring of PAL_GUI_Alpha
	By:qianpinyi
*/

#include <cstdlib>
#include <cstdio>

#include "../PAL_BuildConfig.h"

#include <string>
#include <set>
#include <map>
#include <atomic>
#include <queue>

#include PAL_SDL_HeaderPath
#include PAL_SDL_ttfHeaderPath

#include "../PAL_BasicFunctions/PAL_BasicFunctions_0.cpp"
#include "../PAL_BasicFunctions/PAL_Posize.cpp"
#include "../PAL_BasicFunctions/PAL_StringEX.cpp"
#include "../PAL_BasicFunctions/PAL_Color.cpp"
#include "../PAL_BasicFunctions/PAL_Debug.cpp"
#include "../PAL_DataStructure/PAL_SplayTree.cpp"
#include "../PAL_DataStructure/PAL_Tuple.cpp"
#include "../PAL_DataStructure/PAL_LeastRecentlyUsed.cpp"
#include "../PAL_SDL_EX/PAL_SDL_EX_0.cpp"
#include "../PAL_BasicFunctions/PAL_System.cpp"
#include "../PAL_BasicFunctions/PAL_File.cpp"
#include "../PAL_BasicFunctions/PAL_Time.cpp"

#if PAL_GUI_UseOriginalMain == 1
	#undef main
#endif

#if PAL_CurrentPlatform == PAL_Platform_Windows
	#include <Windows.h>
#endif

#if PAL_SDL_IncludeSyswmHeader == 1
	#include PAL_SDL_syswmHeaderPath
#endif

#if PAL_GUI_EnableTransparentBackground == 1
	#include "../PAL_Images/PAL_ScreenCapture.cpp"
#endif

#define PUI_DefaultFontSize 14
#define PUIT(x) DeleteEndBlank(Charset::AnsiToUtf8(x))//PAL_GUI Text
//#define PUIT(x) (x)

namespace PAL::Legacy::PAL_GUI
{
	using namespace std;
	using namespace PAL_DS;
	
	//Global option:
	//...
	bool PUI_SkipEventFlag=0,
		 PUI_SkipFrameFlag=0,
		 PUI_EnableTouchSimulateMouse=0,
		 PUI_DisablePartialUpdate=0;
	enum
	{
		PUI_PreferredRenderer_Default=0,
		PUI_PreferredRenderer_Direct3d,
		PUI_PreferredRenderer_OpenGL,
		PUI_PreferredRenderer_OpenGLES2,
		PUI_PreferredRenderer_OpenGLES,
		PUI_PreferredRenderer_SDL2Software,
		
		PUI_PreferredRenderer_End
	};
	const char* PUI_PreferredRendererName[PUI_PreferredRenderer_End]=
	{
		"Default",
		"Direct3d",
		"OpenGL",
		"OpenGLES2",
		"OpenGLES",
		"SDL2Software"
	};
	int PUI_PreferredRenderer=PUI_PreferredRenderer_OpenGL;//It is just a hint.
	
	//Parameters:
	unsigned long long PUI_Parameter_TextMemorizeMemoryLimit=8388608;//Default 8MB; It is recommended to set this value after tesing real memorysize needed and select a best value.
	int PUI_Parameter_LongClickOuttime=700,//ms
		PUI_Parameter_DoubleClickGaptime=300,
		PUI_Parameter_MovingTriggerPinch=5,
		PUI_Parameter_WheelSensibility=40,
		PUI_Parameter_LargeLayerInertialScrollInterval=1000.0/60,
		PUI_Parameter_LargeLayerInertialScrollEachBound=1000,
		PUI_Parameter_SideSlideBoundWidth=30,
		PUI_Parameter_MultiClickGaptimeOfLargeLayer=50,
		PUI_Parameter_RefreshRate=0;//if 0,means not synchronize and not limit (or Disabled).
	float PUI_Parameter_MovingTriggerSpeed=0.2,//pixels/ms
		  PUI_Parameter_DoubleClickTouchPointBias=24,
		  PUI_Parameter_TouchSpeedSampleTimeLength=300,
		  PUI_Parameter_TouchSpeedDiscardLimit=0.02,
		  PUI_Parameter_LargeLayerInertialScrollFactor=0.001;//The smaller,the longer
	const float PUI_GlobalScalePercent=1;
	
	//Debug option:
	//...
	Debug_Out PUI_DD(DebugOut_CERR);
	
	bool DEBUG_DisplayBorderFlag=0,
		 DEBUG_EnableWidgetsShowInTurn=0,
		 DEBUG_DisplayPresentLimitFlag=0,
		 DEBUG_EnableDebugThemeColorChange=0,
		 DEBUG_EnableForceQuitShortKey=0;
	
	int DEBUG_WidgetsShowInTurnDelayTime=100,
		DEBUG_ForceQuitType=1;//0,1:exit(0,1) 2:abort()
	
	//Option Functions:
	void PUI_SetPreferredRenderer(int type)
	{
		if (!InRange(type,PUI_PreferredRenderer_Default,PUI_PreferredRenderer_End)) return;
		PUI_PreferredRenderer=type;
	}
	
	//Data
	//...
	struct SharedTexturePtr//Need improve...
	{
		protected:
			int *count=NULL;
			SDL_Texture *tex=NULL;
			
		public:
			inline bool operator == (const SharedTexturePtr &tar) const
			{return count==tar.count&&tex==tar.tex;}
			
			inline bool operator ! () const
			{return tex==NULL;}
			
			inline SDL_Texture* operator () () const
			{return tex;}
			
			inline SDL_Texture* GetPic()
			{return tex;}
			
			SharedTexturePtr& operator = (const SharedTexturePtr &tar)
			{
				if (&tar==this)
					return *this;
				if (tex!=NULL)
				{
					--*count;
					if (*count==0)
					{
						SDL_DestroyTexture(tex);
						delete count;
					}
					tex=NULL;
					count=NULL;
				}
				tex=tar.tex;
				count=tar.count;
				if (tex!=NULL)
					++*count;
				return *this;
			}
			
			~SharedTexturePtr()
			{
				if (tex!=NULL)
				{
					--*count;
					if (*count==0)
					{
						SDL_DestroyTexture(tex);
						delete count;
					}
				}
			}
			
			SharedTexturePtr(const SharedTexturePtr &tar)
			{
				tex=tar.tex;
				count=tar.count;
				if (tex!=NULL)
					++*count;
			}
			
			explicit SharedTexturePtr(SDL_Texture *tarTex)
			{
				tex=tarTex;
				if (tex!=NULL)
					count=new int(1);
			}
			
			explicit SharedTexturePtr() {}
	};
	
	struct PUI_ThemeColor
	{
		enum
		{
			PUI_ThemeColor_Blue=0,
			PUI_ThemeColor_Green,
			PUI_ThemeColor_Red,
			PUI_ThemeColor_Pink,
			PUI_ThemeColor_Orange,
//			PUI_ThemeColor_DarkBlue,
			PUI_ThemeColor_Violet,
//			PUI_ThemeColor_Silvery,
			PUI_ThemeColor_Golden,
//			PUI_ThemeColor_White,
//			PUI_ThemeColor_Gray,
//			PUI_ThemeColor_Black,
			PUI_ThemeColor_DeepGreen,
//			PUI_ThemeColor_Colorful,
			PUI_ThemeColor_UserDefined
		};
		
		/*
			RGBA:
				(X,X,X,>0): NormalColor
				(0,0,0,0): empty color
				(0,pos%256,pos/256,0),pos:1~256*256-1: DynamicUserColor
				()
		*/
	protected:
		static vector <RGBA> DynamicUserColor;//pos:1~256*256-1 RGBA(0,pos%256,pos/256,0) pos0:Reserved
	public:
		RGBA MainColor[8],//RGBA(1,0,0~7,0);
			 BackgroundColor[8],//RGBA(1,1,0~7,0);
			 MainTextColor[2];//RGBA(1,2,0~1,0);
		int CurrentThemeColor=0;
		
		static inline RGBA SetUserColor(const RGBA &co)
		{
			int pos=DynamicUserColor.size();
			RGBA re(0,pos%256,pos/256,0);
			DynamicUserColor.push_back(co);
			return re;
		}
		
		static inline int UserColorToPos(const RGBA &co)//0 means it is not UserColor
		{return co.a!=0||co.r!=0?0:co.g+(int)co.b*256;}

		static inline bool IsUserColor(const RGBA &co)
		{return co.a==0&&co.r==0&&(co.g!=0||co.b!=0);}
		
		static inline bool IsThemeColor(const RGBA &co)
		{return co.a==0&&co.r==1&&co.g<=3;} 

		static inline RGBA& GetUserColor(int pos)
		{
			if (InRange(pos,1,DynamicUserColor.size()-1)) 
				return DynamicUserColor[pos];
			else return DynamicUserColor[0];
		}

		static inline RGBA& GetUserColor(const RGBA &p)
		{return GetUserColor(UserColorToPos(p));}
		
		static inline void ChangeUserColor(int pos,const RGBA &co)
		{
			if (InRange(pos,1,DynamicUserColor.size()-1))
				DynamicUserColor[pos]=co;
		}
		
		static inline void ChangeUserColor(const RGBA &p,const RGBA &co)
		{ChangeUserColor(UserColorToPos(p),co);}
		
		static RGBA GetRealColor(const RGBA &p);
		
		RGBA operator () (const RGBA &p);
		
//		inline RGBA& operator [] (const RGBA &p)
//		{return GetUserColor(p);}
			 
		inline RGBA& operator [] (int x)//0~7:MainThemeColor -65535~-1:DynamicUserColor
		{
			if (InRange(x,0,7))
				return MainColor[x];
			else if (InRange(-x,1,DynamicUserColor.size()-1)) return DynamicUserColor[-x];
			else return PUI_DD[1]<<"ThemeColor[x],x is not in Range[0,7],MainColor[7] is used as needed."<<endl,MainColor[7];
		}
		
		PUI_ThemeColor(int theme=PUI_ThemeColor_Blue)
		{
			switch (theme)
			{
				default:
					theme=PUI_ThemeColor_UserDefined;
				case PUI_ThemeColor_Blue:
					MainColor[0]={224,236,255,255};
					MainColor[1]={192,217,255,255};
					MainColor[2]={160,197,255,255};
					MainColor[3]={128,178,255,255};
					MainColor[4]={96,158,255,255};
					MainColor[5]={64,139,255,255};
					MainColor[6]={32,119,255,255};
					MainColor[7]={0,100,255,255};
					break;
				case PUI_ThemeColor_Green:
					MainColor[0]={224,255,224,255};
					MainColor[1]={192,255,192,255};
					MainColor[2]={160,255,160,255};
					MainColor[3]={128,255,128,255};
					MainColor[4]={96,255,96,255};
					MainColor[5]={64,255,64,255};
					MainColor[6]={32,255,32,255};
					MainColor[7]={0,255,0,255};
					break;
				case PUI_ThemeColor_Red:
					MainColor[0]={255,140,140,255};
					MainColor[1]={255,120,120,255};
					MainColor[2]={255,100,100,255};
					MainColor[3]={255,80,80,255};
					MainColor[4]={255,60,60,255};
					MainColor[5]={255,40,40,255};
					MainColor[6]={255,20,20,255};
					MainColor[7]={255,0,0,255};
					break;
				case PUI_ThemeColor_Pink:
					MainColor[0]={255,198,222,255};
					MainColor[1]={254,188,200,255};
					MainColor[2]={255,178,191,255};
					MainColor[3]={250,158,173,255};
					MainColor[4]={255,140,153,255};
					MainColor[5]={255,120,131,255};
					MainColor[6]={253,108,118,255};
					MainColor[7]={253,88,99,255};
					break;
				case PUI_ThemeColor_Orange:
					MainColor[0]={255,223,182,255};
					MainColor[1]={255,210,170,255};
					MainColor[2]={255,202,168,255};
					MainColor[3]={255,192,124,255};
					MainColor[4]={255,174,108,255};
					MainColor[5]={255,161,93,255};
					MainColor[6]={255,140,55,255};
					MainColor[7]={255,81,0,255};
					break;
//				case PUI_ThemeColor_DarkBlue:
//					
//					break;
				case PUI_ThemeColor_Violet:
					MainColor[0]={167,171,248,255};
					MainColor[1]={145,155,229,255};
					MainColor[2]={125,127,232,255};
					MainColor[3]={94,108,235,255};
					MainColor[4]={83,97,225,255};
					MainColor[5]={75,84,221,255};
					MainColor[6]={70,73,217,255};
					MainColor[7]={50,64,163,255};
					break;
//				case PUI_ThemeColor_Silvery:
//					
//					break;
				case PUI_ThemeColor_Golden:
					MainColor[0]={255,239,183,255};
					MainColor[1]={255,229,173,255};
					MainColor[2]={255,228,130,255};
					MainColor[3]={255,224,91,255};
					MainColor[4]={255,215,64,255}; 
					MainColor[5]={255,207,52,255};  
					MainColor[6]={239,187,36,255};  
					MainColor[7]={216,165,18,255};  
					break;
//				case PUI_ThemeColor_White:
//					
//					break;
//				case PUI_ThemeColor_Gray:
//					
//					break;
//				case PUI_ThemeColor_Black:
//					
//					break;
				case PUI_ThemeColor_DeepGreen:
					MainColor[0]={134,215,161,255};
					MainColor[1]={122,201,154,255};
					MainColor[2]={96,195,130,255};
					MainColor[3]={64,191,100,255};
					MainColor[4]={47,179,102,255};
					MainColor[5]={51,158,81,255};
					MainColor[6]={39,144,98,255};
					MainColor[7]={11,116,27,255};
					break;
//				case PUI_ThemeColor_Colorful:
//					break;
			}
			
			switch (theme)
			{
				default:
					BackgroundColor[0]={250,250,250,255};
					BackgroundColor[1]={224,224,224,255};
					BackgroundColor[2]={192,192,192,255};
					BackgroundColor[3]={160,160,160,255};
					BackgroundColor[4]={128,128,128,255};
					BackgroundColor[5]={96,96,96,255};
					BackgroundColor[6]={64,64,64,255};
					BackgroundColor[7]={32,32,32,255};
					MainTextColor[0]=RGBA_BLACK;
					MainTextColor[1]=RGBA_WHITE;
					break;
			}
			CurrentThemeColor=theme;
		}
	}ThemeColor;
	vector <RGBA> PUI_ThemeColor::DynamicUserColor(1,RGBA_NONE);

	RGBA PUI_ThemeColor::GetRealColor(const RGBA &p)
	{
		if (IsUserColor(p))
			return GetUserColor(p);
		else if (IsThemeColor(p))
			switch (p.g)
			{
				case 0:	return ThemeColor.MainColor[p.b];
				case 1:	return ThemeColor.BackgroundColor[p.b];
				case 2:	return ThemeColor.MainTextColor[p.b];
				case 3: return RGBA(255,255,255,p.b*32+31);
				default: return ThemeColor.MainColor[7];
			}
		else return p;
	}
	
	RGBA PUI_ThemeColor::operator () (const RGBA &p)
	{
		if (IsUserColor(p))
			return GetUserColor(p);
		else if (IsThemeColor(p))
			switch (p.g)
			{
				case 0:	return MainColor[p.b];
				case 1:	return BackgroundColor[p.b];
				case 2:	return MainTextColor[p.b];
				case 3: return RGBA(255,255,255,p.b*32+31);
				default: return MainColor[7];
			}
		else return p;
	}
	
	const RGBA ThemeColorM[8]={{1,0,0,0},{1,0,1,0},{1,0,2,0},{1,0,3,0},{1,0,4,0},{1,0,5,0},{1,0,6,0},{1,0,7,0}},
			   ThemeColorBG[8]={{1,1,0,0},{1,1,1,0},{1,1,2,0},{1,1,3,0},{1,1,4,0},{1,1,5,0},{1,1,6,0},{1,1,7,0}},
			   ThemeColorMT[2]={{1,2,0,0},{1,2,1,0}},
			   ThemeColorTS[8]={{1,3,0,0},{1,3,1,0},{1,3,2,0},{1,3,3,0},{1,3,4,0},{1,3,5,0},{1,3,6,0},{1,3,7,0}};
	           
	//Font manage
	//...      
	           
	struct PUI_Font_Struct
	{          
		private:  
			TTF_Font *font[128];
			string fontFile;
			int defaultSize=-1;
			
			void Deconstruct()
			{
				for (int i=1;i<=127;++i)
					if (font[i]!=NULL)
						TTF_CloseFont(font[i]),
						font[i]=NULL;
			}

		public:
			inline bool operator ! ()
			{return fontFile.empty();}
			
			inline TTF_Font* operator () ()
			{return font[defaultSize];}
			
			inline TTF_Font* operator [] (int x)
			{
				if (x==0)
					return font[defaultSize];
				if (InRange(x,1,127))
					if (font[x]==NULL)
						return font[x]=TTF_OpenFont(fontFile.c_str(),x);
					else return font[x];
				else return PUI_DD[2]<<"PUI_Font[x] x is not in range[1,127]"<<endl,font[defaultSize];
			}
			
			~PUI_Font_Struct()
			{Deconstruct();}
			
			void SetDefaultSize(int _defaultsize)
			{defaultSize=_defaultsize;}
			
			void SetFontFile(const string &_fontfile)
			{
				Deconstruct();
				fontFile=_fontfile;
				static bool Inited=0;
				if (!Inited)
				{
					if (TTF_Init()!=0)
						PUI_DD[2]<<"Failed to TTF_Init()"<<endl;
					Inited=1;
				}	
				font[defaultSize]=TTF_OpenFont(fontFile.c_str(),defaultSize);
				if (font[defaultSize]==NULL)
					PUI_DD[2]<<"Failed to OpenFont: "<<fontFile<<endl;
			}
			
			PUI_Font_Struct(const string &_fontfile,const int _defaultsize)
			{
				memset(font,0,sizeof font);
				SetDefaultSize(_defaultsize);
				SetFontFile(_fontfile);
			}
			
			PUI_Font_Struct()
			{memset(font,0,sizeof font);}
	}PUI_DefaultFonts;//???
	
	struct PUI_Font
	{
		int Size=0;//>=0:PUI_Font <0:ttfFont
		union//These resources won't be managed.
		{
			PUI_Font_Struct *fonts;
			TTF_Font *ttfFont;
		};
		
		inline TTF_Font* operator () () const
		{
			if (Size==-1) return ttfFont;
			else return (*fonts)[Size];
		}
		
		bool operator == (const PUI_Font &tar) const
		{
			if (Size!=tar.Size) return 0;
			if (Size==-1) return ttfFont==tar.ttfFont;
			else return fonts==tar.fonts;
		}
		
		bool operator < (const PUI_Font &tar) const//Just for compare in container such as map
		{
			if (Size==tar.Size)
				if (Size==-1) return ttfFont<tar.ttfFont;
				else return fonts<tar.fonts;
			else return Size<tar.Size;
		}

		PUI_Font(PUI_Font_Struct *_fonts=&PUI_DefaultFonts,int _size=0):fonts(_fonts),Size(_size) {}
		
		PUI_Font(PUI_Font_Struct &_fonts,int _size=0):fonts(&_fonts),Size(_size) {}
		
		PUI_Font(int _size):fonts(&PUI_DefaultFonts),Size(_size) {}
		
		PUI_Font(TTF_Font *ttffont):ttfFont(ttffont),Size(-1) {}
	};
	
	inline int GetStringWidth(const string &str,const PUI_Font &font=PUI_Font())
	{
		int re=0;
		TTF_SizeUTF8(font(),str.c_str(),&re,NULL);
		return re;
	}
	
	inline int GetWidthString(const string &str,int width,const PUI_Font &font=PUI_Font())
	{
		//UTF8
		PUI_DD[2]<<"GetWidthString "<<__LINE__<<" is uncompleted function!"<<endl;
		return str.length();
	}
	
	//Surface/Texture/... Functions:
	
	Posize GetTexturePosize(SDL_Texture *tex)
	{
		if (tex==NULL) return ZERO_POSIZE;
		Posize re;
		re.x=re.y=re.w=re.h=0;
		SDL_QueryTexture(tex,NULL,NULL,&re.w,&re.h);
		return re;
	}
	
	SDL_Surface *CreateRGBATextSurface(const char *text,const RGBA &co,const PUI_Font &font=PUI_Font())
	{
		if (text==NULL) return NULL;
		SDL_Surface *sur=TTF_RenderUTF8_Blended(font(),text,RGBAToSDLColor(co));
		SDL_SetSurfaceBlendMode(sur,SDL_BLENDMODE_BLEND);
		SDL_SetSurfaceAlphaMod(sur,co.a);
		return sur;
	}
	
	SDL_Surface *CreateRingSurface(int d1,int d2,const RGBA &co)//return a ring whose radius is d1/2 and d2/2 
	{
		if (d2<=0) return NULL;
		SDL_Surface *sur=SDL_CreateRGBSurfaceWithFormat(0,d2,d2,32,SDL_PIXELFORMAT_RGBA32);
		SDL_SetSurfaceBlendMode(sur,SDL_BLENDMODE_BLEND);
		
		double midXY=d2/2.0,r1=d1/2.0,r2=d2/2.0;
		for (int i=0;i<d2;++i)
			for (int j=0;j<d2;++j)
			{
				double x=i+0.5,y=j+0.5;
				double r=sqrt((x-midXY)*(x-midXY)+(y-midXY)*(y-midXY));
				int alpha;
				if (InRange(r,r1,r2))
					alpha=255;
				else if (r>r2)
					alpha=max(0.0,255.0-(r-r2)*500);
				else alpha=max(0.0,255.0-(r1-r)*500);
				*((Uint32*)sur->pixels+i*sur->pitch/4+j)=SDL_MapRGBA(sur->format,co.r,co.g,co.b,alpha);
			}
		return sur;
	}
	
	SDL_Surface* CreateTriangleSurface(int w,int h,const Point &pt1,const Point &pt2,const Point &pt3,const RGBA &co)
	{
		if (w<=0||h<=0||!co)
			return NULL;
		SDL_Surface *sur=SDL_CreateRGBSurfaceWithFormat(0,w,h,32,SDL_PIXELFORMAT_RGBA32);
		SDL_SetSurfaceBlendMode(sur,SDL_BLENDMODE_BLEND);
		SDL_Rect rct;
		Uint32 col=SDL_MapRGBA(sur->format,co.r,co.g,co.b,co.a),col0=SDL_MapRGBA(sur->format,0,0,0,0);
		for (int i=0;i<h;++i)
		{
			int j,k;
			for (j=0;j<w;++j)
			{
				Point v0(j,i),
					  v1=pt1-v0,
					  v2=pt2-v0,
					  v3=pt3-v0;
				if (abs(v1%v2)+abs(v2%v3)+abs(v3%v1)==abs((pt2-pt1)%(pt3-pt1)))
					break;
			}
			for (k=w-1;k>=j;--k)
			{
				Point v0(k,i),
					  v1=pt1-v0,
					  v2=pt2-v0,
					  v3=pt3-v0;
				if (abs(v1%v2)+abs(v2%v3)+abs(v3%v1)==abs((pt2-pt1)%(pt3-pt1)))
					break;
			}
			if (j>0)
			{
				rct={0,i,j,1};
				SDL_FillRect(sur,&rct,col0);
			}
			if (k>j)
			{
				rct={j,i,k-j+1,1};
				SDL_FillRect(sur,&rct,col);
			}
			if (k<w-1)
			{
				rct={k+1,i,w-k-1,1};
				SDL_FillRect(sur,&rct,col0);
			}
		}
		return sur;
	}
	
	SDL_Surface* CreateSpotLightSurface(int r,const RGBA &co={255,255,255,100})//return a round whose radius is r,alpha color decrease from center to edge 
	{
		if (r<=0) return NULL;
		SDL_Surface *sur=SDL_CreateRGBSurfaceWithFormat(0,r<<1,r<<1,32,SDL_PIXELFORMAT_RGBA32);
		SDL_SetSurfaceBlendMode(sur,SDL_BLENDMODE_BLEND);
		
		for (int i=0;i<=r<<1;++i)
			for (int j=0;j<=r<<1;++j)
				*((Uint32*)sur->pixels+i*sur->pitch/4+j)=SDL_MapRGBA(sur->format,co.r,co.g,co.b,co.a*max(0.0,1-((i-r)*(i-r)+(j-r)*(j-r))*1.0/(r*r)));
		return sur;
	}
	
	RGBA GetSDLSurfacePixel(SDL_Surface *sur,const Point &pt)//maybe format has some problem
	{
		if (sur==NULL||!Posize(0,0,sur->w-1,sur->h-1).In(pt))
			return RGBA_NONE;
		RGBA re=RGBA_NONE;
		SDL_LockSurface(sur);
		Uint32 col=((Uint32*)sur->pixels)[(pt.y*sur->w+pt.x)];
		SDL_UnlockSurface(sur);
		SDL_GetRGBA(col,sur->format,&re.r,&re.g,&re.b,&re.a);
		return re;
	}
	
	void SetSDLSurfacePixel(SDL_Surface *sur,const Point &pt,const RGBA &co)
	{
		if (sur==NULL||!Posize(0,0,sur->w-1,sur->h-1).In(pt))
			return;
		SDL_Rect rct={pt.x,pt.y,1,1};
		Uint32 col=SDL_MapRGBA(sur->format,co.r,co.g,co.b,co.a);
		SDL_FillRect(sur,&rct,col);
	}

	//PUI_Event
	class PUI_Window;
	class PUI_PosEvent;
	class PUI_MouseEvent;
	class PUI_TouchEvent;
	class PUI_PenEvent;
	class PUI_KeyEvent;
	class PUI_WheelEvent;
	class PUI_TextEvent;
	class PUI_RenderEvent;
	class PUI_ForceRenderEvent;
	class PUI_RefreshEvent;
	class PUI_WindowEvent;
	class PUI_WindowTBEvent;
	class PUI_DropEvent;
	class PUI_UserEvent;
	class PUI_SDLUserEvent;
	class PUI_FunctionEvent;
	template <class T> class PUI_UserEventT;
	class PUI_Event
	{
		friend int PUI_SolveEvent(PUI_Event*);
		protected:
			static PUI_Event *_NowSolvingEvent; 
		
		public:
			enum
			{
				Event_NONE=0,
				Event_Common,

				Event_PosEventBegin,//for check if PosEvent using InRange
					Event_MouseEvent,
					Event_TouchEvent,
					Event_PenEvent,
					Event_VirtualPos,
				Event_PosEventEnd,
				
				Event_KeyEvent,
				Event_WheelEvent,
				Event_TextEditEvent,
				Event_TextInputEvent,
				Event_RenderEvent,//Draw something in screen??
				Event_ForceRenderEvent,
				Event_WindowEvent,
				Event_WindowTBEvent,
				Event_DropEvent,
				Event_UserEvent,
				Event_SDLUserEvent,
				Event_FunctionEvent,
				Event_RefreshEvent,
				Event_Quit,
				Event_End
			};
			static unsigned int UserRegisteredEventCount;
			
			unsigned int type=Event_NONE,
				   		 timeStamp=0;
			
			inline static unsigned int RegisterEvent(int count=1)
			{
				unsigned int re=Event_End+UserRegisteredEventCount+1;
				UserRegisteredEventCount+=count;
				return re;
			}
			
			static inline const PUI_Event* NowSolvingEvent()
			{return _NowSolvingEvent;}
			
			inline PUI_PosEvent* PosEvent() const
			{return (PUI_PosEvent*)this;}
			
			inline PUI_MouseEvent* MouseEvent() const
			{return (PUI_MouseEvent*)this;}
			
			inline PUI_TouchEvent* TouchEvent() const
			{return (PUI_TouchEvent*)this;}
			
			inline PUI_PenEvent* PenEvent() const
			{return (PUI_PenEvent*)this;}
			
			inline PUI_KeyEvent* KeyEvent() const
			{return (PUI_KeyEvent*)this;}
			
			inline PUI_WheelEvent* WheelEvent() const
			{return (PUI_WheelEvent*)this;}
			
			inline PUI_TextEvent* TextEvent() const
			{return (PUI_TextEvent*)this;}
			
			inline PUI_RenderEvent* RenderEvent() const
			{return (PUI_RenderEvent*)this;}
			
			inline PUI_ForceRenderEvent* ForceRenderEvent() const
			{return (PUI_ForceRenderEvent*)this;}
			
			inline PUI_RefreshEvent* RefreshEvent() const
			{return (PUI_RefreshEvent*)this;}
			
			inline PUI_WindowEvent* WindowEvent() const
			{return (PUI_WindowEvent*)this;}
			
			inline PUI_WindowTBEvent* WindowTBEvent() const
			{return (PUI_WindowTBEvent*)this;}
			
			inline PUI_DropEvent* DropEvent() const
			{return (PUI_DropEvent*)this;}
			
			inline PUI_UserEvent* UserEvent() const
			{return (PUI_UserEvent*)this;}
			
			inline PUI_SDLUserEvent* SDLUserEvent() const
			{return (PUI_SDLUserEvent*)this;}
			
			inline PUI_FunctionEvent* FunctionEvent() const
			{return (PUI_FunctionEvent*)this;}
			
			virtual ~PUI_Event() {} 
			
			PUI_Event(const PUI_Event&)=delete;
			
			PUI_Event(unsigned int _type):type(_type) {timeStamp=SDL_GetTicks();}
			
			PUI_Event(unsigned int _type,unsigned int _timestamp):type(_type),timeStamp(_timestamp) {}
			
			PUI_Event() {}
	};
	unsigned int PUI_Event::UserRegisteredEventCount=0;
	PUI_Event *PUI_Event::_NowSolvingEvent=nullptr;
	
	class PUI_PosEvent:public PUI_Event
	{
		public:
			enum
			{
				Pos_NONE=0,
				Pos_Down,
				Pos_Up,
				Pos_Motion
			};
			
			enum
			{
				Button_None=0,
				Button_MainClick,
				Button_SubClick,
				Button_OtherClick
			};
			
			Point pos=ZERO_POINT,
				  delta=ZERO_POINT;
			PUI_Window *win=NULL;
//			unsigned int devID=0;//device ID
			unsigned char posType=Pos_NONE,
						  button=Button_None,//is mainClick(mouse left etc) or subClick(mouseRight etc) or other
						  clicks=0;
			
			PUI_PosEvent(unsigned int _type,unsigned int _timestamp,const Point &_pos,const Point &_delta,PUI_Window *_win,unsigned char _postype,unsigned char _button,unsigned char _clicks)
						:PUI_Event(_type,_timestamp),pos(_pos),delta(_delta),win(_win),posType(_postype),button(_button),clicks(_clicks) {}
			
			PUI_PosEvent() {}
	};
	
	class PUI_MouseEvent:public PUI_PosEvent
	{
		public:
			enum
			{
				Mouse_NONE=0,
				Mouse_Left=1,
				Mouse_Middle=2,
				Mouse_Right=4,
				Mouse_X1=8,
				Mouse_X2=16
			};
			
			unsigned char which=Mouse_NONE,//which button
						  state=Mouse_NONE;//button state(bit or of all)
	};
	
	class PUI_TouchEvent:public PUI_PosEvent
	{
		public:
			enum
			{
				Ges_NONE=0,
				Ges_ShortClick,//count==1
				Ges_LongClick,//count==1
				Ges_Moving,//count==1
				Ges_MultiClick,//count>=2
				Ges_MultiMoving,//count>=2
				Ges_End//last finger release,it means a gesture is completed
			};
			
			struct Finger
			{
				Point pos=ZERO_POINT;
				unsigned int downTime=0;
				PUI_Window *win=NULL;
				float pressure=0;
				SDL_FingerID fingerID=0;
				static deque <Doublet<unsigned int,Point> > CenterPosTrack;
				static float gestureMovingDist;
				static unsigned int lastUpdateTime,
									gestureStartTime;
				static unsigned char count,
									 gesture;
			};
			static Finger fingers[10];
			
			//pos:center of multi gesture
			float pressure=0,
//				  dTheta=0,
				  speedX=0,
				  speedY=0;//pixels per ms
			unsigned int duration=0,//In ticks
						 diameter=0;
			int dDiameter=0;
			unsigned char gesture=Ges_NONE,
						  gesChanged=0,//0,1
						  count=0,//0~10
						  countChanged=0;//0,1
			
			inline float speed() const
			{return sqrt(speedX*speedX+speedY*speedY);}
			
			inline float dDiameterPercent() const
			{return diameter*1.0/max(1,(int)diameter-dDiameter);}
	};
	float PUI_TouchEvent::Finger::gestureMovingDist=0;
	deque <Doublet<unsigned int,Point> > PUI_TouchEvent::Finger::CenterPosTrack;
	unsigned int PUI_TouchEvent::Finger::lastUpdateTime=0;
	unsigned int PUI_TouchEvent::Finger::gestureStartTime=0;
	unsigned char PUI_TouchEvent::Finger::count=0;
	unsigned char PUI_TouchEvent::Finger::gesture=0;
	PUI_TouchEvent::Finger PUI_TouchEvent::fingers[10];
	
	class PUI_PenEvent:public PUI_PosEvent
	{
		public:
			enum
			{
				Stat_None=0,
				Stat_Barrel=1,
				Stat_Inverted=2,
				Stat_Eraser=4
			};
			float pressure=0,//0~1
				  theta=0,//0~359
				  tiltX=0,//-90~90
				  tiltY=0;//-90~90
			unsigned char stat=0,
						  contactted=0;//0,1
	};
	
	class PUI_KeyEvent:public PUI_Event
	{
		public:
			enum
			{
				Key_NONE=0,
				Key_Down,
				Key_Up,
				Key_Hold
			};
			
			PUI_Window *win=NULL;
			unsigned int keyCode=0;
//			unsigned int devID=0;
			unsigned short mod=0;
			unsigned char keyType=0;
			
			bool IsDownOrHold()
			{return keyType==Key_Down||keyType==Key_Hold;}
	};
	
	class PUI_WheelEvent:public PUI_Event
	{
		public:
			PUI_Window *win=NULL;
//			unsigned int devID=0;
			int dx,dy;
	};
	
	class PUI_TextEvent:public PUI_Event
	{
		public:
			PUI_Window *win=NULL;
			string text;//encoding UTF-8
			int cursor=0,
				length=0;
	};
	
	class PUI_RenderEvent:public PUI_Event
	{
		public:
			//...
	};
	
	class PUI_ForceRenderEvent:public PUI_Event
	{
		public:
			PUI_Window *win=NULL;
			Posize Area=ZERO_POSIZE;

			PUI_ForceRenderEvent(PUI_Window *_win,const Posize &ps=ZERO_POSIZE):PUI_Event(Event_ForceRenderEvent),win(_win),Area(ps) {}
			
			PUI_ForceRenderEvent():PUI_Event(Event_ForceRenderEvent) {}
	};
	
	class PUI_RefreshEvent:public PUI_Event
	{
		public:
			static unsigned int NewestRefreshCode;
			unsigned int CurrentRefreshCode=0;
			
			inline bool CurrentIsNewest() const
			{return CurrentRefreshCode==NewestRefreshCode;}
			
			PUI_RefreshEvent():PUI_Event(Event_RefreshEvent)
			{
				CurrentRefreshCode=++NewestRefreshCode;
			}
	};
	unsigned int PUI_RefreshEvent::NewestRefreshCode=0;
	
	class PUI_WindowEvent:public PUI_Event
	{
		public:
			enum
			{
				Window_None=0,
				Window_Shown,
				Window_Hidden,
				Window_Exposed,
				Window_Moved,
				Window_Resized,
				Window_SizeChange,
				Window_Minimized,
				Window_Maximized,
				Window_Restored,
				Window_Enter,
				Window_Leave,
				Window_GainFocus,
				Window_LoseFocus,
				Window_Close,
				Window_TakeFocus,
				Window_HitTest
			};
			PUI_Window *win=NULL;
			unsigned int eventType=Window_None,
						 data1=0,
						 data2=0;
	};
	
	class PUI_WindowTBEvent:public PUI_Event
	{
		public:
			PUI_Window *win=NULL;
			SDL_Surface *sur=NULL;
			Posize updatedPs;
	};
	
	class PUI_DropEvent:public PUI_Event
	{
		public:
			enum
			{
				Drop_None=0,
				Drop_Begin,
				Drop_File,
				Drop_Text,
				Drop_Complete
			};
			PUI_Window *win=NULL;
			unsigned int drop=Drop_None;
			string str;
	};
	
	class PUI_UserEvent:public PUI_Event
	{
		public:
			unsigned int code=0;
			void *data1=NULL;
			void *data2=NULL;
		
		PUI_UserEvent(unsigned int _type,unsigned int _timeStamp,unsigned int _code=0,void *_data1=NULL,void *_data2=NULL)
		:PUI_Event(_type,_timeStamp),code(_code),data1(_data1),data2(_data2) {}
		
		PUI_UserEvent() {}
	};
	
	Uint32 PUI_SDL_USEREVENT=0;
	class PUI_SDLUserEvent:public PUI_Event
	{
		public:
			unsigned int code=0,SDLtype=0;
			void *data1=NULL;
			void *data2=NULL;
	};
	
	class PUI_FunctionEvent:public PUI_Event
	{
		protected:
			BaseTypeFuncAndData *func=NULL;
			int TriggerCount=0;
		
		public:
			int Trigger(int usercode=0)
			{
				++TriggerCount;
				if (func)
					return func->CallFunc(usercode);
				else return 0;
			}
			
			inline int GetTriggerCount() const
			{return TriggerCount;}
			
			~PUI_FunctionEvent()
			{
				if (func)
					delete func;
			}
			
			PUI_FunctionEvent(BaseTypeFuncAndData *_func):PUI_Event(Event_FunctionEvent),func(_func) {}
	};
	
	inline void PUI_SendEvent(PUI_Event *event)
	{SendSDLUserEventMsg(PUI_SDL_USEREVENT,0,event);}
	
	inline void PUI_SendUserEvent(unsigned int type,unsigned int code=0,void *data1=nullptr,void *data2=nullptr)
	{PUI_SendEvent(new PUI_UserEvent(type,SDL_GetTicks(),code,data1,data2));}
	
	inline void PUI_SendFunctionEvent(BaseTypeFuncAndData *func)
	{PUI_SendEvent(new PUI_FunctionEvent(func));}
	
	inline void PUI_SendFunctionEvent(void (*func)(void*),void *funcdata=nullptr)
	{PUI_SendEvent(new PUI_FunctionEvent(new VoidFuncAndData(func,funcdata)));}
	
	template <typename T> inline void PUI_SendFunctionEvent(void (*func)(T&),const T &funcdata)
	{PUI_SendEvent(new PUI_FunctionEvent(new TypeFuncAndDataV<T>(func,funcdata)));}
	
	template <typename T> inline void PUI_SendFunctionEvent(int (*func)(T&,int),const T &funcdata)
	{PUI_SendEvent(new PUI_FunctionEvent(new TypeFuncAndData<T>(func,funcdata)));}
	
	class SynchronizeFunctionData
	{
		protected:
			SDL_sem *sem=NULL;
			int *ReturnCode=NULL;
			
		public:
			inline bool Valid() const
			{return sem!=NULL;}
			
			inline void Continue(int returnCode=0)//Can only be called once
			{
				*ReturnCode=returnCode;
				SDL_SemPost(sem);
				sem=NULL;
			}
			
			SynchronizeFunctionData(SDL_sem *_sem,int *_returncode):sem(_sem),ReturnCode(_returncode) {}
			
			SynchronizeFunctionData() {}
	};
	
	inline int PUI_SendSynchronizeFunctionEvent(void (*func)(SynchronizeFunctionData&))
	{
		int re=0;
		SDL_sem *sem=SDL_CreateSemaphore(0);
		SynchronizeFunctionData data(sem,&re);
		PUI_SendFunctionEvent(new TypeFuncAndDataV<SynchronizeFunctionData>(func,data));
		SDL_SemWait(sem);
		SDL_DestroySemaphore(sem);
		return re;
	}
	
	template <class T> class SynchronizeFunctionDataT:public SynchronizeFunctionData
	{
		protected:
			T *tar=NULL;
			
		public:
			inline T* operator -> ()
			{return tar;}
			
			inline T& operator () ()
			{return *tar;}
			
			SynchronizeFunctionDataT(SDL_sem *_sem,int *_returncode,T *_tar):SynchronizeFunctionData(_sem,_returncode),tar(_tar) {}
			
			SynchronizeFunctionDataT() {}
	};
	
	template <typename T> inline int PUI_SendSynchronizeFunctionEvent(void (*func)(SynchronizeFunctionDataT <T>&),T &funcdata)
	{
		int re=0;
		SDL_sem *sem=SDL_CreateSemaphore(0);
		SynchronizeFunctionDataT <T> data(sem,&re,&funcdata);
		PUI_SendFunctionEvent(new TypeFuncAndDataV<SynchronizeFunctionDataT<T> >(func,data));
		SDL_SemWait(sem);
		SDL_DestroySemaphore(sem);
		return re;
	}
	
	SDL_TimerID SynchronizeRefreshTimerID=0;
	void SetSynchronizeRefreshTimerOnOff(bool on)
	{
		if (on==(SynchronizeRefreshTimerID!=0)) return;
		if (on)
			SynchronizeRefreshTimerID=SDL_AddTimer(1000.0/PUI_Parameter_RefreshRate,
				[](Uint32 itv,void*)->Uint32
				{
					PUI_SendEvent(new PUI_RefreshEvent());
					return itv;
				},NULL);
		else SDL_RemoveTimer(SynchronizeRefreshTimerID),SynchronizeRefreshTimerID=0;
	}
	
	inline bool IsSynchronizeRefreshTimerOn()
	{return SynchronizeRefreshTimerID!=0;}
	
	//PUI_Window
	class Widgets;
	using WidgetPtr=GroupPtr <Widgets>;
	class Layer;
	class WindowBackgroundLayer;
	class PUI_Window//A window(to support multi windows)
	{
		friend bool PUI_PresentWidgets();
		friend void PUI_PresentWidgets(PUI_Window *win,Posize lmt);
		friend void PUI_PresentWidgets(Widgets *tar);
		friend void PUI_UpdateWidgetsPosize(Widgets *wg);
		friend void PUI_UpdateWidgetsPosize(PUI_Window *win);
		friend int PUI_SolveEvent(PUI_Event *event);
		friend class Widgets;
		friend class WindowBackgroundLayer;
		
		public:
			enum
			{
				PUI_RendererType_Unknown=0,
				PUI_RendererType_Direct3D,
				PUI_RendererType_OpenGL,
				PUI_RendererType_OpenGLES2,
				PUI_RendererType_OpenGLES,
				PUI_RendererType_SDL2SoftWare
			};
			
			static const unsigned int PUI_WINPS_CENTER,
					  	 	 		  PUI_WINPS_UNDEFINE,
					  	 	 		  PUI_FLAG_RESIZEABLE=1,
					  	 			  PUI_FLAG_SOFTWARE=2,
									  PUI_FLAG_KEEPTOP=4,
									  PUI_FLAG_BORDERLESS=8;
									
		protected:
			static set <PUI_Window*> AllWindow;
			static map <unsigned int,PUI_Window*> WinOfSDLWinID;
			static int WindowCnt;
			static bool NeedFreshScreenAll,
						CurrentRefreshReady;
			
			string WindowTitle;
			SDL_Window *win=NULL;
			SDL_Renderer *ren=NULL;
			unsigned int SDLWinID;
			
			unsigned int PUI_WindowFlag=0;
			
			unsigned int RendererType=0;
			string RendererName;
			RGBA BackgroundColor;
			bool NeedRefreshFullScreen=0,
				 IsTransparentBackground=0;
			unsigned int TransparentBackgroundUpdateInterval=100;
			SDL_TimerID Timer_UpdateTransparentBackground=0;
			SharedTexturePtr TransparentBackgroundPic;
			Posize TransparentBackgroundPicPS;
			
//			SDL_Event *NowSolvingEvent=NULL;
			
			int NeedFreshScreen=1;//1:Normal 2:CanSkip
			bool NeedUpdatePosize=1,
				 NeedSolveEvent=0,
				 NeedSolvePosEvent=0,
				
				 PosFocused=1,
				 KeyboardFocused=1,
				 Hidden=0;
//			int NowSolvingPosEventMode=0;//0:common 1:from LoseFocus 2:from OccupyPos 3:virtual PosEvent
			float ScalePercent=PUI_GlobalScalePercent;
			
			Posize WinPS,
				   PresentLimit;
		
			Point _NowPos;
			
			WindowBackgroundLayer *_BackGroundLayer=NULL;
			Layer *_MenuLayer=NULL;
			
//			struct LoseFocusLinkTable
//			{
//				LoseFocusLinkTable *nxt=NULL;
//				Widgets *wg=NULL;
//				
//				LoseFocusLinkTable(Widgets *tar):wg(tar) {}
//			};
//			LoseFocusLinkTable *LoseFocusWgHead=NULL;//The original OccupyPos
//			int LoseFocusState=0;
			set <Widgets*> LoseFocusTargets;
			
			static Widgets *OccupyPosWg,
						   *KeyboardInputWg;
//					*TouchCapturedWg=NULL;//if captured by some widget,other widget should return untrigger state,and it should be set NULL by that widget when Ges_End.
			
			BaseTypeFuncAndData *CloseFunc=NULL;//The function to call when closed button was click
			
			struct TextTextureIndexInfo
			{
				string text;
				RGBA co;
				PUI_Font font;
				
				unsigned long long memorySize=0;
//				unsigned int timecode=0;//In ticks
				
				bool operator < (const TextTextureIndexInfo &tar) const
				{
					if (text==tar.text)
						if (co==tar.co)
							return font<tar.font;
						else return co.ToIntRGBA()<tar.co.ToIntRGBA();
					else return text<tar.text;
				}
				
				void SetWithTextureSize(unsigned long long size=0)
				{memorySize=text.length()+sizeof(co)+sizeof(font)+size;}
				
				TextTextureIndexInfo(const string &str,const RGBA &_co,const PUI_Font &_font)
				:text(str),co(_co),font(_font)
				{
					SetWithTextureSize();
				}
			};
			LRU_LinkHashTable <TextTextureIndexInfo,SharedTexturePtr> TextTextureMemorizeData;
			bool EnableTextTextureMemorize=1;

			void RefreshWinPS()
			{
				SDL_GetWindowPosition(win,&WinPS.x,&WinPS.y);
				SDL_GetWindowSize(win,&WinPS.w,&WinPS.h);
				NeedUpdatePosize=1;
			}
			
		public:
			static set <PUI_Window*>& GetAllWindowSet()
			{return AllWindow;}
			
			static PUI_Window* GetWindowBySDLID(int _ID)
			{
				auto mp=WinOfSDLWinID.find(_ID);
				if (mp==WinOfSDLWinID.end())
					return NULL;
				else return mp->second;
			}
			
			inline unsigned int GetSDLWinID()
			{return SDLWinID;}
			
			inline SDL_Window* GetSDLWindow()
			{return win;}
			
			inline SDL_Renderer* GetSDLRenderer()
			{return ren;}
			
			static unsigned int GetRendererTypeFromSDLRendererDriverName(const string &name)
			{
				static map <string,unsigned int> ma;
				if (ma.empty())
				{
					ma["direct3d"]=PUI_RendererType_Direct3D;
					ma["opengl"]=PUI_RendererType_OpenGL;
					ma["opengles2"]=PUI_RendererType_OpenGLES2;
					ma["opengles"]=PUI_RendererType_OpenGLES;
					ma["software"]=PUI_RendererType_SDL2SoftWare;
				}
				auto mp=ma.find(name);
				if (mp==ma.end())
					return 0;
				else return mp->second;
			}
			
			static vector <Doublet<unsigned,string> > GetAvaliableRenderers()
			{
				vector <Doublet<unsigned,string> > re;
				int renderdrivernum=SDL_GetNumRenderDrivers();
				SDL_RendererInfo info;
				for (int i=0;i<renderdrivernum;++i)
					SDL_GetRenderDriverInfo(i,&info),
					re.push_back({GetRendererTypeFromSDLRendererDriverName(info.name),info.name});
				return re;
			}
			
			inline void SetWindowGrab(bool enable)
			{SDL_SetWindowGrab(win,enable?SDL_TRUE:SDL_FALSE);}
			
			inline const Point& NowPos()
			{return _NowPos;}
			
			inline WindowBackgroundLayer* BackGroundLayer()
			{return _BackGroundLayer;}
			
			inline Layer* MenuLayer()
			{return _MenuLayer;}
			
			inline void SetWindowTitle(const string &title)
			{
				WindowTitle=title;
				SDL_SetWindowTitle(win,title.c_str());
			}
			
			inline string GetWindowTitle() const
			{return WindowTitle;}

			inline Posize GetWinPS() const
			{
				if (ScalePercent==1) return WinPS;
				else return WinPS/ScalePercent;
			}
			
			inline Posize GetWinPSPhysical() const
			{return WinPS;}

			inline Posize GetScreenPS() const
			{
				SDL_Rect rct;
				SDL_GetDisplayBounds(SDL_GetWindowDisplayIndex(win),&rct);
				return SDLRectToPosize(rct);
			}
			
			inline Posize GetWinSize() const
			{
				if (ScalePercent==1) return WinPS.ToOrigin();
				else return WinPS.ToOrigin()/ScalePercent;
			}
			
			inline Posize GetWinSizePhysical() const
			{return WinPS.ToOrigin();}
			
			inline Uint32 GetSDLWindowFlags()
			{return	SDL_GetWindowFlags(win);}
			
			inline void SetMinWinSize(int _w,int _h)
			{
				if (ScalePercent==1) SDL_SetWindowMinimumSize(win,_w,_h);
				else SDL_SetWindowMinimumSize(win,_w*ScalePercent,_h*ScalePercent);
			}
			
			inline void SetMinWinSizePhysical(int _w,int _h)
			{SDL_SetWindowMinimumSize(win,_w,_h);}
			
			inline void SetMaxWinSize(int _w,int _h)
			{
				if (ScalePercent==1) SDL_SetWindowMaximumSize(win,_w,_h);
				else SDL_SetWindowMaximumSize(win,_w*ScalePercent,_h*ScalePercent);
			}
			
			inline void SetMaxWinSizePhysical(int _w,int _h)
			{SDL_SetWindowMaximumSize(win,_w,_h);}
			
			inline void SetWindowPos(const Point &pos)
			{
				if (ScalePercent==1) WinPS.x=pos.x,WinPS.y=pos.y;
				else WinPS.x=pos.x*ScalePercent,WinPS.y=pos.y*ScalePercent;
				SDL_SetWindowPosition(win,WinPS.x,WinPS.y);
			}
			
			inline void SetWindowPosPhysical(const Point &pos)
			{SDL_SetWindowPosition(win,WinPS.x,WinPS.y);}
			
			inline void SetWindowSize(int w,int h)
			{
				if (ScalePercent==1) WinPS.w=w,WinPS.h=h;
				else WinPS.w=w*ScalePercent,WinPS.h=h*ScalePercent;
				SDL_SetWindowSize(win,WinPS.w,WinPS.h);
			}
			
			inline void SetWindowSizePhysical(int w,int h)
			{SDL_SetWindowSize(win,WinPS.w,WinPS.h);}
			
			inline void SetWinPS(const Posize &winps)
			{
				SetWindowPos(winps.GetLU());
				SetWindowSize(winps.w,winps.h);
			}
			
			inline void SetWinPSPhysical(const Posize &winps)
			{
				SetWindowPosPhysical(winps.GetLU());
				SetWindowSizePhysical(winps.w,winps.h);
			}
			
			inline void SetWindowsResizable(bool enable)
			{SDL_SetWindowResizable(win,enable?SDL_TRUE:SDL_FALSE);}
			
			inline void SetWindowIcon(SDL_Surface *icon)
			{SDL_SetWindowIcon(win,icon);}
			
			inline void MaximizeWindow()
			{SDL_MaximizeWindow(win);}
			
			inline void MinimizeWindow()
			{SDL_MinimizeWindow(win);}
			
			inline void RestoreWindow()
			{SDL_RestoreWindow(win);}
			
			inline void RaiseWindow()
			{SDL_RaiseWindow(win);}
			
			inline void HideWindow()
			{
				Hidden=1;
				SDL_HideWindow(win);
			}
			
			inline bool IsWindowHidden()
			{return Hidden;}
			
			inline void ShowWindow()
			{
				Hidden=0;
				SDL_ShowWindow(win);
//				SetNeedFreshScreen();
				SetNeedUpdatePosize();
				SetPresentArea(WinPS.ToOrigin());
			}
			
			inline void SetFullScreen()
			{
				SDL_SetWindowFullscreen(win,SDL_WINDOW_FULLSCREEN);
				RefreshWinPS();
			}
			
			inline void SetWindowFullScreen()
			{
				SDL_SetWindowFullscreen(win,SDL_WINDOW_FULLSCREEN_DESKTOP);
				RefreshWinPS();
			}
			
			inline void CancelFullScreen()
			{
				SDL_SetWindowFullscreen(win,0);
				RefreshWinPS();
			}
			
			inline bool IsFullScreen()
			{return SDL_GetWindowFlags(win)&SDL_WINDOW_FULLSCREEN;}
				
			inline void SetWindowBordered(bool bordered)
			{SDL_SetWindowBordered(win,bordered?SDL_TRUE:SDL_FALSE);}
			
			inline void SetWindowOpacity(float opa)
			{SDL_SetWindowOpacity(win,opa);}
			
			inline float GetWindowOpacity()
			{
				float re=1;
				SDL_GetWindowOpacity(win,&re);
				return re;
			}
			
			inline float GetScalePercent() const
			{return ScalePercent;}
			
			static inline void SetOccupyPosWg(Widgets *tar)
			{OccupyPosWg=tar;}
			
			static inline Widgets* GetOccupyPosWg()
			{return OccupyPosWg;}
			
//			inline void SetTouchCapturedWg(Widgets *tar)
//			{TouchCapturedWg=tar;}
//			
//			inline Widgets* GetTouchCapturedWg()
//			{return TouchCapturedWg;}
			
			static inline void SetKeyboardInputWg(Widgets *tar)
			{KeyboardInputWg=tar;}
			
			static inline Widgets* GetKeyboardInputWg()
			{return KeyboardInputWg;}
			
//			inline SDL_Event* GetNowSolvingEvent()
//			{return NowSolvingEvent;}
			
//			inline void SetNowSolvingEvent(SDL_Event &tar)
//			{NowSolvingEvent=&tar;}
			
			static void SetNeedFreshScreenAll()
			{NeedFreshScreenAll=1;}
			
			static bool IsNeedFreshScreenAll()
			{return NeedFreshScreenAll;}
			
			static void ClearFreshAllScreenFlag()
			{NeedFreshScreenAll=0;}
			
			inline void ClearFreshScreenFlag()
			{NeedFreshScreen=0;}
			
//			inline void SetNeedFreshScreen(int skipFlag=0)//0: cannot skip 1:can accelerate 2:can skip
//			{
//				if (NeedFreshScreen==0)
//					NeedFreshScreen=skipFlag+1;
//				else NeedFreshScreen=min(NeedFreshScreen,skipFlag+1);
//			}

//			inline void SetNeedFreshScreen()
//			{NeedFreshScreen=1;}
			
			inline bool IsNeedFreshScreen()
			{return NeedFreshScreen;}
			
//			inline int NeedFreshScreenFlag()
//			{return NeedFreshScreen;}
			
			inline void SetNeedUpdatePosize()
			{NeedUpdatePosize=1;}
			
			inline bool IsNeedUpdatePosize()
			{return NeedUpdatePosize;}
			
			inline void StopSolveEvent()
			{NeedSolveEvent=0;}
			
			inline bool IsNeedSolveEvent()
			{return NeedSolveEvent;}
			
			inline void StopSolvePosEvent()
			{NeedSolvePosEvent=0;}
			
			inline bool IsNeedSolvePosEvent()
			{return NeedSolvePosEvent;}
			
			inline bool IsPosFocused()
			{return PosFocused;}
			
			inline bool IsKeyboardFocused()
			{return KeyboardFocused;}
			
			inline void SetMousePoint(const Point &pt)
			{
				if (ScalePercent==1) SDL_WarpMouseInWindow(win,pt.x,pt.y);
				else SDL_WarpMouseInWindow(win,pt.x*ScalePercent,pt.y*ScalePercent);
			}
			
			inline void SetMousePointPhysical(const Point &pt)
			{SDL_WarpMouseInWindow(win,pt.x,pt.y);}
			
			inline void SetPresentArea(const Posize &ps)
			{
				if (ps.Size()<=0)
					return;
				NeedFreshScreen=1;
				PresentLimit|=ps;//<<Better method to decide which places to present.
			}
			
//			inline int GetNowSolvingPosEventMode()
//			{return NowSolvingPosEventMode;}
//			
//			inline void SetNowSolvingPosEventMode(int mode)
//			{NowSolvingPosEventMode=mode;}
			
			void SetBackgroundColor(const RGBA &co);
			
			void SetCloseFunc(BaseTypeFuncAndData *func)
			{
				if (CloseFunc!=NULL)
					delete CloseFunc;
				CloseFunc=func;
			}
			
			
//			struct RenderTargetData
//			{
//				SDL_Texture *tex=NULL;
//				Posize texps;
//				
//				RenderTargetData(SDL_Texture *_tex,const Posize &ps):tex(_tex),texps(ps) {}
//			};
//		
//		protected:
//			stack <RenderTargetData> RenderTargetStack;
//		public:
//			
//			inline void PushRenderTarget(const RenderTargetData &tar)
//			{
//				RenderTargetStack.push(tar);
//				SDL_SetRenderTarget(ren,tar.tex);
//			}
//			
//			inline void PopRenderTarget()
//			{
//				RenderTargetStack.pop();
//				SDL_SetRenderTarget(ren,RenderTargetStack.empty()?NULL:RenderTargetStack.top().tex);
//			}
			
		protected:
			inline void SDLRenderCopyWithScale(SDL_Texture *tex,const Posize &src,const Posize &dst)
			{
				SDL_Rect srct=PosizeToSDLRect(src),
					 	 drct=PosizeToSDLRect(ScalePercent==1?dst:dst*ScalePercent);
				SDL_RenderCopy(ren,tex,&srct,&drct);
			}
			
		public:
			
			inline void SetRenderColor(const RGBA &co)
			{SDL_SetRenderDrawColor(ren,co.r,co.g,co.b,co.a);}
			
			inline void RenderClear()
			{
				SetRenderColor(BackgroundColor);
				SDL_RenderClear(ren);
			}
			
			inline void RenderDrawPoint(const Point &pt)
			{SDL_RenderDrawPoint(ren,pt.x,pt.y);}
			
			inline void RenderDrawPoint(const Point &pt,const RGBA &co)
			{
				SetRenderColor(co);
				RenderDrawPoint(pt);
			}
			
			void RenderDrawLine(const Point &pt1,const Point &pt2)
			{
//				if (!RenderTargetStack.empty())
//					pt1=pt1-RenderTargetStack.top().texps.GetLU(),
//					pt2=pt2-RenderTargetStack.top().texps.GetLU();
				if (ScalePercent==1)
					SDL_RenderDrawLine(ren,pt1.x,pt1.y,pt2.x,pt2.y);
				else//??
				{
					Point p1=pt1*ScalePercent,
						  p2=pt2*ScalePercent;
					SDL_RenderDrawLine(ren,p1.x,p1.y,p2.x,p2.y);
				}
			}
			
			inline void RenderDrawLine(const Point &pt1,const Point &pt2,const RGBA &co)
			{
				SetRenderColor(co);
				RenderDrawLine(pt1,pt2);
			}
			
			inline void RenderFillRect(const Posize &ps,const RGBA &co)
			{
				if (!co.HaveColor()||ps.Size()<=0) return;
				SetRenderColor(co);
				SDL_Rect rct=PosizeToSDLRect(ScalePercent==1?ps:ps*ScalePercent);//(RenderTargetStack.empty()?ps:(ps-RenderTargetStack.top().texps));
				SDL_RenderFillRect(ren,&rct);
			}
			
			void RenderDrawRectWithLimit(const Posize &ps,const RGBA &co,Posize lmt)
			{
				lmt=lmt&ps;
				if (lmt.Size()<=0||!co.HaveColor())
					return;
				
				RenderFillRect(Posize(ps.x,ps.y,ps.w-1,1)&lmt,co);
				RenderFillRect(Posize(ps.x2(),ps.y,1,ps.h-1)&lmt,co);
				RenderFillRect(Posize(ps.x+1,ps.y2(),ps.w-1,1)&lmt,co);
				RenderFillRect(Posize(ps.x,ps.y+1,1,ps.h-1)&lmt,co);
			}
			
			inline void RenderCopy(SDL_Texture *tex,const Point &targetPt)
			{
				if (tex==NULL) return;
				Posize tex_PS=GetTexturePosize(tex);
				SDLRenderCopyWithScale(tex,tex_PS,tex_PS+targetPt);
			}
			
			inline void RenderCopy(SDL_Texture *tex,const Posize &targetPS)
			{
				if (tex==NULL||targetPS.Size()<=0) return;
				Posize tex_PS=GetTexturePosize(tex);
				SDLRenderCopyWithScale(tex,tex_PS,targetPS);
			}
			
			inline void RenderCopy(SDL_Texture *tex,Posize tex_PS,const Posize &targetPS)
			{
				if (targetPS.Size()<=0||tex_PS.Size()<=0||tex==NULL) return;
				if (tex_PS==ZERO_POSIZE) tex_PS=GetTexturePosize(tex);
				SDLRenderCopyWithScale(tex,tex_PS,targetPS);
			}
			
			inline void RenderCopy(SDL_Texture *tex,Posize tex_PS,const Point &pt)
			{
				if (tex_PS.Size()<=0||tex==NULL) return;
				if (tex_PS==ZERO_POSIZE) tex_PS=GetTexturePosize(tex);
				SDLRenderCopyWithScale(tex,tex_PS,Posize(pt.x,pt.y,tex_PS.w,tex_PS.h));
			}
			
			inline void RenderCopyWithLmt(SDL_Texture *tex,Posize tex_PS,const Posize &targetPS,const Posize &lmt)
			{
				if (lmt.Size()<=0||targetPS.Size()<=0||tex_PS.Size()<=0||tex==NULL) return;
				if (tex_PS==ZERO_POSIZE) tex_PS=GetTexturePosize(tex);
				RenderCopy(tex,(((lmt&targetPS)-targetPS).Flexible(tex_PS.w*1.0/targetPS.w,tex_PS.h*1.0/targetPS.h)+tex_PS)&tex_PS,lmt&targetPS);
			}
			
			inline void RenderCopyWithLmt(SDL_Texture *tex,const Posize &targetPS,const Posize &lmt)
			{
				if (lmt.Size()<=0||targetPS.Size()<=0||tex==NULL) return;
				Posize texPs=GetTexturePosize(tex);
				RenderCopy(tex,((lmt&targetPS)-targetPS).Flexible(texPs.w*1.0/targetPS.w,texPs.h*1.0/targetPS.h)&texPs,lmt&targetPS);
			}
			
			inline void RenderCopyWithLmt(SDL_Texture *tex,Posize tex_PS,const Point &pt,const Posize &lmt)
			{
				if (lmt.Size()<=0||tex_PS.Size()<=0||tex==NULL) return;
				Posize texPs=GetTexturePosize(tex);
				if (tex_PS==ZERO_POSIZE) tex_PS=texPs;
				Posize targetPS={pt.x,pt.y,tex_PS.w,tex_PS.h};
				RenderCopy(tex,((lmt&targetPS)-targetPS)&tex_PS,lmt&targetPS);
			}
			
			inline void RenderCopyWithLmt(SDL_Texture *tex,const Point &pt,const Posize &lmt)
			{
				if (lmt.Size()<=0||tex==NULL) return;
				Posize texPs=GetTexturePosize(tex);
				Posize targetPS={pt.x,pt.y,texPs.w,texPs.h};
				RenderCopy(tex,((lmt&targetPS)-targetPS)&texPs,lmt&targetPS);
			}
			
			inline void RenderCopyWithLmt_Centre(SDL_Texture *tex,Posize targetPS,const Posize &lmt)
			{
				if (lmt.Size()<=0||tex==NULL) return;
				Posize texPs=GetTexturePosize(tex);
				targetPS.x+=targetPS.w-texPs.w>>1;
				targetPS.y+=targetPS.h-texPs.h>>1;
				targetPS.w=texPs.w;
				targetPS.h=texPs.h;
				RenderCopy(tex,((lmt&targetPS)-targetPS)&texPs,lmt&targetPS);
			}
			
			inline void RenderCopyWithLmt_Centre(SDL_Texture *tex,const Point &pt,const Posize &lmt)
			{
				if (lmt.Size()<=0||tex==NULL) return;
				Posize texPs=GetTexturePosize(tex);
				Posize targetPS={pt.x-texPs.w/2,pt.y-texPs.h/2,texPs.w,texPs.h};
				RenderCopy(tex,((lmt&targetPS)-targetPS)&texPs,lmt&targetPS);
			}
			
			inline SDL_Texture *CreateTextureFromSurface(SDL_Surface *sur)
			{
				if (sur==NULL) return NULL;
				return SDL_CreateTextureFromSurface(ren,sur);
			}
			
			inline SDL_Texture *CreateTextureFromSurfaceAndDelete(SDL_Surface *sur)
			{
				if (sur==NULL) return NULL;
				SDL_Texture *tex=SDL_CreateTextureFromSurface(ren,sur);
				SDL_FreeSurface(sur);
				return tex;
			}
			
			inline SDL_Texture *CreateRGBATextTexture(const char *text,const RGBA &co,const PUI_Font &font=PUI_Font())
			{return CreateTextureFromSurfaceAndDelete(CreateRGBATextSurface(text,co,font));}
			
			inline void SetEnableTextTextureMemorize(bool on)
			{
				EnableTextTextureMemorize=on;
				if (!on)
					TextTextureMemorizeData.Clear();
			}
			
			inline void SetTextMemorizeMemoryLimit(unsigned long long limit=0)
			{
				if (limit==0) limit=PUI_Parameter_TextMemorizeMemoryLimit;
				TextTextureMemorizeData.SetSizeLimit(limit);
			}
			
			void RenderDrawText(const string &str,const Posize &tarPS,const Posize &lmt,const int mode=0,const RGBA &co=RGBA_NONE,const PUI_Font &font=PUI_Font())
			{
				if (lmt.Size()<=0||tarPS.Size()<=0) return;
				RGBA realCo=!co?ThemeColor.MainTextColor[0]:co;
				SharedTexturePtr tex;
				Posize texPs,DBGborderPS;
				if (EnableTextTextureMemorize)
				{
					TextTextureIndexInfo info(str,realCo,font);
					SharedTexturePtr *stp=TextTextureMemorizeData.Get(info);
					if (stp==NULL)
					{
//						DD[3]<<"Cannot find "<<str<<"."<<" TextLRU current size "<<TextTextureMemorizeData.Size()<<endl;
						tex=SharedTexturePtr(CreateRGBATextTexture(str.c_str(),realCo,font));
						texPs=GetTexturePosize(tex());
						info.SetWithTextureSize(texPs.w*texPs.h*4);//??May not be accurate...
						TextTextureMemorizeData.Insert(info,tex,info.memorySize);
					}
					else tex=*stp,texPs=GetTexturePosize(tex());
				}
				else tex=SharedTexturePtr(CreateRGBATextTexture(str.c_str(),realCo,font)),texPs=GetTexturePosize(tex());
				
				Posize src,dst;
				switch (mode)//0:mid -1:Left 1:Right
				{
					default:
						PUI_DD[1]<<"RenderDrawText wrong mode :"<<mode<<" and the default mode 0 will be used."<<endl;
					case 0:
						src=((lmt&tarPS)-tarPS-Point(tarPS.w-texPs.w>>1,tarPS.h-texPs.h>>1))&texPs;
						dst=lmt&tarPS&(DBGborderPS=Posize((tarPS.w-texPs.w>>1)+tarPS.x,(tarPS.h-texPs.h>>1)+tarPS.y,texPs.w,texPs.h));
						break;
					case 1:
						src=((lmt&tarPS)-tarPS-Point(tarPS.w-texPs.w,tarPS.h-texPs.h>>1))&texPs;
						dst=lmt&tarPS&(DBGborderPS=Posize(tarPS.w-texPs.w+tarPS.x,(tarPS.h-texPs.h>>1)+tarPS.y,texPs.w,texPs.h));
						break;
					case -1:
						src=((lmt&tarPS)-tarPS)&texPs;
						dst=lmt&tarPS&(DBGborderPS=Posize(tarPS.x,(tarPS.h-texPs.h>>1)+tarPS.y,texPs.w,texPs.h));
						break;
				}
				RenderCopy(tex(),src,dst);
		
				if (DEBUG_DisplayBorderFlag)
					Debug_DisplayBorder(DBGborderPS,RGBA(255,178,178,255));
			}
			
			void Debug_DisplayBorder(const Posize &ps,const RGBA &co={255,0,0,200})
			{
				if (DEBUG_DisplayBorderFlag)
				{
					SetRenderColor(co);
					RenderDrawLine({ps.x+5,ps.y},{ps.x2()-5,ps.y});
					RenderDrawLine({ps.x+5,ps.y2()},{ps.x2()-5,ps.y2()});
					RenderDrawLine({ps.x,ps.y+5},{ps.x,ps.y2()-5});
					RenderDrawLine({ps.x2(),ps.y+5},{ps.x2(),ps.y2()-5});
					RenderDrawLine({ps.x+5,ps.y},{ps.x,ps.y+5});
					RenderDrawLine({ps.x2()-5,ps.y},{ps.x2(),ps.y+5});
					RenderDrawLine({ps.x+5,ps.y2()},{ps.x,ps.y2()-5});
					RenderDrawLine({ps.x2()-5,ps.y2()},{ps.x2(),ps.y2()-5});
					
					SetRenderColor(RGBA(0,0,255,200));
					RenderDrawLine({ps.x,ps.y},{ps.x+4,ps.y});
					RenderDrawLine({ps.x,ps.y},{ps.x,ps.y+4});
					RenderDrawLine({ps.x2(),ps.y},{ps.x2()-4,ps.y});
					RenderDrawLine({ps.x2(),ps.y},{ps.x2(),ps.y+4});
					RenderDrawLine({ps.x,ps.y2()},{ps.x+4,ps.y2()});
					RenderDrawLine({ps.x,ps.y2()},{ps.x,ps.y2()-4});
					RenderDrawLine({ps.x2(),ps.y2()},{ps.x2()-4,ps.y2()});
					RenderDrawLine({ps.x2(),ps.y2()},{ps.x2(),ps.y2()-4});
				}
			}
			
			inline void DEBUG_DisplayPresentLimit()
			{
				SetRenderColor({0,255,0,127});
				for (int i=0;i<=2;++i)
					RenderDrawRectWithLimit(PresentLimit.Shrink(i),{0,255,0,127},LARGE_POSIZE);
			}
			
		#if PAL_SDL_IncludeSyswmHeader == 1
			#if PAL_CurrentPlatform == PAL_Platform_Windows
			HWND GetWindowsHWND()
			{
				SDL_SysWMinfo info;
				SDL_VERSION(&info.version);
				if (SDL_GetWindowWMInfo(win,&info)==SDL_TRUE&&info.subsystem==SDL_SYSWM_WINDOWS)
					return info.info.win.window;
				else return 0;
			}
			
			HDC GetWindowsHDC()
			{
				SDL_SysWMinfo info;
				SDL_VERSION(&info.version);
				if (SDL_GetWindowWMInfo(win,&info)==SDL_TRUE&&info.subsystem==SDL_SYSWM_WINDOWS)
					return info.info.win.hdc;
				else return 0;
			}
			
			HINSTANCE GetWindowsHINSTANCE()
			{
				SDL_SysWMinfo info;
				SDL_VERSION(&info.version);
				if (SDL_GetWindowWMInfo(win,&info)==SDL_TRUE&&info.subsystem==SDL_SYSWM_WINDOWS)
					return info.info.win.hinstance;
				else return 0;
			}
			
			Triplet <HWND,HDC,HINSTANCE> GetWindowsSysWM()
			{
				SDL_SysWMinfo info;
				SDL_VERSION(&info.version);
				if (SDL_GetWindowWMInfo(win,&info)==SDL_TRUE&&info.subsystem==SDL_SYSWM_WINDOWS)
					return {info.info.win.window,info.info.win.hdc,info.info.win.hinstance};
				else return {0,0,0};
			}
			
			static PUI_Window* GetPUIWindowFromWinHWND(HWND hwnd)//Slow??
			{
				for (auto sp:AllWindow)
					if (sp->GetWindowsHWND()==hwnd)
						return sp;
				return NULL;
			}
			
			void SetForeground()
			{
				HWND hWnd=GetWindowsHWND();
				HWND hForeWnd=::GetForegroundWindow();
				DWORD dwForeID=::GetWindowThreadProcessId(hForeWnd,NULL);
				DWORD dwCurID=::GetCurrentThreadId();
				::AttachThreadInput(dwCurID,dwForeID,TRUE);
				::ShowWindow(hWnd,SW_SHOWNORMAL);
				::SetWindowPos(hWnd,HWND_TOPMOST,0,0,0,0,SWP_NOSIZE|SWP_NOMOVE);
				::SetWindowPos(hWnd,HWND_NOTOPMOST,0,0,0,0, SWP_NOSIZE|SWP_NOMOVE);
				::SetForegroundWindow(hWnd);
				::AttachThreadInput(dwCurID,dwForeID,FALSE);
			}
            #endif
            //...
		#endif
		
		protected:
			#if PAL_GUI_EnableTransparentBackground == 1
			SDL_Surface *TransparentBackgroundTimerLastSurface=NULL;
			
			static unsigned int TimerFunc_UpdateTransparentBackground(unsigned int itv,void *userdata)//??
			{
				PUI_Window *win=(PUI_Window*)userdata;
				static ScreenCapture sc;
				sc.Init();
				sc.SetExcludeWindow(win->GetWindowsHWND());
				if (sc.Capture(win->WinPS)==0)
				{
					SDL_Surface *sur=sc.DeliverCaptureResult();
					if (sur!=NULL)
					{
						PUI_WindowTBEvent *event=new PUI_WindowTBEvent;
						event->type=PUI_Event::Event_WindowTBEvent;
						event->timeStamp=SDL_GetTicks();
						event->win=win;
						event->sur=sur;
						event->updatedPs=win->WinPS.ToOrigin();
						PUI_SendEvent(event);
					}
				}
				return itv;
			}
			#endif
			
			void SetBackgroundTransparentTimerOnOff(bool on)
			{
				if (on==(Timer_UpdateTransparentBackground!=0)) return;
				#if PAL_GUI_EnableTransparentBackground == 1
				if (on)
				{
					Timer_UpdateTransparentBackground=SDL_AddTimer(TransparentBackgroundUpdateInterval,TimerFunc_UpdateTransparentBackground,this);
				}
				else
				{
					SDL_RemoveTimer(Timer_UpdateTransparentBackground),Timer_UpdateTransparentBackground=0;//Is it blocked untill timer func run OK?
					TransparentBackgroundPic=SharedTexturePtr();
					if (TransparentBackgroundTimerLastSurface!=NULL)
						SDL_FreeSurface(TransparentBackgroundTimerLastSurface);
					TransparentBackgroundTimerLastSurface=NULL;
				}
				#endif
			}
			
		public:
			int SetWindowBackgroundTransparent(bool on)
			{
				if (PAL_GUI_EnableTransparentBackground!=1)
					return 1;
				else if (on==IsTransparentBackground) return 0;
				else if (on)
				{
					if (KeyboardFocused)
						SetBackgroundTransparentTimerOnOff(1);
				}
				else SetBackgroundTransparentTimerOnOff(0);
				IsTransparentBackground=on;
				return 0;
			}
			
			inline void SetTransparentBackgroundUpdateInterval(unsigned interval=100)//??
			{
				TransparentBackgroundUpdateInterval=EnsureInRange(interval,7,5000);
				if (Timer_UpdateTransparentBackground!=0)
				{
					SetBackgroundTransparentTimerOnOff(0);
					SetBackgroundTransparentTimerOnOff(1);
				}
			}
			
			PUI_Window(const Posize &winps,const string &title,unsigned int flag);
			
			PUI_Window(SDL_Window *_win);
			
			~PUI_Window();
	};
	set <PUI_Window*> PUI_Window::AllWindow;
	map <unsigned int,PUI_Window*> PUI_Window::WinOfSDLWinID;
	int PUI_Window::WindowCnt=0;
	bool PUI_Window::NeedFreshScreenAll=1,
		 PUI_Window::CurrentRefreshReady=1;
	const unsigned int PUI_Window::PUI_WINPS_CENTER=SDL_WINDOWPOS_CENTERED,
			  		   PUI_Window::PUI_WINPS_UNDEFINE=SDL_WINDOWPOS_UNDEFINED;
	Widgets *PUI_Window::OccupyPosWg=NULL,
			*PUI_Window::KeyboardInputWg=NULL;
	
	PUI_Window *MainWindow=NULL,
			   *CurrentWindow=NULL;
			   
	#define PUI_WINPS_DEFAULT Posize(PAL_GUI::PUI_Window::PUI_WINPS_CENTER,PAL_GUI::PUI_Window::PUI_WINPS_CENTER,1280,720)
	#define PUI_WINPS_DEFAULT_S Posize(PAL_GUI::PUI_Window::PUI_WINPS_CENTER,PAL_GUI::PUI_Window::PUI_WINPS_CENTER,640,480)
	#define PUI_WINPS_DEFAULT_M Posize(PAL_GUI::PUI_Window::PUI_WINPS_CENTER,PAL_GUI::PUI_Window::PUI_WINPS_CENTER,1280,720)
	#define PUI_WINPS_DEFAULT_L Posize(PAL_GUI::PUI_Window::PUI_WINPS_CENTER,PAL_GUI::PUI_Window::PUI_WINPS_CENTER,1600,900)
	#define PUI_WINPS_CENTER(w,h) Posize(PAL_GUI::PUI_Window::PUI_WINPS_CENTER,PAL_GUI::PUI_Window::PUI_WINPS_CENTER,w,h)
	#define PUI_WINPS_UNDEFINE(w,h) Posize(PAL_GUI::PUI_Window::PUI_WINPS_UNDEFINE,PAL_GUI::PUI_Window::PUI_WINPS_UNDEFINE,w,h)
//	#define PUI_WINPS_FULLSCREEN Posize()
	#define PUI_FA_MAINWINDOW PAL_GUI::MainWindow->BackGroundLayer()
	
	inline SDL_Texture *CreateTextureFromSurface(SDL_Surface *sur)
	{return CurrentWindow->CreateTextureFromSurface(sur);}
	
	inline SDL_Texture *CreateTextureFromSurfaceAndDelete(SDL_Surface *sur)
	{return CurrentWindow->CreateTextureFromSurfaceAndDelete(sur);}
	
	inline Point PUI_GetGlobalMousePoint()
	{
		Point re;
		SDL_GetGlobalMouseState(&re.x,&re.y);
		return re;
	}
	
	inline unsigned int PUI_GetGlobalMouseState()
	{return SDL_GetGlobalMouseState(NULL,NULL);}
	
	inline unsigned int PUI_GetGlobalMouseState(Point &pt)
	{return SDL_GetGlobalMouseState(&pt.x,&pt.y);}
	
	inline int PUI_SetGlobalMousePoint(const Point &pt)
	{return SDL_WarpMouseGlobal(pt.x,pt.y);}
	
	inline void PUI_SetShowCursor(bool enable)
	{SDL_ShowCursor(enable?SDL_ENABLE:SDL_DISABLE);}
	
	inline bool PUI_IsShowCursor()
	{return SDL_ShowCursor(SDL_QUERY)==SDL_ENABLE;}
	
	inline int PUI_SetRelativeMouseMode(bool enable)
	{return SDL_SetRelativeMouseMode(enable?SDL_TRUE:SDL_FALSE);}
	
	//Special functions:
	//...
	SDL_TimerID PUI_Timer_PerSecondID=0;
	Uint32 PUI_Event_TimerPerSecond=0;
	Uint32 PUI_Timer_PerSecond(Uint32 itv,void*)
	{return PUI_SendUserEvent(PUI_Event_TimerPerSecond),itv;}
	
	void PUI_SetTimerPerSecondOnOff(bool on)
	{
		if (on==(PUI_Timer_PerSecondID!=0))
			return;
		PUI_Timer_PerSecondID=on;
		if (on) PUI_Timer_PerSecondID=SDL_AddTimer(1000,PUI_Timer_PerSecond,NULL);
		else SDL_RemoveTimer(PUI_Timer_PerSecondID),PUI_Timer_PerSecondID=0;
	}
	
	Uint32 PUI_EVENT_UpdateTimer=0;
	struct PUI_UpdateTimerData
	{
		Widgets *tar=NULL;//the target to uptate
		unsigned int tarID=0;
		int cnt=0;//0:Stop >0:use same interval cnt times(at this time stack is empty) -1:use same interval infinite times  -2:use intervals in stack;
		stack <Uint32> sta;
		atomic_char *enableFlag=NULL;//0:stop(set by timerFunc) 1:running 2:stop(set by outside func) 3:stop and delete atomic(set by ouside func)
									 //when set 2,timer will stop,and it will be set 0 when stoped; It won't be deleted after Data decontructed
		void *data=NULL;
	
		PUI_UpdateTimerData(Widgets *_tar,int _cnt,atomic_char *_enableflag,void *_data);
		
		PUI_UpdateTimerData() {}
	};
	
	Uint32 PUI_UpdateTimer(Uint32 interval,void *param)//mainly used to support animation effect
	{
		PUI_UpdateTimerData *p=(PUI_UpdateTimerData*)param;
//		PUI_DD[3]<<"PUI_UpdateTimer:  cnt "<<(p->cnt)<<endl;
	    
	    if (p->enableFlag!=NULL)
	    	if (*(p->enableFlag)!=1)
	    	{
	    		if (*(p->enableFlag)==3)
	    			delete p->enableFlag;
				else *(p->enableFlag)=0;
	    		delete p;
	    		return 0;
			}
		
		PUI_SendUserEvent(PUI_EVENT_UpdateTimer,p->cnt==-2?p->sta.size():p->cnt,new int(p->tarID),p->data);
		//code:how many times to run it needs
	    
	    if (p->cnt==-2)//use interval in stack
		    if (p->sta.empty())
			{
		    	if (p->enableFlag!=NULL)
		    		if (*(p->enableFlag)==3)
	    				delete p->enableFlag;
					else *(p->enableFlag)=0;
				delete p;
		    	return 0;
			}
			else
			{
				Uint32 t=p->sta.top();
				p->sta.pop();
				if (t==0)
					t=1;
				return t;
			}
	    else if (p->cnt==-1)//infinite times
	    	return interval;
	    else 
			if (p->cnt==0)//run to end
		    {
		    	if (p->enableFlag!=NULL) 
		    		if (*(p->enableFlag)==3)
	    				delete p->enableFlag;
					else *(p->enableFlag)=0;
		    	delete p;
		    	return 0;
		    }
		    else//this time end
			{
				--p->cnt;
				return interval;
			}
	}
	
	void SetCurrentIMEWindowPos(const Point &cursorPt,const Posize &excludePS,PUI_Window *Win)
	{
	#if PAL_CurrentPlatform == PAL_Platform_Windows
		#if PAL_GUI_UseWindowsIME == 1
		SDL_SysWMinfo info;
		SDL_VERSION(&info.version);
		if (SDL_GetWindowWMInfo(Win->GetSDLWindow(),&info))
			if (info.subsystem==SDL_SYSWM_WINDOWS)
			{
				HIMC imc=ImmGetContext(info.info.win.window);
				
				COMPOSITIONFORM cf;
				cf.dwStyle=CFS_FORCE_POSITION;
				cf.ptCurrentPos.x=cursorPt.x;
				cf.ptCurrentPos.y=cursorPt.y+16;
				ImmSetCompositionWindow(imc,&cf);
				
				CANDIDATEFORM candf;
				candf.dwIndex=0;
				candf.dwStyle=CFS_CANDIDATEPOS;
				candf.ptCurrentPos.x=cursorPt.x;
				candf.ptCurrentPos.y=cursorPt.y+14;
				candf.rcArea.bottom=excludePS.y2();
				candf.rcArea.left=excludePS.x;
				candf.rcArea.right=excludePS.x2();
				candf.rcArea.top=excludePS.y;
				ImmSetCandidateWindow(imc,&candf);
				
				ImmReleaseContext(info.info.win.window,imc);
			}
		#endif
	#endif
	}
	
	void TurnOnOffIMEWindow(bool on,PUI_Window *Win,bool forced=0)//There still exsits bugs related to this functions
	{
		static bool lastState=1;
		if (on==lastState&&!forced) return;
		lastState=on;
	#if PAL_CurrentPlatform == PAL_Platform_Windows
		#if PAL_GUI_UseWindowsIME == 1
		SDL_SysWMinfo info;
		SDL_VERSION(&info.version);
		if (SDL_GetWindowWMInfo(Win->GetSDLWindow(),&info))
			if (info.subsystem==SDL_SYSWM_WINDOWS)
			{
				HIMC imc=ImmGetContext(info.info.win.window);
				PUI_DD[3]<<"ImmSetOpenStatus "<<ImmSetOpenStatus(imc,on)<<endl;
				PUI_DD[3]<<"ImmReleaseContext "<<ImmReleaseContext(info.info.win.window,imc)<<endl;
			}
		#endif
	#endif
	}
	
	//Widgets:
	//...
	
	class PosizeEX
	{
		friend class Widgets;
		protected: 
			Widgets *wg=nullptr;
			PosizeEX *nxt=nullptr;
//			int UsedCnt=0;
			
			Posize& GetWidgetsRawrPS();
			
			Posize& GetWidgetsRawgPS();
			
			PosizeEX()//Ban construct PosizeEX
			{
				
			}
			
		public:
			virtual void GetrPS(Posize &ps)=0;
			
			virtual ~PosizeEX()
			{
				if (nxt!=nullptr)
					delete nxt;
			}
	};
	
	class Widgets
	{
		friend bool PUI_PresentWidgets();
		friend void PUI_PresentWidgets(PUI_Window *win,Posize lmt);
		friend void PUI_PresentWidgets(Widgets *tar);
		friend void PUI_UpdateWidgetsPosize(Widgets *wg);
		friend void PUI_UpdateWidgetsPosize(PUI_Window *win);
		friend int PUI_SolveEvent(PUI_Event *event);
		friend class PUI_Window;
		friend class PosizeEX;
		
		public:
			enum WidgetType
			{
				WidgetType_Widgets=0,
				WidgetType_Layer,
				WidgetType_Button,
				WidgetType_LargeLayerWithScrollBar,
				WidgetType_TinyText,
				WidgetType_CheckBox,
				WidgetType_SingleChoiceButton,
				WidgetType_Slider,
				WidgetType_TwinLayerWithDivideLine,
				WidgetType_ShapedPictureButton,
				WidgetType_PictureBox,
				WidgetType_SimpleListView,
				WidgetType_SimpleListView_MultiColor,
				WidgetType_ProgressBar,
				WidgetType_Menu1,
				WidgetType_SwitchButton,
				WidgetType_TextEditLine,
				WidgetType_SimpleBlockView,
				WidgetType_TextEditBox,
				WidgetType_TabLayer,
				WidgetType_AddressSection,
				WidgetType_FileAddressSection,
				WidgetType_TinyText2,
				WidgetType_SimpleTreeView,
				WidgetType_DetailedListView,
				WidgetType_MessageBoxLayer,
				WidgetType_MessageBoxButton,
				WidgetType_ListViewTemplate,
				WidgetType_LayerForListViewTemplate,
				WidgetType_DropDownButton,
				WidgetType_EventLayer,
				WidgetType_PhotoViewer,
				WidgetType_BlockViewTemplate,
				WidgetType_LayerForBlockViewTemplate,
				WidgetType_BorderRectLayer,
				WidgetType_FullFillSlider,
				WidgetType_DragableLayer,
				WidgetType_SimpleFileSelectBox,
				WidgetType_BaseButton,//However, it will never appear in WidgetType
				WidgetType_GestureSenseLayer,
				WidgetType_WindowBackgroundLayer,
				WidgetType_SimpleDateTimeBox,
				WidgetType_PosEventLayer,
				WidgetType_ShowLayer,
				WidgetType_SimpleTextBox,
				WidgetType_DrawerLayer,
				WidgetType_Button2,
				WidgetType_AxisButton,
				WidgetType_TwoStateButton,
				WidgetType_ToastText, 
				WidgetType_KeyboardButton,
				
				WidgetType_UserAssignStartID
			};
			static map <WidgetType,string> UserDefinedWidgetName;
			
			static const char* WidgetTypeToName(WidgetType type)
			{
				switch (type)
				{
					case WidgetType_Widgets:					return "Widgets";
					case WidgetType_Layer:						return "Layer";
					case WidgetType_Button:						return "Button";
					case WidgetType_LargeLayerWithScrollBar:	return "LargeLayerWithScrollBar";
					case WidgetType_TinyText:					return "TinyText";
					case WidgetType_CheckBox:					return "CheckBox";
					case WidgetType_SingleChoiceButton:			return "SingleChoiceButton";
					case WidgetType_Slider:						return "Slider";
					case WidgetType_TwinLayerWithDivideLine:	return "TwinLayerWithDivideLine";
					case WidgetType_ShapedPictureButton:		return "ShapedPictureButton";
					case WidgetType_PictureBox:					return "PictureBox";
					case WidgetType_SimpleListView:				return "SimpleListView";
					case WidgetType_SimpleListView_MultiColor:	return "SimpleListView_MultiColor";
					case WidgetType_ProgressBar:				return "ProgressBar";
					case WidgetType_Menu1:						return "Menu1";
					case WidgetType_SwitchButton:				return "SwitchButton";
					case WidgetType_TextEditLine:				return "TextEditLine";
					case WidgetType_SimpleBlockView:			return "SimpleBlockView";
					case WidgetType_TextEditBox:				return "TextEditBox";
					case WidgetType_TabLayer:					return "TabLayer";
					case WidgetType_AddressSection:				return "AddressSection";
					case WidgetType_FileAddressSection:			return "FileAddressSection";
					case WidgetType_TinyText2:					return "TinyText2";
					case WidgetType_SimpleTreeView:				return "SimpleTreeView";
					case WidgetType_DetailedListView:			return "DetailedListView";
					case WidgetType_MessageBoxLayer:			return "MessageBoxLayer";
					case WidgetType_MessageBoxButton:			return "MessageBoxButton";
					case WidgetType_ListViewTemplate:			return "ListViewTemplate";
					case WidgetType_LayerForListViewTemplate:	return "LayerForListViewTemplate";
					case WidgetType_DropDownButton:				return "DropDownButton";
					case WidgetType_EventLayer:					return "EventLayer";
					case WidgetType_PhotoViewer:				return "PhotoViewer";
					case WidgetType_BlockViewTemplate:			return "BlockViewTemplate";
					case WidgetType_LayerForBlockViewTemplate:	return "LayerForBlockViewTemplate";
					case WidgetType_BorderRectLayer:			return "BorderRectLayer";
					case WidgetType_FullFillSlider:				return "FullFillSlider";
					case WidgetType_DragableLayer:				return "DragableLayer";
					case WidgetType_SimpleFileSelectBox:		return "SimpleFileSelectBox";
					case WidgetType_BaseButton:					return "BaseButton";
					case WidgetType_GestureSenseLayer:			return "GestureSenseLayer";
					case WidgetType_WindowBackgroundLayer:		return "WindowBackgroundLayer";
					case WidgetType_SimpleDateTimeBox:			return "SimpleDateTimeBox";
					case WidgetType_PosEventLayer:				return "PosEventLayer";
					case WidgetType_ShowLayer:					return "ShowLayer";
					case WidgetType_SimpleTextBox:				return "SimpleTextBox";
					case WidgetType_DrawerLayer:				return "DrawerLayer";
					case WidgetType_Button2:					return "Button2";
					case WidgetType_AxisButton:					return "AxisButton";
					case WidgetType_TwoStateButton: 			return "TwoStateButton";
					case WidgetType_ToastText:					return "ToastText";
					case WidgetType_KeyboardButton:				return "KeyboardButton";
					
					default:
					{
						auto mp=UserDefinedWidgetName.find(type);
						if (mp!=UserDefinedWidgetName.end())
							return mp->second.c_str();//??
						else return "UserDefinedWidget";
					}
				}
			}
			
			static WidgetType GetNewWidgetTypeID(const string &widgettypename="")
			{
				static int re=WidgetType_UserAssignStartID;
				++re;
				if (widgettypename!="")
					UserDefinedWidgetName[(WidgetType)re]=widgettypename;
				return (WidgetType)re;
			}
			
			struct WidgetID
			{
				int ID=0;
				string name;
				
				WidgetID(const string &namae):name(namae) {}
				
				WidgetID(const char *namae):name(namae) {}
				
				WidgetID(int _ID=0):ID(_ID) {}
			};
			
			friend Debug_Out& operator << (Debug_Out &dd,const WidgetID &idname)
			{
				if (idname.name=="")
					return dd<<idname.ID;
				else return dd<<idname.ID<<":"<<idname.name;
			}
			
			enum//Properties:
			{
				PosMode_None=0,
				PosMode_Default=1,//Default order in SolvePosEvent
				PosMode_Pre=2,//?? Pre solve before solve child
				PosMode_Nxt=4,//Solve after solving child
				PosMode_Occupy=8,//Current pos is occupied
				PosMode_Direct=16,//Direct call CheckPos rather than call from SolvePosEvent
				PosMode_LoseFocus=32,//Current is processing loseFocus
				PosMode_ForceLoseFocus=64//Force loseFocus
			};
			
		protected:
			static map <int,Widgets*> WidgetsID;
			static map <string,Widgets*> WidgetsName;
			static queue <Widgets*> WidgetsToDeleteAfterEvent;
			
//			int ID=0;
			WidgetPtr ThisWg;
			WidgetID IDName;
			WidgetType Type=WidgetType_Widgets;
			int Depth=0;
			bool MultiWidgets=0;
//			Widgets *MultiWidgetsRoot=nullptr;//if NULL,means it is not a MultiWidgets
			Posize rPS=ZERO_POSIZE,gPS=ZERO_POSIZE,//Relative Posize,Global Posize
				   CoverLmt=ZERO_POSIZE;//Last limit posize,used for partial update
			PUI_Window *Win=nullptr;
			PosizeEX *PsEx=nullptr;
			bool Enabled=1,
				 Selected=0;
			Widgets *fa=NULL,
					*preBrother=nullptr,
					*nxtBrother=nullptr,
					*childWg=nullptr;
			bool NeedLoseFocus=0;
			
			void SetDelayDeleteThis()
			{WidgetsToDeleteAfterEvent.push(this);}
			
			void SetNeedLoseFocus()
			{
				if (NeedLoseFocus)
					return;
				NeedLoseFocus=1;
//				PUI_Window::LoseFocusLinkTable *p=new PUI_Window::LoseFocusLinkTable(this);
//				p->nxt=Win->LoseFocusWgHead;
//				Win->LoseFocusWgHead=p;
//				++Win->LoseFocusState;
				Win->LoseFocusTargets.insert(this);
			}
			
			void RemoveNeedLoseFocus()
			{
				if (!NeedLoseFocus)
					return;
				NeedLoseFocus=0;
				Win->LoseFocusTargets.erase(this);
//				PUI_Window::LoseFocusLinkTable *p=Win->LoseFocusWgHead;
//				if (p==nullptr)
//					PUI_DD[2]<<IDName<<" need lose focus, however LoseFocusWgHead is nullptr!"<<endl;
//				else if (Win->LoseFocusWgHead->wg==this)
//				{
//					Win->LoseFocusWgHead=Win->LoseFocusWgHead->nxt;
//					delete p;
//					--Win->LoseFocusState;
//					return;
//				}
//				else
//				{
//					for (PUI_Window::LoseFocusLinkTable *q=Win->LoseFocusWgHead->nxt;q;p=q,q=q->nxt)
//						if (q->wg==this)
//						{
//							p->nxt=q->nxt;
//							delete q;
//							q=p;
//							--Win->LoseFocusState;
//							return;
//						}
//					PUI_DD[2]<<"Failed to find this LoseFocusWidget "<<IDName<<endl;
//				}
			}
			
			int AutoGetID()
			{
				static int re=10000;
				++re;
				if (re>=2000000000)
					re=10000;
				while (WidgetsID.find(re)!=WidgetsID.end())
					++re;
				return re;
			}

			void SplitFromFa()//Split this Node from linktable
			{
				if (fa==NULL) return;
				if (fa->childWg==this)
					fa->childWg=nxtBrother;
				else preBrother->nxtBrother=nxtBrother;
				if (nxtBrother!=NULL)
					nxtBrother->preBrother=preBrother;
				fa=preBrother=nxtBrother=NULL;
			}
			
			void UpdateFa()
			{
				Win=fa->Win;
				Depth=fa->Depth+1;
				if (nxtBrother!=NULL)
					nxtBrother->UpdateFa();
				if (childWg!=NULL)
					childWg->UpdateFa();
			}
			
			const Posize& GetFaCoverLmt()
			{
				if (fa==NULL)
					return gPS;
				else return fa->CoverLmt;
			}
			
			virtual void CalcPsEx()
			{
				if (PsEx!=NULL)
					PsEx->GetrPS(rPS);
				Posize lastPs=CoverLmt;
				bool flag=gPS==ZERO_POSIZE;//??
				if (fa!=NULL)
					gPS=rPS+fa->gPS;
				else gPS=rPS;
				CoverLmt=gPS&GetFaCoverLmt();
				if (!(lastPs==CoverLmt)||flag)
					Win->SetPresentArea(lastPs|CoverLmt);
			}
			
			virtual void ReceiveKeyboardInput(const PUI_TextEvent *event) {}
			
			virtual void CheckEvent(const PUI_Event *event) {}
			
			virtual void CheckPos(const PUI_PosEvent *event,int mode) {}
			
			virtual void Show(Posize &lmt) {}
			
			virtual void _SolveEvent(const PUI_Event *event)
			{
				if (!Win->NeedSolveEvent) 
					return;
				if (nxtBrother!=NULL)
					nxtBrother->_SolveEvent(event);
				if (!Enabled) return;
				if (childWg!=NULL)
					childWg->_SolveEvent(event);
				if (Win->NeedSolveEvent)
					CheckEvent(event);
			}
			
			virtual void _SolvePosEvent(const PUI_PosEvent *event,int mode)
			{
				if (!Win->NeedSolvePosEvent)
					return;
				if (Enabled)
					if (gPS.In(event->pos))
					{
						if (Win->NeedSolvePosEvent)
							CheckPos(event,mode|PosMode_Pre);
						if (childWg!=NULL)
							childWg->_SolvePosEvent(event,mode);
						if (Win->NeedSolvePosEvent)
							CheckPos(event,mode|PosMode_Nxt|PosMode_Default);
					}
				if (nxtBrother!=NULL)
					nxtBrother->_SolvePosEvent(event,mode);
			}
			
			virtual void _PresentWidgets(Posize lmt)
			{
				if (nxtBrother!=NULL)
					nxtBrother->_PresentWidgets(lmt);
				if (Enabled)
				{
					lmt=lmt&gPS;
					Show(lmt);
					if (DEBUG_EnableWidgetsShowInTurn)
						SDL_Delay(DEBUG_WidgetsShowInTurnDelayTime),
						SDL_RenderPresent(Win->ren);
					if (childWg!=NULL)
						childWg->_PresentWidgets(lmt);
				}
			}
			
			virtual void _UpdateWidgetsPosize()
			{
				if (nxtBrother!=NULL)
					nxtBrother->_UpdateWidgetsPosize();
				if (!Enabled) return;
				CalcPsEx();
				if (childWg!=NULL)
					childWg->_UpdateWidgetsPosize();
			}
			
			static inline void SolveEventOf(Widgets *tar,const PUI_Event *event)//To break the limit of class permission between base and derived class. 
			{tar->_SolveEvent(event);}
			
			static inline void SolvePosEventOf(Widgets *tar,const PUI_PosEvent *event,int mode)//They are ugly,but useful...
			{tar->_SolvePosEvent(event,mode);}
			
			static inline void PresentWidgetsOf(Widgets *tar,const Posize &ps)
			{tar->_PresentWidgets(ps);}
			
			static inline void UpdateWidgetsPosizeOf(Widgets *tar)
			{tar->_UpdateWidgetsPosize();}
			
			static inline void TemporarySetEnabled(Widgets *tar,bool onoff)//For inner special use...
			{tar->Enabled=onoff;}
			
			Widgets()
			{
				ThisWg.SetTarget(this);
			}
			
			Widgets(const WidgetID &_ID,WidgetType _type):Widgets()
			{
				SetID(_ID);
				Type=_type;
				PUI_DD[0]<<"Create "<<WidgetTypeToName(Type)<<" "<<IDName<<endl;
			}
			
			Widgets(const WidgetID &_ID,WidgetType _type,Widgets *_fa,const Posize &_rPS):Widgets(_ID,_type)
			{
				SetFa(_fa);
				SetrPS(_rPS);
			}
			
			Widgets(const WidgetID &_ID,WidgetType _type,Widgets *_fa,PosizeEX *psex):Widgets(_ID,_type)
			{
				SetFa(_fa);
				AddPsEx(psex);
			}
			
		public:
			inline static Widgets* GetWidgetsByID(int _ID)
			{
				auto mp=WidgetsID.find(_ID);
				if (mp==WidgetsID.end())
					return nullptr;
				else return mp->second;
			}
			
			inline static Widgets* GetWidgetsByName(const string &name)
			{
				auto mp=WidgetsName.find(name);
				if (mp==WidgetsName.end())
					return nullptr;
				else return mp->second;
			}
			
			inline static const map <int,Widgets*>& GetWidgetsIDmap()
			{return WidgetsID;}
			
			inline static const map <string,Widgets*>& GetWidgetsNameMap()
			{return WidgetsName;}
			
			inline static void SetDelayDeleteWidget(Widgets *tar)
			{
				if (tar!=nullptr)
					WidgetsToDeleteAfterEvent.push(tar);
			}
			
			inline void ForceUpdateWidgetTreePosize()//??
			{
				CalcPsEx();
				if (childWg!=NULL)
					childWg->_UpdateWidgetsPosize();
			}
			
			inline void DelayDelete()
			{WidgetsToDeleteAfterEvent.push(this);}
			
			void ClearAllChilds()//Be careful??
			{
				for (Widgets *p=childWg;p;p=p->nxtBrother)
					p->DelayDelete();
			}
			
			inline int GetID() const
			{return IDName.ID;}
			
			inline string GetName() const
			{return IDName.name;}
			
			inline WidgetID GetIDName() const
			{return IDName;}
		
			void SetID(WidgetID _ID)//if _ID==0 auto set ID
			{
				if (_ID.ID==0)
					_ID.ID=AutoGetID();
				if (_ID.ID!=IDName.ID)
				{
					auto mpID=WidgetsID.find(_ID.ID);
					if (mpID!=WidgetsID.end())
					{
						_ID.ID=AutoGetID();
						PUI_DD[2]<<"ID "<<mpID->first<<" conflicted! Auto assign."<<endl;
					}
					WidgetsID.erase(IDName.ID);
				}
				if (_ID.name!=""&&_ID.name!=IDName.name)
				{
					auto mpName=WidgetsName.find(_ID.name);
					if (mpName!=WidgetsName.end())
					{
						_ID.name="";
						PUI_DD[2]<<"Name "<<mpName->first<<" conflict with ID:"<<mpName->second->GetID()<<". No name is set."<<endl;
					}
					WidgetsName.erase(IDName.name);
				}
				IDName=_ID;
				WidgetsID[IDName.ID]=this;
				WidgetsName[IDName.name]=this;
			}
				
			inline WidgetType GetType() const
			{return Type;}
			
//			Posize& GetrPS_Ref()
//			{return rPS;}
			
//			Posize& GetgPS_Ref()
//			{return gPS;}
			
			inline Posize GetrPS()
			{return rPS;}
			
			virtual void SetrPS(const Posize &ps)//virtual??
			{
				CoverLmt=rPS=ps;
				//...
				if (Win!=NULL)
					Win->SetNeedUpdatePosize();
			}
			
			inline Posize GetgPS()
			{return gPS;}
			
			const inline Posize& GetCoverLmt()
			{return CoverLmt;}
			
			inline void SetEnabled(bool bo)//virtual??
			{
				Enabled=bo;
				Win->NeedFreshScreen=1;
				Win->SetPresentArea(CoverLmt);
				Win->SetNeedUpdatePosize();
			}
			
			inline bool GetEnabled()
			{return Enabled;}
			
			inline PUI_Window* GetWin()
			{return Win;}

			inline Widgets* GetFa()
			{return fa;}
			
			inline int GetDepth()
			{return Depth;}
			
			void SetFa(Widgets *_fa)//Add in the linktable head 
			{
				SplitFromFa();
				if (_fa==NULL) return;
				fa=_fa;
				Win=fa->Win;
				Depth=fa->Depth+1;
				nxtBrother=fa->childWg;
				if (nxtBrother!=NULL)
					nxtBrother->preBrother=this;
				fa->childWg=this;
				if (childWg!=NULL)
					childWg->UpdateFa();
			}
			
			void AddPsEx(PosizeEX *psex)
			{
				if (PsEx!=NULL)
					psex->nxt=PsEx;
				PsEx=psex;
				PsEx->wg=this;
				CalcPsEx();//??
				Win->SetNeedUpdatePosize();
			}
			
			inline void RemoveAllPsEx()
			{DeleteToNULL(PsEx);}
			
			inline void ReAddPsEx(PosizeEX *psex)
			{
				RemoveAllPsEx();
				AddPsEx(psex);
			}
			
			inline WidgetPtr This()
			{return ThisWg;}
			
			virtual ~Widgets()
			{
				PUI_DD[0]<<"Delete "<<WidgetTypeToName(Type)<<" "<<IDName<<endl;
				delete PsEx;
				while (childWg!=nullptr)
					delete childWg;
				SplitFromFa();
				if (NeedLoseFocus)
					RemoveNeedLoseFocus();
				if (Win->OccupyPosWg==this)
					Win->OccupyPosWg=nullptr;
				if (Win->KeyboardInputWg==this)
					Win->KeyboardInputWg=nullptr;
				if (IDName.ID!=0)
					WidgetsID.erase(IDName.ID);
				if (IDName.name!="")
					WidgetsName.erase(IDName.name);
									
				if (Enabled)
				{
					Win->NeedFreshScreen=1;
					Win->SetPresentArea(CoverLmt);
					Win->SetNeedUpdatePosize();
				}
				
				ThisWg.SetTarget(nullptr);
				PUI_DD[0]<<"Delete "<<WidgetTypeToName(Type)<<" "<<IDName<<" OK"<<endl;//efficiency??
			}
	};
	
	map <Widgets::WidgetType,string> Widgets::UserDefinedWidgetName;
	map <int,Widgets*> Widgets::WidgetsID;
	map <string,Widgets*> Widgets::WidgetsName;
	queue <Widgets*> Widgets::WidgetsToDeleteAfterEvent;
	
	template <typename T> T*& DelayDeleteToNULL(T *&tar)
	{
		if (tar!=NULL)
		{
			tar->DelayDelete();
			tar=nullptr;
		}
		return tar;
	}
	
	Posize& PosizeEX::GetWidgetsRawrPS()
	{return wg->rPS;}
			
	Posize& PosizeEX::GetWidgetsRawgPS()
	{return wg->gPS;}
	
	PUI_UpdateTimerData::PUI_UpdateTimerData(Widgets *_tar,int _cnt=0,atomic_char *_enableflag=NULL,void *_data=NULL)
	:tar(_tar),cnt(_cnt),enableFlag(_enableflag),data(_data)
	{
		if (tar!=NULL)
			tarID=tar->GetID();
	}
	
	class PosizeEX_Fa6:public PosizeEX
	{
		protected:
			Uint8 xNotConsider=0,yNotConsider=0;//faDep=1;//faDep:relative to which father(Cannot use yet)
			double ra,rb,rc,rd;//Positive:pixels Negative:proportion
						
		public:
			virtual void GetrPS(Posize &ps)
			{
				if (nxt!=NULL) nxt->GetrPS(ps);//Linktable:Run GetrPS in the order it created 
				Posize psfa=wg->GetFa()->GetrPS();
					
				double a=ra<0?(-ra)*psfa.w:ra,
					   b=rb<0?(-rb)*psfa.w:rb,
					   c=rc<0?(-rc)*psfa.h:rc,
					   d=rd<0?(-rd)*psfa.h:rd;
				switch (xNotConsider)
				{
					case 1: ps.w=a;ps.SetX2_ChangeX(psfa.w-b-1);break;
					case 2: ps.x=a;ps.SetX2(psfa.w-b-1);break;
					case 3: ps.x=a;ps.w=b;break;
				}
				
				switch (yNotConsider)
				{
					case 1: ps.h=c;ps.SetY2_ChangeY(psfa.h-d-1);break;
					case 2: ps.y=c;ps.SetY2(psfa.h-d-1);break;
					case 3: ps.y=c;ps.h=d;break;
				}
			}
			
			PosizeEX_Fa6(Uint8 _xn,Uint8 _yn,double a,double b,double c,double d)
			:xNotConsider(_xn),yNotConsider(_yn),ra(a),rb(b),rc(c),rd(d)
			{
				if (!InRange(xNotConsider,0,3)) PUI_DD[2]<<"PosizeEX: xNotConsiderValue is set "<<xNotConsider<<" which cannot be resolved!"<<endl;
				if (!InRange(yNotConsider,0,3)) PUI_DD[2]<<"PosizeEX: yNotConsiderValue is set "<<yNotConsider<<" which cannot be resolved!"<<endl;
			}
	};
	#define PosizeEX_Fa6_Full PosizeEX_Fa6(2,2,0,0,0,0)
	
	class PosizeEX_MidFa:public PosizeEX
	{
		protected:
			Posize CenterShift;
		
		public:
			virtual void GetrPS(Posize &ps)
			{
				if (nxt!=NULL) nxt->GetrPS(ps);
				Posize psfa=wg->GetFa()->GetrPS();
				
				if (CenterShift.Size()!=0)
					ps.w=CenterShift.w,
					ps.h=CenterShift.h;
				ps.x=(psfa.w-ps.w>>1)+CenterShift.x;
				ps.y=(psfa.h-ps.h>>1)+CenterShift.y;
			}
			
			PosizeEX_MidFa(int w,int h)
			:CenterShift(0,0,w,h) {}
			
			PosizeEX_MidFa(const Point &centreshift=ZERO_POINT)
			:CenterShift(centreshift.x,centreshift.y,0,0) {}
			
			PosizeEX_MidFa(const Posize &centreshift)
			:CenterShift(centreshift) {}
	};
	
	class PosizeEX_MidFa_Single:public PosizeEX
	{
		protected:
			bool IsY;
			int delta;
					
		public:
			virtual void GetrPS(Posize &ps)
			{
				if (nxt!=NULL) nxt->GetrPS(ps);
				Posize psfa=wg->GetFa()->GetrPS();
				
				if (IsY) ps.y=(psfa.h-ps.h>>1)+delta;
				else ps.x=(psfa.w-ps.w>>1)+delta;
			}
			
			PosizeEX_MidFa_Single(bool isY,int _delta=0):IsY(isY),delta(_delta) {}
	};
	
	class PosizeEX_Nearby:public PosizeEX
	{
		protected:
			Widgets *tar=NULL;
			
			
		public:
			virtual void GetrPS(Posize &ps)
			{
				
			}

	};
	
	class PosizeEX_Moving:public PosizeEX
	{
		
	};
	
	class Layer:public Widgets
	{
		protected:
			RGBA LayerColor=RGBA_NONE;
		
			virtual void Show(Posize &lmt)
			{
				Win->RenderFillRect(lmt,ThemeColor(LayerColor));
				Win->Debug_DisplayBorder(gPS);
			}
			
			Layer(const WidgetID &_ID,WidgetType _type,Widgets *_fa,const Posize &_rPS)
			:Widgets(_ID,_type,_fa,_rPS) {}
			
			Layer(const WidgetID &_ID,WidgetType _type,Widgets *_fa,PosizeEX *psex)
			:Widgets(_ID,_type,_fa,psex) {}
			
		public:
			void SetLayerColor(const RGBA &co)
			{
				LayerColor=co;
				Win->SetPresentArea(CoverLmt);
			}
			
			inline RGBA GetLayerColor() const
			{return LayerColor;}
			
			Layer(const WidgetID &_ID,Widgets *_fa,const Posize &_rPS)
			:Layer(_ID,WidgetType_Layer,_fa,_rPS) {}
			
			Layer(const WidgetID &_ID,Widgets *_fa,PosizeEX *psex)
			:Layer(_ID,WidgetType_Layer,_fa,psex) {}
	};
	
	class WindowBackgroundLayer:public Widgets
	{
		protected:
			RGBA LayerColor=RGBA_NONE;
		
			virtual void Show(Posize &lmt)
			{
				if (Win->IsTransparentBackground&&Win->KeyboardFocused&&Win->TransparentBackgroundPic()!=NULL&&Win->TransparentBackgroundPicPS==Win->GetWinSizePhysical())//??
				{
					Posize ps=lmt&Win->GetWinSize();
					Win->SDLRenderCopyWithScale(Win->TransparentBackgroundPic(),ps,ps);
				}
				else Win->RenderFillRect(lmt,ThemeColor(LayerColor));
				Win->Debug_DisplayBorder(gPS);
			}
			
		public:
			inline void SetLayerColor(const RGBA &co)
			{
				LayerColor=co;
				Win->SetPresentArea(CoverLmt);
			}
			
			inline RGBA GetLayerColor() const
			{return LayerColor;}
			
			WindowBackgroundLayer(const WidgetID &_ID,Widgets *_fa,const Posize &_rPS)
			:Widgets(_ID,WidgetType_WindowBackgroundLayer,_fa,_rPS) {}
			
			WindowBackgroundLayer(const WidgetID &_ID,Widgets *_fa,PosizeEX *psex)
			:Widgets(_ID,WidgetType_WindowBackgroundLayer,_fa,psex) {}
	};
	
	void PUI_Window::SetBackgroundColor(const RGBA &co=RGBA_WHITE)
	{
		BackgroundColor=co;
		if (_BackGroundLayer!=NULL)
			if (NeedRefreshFullScreen)
				_BackGroundLayer->SetLayerColor(RGBA_NONE);
			else _BackGroundLayer->SetLayerColor(co);
		else PUI_DD[2]<<"BackGroundLayer of Window "<<WindowTitle<<" is not created yet!"<<endl;
	}
	
	PUI_Window::PUI_Window(const Posize &winps,const string &title="",unsigned int flag=PUI_FLAG_RESIZEABLE)
	:WindowTitle(title),TextTextureMemorizeData(PUI_Parameter_TextMemorizeMemoryLimit)
	{
		PUI_DD[0]<<"Create window "<<WindowTitle<<endl;
		++WindowCnt;
		AllWindow.insert(this);
		
		unsigned int flagWin=0,flagRen=0;
		if (flag&PUI_FLAG_RESIZEABLE) flagWin|=SDL_WINDOW_RESIZABLE;
		if (flag&PUI_FLAG_SOFTWARE) flagRen|=SDL_RENDERER_SOFTWARE;
		else flagRen|=SDL_RENDERER_ACCELERATED;
		if (flag&PUI_FLAG_KEEPTOP) flagWin|=SDL_WINDOW_ALWAYS_ON_TOP;
		if (flag&PUI_FLAG_BORDERLESS) flagWin|=SDL_WINDOW_BORDERLESS;
		
		vector <Doublet<unsigned,string> > renderVec=PUI_Window::GetAvaliableRenderers();
		int rendererIndex=-1;
		for (int i=0;i<renderVec.size();++i)
			if (renderVec[i].a==PUI_PreferredRenderer)
				rendererIndex=i;
		
		win=SDL_CreateWindow(WindowTitle.c_str(),winps.x,winps.y,winps.w,winps.h,flagWin);
		ren=SDL_CreateRenderer(win,rendererIndex,flagRen);
		SDL_SetRenderDrawBlendMode(ren,SDL_BLENDMODE_BLEND);
		WinOfSDLWinID[SDLWinID=SDL_GetWindowID(win)]=this;
		
		RefreshWinPS();
		PresentLimit=GetWinSize();
		
		KeyboardFocused=bool(SDL_GetWindowFlags(win)&SDL_WINDOW_INPUT_FOCUS);//??
		
		if (win==NULL||ren==NULL)
			PUI_DD[2]<<"Cannot create window or renderer!"<<endl;
		
		SDL_RendererInfo info;
		if (SDL_GetRendererInfo(ren,&info))
			PUI_DD[2]<<"Window "<<WindowTitle<<" failed to get renderer info! "<<SDL_GetError()<<endl;
		else
		{
			RendererName=info.name;
			RendererType=GetRendererTypeFromSDLRendererDriverName(RendererName);
			PUI_DD[0]<<"Current renderer of window "<<WindowTitle<<": "<<RendererName<<endl;
			NeedRefreshFullScreen=InThisSet(RendererType,PUI_RendererType_OpenGL,PUI_RendererType_OpenGLES,PUI_RendererType_OpenGLES2);
		}
		
		_BackGroundLayer=new WindowBackgroundLayer(0,NULL,GetWinSize());
		_MenuLayer=new Layer(0,NULL,GetWinSize());
		_BackGroundLayer->Win=_MenuLayer->Win=this;
		SetBackgroundColor();
		PUI_DD[0]<<"Create window "<<WindowTitle<<" OK"<<endl;
	}
	
	PUI_Window::PUI_Window(SDL_Window *_win)
	:TextTextureMemorizeData(PUI_Parameter_TextMemorizeMemoryLimit)
	{
		PUI_DD[0]<<"Create window from SDL_Window*"<<endl;
		++WindowCnt;
		AllWindow.insert(this);
		win=_win;
		WindowTitle=SDL_GetWindowTitle(_win);
		
		unsigned int flagRen=SDL_RENDERER_ACCELERATED;//??
		
		vector <Doublet<unsigned,string> > renderVec=PUI_Window::GetAvaliableRenderers();
		int rendererIndex=-1;
		for (int i=0;i<renderVec.size();++i)
			if (renderVec[i].a==PUI_PreferredRenderer)
				rendererIndex=i;
		
		ren=SDL_CreateRenderer(win,rendererIndex,flagRen);
		SDL_SetRenderDrawBlendMode(ren,SDL_BLENDMODE_BLEND);
		WinOfSDLWinID[SDLWinID=SDL_GetWindowID(win)]=this;
		
		RefreshWinPS();
		PresentLimit=GetWinSize();
		
		KeyboardFocused=bool(SDL_GetWindowFlags(win)&SDL_WINDOW_INPUT_FOCUS);//??
		
		if (win==NULL||ren==NULL)
			PUI_DD[2]<<"Cannot create window or renderer!"<<endl;
		
		SDL_RendererInfo info;
		if (SDL_GetRendererInfo(ren,&info))
			PUI_DD[2]<<"Window "<<WindowTitle<<" failed to get renderer info! "<<SDL_GetError()<<endl;
		else
		{
			RendererName=info.name;
			RendererType=GetRendererTypeFromSDLRendererDriverName(RendererName);
			PUI_DD[0]<<"Current renderer of window "<<WindowTitle<<": "<<RendererName<<endl;
			NeedRefreshFullScreen=InThisSet(RendererType,PUI_RendererType_OpenGL,PUI_RendererType_OpenGLES,PUI_RendererType_OpenGLES2);
		}
		
		_BackGroundLayer=new WindowBackgroundLayer(0,NULL,GetWinSize());
		_MenuLayer=new Layer(0,NULL,GetWinSize());
		_BackGroundLayer->Win=_MenuLayer->Win=this;
		SetBackgroundColor();
		PUI_DD[0]<<"Create window "<<WindowTitle<<" OK"<<endl;
	}
	
	PUI_Window::~PUI_Window()
	{
		PUI_DD[0]<<"Delete window "<<WindowTitle<<endl;
		
		if (CloseFunc!=NULL)
			delete CloseFunc;
		
		SetWindowBackgroundTransparent(0);
		
		delete _BackGroundLayer;
		delete _MenuLayer;
		
		TextTextureMemorizeData.Clear();//delete SharedTexturePtr related object before renderer is destroyed, otherwise crash may happen.
		TransparentBackgroundPic=SharedTexturePtr();
				
		WinOfSDLWinID.erase(SDLWinID);
		SDL_DestroyRenderer(ren);
		SDL_DestroyWindow(win);
		AllWindow.erase(this);
		--WindowCnt;
		PUI_DD[0]<<"Delete window "<<WindowTitle<<" OK"<<endl;
	}
	
	class BaseButton:public Widgets
	{
		protected:
			enum
			{
				Stat_NoFocus=0,
				Stat_Focus=1,
				Stat_Down=2,
			}stat=Stat_NoFocus;//Begin of many changes...(¨s¡ã¡õ¡ã£©¨s¦à ©ß©¥©ß
			enum
			{
				Button_Main=0,
				Button_Sub,
				Button_Extra,
				Button_Down
			};
			bool ThisButtonSolveSubClick=0,
				 ThisButtonSolveExtraClick=0,
				 ThisButtonSolvePressedClick=0;

			virtual bool InButton(const Point &pos)//Determine the button shape;
			{return CoverLmt.In(pos);}

			virtual void TriggerButtonFunction(int buttonType) {}
			
			virtual void CheckPos(const PUI_PosEvent *event,int mode)
			{
				if (mode&PosMode_LoseFocus)
				{
					if (stat!=Stat_NoFocus)
						if (!Win->IsPosFocused()||!InButton(event->pos)||NotInSet(Win->GetOccupyPosWg(),this,nullptr)||mode&PosMode_ForceLoseFocus)
						{
							stat=Stat_NoFocus;
							RemoveNeedLoseFocus();
							Win->SetPresentArea(CoverLmt);
						}
				}
				else if ((mode&PosMode_Default)&&InButton(event->pos)/*??*/)
					switch (event->posType)
					{
						case PUI_PosEvent::Pos_Down:
							if (event->button==PUI_PosEvent::Button_MainClick||ThisButtonSolveSubClick&&event->button==PUI_PosEvent::Button_SubClick||ThisButtonSolveExtraClick)
							{
								PUI_DD[0]<<"BaseButton "<<IDName<<" click"<<endl;
								stat=Stat_Down;
								if (ThisButtonSolvePressedClick)
									TriggerButtonFunction(Button_Down);
								SetNeedLoseFocus();
								Win->StopSolvePosEvent();
								Win->SetPresentArea(CoverLmt);
							}
							break;
						case PUI_PosEvent::Pos_Up:
							if (stat==Stat_Down)
							{
								PUI_DD[0]<<"BaseButton "<<IDName<<" function"<<endl;
								if (ThisButtonSolveSubClick&&event->button==PUI_PosEvent::Button_SubClick)
									TriggerButtonFunction(Button_Sub);
								else if (ThisButtonSolveExtraClick&&event->button==PUI_PosEvent::Button_OtherClick)
									TriggerButtonFunction(Button_Extra);
								else TriggerButtonFunction(Button_Main);
								if (event->type==event->Event_TouchEvent)
								{
									stat=Stat_NoFocus;
									RemoveNeedLoseFocus();
								}
								else stat=Stat_Focus;
								Win->StopSolvePosEvent();
								Win->SetPresentArea(CoverLmt);
							}
							break;
						case PUI_PosEvent::Pos_Motion:
							if (stat==Stat_NoFocus)
							{
								stat=Stat_Focus;
								SetNeedLoseFocus();
								Win->SetPresentArea(CoverLmt);
							}
							Win->StopSolvePosEvent();
							break;
					}
			}
			
			BaseButton(const WidgetID &_ID,WidgetType _type,Widgets *_fa,const Posize &_rPS)
			:Widgets(_ID,_type,_fa,_rPS) {}
			
			BaseButton(const WidgetID &_ID,WidgetType _type,Widgets *_fa,PosizeEX *psex)
			:Widgets(_ID,_type,_fa,psex) {}
			
		public:
			inline void TriggerFunc()
			{TriggerButtonFunction(Button_Main);}
			
			inline void TriggerMainClick()
			{TriggerButtonFunction(Button_Main);}
			
			inline void TriggerSubClick()
			{TriggerButtonFunction(Button_Sub);}
	};
	
	template <class T> class Button:public BaseButton
	{
		protected:
			string Text;
			int textMode=0;
			void (*func)(T&)=NULL;
			T funcData;
			RGBA ButtonColor[3]{ThemeColorM[1],ThemeColorM[3],ThemeColorM[5]},
				 TextColor=ThemeColorMT[0];

			virtual void TriggerButtonFunction(int buttonType)
			{
				if (buttonType==Button_Main&&func!=nullptr)
					func(funcData);
			}
			
			virtual void Show(Posize &lmt)
			{
				Win->RenderFillRect(lmt,ThemeColor(ButtonColor[stat]));
				Win->RenderDrawText(Text,gPS,lmt,textMode,ThemeColor(TextColor));
				Win->Debug_DisplayBorder(gPS);
			}
			
			Button(const WidgetID &_ID,WidgetType _type,Widgets *_fa,const Posize &_rPS,const string &_text,void (*_func)(T&),const T &_funcdata)
			:BaseButton(_ID,_type,_fa,_rPS),Text(_text),func(_func),funcData(_funcdata) {}
			
			Button(const WidgetID &_ID,WidgetType _type,Widgets *_fa,PosizeEX *psex,const string &_text,void (*_func)(T&),const T &_funcdata)
			:BaseButton(_ID,_type,_fa,psex),Text(_text),func(_func),funcData(_funcdata) {}
			
			Button(const WidgetID &_ID,WidgetType _type,Widgets *_fa,const Posize &_rPS,const string &_text)
			:BaseButton(_ID,_type,_fa,_rPS),Text(_text) {}
			
			Button(const WidgetID &_ID,WidgetType _type,Widgets *_fa,PosizeEX *psex,const string &_text)
			:BaseButton(_ID,_type,_fa,psex),Text(_text) {}
		
		public:
			void SetTextColor(const RGBA &co)
			{
				TextColor=!co?ThemeColorMT[0]:co;
				Win->SetPresentArea(CoverLmt);
			}
			
			void SetButtonColor(int p,const RGBA &co)
			{
				if (InRange(p,0,2))
				{
					ButtonColor[p]=!co?ThemeColorM[p*2+1]:co;
					Win->SetPresentArea(CoverLmt);
				}
				else PUI_DD[1]<<"SetButtonColor error: p "<<p<<" is not in range[0,2]"<<endl;
			}
			
			inline void SetButtonText(const string &_text)
			{
				Text=_text;
				Win->SetPresentArea(CoverLmt);
			}
			
			inline void SetTextMode(int mode)
			{
				textMode=mode;
				Win->SetPresentArea(CoverLmt);
			}
			
			inline T& GetFuncData()
			{return funcData;}
			
			inline void SetFunc(void (*_func)(T&),const T &_funcdata)
			{
				func=_func;
				funcData=_funcdata;
			}
			
			Button(const WidgetID &_ID,Widgets *_fa,const Posize &_rPS,const string &_text,void (*_func)(T&),const T &_funcdata)
			:Button(_ID,WidgetType_Button,_fa,_rPS,_text,_func,_funcdata) {}
			
			Button(const WidgetID &_ID,Widgets *_fa,PosizeEX *psex,const string &_text,void (*_func)(T&),const T &_funcdata)
			:Button(_ID,WidgetType_Button,_fa,psex,_text,_func,_funcdata) {}
			
			Button(const WidgetID &_ID,Widgets *_fa,const Posize &_rPS,const string &_text)
			:Button(_ID,WidgetType_Button,_fa,_rPS,_text) {}
			
			Button(const WidgetID &_ID,Widgets *_fa,PosizeEX *psex,const string &_text)
			:Button(_ID,WidgetType_Button,_fa,psex,_text) {}
	};
	
	class TinyText:public Widgets
	{
		protected:
			int Mode=0;//0:mid -1:Left 1:Right
			string text;
			PUI_Font font;
//			bool AutoW=0;
			RGBA TextColor=ThemeColorMT[0];
			
			virtual void Show(Posize &lmt)
			{
				Win->RenderDrawText(text,gPS,lmt,Mode,ThemeColor(TextColor),font);
				Win->Debug_DisplayBorder(gPS);
			}
			
			TinyText(const WidgetID &_ID,WidgetType _type,Widgets *_fa,const Posize &_rPS,const string &str,int _mode=0,const RGBA &textColor=RGBA_NONE)
			:Widgets(_ID,_type,_fa,_rPS),text(str),Mode(_mode),TextColor(textColor) {}
			
			TinyText(const WidgetID &_ID,WidgetType _type,Widgets *_fa,PosizeEX *psex,const string &str,int _mode=0,const RGBA &textColor=RGBA_NONE)
			:Widgets(_ID,_type,_fa,psex),text(str),Mode(_mode),TextColor(textColor) {}
			
		public:
			inline void SetTextColor(const RGBA &co)
			{
				TextColor=!co?ThemeColorMT[0]:co;
				Win->SetPresentArea(CoverLmt);
			}
			
			inline void SetText(const string &str)
			{
				text=str;
				Win->SetPresentArea(CoverLmt);
			}
			
			inline string GetText()
			{return text;}
			
			inline void SetFontSize(int size)
			{
				font.Size=size;
				Win->SetPresentArea(CoverLmt);
			}
			
			inline void SetFont(const PUI_Font &tar)
			{
				font=tar;
				Win->SetPresentArea(CoverLmt);
			}
			
			TinyText(const WidgetID &_ID,Widgets *_fa,const Posize &_rPS,const string &str,int _mode=0,const RGBA &textColor=RGBA_NONE)
			:TinyText(_ID,WidgetType_TinyText,_fa,_rPS,str,_mode,textColor) {}
			
			TinyText(const WidgetID &_ID,Widgets *_fa,PosizeEX *psex,const string &str,int _mode=0,const RGBA &textColor=RGBA_NONE)
			:TinyText(_ID,WidgetType_TinyText,_fa,psex,str,_mode,textColor) {}
	};
	
	class TinyText2:public TinyText
	{
		protected:
			int ShowFullTextMode=1;//0:off 1:Mouse focus outtime 2:Mouse focus outtime or touch long click
			
			virtual void CheckPos(const PUI_PosEvent *event,int mode)
			{
				//...
			}

		public:
			void SetShowFullText(bool on)
			{
				//...
			}
			
			inline void SetShowFullTextMode(int showfulltextmode)
			{ShowFullTextMode=showfulltextmode;}
			
			TinyText2(const WidgetID &_ID,Widgets *_fa,const Posize &_rps,const string &str,int _mode=0,const RGBA &textColor=RGBA_NONE,int showfulltextmode=1)
			:TinyText(_ID,WidgetType_TinyText2,_fa,_rps,str,_mode,textColor),ShowFullTextMode(showfulltextmode) {}
		
			TinyText2(const WidgetID &_ID,Widgets *_fa,PosizeEX *psex,const string &str,int _mode=0,const RGBA &textColor=RGBA_NONE,int showfulltextmode=1)
			:TinyText(_ID,WidgetType_TinyText2,_fa,psex,str,_mode,textColor),ShowFullTextMode(showfulltextmode) {}
	};
	
	class ToastText:public Widgets
	{
		protected:
			
			
		public:
			
			
	};

	template <class T> class CheckBox:public BaseButton
	{
		protected:
			bool on=0;
			void (*func)(T&,bool)=NULL;//bool: on
			T funcData;
			RGBA BorderColor[3]{ThemeColorM[3],ThemeColorM[5],ThemeColorM[7]},
				 ChooseColor=ThemeColorM[7];
			
			virtual void TriggerButtonFunction(int buttonType)
			{
				if (buttonType==Button_Main)
					SwitchOnOff(1);
			}
			
			virtual void Show(Posize &lmt)
			{
				Win->RenderDrawRectWithLimit(gPS,ThemeColor(BorderColor[stat]),lmt);
				if (on)
					Win->RenderFillRect(lmt&gPS.Shrink(3),ThemeColor(ChooseColor));

				Win->Debug_DisplayBorder(gPS);
			}
			
			CheckBox(const WidgetID &_ID,WidgetType _type,Widgets *_fa,const Posize &_rPS,bool defaultOnOff,void(*_func)(T&,bool),const T &_funcData)
			:BaseButton(_ID,_type,_fa,_rPS),on(defaultOnOff),func(_func),funcData(_funcData) {}
			
			CheckBox(const WidgetID &_ID,WidgetType _type,Widgets *_fa,PosizeEX* psex,bool defaultOnOff,void(*_func)(T&,bool),const T &_funcData)
			:BaseButton(_ID,_type,_fa,psex),on(defaultOnOff),func(_func),funcData(_funcData) {}
			
			CheckBox(const WidgetID &_ID,WidgetType _type,Widgets *_fa,const Posize &_rPS,bool defaultOnOff)
			:BaseButton(_ID,_type,_fa,_rPS),on(defaultOnOff) {}
			
			CheckBox(const WidgetID &_ID,WidgetType _type,Widgets *_fa,PosizeEX* psex,bool defaultOnOff)
			:BaseButton(_ID,_type,_fa,psex),on(defaultOnOff) {}
			
		public:
			inline void SetBorderColor(int p,const RGBA &co)
			{
				if (InRange(p,0,2))
				{
					BorderColor[p]=!co?ThemeColorM[p*2+1]:co;
					Win->SetPresentArea(CoverLmt);
				}
				else PUI_DD[1]<<"CheckBox: SetBorderColor: p "<<p<<" is not in Range[0,2]"<<endl;
			}
			
		 	inline void SetChooseColor(const RGBA &co)
			{
				ChooseColor=!co?ThemeColorM[5]:co;
				Win->SetPresentArea(CoverLmt);
			}
			
			inline T& GetFuncData()
			{return funcData;}
			
			inline void SetFunc(void (*_func)(T&,bool),const T &_funcdata)
			{
				func=_func;
				funcData=_funcdata;
			}
			
			virtual void SetOnOff(bool _on,bool TriggerFunc=1)
			{
				if (on==_on) return;
				on=_on;
				PUI_DD[0]<<"CheckBox "<<IDName<<" switch to "<<on<<endl;
				if (TriggerFunc)
					if (func!=NULL)
						func(funcData,on);
				Win->SetPresentArea(CoverLmt);
			}
			
			inline void SwitchOnOff(bool TriggerFunc=1)
			{SetOnOff(!on,TriggerFunc);}
			
			inline bool GetOnOff()
			{return on;}
			
			CheckBox(const WidgetID &_ID,Widgets *_fa,const Posize &_rPS,bool defaultOnOff,void(*_func)(T&,bool),const T &_funcData)
			:CheckBox(_ID,WidgetType_CheckBox,_fa,_rPS,defaultOnOff,_func,_funcData) {}
			
			CheckBox(const WidgetID &_ID,Widgets *_fa,PosizeEX* psex,bool defaultOnOff,void(*_func)(T&,bool),const T &_funcData)
			:CheckBox(_ID,WidgetType_CheckBox,_fa,psex,defaultOnOff,_func,_funcData) {}
			
			CheckBox(const WidgetID &_ID,Widgets *_fa,const Posize &_rPS,bool defaultOnOff)
			:CheckBox(_ID,WidgetType_CheckBox,_fa,_rPS,defaultOnOff) {}
			
			CheckBox(const WidgetID &_ID,Widgets *_fa,PosizeEX* psex,bool defaultOnOff)
			:CheckBox(_ID,WidgetType_CheckBox,_fa,psex,defaultOnOff) {}
	};
	
	template <class T> class SwitchButton:public CheckBox <T>//This is also a widget to test animation
	{
		protected:
			RGBA ChunkColor=ThemeColorMT[0],
				 UnChoosenColor[3]{ThemeColorBG[1],ThemeColorBG[3],ThemeColorBG[5]};
			double chunkPercent=0;
			int ChunkWidth=12,
				UpdateInterval=10,
				UpdataCnt=10;
			SDL_TimerID IntervalTimerID=0;
			
			virtual void CheckEvent(const PUI_Event *event)
			{
				if (event->type==PUI_EVENT_UpdateTimer)
					if (event->UserEvent()->data1==this)
					{
						if (event->UserEvent()->code==0)
							IntervalTimerID=0;
						chunkPercent=this->on?1-event->UserEvent()->code*1.0/UpdataCnt:event->UserEvent()->code*1.0/UpdataCnt;
						this->Win->StopSolveEvent();
						this->Win->SetPresentArea(this->CoverLmt);
					}
			}
			
			virtual void Show(Posize &lmt)
			{
				this->Win->RenderDrawRectWithLimit(this->gPS,ThemeColor(this->BorderColor[this->stat]),lmt);
				this->Win->RenderDrawRectWithLimit(this->gPS.Shrink(1),ThemeColor(this->BorderColor[this->stat]),lmt);
				
				this->Win->RenderFillRect(this->gPS.Shrink(4)&lmt,ThemeColor(UnChoosenColor[this->stat])); 
				this->Win->RenderFillRect(Posize(this->gPS.x+4,this->gPS.y+4,(this->gPS.w-ChunkWidth)*chunkPercent-4,this->gPS.h-8)&lmt,ThemeColor(this->ChooseColor));
				this->Win->RenderFillRect(Posize(this->gPS.x+(this->gPS.w-ChunkWidth)*chunkPercent,this->gPS.y,ChunkWidth,this->gPS.h)&lmt,ThemeColor(ChunkColor));//ThisColor OK?
				
				this->Win->Debug_DisplayBorder(this->gPS);
			}
		
		public:
			inline void SetChunkColor(const RGBA &co)
			{
				ChunkColor=!co?ThemeColorMT[0]:0;
				this->Win->SetPresentArea(this->CoverLmt);
			}
			
			inline void SetUnchoosenColor(int p,const RGBA &co)
			{
				if (InRange(p,0,2))
				{
					UnChoosenColor[p]=!co?ThemeColorBG[p*2+1]:co;
					this->Win->SetPresentArea(this->CoverLmt);
				}
				else PUI_DD[1]<<"SwitchButton: SetUnchoosenColor: p "<<p<<" is not in Range[0,2]"<<endl;
			}
			
			inline void SetChunkWidth(int w)
			{
				ChunkWidth=w;
				this->Win->SetPresentArea(this->CoverLmt);
			}
			
			virtual void SetOnOff(bool _on,bool TriggerFunc=1)
			{
				if (this->on==_on) return;
				CheckBox<T>::SetOnOff(_on,TriggerFunc);
				if (IntervalTimerID!=0)
					SDL_RemoveTimer(IntervalTimerID);
				IntervalTimerID=SDL_AddTimer(UpdateInterval,PUI_UpdateTimer,new PUI_UpdateTimerData(this,UpdataCnt));
			}
			
			virtual ~SwitchButton()
			{
				if (IntervalTimerID!=0)
					SDL_RemoveTimer(IntervalTimerID);
			}
			
			SwitchButton(const Widgets::WidgetID &_ID,Widgets *_fa,const Posize &_rPS,bool defaultOnOff,void(*_func)(T&,bool),const T &_funcData)
			:CheckBox<T>::CheckBox(_ID,Widgets::WidgetType_SwitchButton,_fa,_rPS,defaultOnOff,_func,_funcData),chunkPercent(defaultOnOff)
			{CheckBox<T>::ChooseColor=ThemeColorM[1];}
			
			SwitchButton(const Widgets::WidgetID &_ID,Widgets *_fa,PosizeEX* psex,bool defaultOnOff,void(*_func)(T&,bool),const T &_funcData)
			:CheckBox<T>::CheckBox(_ID,Widgets::WidgetType_SwitchButton,_fa,psex,defaultOnOff,_func,_funcData),chunkPercent(defaultOnOff)
			{CheckBox<T>::ChooseColor=ThemeColorM[1];}
			
			SwitchButton(const Widgets::WidgetID &_ID,Widgets *_fa,const Posize &_rPS,bool defaultOnOff)
			:CheckBox<T>::CheckBox(_ID,Widgets::WidgetType_SwitchButton,_fa,_rPS,defaultOnOff),chunkPercent(defaultOnOff)
			{CheckBox<T>::ChooseColor=ThemeColorM[1];}
			
			SwitchButton(const Widgets::WidgetID &_ID,Widgets *_fa,PosizeEX* psex,bool defaultOnOff)
			:CheckBox<T>::CheckBox(_ID,Widgets::WidgetType_SwitchButton,_fa,psex,defaultOnOff),chunkPercent(defaultOnOff)
			{CheckBox<T>::ChooseColor=ThemeColorM[1];}
	};
	
	template <class T> class SingleChoiceButton:public Widgets
	{
		protected :
			enum
			{
				Stat_NoFocus=0,
				Stat_UpFocus=1,
				Stat_Down=2
			}stat=Stat_NoFocus;
			int ButtonCnt=0,
				CurrentChoose=-1,//count from 0;
				ChosenChoice=-1,
				eachChoiceHeight=20,
				ringD1=10,
				ringD2=12;
			SDL_Texture *ringTex=NULL,
						*roundTex=NULL;
			vector <string> Text;
			void (*func)(T&,const string&,int)=NULL;//int:choose
			T funcData;
			RGBA TextColor=ThemeColorMT[0],
				 ButtonColor[2]{ThemeColorM[0],ThemeColorM[1]},
				 RingColor[2]{ThemeColorM[5],ThemeColorM[7]},
				 LastRealRingColor[2]{RGBA_NONE,RGBA_NONE};//ring,round
			
			int GetCurrentChoose(int y)
			{
				int re=(y-gPS.y)/eachChoiceHeight;
				if (InRange(re,0,ButtonCnt-1))
					return re;
				else return -1;
			}
			
			virtual void CheckEvent(const PUI_Event *event)
			{
				//...
			}
			
			virtual void CheckPos(const PUI_PosEvent *event,int mode)
			{
				if (mode&PosMode_LoseFocus)
				{
					if (stat!=Stat_NoFocus)
						if (!Win->IsPosFocused()||!CoverLmt.In(event->pos)||mode&PosMode_ForceLoseFocus)
						{
							stat=Stat_NoFocus;
							RemoveNeedLoseFocus();
							Win->SetPresentArea(Posize(gPS.x,gPS.y+CurrentChoose*eachChoiceHeight,gPS.w,eachChoiceHeight));
							CurrentChoose=-1;
						}
				}
				else if (mode&PosMode_Default)
					switch (event->posType)
					{
						case PUI_PosEvent::Pos_Down:
							if (event->button==PUI_PosEvent::Button_MainClick)
							{
								if (CurrentChoose!=-1)
									Win->SetPresentArea(Posize(gPS.x,gPS.y+CurrentChoose*eachChoiceHeight,gPS.w,eachChoiceHeight)&CoverLmt);
								CurrentChoose=GetCurrentChoose(event->pos.y);
								if (CurrentChoose!=-1)
								{
									PUI_DD[0]<<"SingleChoiceButton "<<IDName<<" click "<<CurrentChoose<<endl;
									stat=Stat_Down;
									Win->StopSolvePosEvent();
									Win->SetPresentArea(Posize(gPS.x,gPS.y+CurrentChoose*eachChoiceHeight,gPS.w,eachChoiceHeight)&CoverLmt);
								}
							}
							break;
						case PUI_PosEvent::Pos_Up:
							if (stat==Stat_Down)
							{
								if (CurrentChoose!=-1)
									Win->SetPresentArea(Posize(gPS.x,gPS.y+CurrentChoose*eachChoiceHeight,gPS.w,eachChoiceHeight)&CoverLmt);
								if (ChosenChoice!=-1)
									Win->SetPresentArea(Posize(gPS.x,gPS.y+ChosenChoice*eachChoiceHeight,gPS.w,eachChoiceHeight)&CoverLmt);
								CurrentChoose=GetCurrentChoose(event->pos.y);
								if (CurrentChoose!=-1)
								{
									ChosenChoice=CurrentChoose;
									PUI_DD[0]<<"SingleChoiceButton "<<IDName<<" choose "<<CurrentChoose<<endl;
									if (func!=NULL)
										func(funcData,Text[CurrentChoose],CurrentChoose);
									stat=Stat_UpFocus;
								}
								if (ChosenChoice!=-1)
									Win->SetPresentArea(Posize(gPS.x,gPS.y+ChosenChoice*eachChoiceHeight,gPS.w,eachChoiceHeight)&CoverLmt);
								Win->StopSolvePosEvent();
							}
							break;
						case PUI_PosEvent::Pos_Motion:
							if (stat==Stat_NoFocus)
							{
								stat=Stat_UpFocus;
								SetNeedLoseFocus();
								CurrentChoose=GetCurrentChoose(event->pos.y);
								if (CurrentChoose!=-1)
									Win->SetPresentArea(Posize(gPS.x,gPS.y+CurrentChoose*eachChoiceHeight,gPS.w,eachChoiceHeight)&CoverLmt);
							}
							else if (CurrentChoose!=GetCurrentChoose(event->pos.y))
							{
								Win->SetPresentArea(Posize(gPS.x,gPS.y+CurrentChoose*eachChoiceHeight,gPS.w,eachChoiceHeight)&CoverLmt);
								CurrentChoose=GetCurrentChoose(event->pos.y);
								if (CurrentChoose!=-1)
									Win->SetPresentArea(Posize(gPS.x,gPS.y+CurrentChoose*eachChoiceHeight,gPS.w,eachChoiceHeight)&CoverLmt);
							}
							Win->StopSolvePosEvent();
							break;
					}
			}
			
			void Show(Posize &lmt)
			{
				RGBA ringCo=ThemeColor(RingColor[0]),roundCo=ThemeColor(RingColor[1]);
				if (ringTex!=NULL&&ringCo!=LastRealRingColor[0])
					SDL_DestroyTexture(ringTex),
					ringTex=NULL;
				if (roundTex!=NULL&&roundCo!=LastRealRingColor[1])
					SDL_DestroyTexture(roundTex),
					roundTex=NULL;
				for (int i=0;i<ButtonCnt;++i)
				{
					if (i==CurrentChoose)
						Win->RenderFillRect(lmt&Posize(gPS.x,gPS.y+i*eachChoiceHeight,gPS.w,eachChoiceHeight),ThemeColor(ButtonColor[stat-1]));
					if (ringTex==NULL)
						ringTex=CreateTextureFromSurfaceAndDelete(CreateRingSurface(ringD1,ringD2,LastRealRingColor[0]=ringCo));
					Win->RenderCopyWithLmt(ringTex,Posize(gPS.x,gPS.y+i*eachChoiceHeight,eachChoiceHeight,eachChoiceHeight).Shrink(eachChoiceHeight-ringD2>>1),lmt);
					if (i==ChosenChoice)
					{
						if (roundTex==NULL)
							roundTex=CreateTextureFromSurfaceAndDelete(CreateRingSurface(0,ringD1-4,LastRealRingColor[1]=roundCo));
						Win->RenderCopyWithLmt(roundTex,Posize(gPS.x,gPS.y+i*eachChoiceHeight,eachChoiceHeight,eachChoiceHeight).Shrink(eachChoiceHeight-ringD1+4>>1),lmt);
					}
					Win->RenderDrawText(Text[i],{gPS.x+eachChoiceHeight,gPS.y+i*eachChoiceHeight,gPS.w-eachChoiceHeight,eachChoiceHeight},lmt,-1,ThemeColor(TextColor));
					Win->Debug_DisplayBorder({gPS.x,gPS.y+i*eachChoiceHeight,gPS.w,eachChoiceHeight});
				}
				Win->Debug_DisplayBorder(gPS);
			}
			
		public:
			inline void SetTextColor(const RGBA &co)
			{
				TextColor=!co?ThemeColorMT[0]:co;
				Win->SetPresentArea(CoverLmt);
			}
			
			inline void SetButtonColor(int p,const RGBA &co)
			{
				if (p==0||p==1)
				{
					ButtonColor[p]=!co?ThemeColorM[p]:co;
					Win->SetPresentArea(CoverLmt);
				}
				else PUI_DD[1]<<"SingleChoiceButton: SetButtonColor: p "<<p<<" is not 0 or 1"<<endl;
			}
			
			inline void SetRingColor(int p,const RGBA &co)
			{
				if (p==0||p==1)
				{
					RingColor[p]=!co?ThemeColor[p*2+5]:co;
					Win->SetPresentArea(CoverLmt);
				}
				else PUI_DD[1]<<"SingleChoiceButton: SetRingColor: p "<<p<<" is not 0 or 1"<<endl;
			}
			
			virtual void SetrPS(const Posize &ps)//ignore h;
			{
				CoverLmt=gPS=rPS=ps;
				rPS.h=ButtonCnt*eachChoiceHeight;
				Win->SetNeedUpdatePosize();
			}
			
			inline T& GetFuncData()
			{return funcData;}
			
			inline void SetFunc(void (*_func)(T&,const string&,int),const T &_funcdata)
			{func=_func;funcData=_funcdata;}
			
			void SetAccentData(int h,int d1,int d2)
			{
				eachChoiceHeight=h;
				rPS.h=ButtonCnt*eachChoiceHeight;
				ringD1=d1;ringD2=d2;
				if (ringTex!=NULL)
					SDL_DestroyTexture(ringTex),
					ringTex=NULL;
				if (roundTex!=NULL)
					SDL_DestroyTexture(roundTex),
					roundTex=NULL;
				Win->SetNeedUpdatePosize();
				Win->SetPresentArea(CoverLmt);
			}
			
			SingleChoiceButton* AddChoice(const string &str)
			{
				Text.push_back(str);
				++ButtonCnt;
				rPS.h=ButtonCnt*eachChoiceHeight;
				Win->SetNeedUpdatePosize();
				Win->SetPresentArea(Posize(gPS.x,gPS.y2()-eachChoiceHeight,rPS.w,eachChoiceHeight)&CoverLmt);
				return this;
			}
			
			void ClearAllChoice()
			{
				Text.clear();
				ButtonCnt=0;
				rPS.h=0;
				Win->SetNeedUpdatePosize();
				Win->SetPresentArea(CoverLmt);
			}
			
//			void EraseChoice(int p)
			
			void ChooseChoice(int p,bool triggerfunc=1)
			{
				if (InRange(p,0,ButtonCnt-1))
				{
					if (InRange(ChosenChoice,0,ButtonCnt-1))
						Win->SetPresentArea(Posize(gPS.x,gPS.y+ChosenChoice*eachChoiceHeight,gPS.w,eachChoiceHeight));
					ChosenChoice=p;
					if (InRange(ChosenChoice,0,ButtonCnt-1))
						Win->SetPresentArea(Posize(gPS.x,gPS.y+ChosenChoice*eachChoiceHeight,gPS.w,eachChoiceHeight));
					if (triggerfunc&&func!=NULL)
						func(funcData,Text[ChosenChoice],p);
				}
				else PUI_DD[2]<<"SingleChoiceButton: SetChoice: p "<<p<<" is not in Range[0,ButtonCnt "<<ButtonCnt<<")"<<endl;
			}
			
			inline int GetChoice()
			{return ChosenChoice;}
			
			void SetChoiceText(int p,const string &str)
			{
				if (ButtonCnt==0) return;
				p=EnsureInRange(p,0,ButtonCnt-1);
				Text[p]=str;
				Win->SetPresentArea(Posize(gPS.x,gPS.y+p*eachChoiceHeight,gPS.w,eachChoiceHeight)&CoverLmt);
			}
			
			inline string GetChoiceText(int p=-1)
			{
				if (ButtonCnt==0) return "";
				if (p==-1) return Text[ChosenChoice];
				else return Text[EnsureInRange(p,0,ButtonCnt-1)];
			}
			
			void TriggerFunc(int p)
			{
				if (func==NULL)
					return;
				Win->SetPresentArea((Posize(gPS.x,gPS.y+eachChoiceHeight*p,gPS.w,eachChoiceHeight)|Posize(gPS.x,gPS.y+eachChoiceHeight*ChosenChoice,gPS.w,eachChoiceHeight))&CoverLmt);
				ChosenChoice=EnsureInRange(p,0,ButtonCnt-1);
				func(funcData,Text[ChosenChoice],ChosenChoice);
			}
			
			virtual ~SingleChoiceButton()
			{
				ClearAllChoice();
				if (ringTex!=NULL)
					SDL_DestroyTexture(ringTex);
				if (roundTex!=NULL)
					SDL_DestroyTexture(roundTex);
			}
			
			SingleChoiceButton(const WidgetID &_ID,Widgets *_fa,const Posize &_rps,void(*_func)(T&,const string&,int),const T &_funcData)
			:Widgets(_ID,WidgetType_SingleChoiceButton,_fa,_rps),func(_func),funcData(_funcData) {}
			
			SingleChoiceButton(const WidgetID &_ID,Widgets *_fa,PosizeEX* psex,void(*_func)(T&,const string&,int),const T &_funcData)
			:Widgets(_ID,WidgetType_SingleChoiceButton,_fa,psex),func(_func),funcData(_funcData) {}
			
			SingleChoiceButton(const WidgetID &_ID,Widgets *_fa,const Posize &_rps)
			:Widgets(_ID,WidgetType_SingleChoiceButton,_fa,_rps) {}
			
			SingleChoiceButton(const WidgetID &_ID,Widgets *_fa,PosizeEX* psex)
			:Widgets(_ID,WidgetType_SingleChoiceButton,_fa,psex) {}
	};
	
	template <class T,class T2> class Button2:public Button<T>
	{
		protected:
			void (*subFunc)(T2&)=NULL;
			T2 subFuncdata;
			
			virtual void TriggerButtonFunction(int buttonType)
			{
				if (buttonType==this->Button_Main&&this->func!=nullptr)
					this->func(this->funcData);
				else if (buttonType==this->Button_Sub&&subFunc!=nullptr)
					subFunc(subFuncdata);
			}
			
		public:
			inline T2& GetSubFuncData()
			{return subFuncdata;}
			
			inline void SetSubFunc(void (*_subfunc)(T2&),const T &_subfuncdata)
			{
				subFunc=_subfunc;
				subFuncdata=_subfuncdata;
			}
			
			Button2(const Widgets::WidgetID &_ID,Widgets *_fa,const Posize &_rPS,const string &_text,void (*_func)(T&),const T &_funcdata,void (*_subfunc)(T2&),const T2 &_subfuncdata)
			:Button<T>(_ID,Widgets::WidgetType_Button2,_fa,_rPS,_text,_func,_funcdata),subFunc(_subfunc),subFuncdata(_subfuncdata) {this->ThisButtonSolveSubClick=1;}
			
			Button2(const Widgets::WidgetID &_ID,Widgets *_fa,PosizeEX *psex,const string &_text,void (*_func)(T&),const T &_funcdata,void (*_subfunc)(T2&),const T2 &_subfuncdata)
			:Button<T>(_ID,Widgets::WidgetType_Button2,_fa,psex,_text,_func,_funcdata),subFunc(_subfunc),subFuncdata(_subfuncdata) {this->ThisButtonSolveSubClick=1;}
			
			Button2(const Widgets::WidgetID &_ID,Widgets *_fa,const Posize &_rPS,const string &_text)
			:Button<T>(_ID,Widgets::WidgetType_Button2,_fa,_rPS,_text) {this->ThisButtonSolveSubClick=1;}
			
			Button2(const Widgets::WidgetID &_ID,Widgets *_fa,PosizeEX *psex,const string &_text)
			:Button<T>(_ID,Widgets::WidgetType_Button2,_fa,psex,_text) {this->ThisButtonSolveSubClick=1;}
	};
	
	class KeyboardButton:public BaseButton
	{
		protected:
			static Uint32 TimerInterval;
			static SDL_TimerID IntervalTimerID;
			static set <KeyboardButton*> AllKeyboardButton;
			
			BaseTypeFuncAndData *func=nullptr;
			string Text;
			int textMode=0;
			bool EnableRepeatDownEvent=1;
			bool RepeadDownEventOn=0;
			int DiscardRepeatFirstly=0;
			RGBA ButtonColor[3]{ThemeColorM[1],ThemeColorM[3],ThemeColorM[5]},
				 TextColor=ThemeColorMT[0];

			void SetRepeatDownEvent(bool on)
			{
				if (RepeadDownEventOn==on) return;
				RepeadDownEventOn=on;
				if (on&&EnableRepeatDownEvent)
				{
					DiscardRepeatFirstly=500/TimerInterval;
					if (AllKeyboardButton.empty())
						IntervalTimerID=SDL_AddTimer(TimerInterval,KeyboardTimerFunc,nullptr);
					AllKeyboardButton.insert(this);
				}
				else
				{
					AllKeyboardButton.erase(this);
					if (AllKeyboardButton.empty())
						SDL_RemoveTimer(IntervalTimerID),
						IntervalTimerID=0;
				}
			}
			
			virtual void TriggerButtonFunction(int buttonType)
			{
				SetRepeatDownEvent(buttonType==Button_Down);
				if (func!=nullptr)
					func->CallFunc(buttonType);
			}
			
			inline void DownButtonFunctionFromTimer()
			{
				if (RepeadDownEventOn)
					if (stat!=Stat_Down)
						SetRepeatDownEvent(0);
					else if (--DiscardRepeatFirstly<=0)
						if (func!=nullptr)
							func->CallFunc(Button_Down);
			}
			
			virtual void Show(Posize &lmt)
			{
				Win->RenderFillRect(lmt,ThemeColor(ButtonColor[stat]));
				Win->RenderDrawText(Text,gPS,lmt,textMode,ThemeColor(TextColor));
				Win->Debug_DisplayBorder(gPS);
			}

			static Uint32 KeyboardTimerFunc(Uint32 itv,void *funcdata)
			{
				PUI_SendFunctionEvent([](void*)
				{
					for (auto sp:AllKeyboardButton)
						sp->DownButtonFunctionFromTimer();
				});
				return TimerInterval;
			}
			
		public:
			void SetTextColor(const RGBA &co)
			{
				TextColor=!co?ThemeColorMT[0]:co;
				Win->SetPresentArea(CoverLmt);
			}
			
			void SetButtonColor(int p,const RGBA &co)
			{
				if (InRange(p,0,2))
				{
					ButtonColor[p]=!co?ThemeColorM[p*2+1]:co;
					Win->SetPresentArea(CoverLmt);
				}
				else PUI_DD[1]<<"SetButtonColor error: p "<<p<<" is not in range[0,2]"<<endl;
			}
			
			inline void SetButtonText(const string &_text)
			{
				Text=_text;
				Win->SetPresentArea(CoverLmt);
			}
			
			inline void SetTextMode(int mode)
			{
				textMode=mode;
				Win->SetPresentArea(CoverLmt);
			}
			
			inline void SetFunc(BaseTypeFuncAndData *_func)
			{
				if (func!=nullptr)
					delete func;
				func=_func;
			}
			
			inline void SetEnableRepeatDownEvent(bool on)
			{EnableRepeatDownEvent=on;}
			
			~KeyboardButton()
			{
				SetRepeatDownEvent(0);
				delete func;
			}
			
			KeyboardButton(const WidgetID &_ID,Widgets *_fa,const Posize &_rPS,const string &_text,BaseTypeFuncAndData *_func=nullptr)
			:BaseButton(_ID,WidgetType_KeyboardButton,_fa,_rPS),Text(_text),func(_func)
			{
				ThisButtonSolveSubClick=1;
				ThisButtonSolveExtraClick=1;
				ThisButtonSolvePressedClick=1;
			}
			
			KeyboardButton(const WidgetID &_ID,Widgets *_fa,PosizeEX *psex,const string &_text,BaseTypeFuncAndData *_func=nullptr)
			:BaseButton(_ID,WidgetType_KeyboardButton,_fa,psex),Text(_text),func(_func)
			{
				ThisButtonSolveSubClick=1;
				ThisButtonSolveExtraClick=1;
				ThisButtonSolvePressedClick=1;
			}
	};
	Uint32 KeyboardButton::TimerInterval=34;
	SDL_TimerID KeyboardButton::IntervalTimerID=0;
	set <KeyboardButton*> KeyboardButton::AllKeyboardButton;
	
	template <class T> class TwoStateButton:public BaseButton
	{
		protected:
			int currentState=0;
			string Text[2];
			int textMode=0;
			void (*func)(T&,bool)=NULL;
			T funcData;
			RGBA ButtonColor[2][3]{{ThemeColorBG[1],ThemeColorBG[3],ThemeColorBG[5]},
								   {ThemeColorM[1],ThemeColorM[3],ThemeColorM[5]}},
				 TextColor[2]{ThemeColorMT[0],ThemeColorMT[0]};

			virtual void TriggerButtonFunction(int buttonType)
			{
				if (buttonType==Button_Main&&func!=nullptr)
					func(funcData,currentState);
			}
			
			virtual void Show(Posize &lmt)
			{
				Win->RenderFillRect(lmt,ThemeColor(ButtonColor[currentState][stat]));
				Win->RenderDrawText(Text[currentState],gPS,lmt,textMode,ThemeColor(TextColor[currentState]));
				Win->Debug_DisplayBorder(gPS);
			}
			
		public:
			void SetTextColor(bool state01,const RGBA &co)
			{
				TextColor[state01]=!co?ThemeColorMT[0]:co;
				Win->SetPresentArea(CoverLmt);
			}
			
			void SetButtonColor(bool state01,int p,const RGBA &co)
			{
				if (InRange(p,0,2))
				{
					ButtonColor[state01][p]=!co?(state01?ThemeColorM[p*2+1]:ThemeColorBG[p*2+1]):co;
					Win->SetPresentArea(CoverLmt);
				}
				else PUI_DD[1]<<"SetButtonColor error: p "<<p<<" is not in range[0,2]"<<endl;
			}
			
			inline void SetButtonText(bool state01,const string &_text)
			{
				Text[state01]=_text;
				Win->SetPresentArea(CoverLmt);
			}
			
			inline void SetTextMode(int mode)
			{
				textMode=mode;
				Win->SetPresentArea(CoverLmt);
			}
			
			inline T& GetFuncData()
			{return funcData;}
			
			inline void SetFunc(void (*_func)(T&,bool),const T &_funcdata)
			{
				func=_func;
				funcData=_funcdata;
			}
			
			void SwitchState(bool state01,bool trigerFunc=0)
			{
				if (state01==currentState)
					return;
				currentState=state01;
				Win->SetPresentArea(CoverLmt);
				if (trigerFunc)
					if (func!=nullptr)
						func(funcData,currentState);
			}
			
			TwoStateButton(const WidgetID &_ID,Widgets *_fa,const Posize &_rPS,const string &_text0,const string &_text1,void (*_func)(T&,bool),const T &_funcdata)
			:BaseButton(_ID,WidgetType_TwoStateButton,_fa,_rPS),Text{_text0,_text1},func(_func),funcData(_funcdata) {}
			
			TwoStateButton(const WidgetID &_ID,Widgets *_fa,PosizeEX *psex,const string &_text0,const string &_text1,void (*_func)(T&,bool),const T &_funcdata)
			:BaseButton(_ID,WidgetType_TwoStateButton,_fa,psex),Text{_text0,_text1},func(_func),funcData(_funcdata) {}
			
			TwoStateButton(const WidgetID &_ID,Widgets *_fa,const Posize &_rPS,const string &_text0,const string &_text1)
			:BaseButton(_ID,WidgetType_TwoStateButton,_fa,_rPS),Text{_text0,_text1} {}
			
			TwoStateButton(const WidgetID &_ID,Widgets *_fa,PosizeEX *psex,const string &_text0,const string &_text1)
			:BaseButton(_ID,WidgetType_TwoStateButton,_fa,psex),Text{_text0,_text1} {}
	};
	
	template <class T> class ShapedPictureButton:public BaseButton
	{
		protected:
			string Text;
			SDL_Texture *pic[3]{NULL,NULL,NULL};//this pic should have the same size with rPS
			SDL_Surface *sur0=NULL;//mask
			bool AutoDeletePic[3]{0,0,0},
				 AutoDeleteSur0=0,
//				 OnlyOnePic=1,//only use sur0,pic0,pic1 and 2 auto create;(not usable yet)
				 ThroughBlankPixel=1;
			int ThroughLmtValue=0;//rgba.a not greater than this will through
			void (*func)(T&,ShapedPictureButton <T>*)=NULL;
			T funcData;
			RGBA TextColor=ThemeColorMT[0];
		
			virtual bool InButton(const Point &pos)
			{return CoverLmt.In(pos)&&(!ThroughBlankPixel||GetSDLSurfacePixel(sur0,pos-gPS.GetLU()).a>ThroughLmtValue);}
		
			virtual void TriggerButtonFunction(int buttonType)
			{
				if (buttonType==Button_Main&&func!=nullptr)
					func(funcData,this);
			}
		
			virtual void Show(Posize &lmt)//So short ^v^
			{
				Win->RenderCopyWithLmt(pic[stat],gPS,lmt);
				Win->RenderDrawText(Text,gPS,lmt,0,ThemeColor(TextColor));

				Win->Debug_DisplayBorder(gPS);
			}
			
		public:
			inline void SetTextColor(const RGBA &co)
			{
				TextColor=!co?ThemeColorMT[0]:co;
				Win->SetPresentArea(CoverLmt);
			}
			
			void SetMaskPic(SDL_Surface *sur,bool AutoDelete,bool UseThisSurAsPic=0)
			{
				if (sur==NULL) return;
				if (AutoDeleteSur0)
					SDL_FreeSurface(sur0);
				sur0=sur;
				AutoDeleteSur0=AutoDelete;
				if (UseThisSurAsPic)
					PUI_DD[2]<<"ShapedPictureButton: UseThisSurAsPic is not usable yet"<<endl;
			}
			
			void SetButtonPic(int p,SDL_Texture *tex,bool AutoDelete)
			{
				if (InRange(p,0,2))
				{
					if (AutoDeletePic[p])
						SDL_DestroyTexture(pic[p]);
					pic[p]=tex;
					AutoDeletePic[p]=AutoDelete;
					Win->SetPresentArea(CoverLmt);
				}
				else PUI_DD[1]<<"ShapedPictureButton: SetButtonPic: p "<<p<<" is not in Range[0,2]"<<endl;
			}
			
			inline T& GetFuncData()
			{return funcData;}
			
			inline void SetFunc(void (*_func)(T&,ShapedPictureButton<T>*),const T &_funcdata)
			{
				func=_func;
				funcData=_funcdata;
			}
			
			inline void SetText(const string &_text)
			{
				Text=_text;
				Win->SetPresentArea(CoverLmt);
			}
			
		 	inline void SetThroughBlankPixel(int x=-1)//x==-1 means it won't through any pixel
			{
				if (x==-1)
					ThroughBlankPixel=0;
				else ThroughBlankPixel=1,ThroughLmtValue=x;
			}
			
			virtual ~ShapedPictureButton()
			{
				for (int i=0;i<=2;++i)
					if (AutoDeletePic[i])
						SDL_DestroyTexture(pic[i]);
				if (AutoDeleteSur0)
					SDL_FreeSurface(sur0);
			}
			
			ShapedPictureButton(const WidgetID &_ID,Widgets *_fa,const Posize &_rPS,void (*_func)(T&,ShapedPictureButton<T>*),const T &_funcdata)
			:BaseButton(_ID,WidgetType_ShapedPictureButton,_fa,_rPS),func(_func),funcData(_funcdata) {}
			
			ShapedPictureButton(const WidgetID &_ID,Widgets *_fa,PosizeEX *psex,void (*_func)(T&,ShapedPictureButton<T>*),const T &_funcdata)
			:BaseButton(_ID,WidgetType_ShapedPictureButton,_fa,psex),func(_func),funcData(_funcdata) {}
			
			ShapedPictureButton(const WidgetID &_ID,Widgets *_fa,const Posize &_rPS)
			:BaseButton(_ID,WidgetType_ShapedPictureButton,_fa,_rPS) {}
			
			ShapedPictureButton(const WidgetID &_ID,Widgets *_fa,PosizeEX *psex)
			:BaseButton(_ID,WidgetType_ShapedPictureButton,_fa,psex) {}
	};
	
	template <class T> class AxisController:public Widgets
	{
		protected:
			
			
		public:
			
			
	};
	
	template <class T> class AxisDialButton:public Widgets
	{
		protected:
			
			
		public:
			
			
	};
	
	class DragableLayer:public Widgets
	{
		protected:
			int DragMode=1;//0:Disable 1:MouseLeftClick 2:MouseRightClick 3:MouseLeftDoubleClick
			bool Draging=0;
			RGBA LayerColor=RGBA_NONE;
			Uint32 TimerInterval=12;
			SDL_TimerID IntervalTimerID=0;
			Point PinPt;
			
			void SetTimerOnOff(bool on)
			{
				if ((IntervalTimerID!=0)==on) return;
				if (on) IntervalTimerID=SDL_AddTimer(TimerInterval,PUI_UpdateTimer,new PUI_UpdateTimerData(this,-1));
				else SDL_RemoveTimer(IntervalTimerID),IntervalTimerID=0;
			}
			
			virtual void CheckEvent(const PUI_Event *event)
			{
				if (event->type==PUI_EVENT_UpdateTimer)//??
					if (event->UserEvent()->data1==this)
					{
						if (!Win->IsKeyboardFocused())
							SetTimerOnOff(0),Draging=0;
						Point pt;
						int m=PUI_GetGlobalMouseState(pt);
						if (Win->GetScalePercent()!=1)
							pt=pt/Win->GetScalePercent();
						if ((DragMode==1||DragMode==3)&&!(m&PUI_MouseEvent::Mouse_Left)||DragMode==2&&!(m&PUI_MouseEvent::Mouse_Right))
							SetTimerOnOff(0),Draging=0;
						else Win->SetWindowPos(pt-PinPt);
						Win->StopSolveEvent();
					}
			}
			
			virtual void CheckPos(const PUI_PosEvent *event,int mode)
			{
				if (IntervalTimerID!=0||NotInSet(Win->GetOccupyPosWg(),nullptr,this))
					return;
				else if (mode&(PosMode_Default|PosMode_Occupy))
					switch (event->posType)
					{
						case PUI_PosEvent::Pos_Down:
							if (!Draging&&event->button==PUI_PosEvent::Button_MainClick&&(DragMode==1||DragMode==3&&event->clicks==2)||DragMode==2&&event->button==PUI_PosEvent::Button_SubClick)
							{
								PinPt=event->pos;
								if (event->type==event->Event_MouseEvent)
									SetTimerOnOff(1);
								else Win->SetOccupyPosWg(this);
								Draging=1;
								Win->StopSolvePosEvent();
							}
							break;
						case PUI_PosEvent::Pos_Up:
							if (Draging)
							{
								if (event->type==event->Event_MouseEvent)
									SetTimerOnOff(0);
								else Win->SetOccupyPosWg(NULL);
								Draging=0;
							}
							Win->StopSolvePosEvent();
							break;
						case PUI_PosEvent::Pos_Motion:
							if (Draging)
							{
								Win->SetWindowPos(event->pos-PinPt+Win->GetWinPS().GetLU());
								Win->StopSolveEvent();
							}
							break;
					}
			}
			
			virtual void Show(Posize &lmt)
			{
				Win->RenderFillRect(lmt,ThemeColor(LayerColor));
				Win->Debug_DisplayBorder(gPS);
			}
			
		public:
			void SetLayerColor(const RGBA &co)
			{
				LayerColor=co;
				Win->SetPresentArea(CoverLmt);
			}
			
			inline void SetDragMode(int mode)
			{DragMode=mode;}
			
			~DragableLayer()
			{
				SetTimerOnOff(0);
				Draging=0;
			}
			
			DragableLayer(const WidgetID &_ID,Widgets *_fa,const Posize &_rPS)
			:Widgets(_ID,WidgetType_DragableLayer,_fa,_rPS) {}
			
			DragableLayer(const WidgetID &_ID,Widgets *_fa,PosizeEX *psex)
			:Widgets(_ID,WidgetType_DragableLayer,_fa,psex) {}
	};
	
	template <class T> class DrawerLayer:public Widgets
	{
		public:
			enum
			{
				Extend_Right=0,
				Extend_Up,
				Extend_Left,
				Extend_Down
			};
		
		protected:
			bool Extended=0;
			unsigned int ExtendDirection,
						 UpdateInterval=20,
						 FullExtendTime=200;
			double ExtendPercent=0,//0~1
				   MinPercent=0;
			SDL_TimerID ExtendTimerID=0;
			void (*func)(T&,double,DrawerLayer<T>*)=nullptr;
			T funcdata;
			Layer *DrawerArea=nullptr;
			
			class PosizeEX_DrawerLayer:public PosizeEX
			{
				friend class DrawerLayer;
				protected: 
					DrawerLayer *tar=NULL;
					
				public:
					virtual void GetrPS(Posize &ps)
					{
						if (nxt!=NULL) nxt->GetrPS(ps);
						switch (tar->ExtendDirection)
						{
							case DrawerLayer::Extend_Right:	ps=Posize(0,0,tar->ExtendPercent*tar->rPS.w,tar->rPS.h);	break;
							case DrawerLayer::Extend_Up:	ps=Posize(0,(1-tar->ExtendPercent)*tar->rPS.h,tar->rPS.w,tar->ExtendPercent*tar->rPS.h);	break;
							case DrawerLayer::Extend_Left:	ps=Posize((1-tar->ExtendPercent)*tar->rPS.w,0,tar->ExtendPercent*tar->rPS.w,tar->rPS.h);	break;
							case DrawerLayer::Extend_Down:	ps=Posize(0,0,tar->rPS.w,tar->ExtendPercent*tar->rPS.h);	break;
						}
					}
					
					PosizeEX_DrawerLayer(DrawerLayer *_tar)
					:tar(_tar) {}
			};
			
			void SetTimerOnOff(bool on)
			{
				if ((ExtendTimerID!=0)==on) return;
				if (on) ExtendTimerID=SDL_AddTimer(UpdateInterval,PUI_UpdateTimer,new PUI_UpdateTimerData(this,-1));
				else
				{
					SDL_RemoveTimer(ExtendTimerID);
					ExtendTimerID=0;
				}
			}
			
			virtual void CheckEvent(const PUI_Event *event)
			{
				if (event->type==PUI_EVENT_UpdateTimer)
					if (event->UserEvent()->data1==this&&ExtendTimerID!=0)
					{
						double delta=UpdateInterval*1.0/FullExtendTime/(1-MinPercent);
						ExtendPercent=EnsureInRange(Extended?ExtendPercent+delta:ExtendPercent-delta,MinPercent,1);
						if (Extended&&ExtendPercent==1||!Extended&&ExtendPercent==MinPercent)
							SetTimerOnOff(0);
						if (func!=nullptr)
							func(funcdata,ExtendPercent,this);
						Win->StopSolveEvent();
						Win->SetNeedUpdatePosize();
					}
			}
			
		public:
			inline void SetDrawerColor(const RGBA &co)
			{DrawerArea->SetLayerColor(co);}
			
			inline Layer* Drawer()
			{return DrawerArea;}
			
			inline void SetUpdateIntervalAndFullExtendTime(unsigned int _updateinterval,unsigned int _fullextendtime)
			{
				UpdateInterval=_updateinterval;
				FullExtendTime=_fullextendtime;
			}
			
			inline void SwitchOnOff(bool on)//??
			{
				if (Extended==on) return;
				SetTimerOnOff(1);
				Extended=on;
			}
			
			inline bool GetOnOff() const
			{return Extended;}
			
			inline void SetMinPercent(double per)
			{
				if (0<=per&&per<1)
				{
					MinPercent=per;
					if (ExtendPercent<MinPercent)
						ExtendPercent=MinPercent,
						Win->SetNeedUpdatePosize();
				}
			}
			
			inline void SetFunc(void (*_func)(T&,double,DrawerLayer<T>*),const T &_funcdata)
			{
				func=_func;
				funcdata=_funcdata;
			}
			
			inline void SetFunc(void (*_func)(T&,double,DrawerLayer<T>*))
			{func=_func;}
			
			inline void SetFuncdata(const T &_funcdata)
			{funcdata=_funcdata;}
			
			~DrawerLayer()
			{
				SetTimerOnOff(0);
			}
			
			DrawerLayer(const WidgetID &_ID,Widgets *_fa,const Posize &_rPS,unsigned int extenddirection,void (*_func)(T&,double,DrawerLayer<T>*),const T &_funcdata)
			:Widgets(_ID,WidgetType_DrawerLayer,_fa,_rPS),ExtendDirection(extenddirection),func(_func),funcdata(_funcdata)
			{DrawerArea=new Layer(0,this,new PosizeEX_DrawerLayer(this));}
			
			DrawerLayer(const WidgetID &_ID,Widgets *_fa,PosizeEX *psex,unsigned int extenddirection,void (*_func)(T&,double,DrawerLayer<T>*),const T &_funcdata)
			:Widgets(_ID,WidgetType_DrawerLayer,_fa,psex),ExtendDirection(extenddirection),func(_func),funcdata(_funcdata)
			{DrawerArea=new Layer(0,this,new PosizeEX_DrawerLayer(this));}
			
			DrawerLayer(const WidgetID &_ID,Widgets *_fa,const Posize &_rPS,unsigned int extenddirection)
			:Widgets(_ID,WidgetType_DrawerLayer,_fa,_rPS),ExtendDirection(extenddirection)
			{DrawerArea=new Layer(0,this,new PosizeEX_DrawerLayer(this));}
			
			DrawerLayer(const WidgetID &_ID,Widgets *_fa,PosizeEX *psex,unsigned int extenddirection)
			:Widgets(_ID,WidgetType_DrawerLayer,_fa,psex),ExtendDirection(extenddirection)
			{DrawerArea=new Layer(0,this,new PosizeEX_DrawerLayer(this));}
	};
	
	class LargeLayerWithScrollBar;
	class PosizeEX_InLarge:public PosizeEX
	{
		friend class LargeLayerWithScrollBar;
		protected:
			LargeLayerWithScrollBar *larlay=NULL;
			PosizeEX_InLarge *pre=NULL,*nt=NULL;
			int gap=0,fixedH=0,//if fixedH is 0,means using widgets's heigth
				fullfill=1;//if fullfill < 0, means reserve |fullfill| height after this;
			
		public:
			inline LargeLayerWithScrollBar* GetLargeLayer()
			{return larlay;}
			
			int Fullfill() const
			{return fullfill;}
			
			bool NeedFullFill() const
			{return fullfill==1&&!nt||fullfill<0;}//??
			
			void SetShowPos(int y);

			virtual void GetrPS(Posize &ps);
			
			~PosizeEX_InLarge();

			PosizeEX_InLarge(LargeLayerWithScrollBar *_larlay,int _gap,int _fullfill,int _fixedH);
	};
	
	class LargeLayerWithScrollBar:public Widgets
	{
		friend PosizeEX_InLarge;
		protected:
			enum
			{
				Stat_NoFocus=0,
				Stat_FocusRightBar=1,
				Stat_FocusButtomBar=2,
				Stat_FocusRightChunk=3,
				Stat_FocusButtomChunk=4,
				Stat_DownScrollRight=5,
				Stat_DownScrollButtom=6,
				Stat_FocusLargeLayerNotBar=7,
				Stat_DownFingerMoving=8,
				Stat_FocusRightMinus=9,
				Stat_DownRightMinus=10,
				Stat_FocusRightPlus=11,
				Stat_DownRightPlus=12,
				Stat_FocusButtomMinus=13,
				Stat_DownButtomMinus=14,
				Stat_FocusButtomPlus=15,
				Stat_DownButtomPlus=16,
			}stat=Stat_NoFocus;
			Posize ckPs_Ri,ckPs_Bu,ckBGps_Ri,ckBGps_Bu;//Right,Down
			bool EnableRightBar=0,EnableButtomBar=0;//Remember to adjust after changing the LargeAreaPosize!
			Layer* LargeAreaLayer=NULL;
			Posize largeAreaPS;
			Point ckCentreDelta={0,0};//Current click point to Chunk centre
			int ScrollBarWidth=16,
				MinBarLength=5,
				MinLargeAreaW=0,//when largearea is small than this, it is enabled
				MinLargeAreaH=0,//0 means no limit
				CustomWheelSensibility=0;//0 means use global
			bool AutoHideScrollBar=0,
				 EnableBarGap=1,
				 EnableBarSideButton=0,//I don't want to realize it immediatly...
				 EnableSmoothWheelScroll=0,
				 SmoothScrollData_Direction_y=1,
				 EnableMovingAgencyRender=0;//Experimental Features; if Enabled,TheBackgroundColor should be set no transparency. 
//				 CheckPosTurnFlag=0;
			int SmoothScrollData_Delta=0,
				SmoothScrollData_LeftCnt=0;
			SDL_TimerID SmoothScrollTimerID=0;
			bool EnableInertialMoving=1;
//				 InertialMovingDeltaDirection_y=1;
			double CurrentMovingSpeed_X=0,//pixels per ms
				   CurrentMovingSpeed_Y=0;
			SDL_TimerID InertialMovingTimerID=0;
			PosizeEX_InLarge *LastInLargeWidgetsPsEx=NULL;//if not NULL,enable LargeLayer for Views,it will sum all widgets' height and use it as LargeLayer's height. And some of the feature such as horizon scroll will be disabled //??
			RGBA BackgroundBarColor[4]={ThemeColorTS[1],ThemeColorTS[3],ThemeColorTS[5],ThemeColorTS[7]},
				 BorderColor[4]={ThemeColorM[1],ThemeColorM[3],ThemeColorM[5],ThemeColorM[7]},
				 ChunkColor[4]={ThemeColorM[1],ThemeColorM[3],ThemeColorM[5],ThemeColorM[7]},
				 SideButtonColor[4]={ThemeColorM[0],ThemeColorM[2],ThemeColorM[4],ThemeColorM[6]};
			
			class PosizeEX_LargeLayer:public PosizeEX
			{
				protected: 
					LargeLayerWithScrollBar *tar=NULL;
					
				public:
					virtual void GetrPS(Posize &ps)
					{
						if (nxt!=NULL) nxt->GetrPS(ps);
						ps=tar->largeAreaPS;
					}
					
					PosizeEX_LargeLayer(LargeLayerWithScrollBar *_tar)
					:tar(_tar) {}
			};
			
			void UpdateChunkPs()
			{
				if (EnableRightBar)
				{
					ckBGps_Ri={gPS.x2()-ScrollBarWidth+1,gPS.y,ScrollBarWidth,gPS.h};
					if (EnableBarGap)
						ckPs_Ri=ckBGps_Ri.Shrink(2);
					else ckPs_Ri=ckBGps_Ri;
					ckPs_Ri.h=EnsureInRange((ckBGps_Ri.h-EnableBarSideButton*2*ScrollBarWidth)*gPS.h*1.0/largeAreaPS.h,MinBarLength,ckBGps_Ri.h-EnableBarGap*4-EnableBarSideButton*2*ScrollBarWidth);
					ckPs_Ri.y=(long long)(ckBGps_Ri.h-EnableBarSideButton*2*ScrollBarWidth-EnableBarGap*4-ckPs_Ri.h)*(-largeAreaPS.y)*1.0/(largeAreaPS.h-gPS.h+EnableButtomBar*ScrollBarWidth)+ckBGps_Ri.y+EnableBarGap*2+EnableBarSideButton*ScrollBarWidth;
				}
				else ckPs_Ri=ckBGps_Ri=ZERO_POSIZE;

				if (EnableButtomBar)
				{
					ckBGps_Bu={gPS.x,gPS.y2()-ScrollBarWidth+1,EnableRightBar?gPS.w-ScrollBarWidth:gPS.w,ScrollBarWidth};
					if (EnableBarGap)
						ckPs_Bu=ckBGps_Bu.Shrink(2);
					else ckPs_Bu=ckBGps_Bu;
					ckPs_Bu.w=EnsureInRange((ckBGps_Bu.w-EnableBarSideButton*2*ScrollBarWidth)*gPS.w*1.0/largeAreaPS.w,MinBarLength,ckBGps_Bu.w-EnableBarGap*4-EnableBarSideButton*2*ScrollBarWidth);
					ckPs_Bu.x=(long long)(ckBGps_Bu.w-EnableBarSideButton*2*ScrollBarWidth-EnableBarGap*4-ckPs_Bu.w)*(-largeAreaPS.x)*1.0/(largeAreaPS.w-gPS.w+EnableRightBar*ScrollBarWidth)+ckBGps_Bu.x+EnableBarGap*2+EnableBarSideButton*ScrollBarWidth;			
				}
				else ckPs_Bu=ckBGps_Bu=ZERO_POSIZE;
			}
			
			virtual void CalcPsEx()
			{
				if (PsEx!=NULL)
					PsEx->GetrPS(rPS);
				Posize lastPs=gPS;
				if (fa!=NULL)
					gPS=rPS+fa->GetgPS();
				else gPS=rPS;
				if (!(lastPs==gPS))
					Win->SetPresentArea(lastPs|gPS);
				CoverLmt=gPS&GetFaCoverLmt();
				ResizeLL(MinLargeAreaW==0?0:max(rPS.w,MinLargeAreaW),MinLargeAreaH==0?0:max(rPS.h,MinLargeAreaH));
				//LargeArea may not change,but rPS may change,this function will also update large state
			}
			
			virtual void _UpdateWidgetsPosize()
			{
				if (nxtBrother!=NULL)
					UpdateWidgetsPosizeOf(nxtBrother);
				if (!Enabled) return;
				CalcPsEx();
				if (childWg!=NULL)
					UpdateWidgetsPosizeOf(childWg);
				if (LastInLargeWidgetsPsEx!=NULL)//??
					ResizeLL(rPS.w,max(rPS.h,LastInLargeWidgetsPsEx->GetWidgetsRawrPS().y2()));
			}
			
		public:
			void SetViewPort(int ope,double val)//ope:: 1:SetPosX 2:..Y 3:SetPosPercent 4:..Y 5:MoveX 6:..Y
			{
				if ((ope==5||ope==6)&&val==0) return;
				switch (ope)
				{
					case 1: largeAreaPS.x=-val;	break;
					case 2: largeAreaPS.y=-val;	break;
					case 3:	largeAreaPS.x=(rPS.w-EnableRightBar*ScrollBarWidth-largeAreaPS.w)*val;	break;
					case 4: largeAreaPS.y=(rPS.h-EnableButtomBar*ScrollBarWidth-largeAreaPS.h)*val;	break;
					case 5:	largeAreaPS.x-=val;	break;
					case 6:	largeAreaPS.y-=val;	break;
					default:
						PUI_DD[2]<<"SetViewPort ope "<<ope<<" is not in Range[1,6]"<<endl;
						return;
				}
				if (rPS.w==largeAreaPS.w) largeAreaPS.x=0;
				else largeAreaPS.x=EnsureInRange(largeAreaPS.x,rPS.w-EnableRightBar*ScrollBarWidth-largeAreaPS.w,0);
				if (rPS.h==largeAreaPS.h) largeAreaPS.y=0;
				else largeAreaPS.y=EnsureInRange(largeAreaPS.y,rPS.h-EnableButtomBar*ScrollBarWidth-largeAreaPS.h,0);
				UpdateChunkPs();
				Win->SetNeedUpdatePosize();
				Win->SetPresentArea(CoverLmt);
			}
			
			void ResizeLL(int _ww,int _hh)//_ww or _hh is 0 means not change this; 
			{
//				PUI_DD[0]<<"ResizeLL "<<_ww<<" "<<_hh<<endl;
				if (_ww!=0) largeAreaPS.w=EnsureInRange(_ww,rPS.w,1e9);//?? +1 -1??
				else largeAreaPS.w=EnsureInRange(largeAreaPS.w,rPS.w,1e9);
				if (_hh!=0) largeAreaPS.h=EnsureInRange(_hh,rPS.h,1e9);
				else largeAreaPS.h=EnsureInRange(largeAreaPS.h,rPS.h,1e9);
				EnableButtomBar=largeAreaPS.w>rPS.w;
				EnableRightBar=largeAreaPS.h>rPS.h;
				if (rPS.w==largeAreaPS.w) largeAreaPS.x=0;
				else largeAreaPS.x=EnsureInRange(largeAreaPS.x,rPS.w-EnableRightBar*ScrollBarWidth-largeAreaPS.w,0);
				if (rPS.h==largeAreaPS.h) largeAreaPS.y=0;
				else largeAreaPS.y=EnsureInRange(largeAreaPS.y,rPS.h-EnableButtomBar*ScrollBarWidth-largeAreaPS.h,0);
				UpdateChunkPs();
				Win->SetNeedUpdatePosize();
			}
		
		protected:
			void SetInertialMoving(bool enable)
			{
				if ((InertialMovingTimerID!=0)==enable) return;
//				DD[3]<<"SetInertialMoving "<<enable<<endl;
//				DD[3]<<"#speed "<<CurrentMovingSpeed_X<<" "<<CurrentMovingSpeed_Y<<endl;
				if (enable)
					InertialMovingTimerID=SDL_AddTimer(PUI_Parameter_LargeLayerInertialScrollInterval,PUI_UpdateTimer,new PUI_UpdateTimerData(this,-1,NULL,CONST_Ptr_0));
				else
				{
					SDL_RemoveTimer(InertialMovingTimerID);
					InertialMovingTimerID=0;
					CurrentMovingSpeed_X=CurrentMovingSpeed_Y=0;
				}
			}
			
			void SetSmoothScrollBar(bool IsY,int delta)//On testing...
			{
				SmoothScrollData_Direction_y=IsY;
				SmoothScrollData_Delta+=delta;
				SmoothScrollData_LeftCnt=10;
				if (SmoothScrollTimerID==0)
			 		SmoothScrollTimerID=SDL_AddTimer(10,PUI_UpdateTimer,new PUI_UpdateTimerData(this,-1));//Careful! There exist a bug that the widget is deconstructed but the timer is still running(Use a class to manage the widgets' pointer may be a good way)
			}
			
			void SmoothShiftOnce()
			{
				int d=SmoothScrollData_Delta/SmoothScrollData_LeftCnt;
				if (d==0)
					d=SmoothScrollData_Delta;
				SetViewPort(SmoothScrollData_Direction_y?6:5,d);
				SmoothScrollData_Delta-=d;
				SmoothScrollData_LeftCnt--;
				if (SmoothScrollData_Delta==0||SmoothScrollData_LeftCnt<=0)
					if (SmoothScrollTimerID!=0)
					{
						SDL_RemoveTimer(SmoothScrollTimerID);
						SmoothScrollTimerID=0;
					}
			}
			
			virtual void CheckEvent(const PUI_Event *event)
			{
				if (event->type==PUI_Event::Event_WheelEvent)
				{
					SetInertialMoving(0);
					switch (stat)
					{
						case Stat_NoFocus: break;
						case Stat_FocusRightBar:	case Stat_FocusRightChunk:
							if (EnableRightBar)
							{
								if (EnableSmoothWheelScroll) SetSmoothScrollBar(1,-event->WheelEvent()->dy*(CustomWheelSensibility!=0?CustomWheelSensibility:PUI_Parameter_WheelSensibility));
								else SetViewPort(6,-event->WheelEvent()->dy*(CustomWheelSensibility!=0?CustomWheelSensibility:PUI_Parameter_WheelSensibility));
								Win->StopSolveEvent();
							}
							break;
						case Stat_FocusButtomBar:	case Stat_FocusButtomChunk:
							if (EnableButtomBar)
							{
								if (EnableSmoothWheelScroll) SetSmoothScrollBar(0,-event->WheelEvent()->dy*(CustomWheelSensibility!=0?CustomWheelSensibility:PUI_Parameter_WheelSensibility));
								else SetViewPort(5,-event->WheelEvent()->dy*(CustomWheelSensibility!=0?CustomWheelSensibility:PUI_Parameter_WheelSensibility));
								Win->StopSolveEvent();
							}
							break;
						case Stat_FocusLargeLayerNotBar:
							if (EnableRightBar)
							{
								if (EnableSmoothWheelScroll)
									SetSmoothScrollBar(1,-event->WheelEvent()->dy*(CustomWheelSensibility!=0?CustomWheelSensibility:PUI_Parameter_WheelSensibility)),
									SmoothShiftOnce();
								else SetViewPort(6,-event->WheelEvent()->dy*(CustomWheelSensibility!=0?CustomWheelSensibility:PUI_Parameter_WheelSensibility));
								Win->StopSolveEvent();
							}
							else if (EnableButtomBar)
							{
								if (EnableSmoothWheelScroll)
									SetSmoothScrollBar(0,-event->WheelEvent()->dy*(CustomWheelSensibility!=0?CustomWheelSensibility:PUI_Parameter_WheelSensibility)),
									SmoothShiftOnce();
								else SetViewPort(5,-event->WheelEvent()->dy*(CustomWheelSensibility!=0?CustomWheelSensibility:PUI_Parameter_WheelSensibility));
								Win->StopSolveEvent();
							}
							break;
						default :
							PUI_DD[2]<<"LargeLayerWithScrollBar "<<IDName<<":Wheel Such state "<<stat<<" is not considered yet!"<<endl;
					}
				}
				else if (event->type==PUI_EVENT_UpdateTimer)
					if (event->UserEvent()->data1==this)
						if (event->UserEvent()->data2==CONST_Ptr_0)
						{
							int deltaX=EnsureInRange(CurrentMovingSpeed_X*PUI_Parameter_LargeLayerInertialScrollInterval,-PUI_Parameter_LargeLayerInertialScrollEachBound,PUI_Parameter_LargeLayerInertialScrollEachBound),
								deltaY=EnsureInRange(CurrentMovingSpeed_Y*PUI_Parameter_LargeLayerInertialScrollInterval,-PUI_Parameter_LargeLayerInertialScrollEachBound,PUI_Parameter_LargeLayerInertialScrollEachBound);
							SetViewPort(5,-deltaX);
							SetViewPort(6,-deltaY);
							CurrentMovingSpeed_X*=exp(-PUI_Parameter_LargeLayerInertialScrollFactor*PUI_Parameter_LargeLayerInertialScrollInterval);
							CurrentMovingSpeed_Y*=exp(-PUI_Parameter_LargeLayerInertialScrollFactor*PUI_Parameter_LargeLayerInertialScrollInterval);
							if (fabs(CurrentMovingSpeed_X)<=PUI_Parameter_TouchSpeedDiscardLimit&&fabs(CurrentMovingSpeed_Y)<=PUI_Parameter_TouchSpeedDiscardLimit
								||(CurrentMovingSpeed_X>0&&GetScrollBarPercentX()==0||CurrentMovingSpeed_X<0&&GetScrollBarPercentX()==1)
								&&(CurrentMovingSpeed_Y>0&&GetScrollBarPercentY()==0||CurrentMovingSpeed_Y<0&&GetScrollBarPercentY()==1))
								SetInertialMoving(0);
						}
						else if (SmoothScrollData_LeftCnt>0)
						{
							SmoothShiftOnce();
							Win->StopSolveEvent();
						}
			}
			
			virtual void CheckPos(const PUI_PosEvent *event,int mode)
			{
				if (mode&PosMode_LoseFocus)
				{
					if (stat!=Stat_NoFocus&&Win->GetOccupyPosWg()!=this)
						if (!Win->IsPosFocused()||!CoverLmt.In(event->pos)||(0)||mode&PosMode_ForceLoseFocus)
						{
							stat=Stat_NoFocus;
							RemoveNeedLoseFocus();
							if (EnableRightBar)
								Win->SetPresentArea(ckBGps_Ri&CoverLmt);
							if (EnableButtomBar)
								Win->SetPresentArea(ckBGps_Bu&CoverLmt);
							if (mode&PosMode_Occupy)
								Win->SetOccupyPosWg(NULL);
						}
				}
				else if (mode&(PosMode_Pre|PosMode_Nxt|PosMode_Occupy))
				{
					if (event->type==PUI_Event::Event_TouchEvent&&(mode&(/*PosMode_Nxt???*/PosMode_Pre|PosMode_Occupy))&&
						stat!=Stat_DownScrollButtom&&stat!=Stat_DownScrollRight&&
						(!ckBGps_Ri.In(event->pos)&&!ckBGps_Bu.In(event->pos)||Win->GetOccupyPosWg()==this))//It is not beautiful...
					{
						PUI_TouchEvent *touch=event->TouchEvent();
						if (touch->gesture==touch->Ges_ShortClick||touch->gesture==touch->Ges_MultiClick)
						{
							stat=Stat_DownFingerMoving;
							SetNeedLoseFocus();
							SetInertialMoving(0);
							return;
						}
						else if (stat!=Stat_DownFingerMoving) return;
						else if (touch->gesture==touch->Ges_Moving||touch->gesture==touch->Ges_MultiMoving)//??
						{
							if (Win->GetOccupyPosWg()==this||touch->timeStamp-PUI_TouchEvent::Finger::gestureStartTime>=PUI_Parameter_MultiClickGaptimeOfLargeLayer)//??
							{
								Win->SetOccupyPosWg(this);
								Win->StopSolvePosEvent();
							}
							if (EnableRightBar)
								SetViewPort(6,-touch->delta.y);
							if (EnableButtomBar)
								SetViewPort(5,-touch->delta.x);
							return;
						}
						else if (touch->gesture==touch->Ges_End)
						{
							if (Win->GetOccupyPosWg()==this)//??
							{
								stat=Stat_NoFocus;
								Win->SetOccupyPosWg(NULL);
								Win->StopSolvePosEvent();
							}
							CurrentMovingSpeed_X=touch->speedX;
							CurrentMovingSpeed_Y=touch->speedY;
							if (EnableInertialMoving&&!(CurrentMovingSpeed_X==0&&CurrentMovingSpeed_Y==0))
								SetInertialMoving(1);
							return;
						}
					}
					
					if (mode&(PosMode_Pre|PosMode_Occupy))
						switch (event->posType)
						{
							case PUI_PosEvent::Pos_Down:
								if (event->button==PUI_PosEvent::Button_MainClick)
									if (ckBGps_Ri.In(event->pos))
									{
										stat=Stat_DownScrollRight;
										Win->SetOccupyPosWg(this);
										Win->StopSolvePosEvent();
										Win->SetPresentArea(ckBGps_Ri&CoverLmt);
										SetInertialMoving(0);
										if (ckPs_Ri.In(event->pos))
											ckCentreDelta.y=event->pos.y-ckPs_Ri.midY();
										else
										{
											ckCentreDelta.y=0;
											SetViewPort(4,(event->pos.y-ckPs_Ri.h/2-ckBGps_Ri.y-EnableBarGap*2-EnableBarSideButton*ScrollBarWidth)*1.0/(ckBGps_Ri.h-EnableBarGap*4-EnableBarSideButton*2*ScrollBarWidth-ckPs_Ri.h));
										}
									}
									else if (ckBGps_Bu.In(event->pos))
									{
										stat=Stat_DownScrollButtom;
										Win->SetOccupyPosWg(this);
										Win->StopSolvePosEvent();
										Win->SetPresentArea(ckBGps_Bu&CoverLmt);
										SetInertialMoving(0);
										if (ckPs_Bu.In(event->pos))
											ckCentreDelta.x=event->pos.x-ckPs_Bu.midX();
										else
										{
											ckCentreDelta.x=0;
											SetViewPort(3,(event->pos.x-ckPs_Bu.w/2-ckBGps_Bu.x-EnableBarGap*2-EnableBarSideButton*ScrollBarWidth)*1.0/(ckBGps_Bu.w-EnableBarGap*4-EnableBarSideButton*2*ScrollBarWidth-ckPs_Bu.w));
										}
									}
								break;
							case PUI_PosEvent::Pos_Up:
								if (stat==Stat_DownScrollRight||stat==Stat_DownScrollButtom)
								{
									Win->SetOccupyPosWg(NULL);
									if (ckBGps_Ri.In(event->pos)) stat=Stat_FocusRightChunk;//Need check enable bar?
									else if (ckPs_Bu.In(event->pos)) stat=Stat_FocusButtomChunk;
									else if (ckBGps_Ri.In(event->pos)) stat=Stat_FocusRightBar;
									else if (ckBGps_Bu.In(event->pos)) stat=Stat_FocusButtomBar;
									else if (gPS.In(event->pos)) stat=Stat_FocusLargeLayerNotBar;
									else stat=Stat_NoFocus;
									Win->StopSolvePosEvent();
									Win->SetPresentArea(ckBGps_Ri&CoverLmt);//??
									Win->SetPresentArea(ckBGps_Bu&CoverLmt);//??
								}
								break;
							case PUI_PosEvent::Pos_Motion:
								if (stat==Stat_DownScrollRight)
									SetViewPort(4,(event->pos.y-ckCentreDelta.y-ckPs_Ri.h/2-ckBGps_Ri.y-EnableBarGap*2-EnableBarSideButton*ScrollBarWidth)*1.0/(ckBGps_Ri.h-EnableBarGap*4-EnableBarSideButton*2*ScrollBarWidth-ckPs_Ri.h)),
									Win->StopSolvePosEvent();
								else if (stat==Stat_DownScrollButtom)
									SetViewPort(3,(event->pos.x-ckCentreDelta.x-ckPs_Bu.w/2-ckBGps_Bu.x-EnableBarGap*2-EnableBarSideButton*ScrollBarWidth)*1.0/(ckBGps_Bu.w-EnableBarGap*4-EnableBarSideButton*2*ScrollBarWidth-ckPs_Bu.w)), 
									Win->StopSolvePosEvent();	
								else
								{
									if (stat==Stat_NoFocus)
										SetNeedLoseFocus();
									bool MouseEventFlagChangeFlag=1;
									int temp_stat=stat;
									if (ckPs_Ri.In(event->pos)) stat=Stat_FocusRightChunk;
									else if (ckPs_Bu.In(event->pos)) stat=Stat_FocusButtomChunk;
									else if (ckBGps_Ri.In(event->pos)) stat=Stat_FocusRightBar;
									else if (ckBGps_Bu.In(event->pos)) stat=Stat_FocusButtomBar;
									else if (stat==Stat_DownFingerMoving) break;
									else stat=Stat_FocusLargeLayerNotBar,MouseEventFlagChangeFlag=0;
									
									if (MouseEventFlagChangeFlag)
										Win->StopSolvePosEvent();
									if (temp_stat!=stat)
									{
										if (EnableRightBar)
											Win->SetPresentArea(ckBGps_Ri&CoverLmt);
										if (EnableButtomBar)
											Win->SetPresentArea(ckBGps_Bu&CoverLmt);
									}
								}			
								break;
						}
				}
			}
			
			virtual void Show(Posize &lmt)
			{
				if (EnableRightBar)
				{
					RGBA colBG,colBorder,colCk,colButton;
					switch (stat)
					{
						case 0:	case 2:	case 4:	case 6:
							colBG=ThemeColor(BackgroundBarColor[0]);
							colBorder=ThemeColor(BorderColor[0]);
							colCk=ThemeColor(ChunkColor[0]);
							colButton=ThemeColor(SideButtonColor[0]);
							break;
						case 1:	case 7:	case 8:
							colBG=ThemeColor(BackgroundBarColor[1]);
							colBorder=ThemeColor(BorderColor[1]);
							colCk=ThemeColor(ChunkColor[1]);
							colButton=ThemeColor(SideButtonColor[1]);
							break;
						case 3:	
							colBG=ThemeColor(BackgroundBarColor[2]);
							colBorder=ThemeColor(BorderColor[2]);
							colCk=ThemeColor(ChunkColor[2]);
							colButton=ThemeColor(SideButtonColor[1]);
							break;
						case 5:	
							colBG=ThemeColor(BackgroundBarColor[3]);
							colBorder=ThemeColor(BorderColor[3]);
							colCk=ThemeColor(ChunkColor[3]);
							colButton=ThemeColor(SideButtonColor[1]);
							break;
						case 9:	
						case 10:
						case 11:
						case 12:
						case 13:
						case 14:
						case 15:
						case 16:
						default:
							PUI_DD[2]<<"LargeLayerWithScrollBar "<<IDName<<":Such state "<<stat<<" is not considered yet!"<<endl;
					}
					Win->RenderFillRect(lmt&ckBGps_Ri,colBG);
					Win->RenderDrawRectWithLimit(ckBGps_Ri,colBorder,lmt);
					Win->RenderFillRect(lmt&ckPs_Ri,colCk);
					if (EnableBarSideButton)
						;
					
					Win->Debug_DisplayBorder(ckBGps_Ri);
				}
				
				if (EnableButtomBar)
				{
					RGBA colBG,colBorder,colCk,colButton;
					switch (stat)
					{
						case 0:	case 1:	case 3:	case 5:
							LargeLayerWithScrollBar_Show_case0:
							colBG=ThemeColor(BackgroundBarColor[0]);
							colBorder=ThemeColor(BorderColor[0]);
							colCk=ThemeColor(ChunkColor[0]);
							colButton=ThemeColor(SideButtonColor[0]);
							break;
						case 2:
							LargeLayerWithScrollBar_Show_case2:
							colBG=ThemeColor(BackgroundBarColor[1]);
							colBorder=ThemeColor(BorderColor[1]);
							colCk=ThemeColor(ChunkColor[1]);
							colButton=ThemeColor(SideButtonColor[1]);
							break;
						case 4:	
							colBG=ThemeColor(BackgroundBarColor[2]);
							colBorder=ThemeColor(BorderColor[2]);
							colCk=ThemeColor(ChunkColor[2]);
							colButton=ThemeColor(SideButtonColor[1]);
							break;
						case 6:	
							colBG=ThemeColor(BackgroundBarColor[3]);
							colBorder=ThemeColor(BorderColor[3]);
							colCk=ThemeColor(ChunkColor[3]);
							colButton=ThemeColor(SideButtonColor[1]);
							break;
						case 7:	case 8:
							if (EnableRightBar) goto LargeLayerWithScrollBar_Show_case0;
							else goto LargeLayerWithScrollBar_Show_case2;
						case 9:	
						case 10:
						case 11:
						case 12:
						case 13:
						case 14:
						case 15:
						case 16:
						default:
							PUI_DD[2]<<"LargeLayerWithScrollBar "<<IDName<<":Such state "<<stat<<" is not considered yet!"<<endl;
					}
					Win->RenderFillRect(lmt&ckBGps_Bu,colBG);
					Win->RenderDrawRectWithLimit(ckBGps_Bu,colBorder,lmt);
					Win->RenderFillRect(lmt&ckPs_Bu,colCk);
					if (EnableBarSideButton)
						;
					
					Win->Debug_DisplayBorder(ckBGps_Bu);
				}
				
				Win->Debug_DisplayBorder(gPS);
				Win->Debug_DisplayBorder(largeAreaPS+gPS);
			}
			
			virtual void _PresentWidgets(Posize lmt)
			{
				if (nxtBrother!=NULL)
					PresentWidgetsOf(nxtBrother,lmt);
				if (Enabled)
				{
					lmt=lmt&gPS;
					if (childWg!=NULL)
						PresentWidgetsOf(childWg,lmt);
					Show(lmt);
					if (DEBUG_EnableWidgetsShowInTurn)
						SDL_Delay(DEBUG_WidgetsShowInTurnDelayTime),
						SDL_RenderPresent(Win->GetSDLRenderer());
				}
			}
			
		public:
			inline LargeLayerWithScrollBar* SetLargeAreaColor(const RGBA &co)
			{
				LargeAreaLayer->SetLayerColor(co);
				return this;
			}
			
			inline LargeLayerWithScrollBar* SetBackgroundBarColor(int p,const RGBA &co)
			{
				if (InRange(p,0,3))
				{
					BackgroundBarColor[p]=!co?ThemeColorTS[p*2+1]:co;
					Win->SetPresentArea(ckBGps_Bu&CoverLmt);
					Win->SetPresentArea(ckBGps_Ri&CoverLmt);
				}
				else PUI_DD[1]<<"SetBackgroundBarColor: p "<<p<<" is not in Range[0,3]"<<endl;
				return this;
			}
			
			inline LargeLayerWithScrollBar* SetBorderColor(int p,const RGBA &co)
			{
				if (InRange(p,0,3))
				{
					BorderColor[p]=!co?ThemeColorM[p*2+1]:co;
					Win->SetPresentArea(ckBGps_Bu&CoverLmt);
					Win->SetPresentArea(ckBGps_Ri&CoverLmt);
				}
				else PUI_DD[1]<<"SetBorderColor: p "<<p<<" is not in Range[0,3]"<<endl;
				return this;
			}
			
			inline LargeLayerWithScrollBar* SetChunkColor(int p,const RGBA &co)
			{
				if (InRange(p,0,3))
				{
					ChunkColor[p]=!co?ThemeColorM[p*2+1]:co;
					Win->SetPresentArea(ckBGps_Bu&CoverLmt);
					Win->SetPresentArea(ckBGps_Ri&CoverLmt);
				}
				else PUI_DD[1]<<"ChunkColor: p "<<p<<" is not in Range[0,3]"<<endl;
				return this;
			}
			
			inline LargeLayerWithScrollBar* SetSideButtonColor(int p,const RGBA &co)
			{
				if (InRange(p,0,3))
				{
					SideButtonColor[p]=!co?ThemeColorM[p*2]:co;
					Win->SetPresentArea(ckBGps_Bu&CoverLmt);
					Win->SetPresentArea(ckBGps_Ri&CoverLmt);
				}
				else PUI_DD[1]<<"SideButtonColor: p "<<p<<" is not in Range[0,3]"<<endl;
				return this;
			}
			
			inline void SetEnableBarGap(bool enable)
			{
				EnableBarGap=enable;
				Win->SetNeedUpdatePosize();
				Win->SetPresentArea(ckBGps_Bu&CoverLmt);
				Win->SetPresentArea(ckBGps_Ri&CoverLmt);
			}
			
			inline void SetEnableBarSideButton(bool enable)
			{
				EnableBarSideButton=enable;
				Win->SetNeedUpdatePosize();
				Win->SetPresentArea(ckBGps_Bu&CoverLmt);
				Win->SetPresentArea(ckBGps_Ri&CoverLmt);
			}
			
			inline void SetCustomWheelSensibility(int s)
			{CustomWheelSensibility=s;}
			
			inline bool GetButtomBarEnableState()
			{return EnableButtomBar;}
			
			inline bool GetRightBarEnableState()
			{return EnableRightBar;}
			
			inline int GetScrollBarWidth()
			{return ScrollBarWidth;}
			
			inline double GetScrollBarPercentX()
			{
				if (EnableButtomBar)
					return largeAreaPS.x*1.0/(rPS.w-EnableRightBar*ScrollBarWidth-largeAreaPS.w);
				else return 0;
				
			}
			
			inline double GetScrollBarPercentY()
			{
				if (EnableRightBar)
					return largeAreaPS.y*1.0/(rPS.h-EnableButtomBar*ScrollBarWidth-largeAreaPS.h);
				else return 0;
			}
			
			inline void SetScrollBarWidth(int w)//how to faster and more precise??
			{
				ScrollBarWidth=w;
				Win->SetPresentArea(CoverLmt);
				Win->SetNeedUpdatePosize();
			}
			
			inline void SetMinLargeAreaSize(int _w,int _h)
			{
				MinLargeAreaW=_w;
				MinLargeAreaH=_h;
				Win->SetNeedUpdatePosize();
			}
			
			inline void SetEnableSmoothWheelScroll(bool enable)
			{EnableSmoothWheelScroll=enable;}
			
			inline Layer* LargeArea()
			{return LargeAreaLayer;}
			
			~LargeLayerWithScrollBar()
			{
				SetInertialMoving(0);
				if (SmoothScrollTimerID!=0)
					SDL_RemoveTimer(SmoothScrollTimerID);
			}
			
			LargeLayerWithScrollBar(const WidgetID &_ID,Widgets *_fa,const Posize &_rps,const Posize &_largeareaPS=ZERO_POSIZE)
			:Widgets(_ID,WidgetType_LargeLayerWithScrollBar,_fa,_rps)
			{
				largeAreaPS=_largeareaPS;
				ResizeLL(0,0);
				LargeAreaLayer=new Layer(0,this,new PosizeEX_LargeLayer(this));
				MultiWidgets=1;
			}
			
			LargeLayerWithScrollBar(const WidgetID &_ID,Widgets *_fa,PosizeEX *psex,const Posize &_largeareaPS=ZERO_POSIZE)
			:Widgets(_ID,WidgetType_LargeLayerWithScrollBar,_fa,psex)
			{
				largeAreaPS=_largeareaPS;
				ResizeLL(0,0);
				LargeAreaLayer=new Layer(0,this,new PosizeEX_LargeLayer(this));
				MultiWidgets=1;
			}
	};
	
	void PosizeEX_InLarge::SetShowPos(int y)
	{larlay->SetViewPort(2,y);}
	
	void PosizeEX_InLarge::GetrPS(Posize &ps)
	{
		if (nxt!=NULL)//if not NULL,it may cause unpredictable behaviour
			nxt->GetrPS(ps);
		int h=gap;
		if (pre!=NULL)
			h+=pre->GetWidgetsRawrPS().y2()+1;
		ps.x=0;
		ps.y=h;
		ps.w=larlay->rPS.w;
		if (fixedH!=0)
			ps.h=fixedH;
		if (NeedFullFill())
			ps.h=max(ps.h,larlay->rPS.h-ps.y-(fullfill<0?-fullfill:0));
	}
	
	PosizeEX_InLarge::~PosizeEX_InLarge()
	{
		if (pre)
			pre->nt=nt;
		if (nt)
			nt->pre=pre;
		else larlay->LastInLargeWidgetsPsEx=pre;
	}
	
	PosizeEX_InLarge::PosizeEX_InLarge(LargeLayerWithScrollBar *_larlay,int _gap=0,int _fullfill=1,int _fixedH=0):larlay(_larlay),gap(_gap),fullfill(_fullfill),fixedH(_fixedH)
	{
		pre=larlay->LastInLargeWidgetsPsEx;
		larlay->LastInLargeWidgetsPsEx=this;
		if (pre!=NULL)
			pre->nt=this;
	}
	
	class TwinLayerWithDivideLine:public Widgets
	{
		protected:
			enum
			{
				Stat_NoFocus=0,
				Stat_UpFocus=1,
				Stat_DownSlide=2
			}stat=Stat_NoFocus;
			bool VerticalDivide=1;
			Layer *LayerAreaA=NULL,//if Vertical ,AreaA is up ,else AreaA is Left
				  *LayerAreaB=NULL;
			int DivideLineShowWidth=2,
				DivideLineEventWidth=6,
				ResizeMode=0;//0:keep percent not changing 1:keep A not changing 2:keep B (It will satisfy LimitA and LimitB first) 
			double DivideLinePos=0.5,//Similar with Slider
				   LimitA=-0.1,LimitB=-0.1;//positive or 0:min pixels in self side; negative [-1,0):percent in self side; negative(other) other side max pixels
			Posize DLEventPs,DLShowPs;
			RGBA DivideLineColor[3]={ThemeColorM[1],ThemeColorM[3],ThemeColorM[5]};
			
			void GetAreaPosize(Posize &ps,bool IsB)
			{
				if (IsB)
					if (VerticalDivide)	ps=Posize(rPS.w*DivideLinePos+DivideLineShowWidth/2,0,ceil(rPS.w*(1-DivideLinePos)-DivideLineShowWidth/2),rPS.h);
					else ps=Posize(0,rPS.h*DivideLinePos+DivideLineShowWidth/2,rPS.w,ceil(rPS.h*(1-DivideLinePos)-DivideLineShowWidth/2));
				else
					if (VerticalDivide) ps=Posize(0,0,rPS.w*DivideLinePos-DivideLineShowWidth/2,rPS.h);
					else ps=Posize(0,0,rPS.w,rPS.h*DivideLinePos-DivideLineShowWidth/2);
			}
			
			class PosizeEX_TwinLayer:public PosizeEX
			{
				protected: 
					TwinLayerWithDivideLine *tar;
					bool IsB;
					
				public:
					virtual void GetrPS(Posize &ps)
					{
						if (nxt!=NULL) nxt->GetrPS(ps);
						tar->GetAreaPosize(ps,IsB);
					}
					
					PosizeEX_TwinLayer(TwinLayerWithDivideLine *_tar,bool isB)
					:tar(_tar),IsB(isB) {}
			};
			
			void SetDivideLinePos(double per)
			{
				Win->SetPresentArea(DLShowPs&CoverLmt);
				DivideLinePos=EnsureInRange(per,LimitA>=0?LimitA/(VerticalDivide?rPS.w:rPS.h):(LimitA<-1?1+LimitA/(VerticalDivide?rPS.w:rPS.h):-LimitA),LimitB>=0?1-LimitB/(VerticalDivide?rPS.w:rPS.h):(LimitB<-1?-LimitB/(VerticalDivide?rPS.w:rPS.h):1+LimitB));
				if (VerticalDivide)
					DLEventPs=Posize(gPS.x+gPS.w*DivideLinePos-DivideLineEventWidth/2,gPS.y,DivideLineEventWidth,gPS.h),
					DLShowPs=Posize(gPS.x+gPS.w*DivideLinePos-DivideLineShowWidth/2,gPS.y,DivideLineShowWidth,gPS.h);
				else
					DLEventPs=Posize(gPS.x,gPS.y+gPS.h*DivideLinePos-DivideLineEventWidth/2,gPS.w,DivideLineEventWidth),
					DLShowPs=Posize(gPS.x,gPS.y+gPS.h*DivideLinePos-DivideLineShowWidth/2,gPS.w,DivideLineShowWidth);
				Win->SetNeedUpdatePosize();
				Win->SetPresentArea(DLShowPs&CoverLmt);
			}
			
			virtual void CheckPos(const PUI_PosEvent *event,int mode)
			{
				if (mode&PosMode_LoseFocus)
				{
					if (stat!=Stat_NoFocus&&Win->GetOccupyPosWg()!=this)
						if (!Win->IsPosFocused()||!(DLEventPs&CoverLmt).In(event->pos)||mode&PosMode_ForceLoseFocus)
						{
							stat=Stat_NoFocus;
							RemoveNeedLoseFocus();
							Win->SetPresentArea(DLShowPs&CoverLmt);
						}
				}
				else if (mode&(PosMode_Default|PosMode_Occupy))
					switch (event->posType)
					{
						case PUI_PosEvent::Pos_Down:
							if (event->button==PUI_PosEvent::Button_MainClick)
								if (DLEventPs.In(event->pos))
								{
									PUI_DD[0]<<"TwinLayerWithDivideLine "<<IDName<<" click"<<endl;
									stat=Stat_DownSlide;
									SetNeedLoseFocus();
									Win->SetOccupyPosWg(this);
									Win->StopSolvePosEvent();
									Win->SetPresentArea(DLShowPs);
								}
							break;
						case PUI_PosEvent::Pos_Up:
							if (stat==Stat_DownSlide)
							{
								PUI_DD[0]<<"TwinLayerWithDivideLine "<<IDName<<" lose"<<endl;
								if (event->type==event->Event_TouchEvent)
									stat=Stat_NoFocus;
								else stat=Stat_UpFocus;
								Win->SetOccupyPosWg(NULL);
								Win->StopSolvePosEvent();
								Win->SetPresentArea(DLShowPs);
							}
							break;
						case PUI_PosEvent::Pos_Motion:
							if (stat==Stat_DownSlide)
							{
								SetDivideLinePos(VerticalDivide?(event->pos.x-gPS.x)*1.0/gPS.w:(event->pos.y-gPS.y)*1.0/gPS.h);
								Win->StopSolvePosEvent();
							}
							else if ((DLEventPs&CoverLmt).In(event->pos))
							{
								if (stat==Stat_NoFocus)
								{
									PUI_DD[0]<<"TwinLayerWithDivideLine "<<IDName<<" focus"<<endl;
									stat=Stat_UpFocus;
									SetNeedLoseFocus();
									Win->SetPresentArea(DLShowPs);
								}
								Win->StopSolvePosEvent();
							}
							break;
					}
			}
			
			virtual void Show(Posize &lmt)
			{
				Win->RenderFillRect(DLShowPs&lmt,ThemeColor(DivideLineColor[stat]));
				
				Win->Debug_DisplayBorder(gPS); 
				Win->Debug_DisplayBorder(DLEventPs);
				Win->Debug_DisplayBorder(DLShowPs);
			}
			
			virtual void CalcPsEx()
			{
				Posize lastPs=gPS;
				if (PsEx!=NULL)
					PsEx->GetrPS(rPS);
				if (fa!=NULL)
					gPS=rPS+fa->GetgPS();
				else gPS=rPS;
				CoverLmt=gPS&GetFaCoverLmt();
				if (!(lastPs==gPS))
					Win->SetPresentArea(lastPs|gPS);
				
				if (!(lastPs==gPS))
					switch (ResizeMode)
					{
						case 0:
							SetDivideLinePos(DivideLinePos);
							break;
						case 1:
							if (VerticalDivide) SetDivideLinePos((LayerAreaA->GetrPS().w+DivideLineShowWidth/2.0)/rPS.w);
							else SetDivideLinePos((LayerAreaA->GetrPS().h+DivideLineShowWidth/2.0)/rPS.h);
							break;
						case 2:
							if (VerticalDivide) SetDivideLinePos(1-(LayerAreaB->GetrPS().w+DivideLineShowWidth/2.0)/rPS.w);
							else SetDivideLinePos(1-(LayerAreaB->GetrPS().h+DivideLineShowWidth/2.0)/rPS.h);
							break;
						default:
							PUI_DD[2]<<"TwinLayerWithDivideLin: ResizeMode "<<ResizeMode<<" is not in Range[0,2]"<<endl;
					}
			}
			
			virtual void _PresentWidgets(Posize lmt)
			{
				if (nxtBrother!=NULL)
					PresentWidgetsOf(nxtBrother,lmt);
				if (Enabled)
				{
					lmt=lmt&gPS;
					if (childWg!=NULL)
						PresentWidgetsOf(childWg,lmt);
					Show(lmt);
					if (DEBUG_EnableWidgetsShowInTurn)
						SDL_Delay(DEBUG_WidgetsShowInTurnDelayTime),
						SDL_RenderPresent(Win->GetSDLRenderer());
				}
			}
			
			virtual void _SolvePosEvent(const PUI_PosEvent *event,int mode)
			{
				if (!Win->IsNeedSolvePosEvent())
					return;
				if (Enabled)
					if (gPS.In(event->pos))
					{
						if (Win->IsNeedSolvePosEvent())
							CheckPos(event,mode|PosMode_Pre|PosMode_Default);
						if (childWg!=NULL)
							SolvePosEventOf(childWg,event,mode);
						if (Win->IsNeedSolvePosEvent())
							CheckPos(event,mode|PosMode_Nxt);
					}
				if (nxtBrother!=NULL)
					SolvePosEventOf(nxtBrother,event,mode);
			}
			
		public:
			inline void SetDivideLineColor(int p,const RGBA &co)
			{
				if (InRange(p,0,2))
				{
					DivideLineColor[p]=!co?ThemeColorM[p*2+1]:co;
					Win->SetPresentArea(DLShowPs&CoverLmt);
				}
				else PUI_DD[1]<<"TwinLayerWithDivideLine: SetDivideLineColor: p "<<p<<" is not in Range[0,2]"<<endl;
			}
			
			inline void SetAreaAColor(const RGBA &co)
			{LayerAreaA->SetLayerColor(co);}
			
			inline void SetAreaBColor(const RGBA &co)
			{LayerAreaB->SetLayerColor(co);}
			
			inline void SetDivideLineWidth(int showWidth,int eventWidth)
			{
				DivideLineShowWidth=showWidth;
				DivideLineEventWidth=eventWidth;
				Win->SetNeedUpdatePosize();
			}
			
			inline int GetDivideLineShowWidth() const
			{return DivideLineShowWidth;}
			
			inline int GetDivideLineEventWidth() const
			{return DivideLineEventWidth;}
			
			inline void SetDivideLineMode(int mode,double _limitA,double _limitB)
			{
				ResizeMode=mode;
				LimitA=_limitA;
				LimitB=_limitB;
				SetDivideLinePos(DivideLinePos);
				Win->SetNeedUpdatePosize();
			}
			
			void SetDivideLinePosition(double per)
			{SetDivideLinePos(per);}
			
			double GetDivideLinePosition() const
			{return DivideLinePos;}
			
			inline Layer* AreaA()
			{return LayerAreaA;}
			
			inline Layer* AreaB()
			{return LayerAreaB;}
			
			TwinLayerWithDivideLine(const WidgetID &_ID,Widgets *_fa,const Posize &_rps,bool _verticalDivide,double initPercent=0.5)
			:Widgets(_ID,WidgetType_TwinLayerWithDivideLine,_fa,_rps),VerticalDivide(_verticalDivide)
			{
				LayerAreaA=new Layer(0,this,new PosizeEX_TwinLayer(this,0));
				LayerAreaB=new Layer(0,this,new PosizeEX_TwinLayer(this,1));
				SetDivideLinePos(initPercent);
				MultiWidgets=1;
			}
			
			TwinLayerWithDivideLine(const WidgetID &_ID,Widgets *_fa,PosizeEX *psex,bool _verticalDivide,double initPercent=0.5)
			:Widgets(_ID,WidgetType_TwinLayerWithDivideLine,_fa,psex),VerticalDivide(_verticalDivide)
			{
				LayerAreaA=new Layer(0,this,new PosizeEX_TwinLayer(this,0));
				LayerAreaB=new Layer(0,this,new PosizeEX_TwinLayer(this,1));
				SetDivideLinePos(initPercent);
				MultiWidgets=1;
			}
	};
	
	template <class T> class EventLayer:public Widgets
	{
		protected:
			int (*func)(T&,const PUI_Event*)=NULL;//return 1 to solve it,0 means not solve
			T funcdata;
			
			virtual void CheckEvent(const PUI_Event *event)
			{
				if (func!=NULL)
					if (func(funcdata,event)!=0)
						Win->StopSolveEvent();
			}
			
		public:
			inline T& GetFuncData()
			{return funcdata;}
			
			inline void SetFuncData(const T &_funcdata)
			{funcdata=_funcdata;}
			
			inline void SetFunc(int (*_func)(T&,const PUI_Event*),const T &_funcdata)
			{
				func=_func;
				funcdata=_funcdata;
			}
			
			EventLayer(const WidgetID &_ID,Widgets *_fa,int (*_func)(T&,const PUI_Event*),const T &_funcdata)
			:Widgets(_ID,WidgetType_EventLayer,_fa,ZERO_POSIZE),func(_func),funcdata(_funcdata) {}
	};
	
	template <class T> class PosEventLayer:public Widgets
	{
		protected:
			int (*func)(T&,const PUI_PosEvent*,int)=NULL;//return 1 to solve it,0 means not solve
			T funcdata;
			
			virtual void CheckPos(const PUI_PosEvent *event,int mode)
			{
				if (func!=NULL)
					if (func(funcdata,event,mode)!=0)
						Win->StopSolvePosEvent();
			}
			
		public:
			inline T& GetFuncData()
			{return funcdata;}
			
			inline void SetFuncData(const T &_funcdata)
			{funcdata=_funcdata;}
			
			inline void SetFunc(int (*_func)(T&,const PUI_PosEvent*),const T &_funcdata)
			{
				func=_func;
				funcdata=_funcdata;
			}
			
			PosEventLayer(const WidgetID &_ID,Widgets *_fa,const Posize &_rps,int (*_func)(T&,const PUI_PosEvent*,int),const T &_funcdata)
			:Widgets(_ID,WidgetType_PosEventLayer,_fa,_rps),func(_func),funcdata(_funcdata) {}
			
			PosEventLayer(const WidgetID &_ID,Widgets *_fa,PosizeEX *psex,int (*_func)(T&,const PUI_PosEvent*,int),const T &_funcdata)
			:Widgets(_ID,WidgetType_PosEventLayer,_fa,psex),func(_func),funcdata(_funcdata) {}
	};
	
	template <class T> class ShowLayer:public Widgets
	{
		protected:
			void (*func)(T&,const Posize&,const Posize&)=NULL;//funcdata,gPS,lmt
			T funcdata;
			
			virtual void Show(Posize &lmt)
			{
				if (func!=NULL)
					func(funcdata,gPS,lmt);
				Win->Debug_DisplayBorder(gPS);
			}
			
		public:
			inline T& GetFuncData()
			{return funcdata;}
			
			inline void SetFuncData(const T &_funcdata)
			{funcdata=_funcdata;}
			
			inline void SetFunc(void (*_func)(T&,const Posize&,const Posize&),const T &_funcdata)
			{
				func=_func;
				funcdata=_funcdata;
			}
			
			ShowLayer(const WidgetID &_ID,Widgets *_fa,PosizeEX *psex,void (*_func)(T&,const Posize&,const Posize&),const T &_funcdata)
			:Widgets(_ID,WidgetType_ShowLayer,_fa,psex),func(_func),funcdata(_funcdata) {}
			
			ShowLayer(const WidgetID &_ID,Widgets *_fa,const Posize &_rps,void (*_func)(T&,const Posize&,const Posize&),const T &_funcdata)
			:Widgets(_ID,WidgetType_ShowLayer,_fa,_rps),func(_func),funcdata(_funcdata) {}
	};

	class ColorWeightLayer:public Widgets
	{
		protected:
		
		public:
			
			
	};
	
	template <class T> class GestureSenseLayer:public Widgets
	{
		public:
			enum:unsigned long long//gesture bitmap
			{
				Gb_Click	=1ull,
				Gb_SlideL	=1ull<<5, 
				Gb_SlideR	=1ull<<10,
				Gb_SlideU	=1ull<<15,
				Gb_SlideD	=1ull<<20,
				Gb_FromL	=1ull<<25,
				Gb_FromR	=1ull<<30,
				Gb_FromU	=1ull<<35,
				Gb_FromD	=1ull<<40,
//				Gb_Pinch	=1ull<<45,
//				Gb_Zoom		=1ull<<50
			};
			enum
			{
				G_Click=0,
				G_SlideL,
				G_SlideR,
				G_SlideU,
				G_SlideD,
				G_FromL,
				G_FromR,
				G_FromU,
				G_FromD,
//				G_Pinch,
//				G_Zoom,
				
				G_End=20
			};
			
			struct GestureData
			{
				unsigned int gesture,
							 count;
				double dist,
					   length;
				int stat;//0:ready 1:end -1:cancel
			};
		
		protected:
			unsigned long long ConcernedGesture=0;
			unsigned int CurrentPredictGesture=G_End;
			bool DownInBody=0;
			unsigned int maxCount=0;
			int SideSlideBoundWidth=-1;//-1 means use global parameter
			Point GestureStartPoint;
			double GestureLengthSum=0;
			bool GestureStarted=0;
			void (*func)(T&,const GestureData &,const PUI_TouchEvent*)=NULL;
			T funcdata;
			
			inline bool IsConcernedGesture(unsigned int gesture,unsigned long long countBitmap) const
			{
				if (InRange(gesture,G_Click,G_End-1))
					return ConcernedGesture&(countBitmap<<gesture*5+1);
				else return 0;
			}
			
			void EndTouchSolve()
			{
				RemoveNeedLoseFocus();
				CurrentPredictGesture=G_End;
				maxCount=0;
				DownInBody=0;
				if (Win->GetOccupyPosWg()==this)
					Win->SetOccupyPosWg(NULL);
			}
			
			inline void CallbackFunc(const PUI_TouchEvent *touch,int stat)
			{
				if (func==NULL) return;
				auto getdist=[](const Point &p1,const Point &p2)->float
				{return sqrt((p1.x-p2.x)*(p1.x-p2.x)+(p1.y-p2.y)*(p1.y-p2.y));};
				
				if (stat!=0)
					GestureStarted=0;
				else if (!GestureStarted)
				{
					GestureStarted=1;
					GestureStartPoint=touch->pos;
					GestureLengthSum=0;
				}
				
				GestureLengthSum+=getdist(ZERO_POINT,touch->delta);
				
				GestureData gd;
				gd.gesture=CurrentPredictGesture;
				gd.count=maxCount;
				gd.dist=getdist(GestureStartPoint,touch->pos);
				gd.length=GestureLengthSum;
				gd.stat=stat;
				func(funcdata,gd,touch);
			}
			
			virtual void CheckPos(const PUI_PosEvent *event,int mode)
			{
				if (event->type!=PUI_Event::Event_TouchEvent||func==NULL) return;
				PUI_TouchEvent *touch=event->TouchEvent();
				if (mode&PosMode_LoseFocus)
				{
					if (CurrentPredictGesture!=G_End&&Win->GetOccupyPosWg()!=this)
						if (!CoverLmt.In(touch->pos)/*??*/||mode&PosMode_ForceLoseFocus)
							EndTouchSolve();
				}
				else if (mode&(PosMode_Pre|PosMode_Occupy))
				{
					if (!NeedLoseFocus)
						SetNeedLoseFocus();
					maxCount=max(maxCount,(unsigned int)touch->count);
					switch (touch->gesture)
					{
						case PUI_TouchEvent::Ges_ShortClick:
						case PUI_TouchEvent::Ges_LongClick:
						case PUI_TouchEvent::Ges_MultiClick:
						{
							if (CurrentPredictGesture==G_End)
								CurrentPredictGesture=G_Click;
							DownInBody=gPS.Shrink(SideSlideBoundWidth==-1?PUI_Parameter_SideSlideBoundWidth:SideSlideBoundWidth).In(touch->pos);
							if (IsConcernedGesture(G_Click,1<<maxCount-1))
								CallbackFunc(touch,0);
							break;
						}
						case PUI_TouchEvent::Ges_Moving:
						case PUI_TouchEvent::Ges_MultiMoving:
							if (touch->speed()==0)
								break;
							if (DownInBody)
							{
								if (fabs(touch->speedX)>fabs(touch->speedY))
									if (touch->speedX>=0) CurrentPredictGesture=G_SlideR;
									else CurrentPredictGesture=G_SlideL;
								else
									if (touch->speedY<=0) CurrentPredictGesture=G_SlideU;
									else CurrentPredictGesture=G_SlideD;
								if (IsConcernedGesture(CurrentPredictGesture,1<<maxCount-1))
								{
									Win->SetOccupyPosWg(this);
									Win->StopSolvePosEvent();
									CallbackFunc(touch,0);
								}
								else if (!IsConcernedGesture(CurrentPredictGesture,0x1f<<maxCount-1&0x1f))
								{
									CallbackFunc(touch,-1);
									EndTouchSolve();
								}
							}
							else
							{
								unsigned int lastGes=CurrentPredictGesture;
								if (fabs(touch->speedX)>fabs(touch->speedY))
									if (touch->speedX>=0) CurrentPredictGesture=G_FromL;
									else CurrentPredictGesture=G_FromR;
								else
									if (touch->speedY<=0) CurrentPredictGesture=G_FromD;
									else CurrentPredictGesture=G_FromU;
								if (NotInSet(CurrentPredictGesture,G_Click,G_End,lastGes))
								{
									CallbackFunc(touch,-1);
									EndTouchSolve();
								}
								else if (IsConcernedGesture(CurrentPredictGesture,1<<maxCount-1))
								{
									Win->SetOccupyPosWg(this);
									Win->StopSolvePosEvent();
									CallbackFunc(touch,0);
								}
								else if (!IsConcernedGesture(CurrentPredictGesture,0x1f<<maxCount-1&0x1f))
								{
									CallbackFunc(touch,-1);
									EndTouchSolve();
								}
							}
							break;
						case PUI_TouchEvent::Ges_End:
						{
							if (IsConcernedGesture(CurrentPredictGesture,1<<maxCount-1))//how about those widgets that haven't losefocus?
							{
								CallbackFunc(touch,1);
								Win->StopSolvePosEvent();
							}
							else CallbackFunc(touch,-1);
							EndTouchSolve();
							break;
						}
					}
				}
			}
			
		public:
			inline void SetConcernedGesture(unsigned long long bitmap)
			{ConcernedGesture=bitmap;}
			
			inline void AddConcernedGesture(unsigned long long bitmap)
			{ConcernedGesture|=bitmap;}
			
			inline void AddConcernedGesture(unsigned int gesture,unsigned int count)//count should > 0
			{
				if (InRange(gesture,G_Click,G_End-1)&&InRange(count,1,5))
					ConcernedGesture|=(1ull<<gesture*5)<<count;
			}
			
			inline T& GetFuncData()
			{return funcdata;}
			
			inline void SetFunc(void (*_func)(T&,const GestureData &,const PUI_TouchEvent*),const T &_funcdata)
			{
				func=_func;
				funcdata=_funcdata;
			}
			
			GestureSenseLayer(const WidgetID &_ID,Widgets *_fa,const Posize &_rPS,unsigned long long concernedgesture,void (*_func)(T&,const GestureData &,const PUI_TouchEvent*),const T &_funcdata)
			:Widgets(_ID,WidgetType_GestureSenseLayer,_fa,_rPS),ConcernedGesture(concernedgesture),func(_func),funcdata(_funcdata) {}
			
			GestureSenseLayer(const WidgetID &_ID,Widgets *_fa,PosizeEX *psex,unsigned long long concernedgesture,void (*_func)(T&,const GestureData &,const PUI_TouchEvent*),const T &_funcdata)
			:Widgets(_ID,WidgetType_GestureSenseLayer,_fa,psex),ConcernedGesture(concernedgesture),func(_func),funcdata(_funcdata) {}
	};
	
	template <class T> class TabLayer:public Widgets//Deprecated...
	{
		protected:
			struct TabLayerData
			{
				Layer *lay=NULL;
				string title;
				RGBA TabColor[3],//none,focus/current,click
					 TextColor=ThemeColorMT[0];
				int TabWidth=50;
				SharedTexturePtr pic;
				T funcData;
				
				TabLayerData()
				{
					for (int i=0;i<=2;++i)
						TabColor[i]=ThemeColorM[i*2+1];
				}
				
				TabLayerData(Layer *_lay,const string &_title,const SharedTexturePtr &_pic,const T &_funcdata)
				:lay(_lay),title(_title),pic(_pic),funcData(_funcdata) 
				{
					for (int i=0;i<=2;++i)
						TabColor[i]=ThemeColorM[i*2+1];
				}
			};
			
			enum
			{
				Stat_NoFocus=0,
				Stat_UpFocus=1,
				Stat_ClickLeft=2,
				Stat_ClickRight=3
			}stat=Stat_NoFocus;
			vector <TabLayerData> EachTabLayer;
			int TabCnt=0;
			int	TabHeight=24,
				LeftMostShowLayer=0,
				FocusingPos=-1,
				CurrentShowLayerPos=-1;//-1:means no layer
			Range TabLayerWidthRange={100,400};
			bool ShowCloseX=0,
				 EnableDrag=0,
				 EnableTabScrollAnimation=0,
				 EnableSwitchGradientAnimation=0;
			RGBA TabBarBackgroundColor=ThemeColorBG[0];
			void (*func)(T&,int,int)=NULL;//int1:which int2:stat (1:switch to 2:rightClick)
			
			class PosizeEX_TabLayer:public PosizeEX
			{
				protected: 
					TabLayer *tar=NULL;
					
				public:
					virtual void GetrPS(Posize &ps)
					{
						if (nxt!=NULL) nxt->GetrPS(ps);
						ps=Posize(0,tar->TabHeight,tar->rPS.w,tar->rPS.h-tar->TabHeight);
					}
					
					PosizeEX_TabLayer(TabLayer *_tar)
					:tar(_tar) {}
			};
			
			inline Posize TabBarPosize()
			{return Posize(gPS.x,gPS.y,gPS.w,TabHeight);}
			
			int GetTabFromPos(const Point &pt) 
			{
				if (TabCnt==0) return -1;
				if ((CoverLmt&TabBarPosize()).In(pt))
				{
					for (int i=LeftMostShowLayer,w=0;i<TabCnt;w+=EachTabLayer[i++].TabWidth)
						if (pt.x-gPS.x<w+EachTabLayer[i].TabWidth)
							return i;
					return -1;
				}
				else return -1;
			}

			virtual void CheckEvent(const PUI_Event *event)
			{
				if (event->type==PUI_Event::Event_WheelEvent)
				{
					if (TabBarPosize().In(Win->NowPos()))
					{
						if (event->WheelEvent()->dy>0)
						{
							if (LeftMostShowLayer>0)
							{
								LeftMostShowLayer=max(0,LeftMostShowLayer-event->WheelEvent()->dy);
								Win->SetPresentArea(TabBarPosize());
							}
						}
						else
						{
							int w=0;
							for (int i=LeftMostShowLayer;i<TabCnt;++i)
								w+=EachTabLayer[i].TabWidth;
							if (w>gPS.w)
							{
								for (int i=-event->WheelEvent()->dy;i>=1&&LeftMostShowLayer<TabCnt&&w>gPS.w;--i)
									w-=EachTabLayer[LeftMostShowLayer++].TabWidth;
								Win->SetPresentArea(TabBarPosize());
							}
						}
						Win->StopSolveEvent();
					}
				}
				else if (event->type==PUI_EVENT_UpdateTimer)
					if (event->UserEvent()->data1==this)
					{
						//<<timer animation...
					}
			}
			
			virtual void CheckPos(const PUI_PosEvent *event,int mode)
			{
				//<<TouchEventSolve...
				if (mode&PosMode_LoseFocus)
				{
					if (stat!=Stat_NoFocus)
						if (!Win->IsPosFocused()||!(CoverLmt&TabBarPosize()).In(event->pos)||mode&PosMode_ForceLoseFocus)
						{
							stat=Stat_NoFocus;
							FocusingPos=-1;
							RemoveNeedLoseFocus();
							Win->SetPresentArea(CoverLmt&TabBarPosize());
						}
				}
				else if (mode&PosMode_Default)
					switch (event->posType)
					{
						case PUI_PosEvent::Pos_Down:
							if (event->button==PUI_PosEvent::Button_MainClick)
							{
								FocusingPos=GetTabFromPos(event->pos);
								if (FocusingPos!=-1)
								{
									stat=Stat_ClickLeft;
									if (CurrentShowLayerPos!=FocusingPos)
									{
										PUI_DD[0]<<"TabLayer "<<IDName<<" Switch layer to "<<FocusingPos<<endl;
										if (CurrentShowLayerPos!=-1)
											EachTabLayer[CurrentShowLayerPos].lay->SetEnabled(0);
										CurrentShowLayerPos=FocusingPos;
										EachTabLayer[CurrentShowLayerPos].lay->SetEnabled(1);
										if (func!=NULL)
											func(EachTabLayer[CurrentShowLayerPos].funcData,CurrentShowLayerPos,1);
										Win->SetNeedUpdatePosize();
									}
									Win->StopSolvePosEvent();
									Win->SetPresentArea(CoverLmt&TabBarPosize());
								}
								if (event->PosEvent()->button==PUI_PosEvent::Button_SubClick)
									stat=Stat_ClickRight;
							}
							break;
						case PUI_PosEvent::Pos_Up:
							if (stat==Stat_ClickLeft||stat==Stat_ClickRight)
							{
								if (stat==Stat_ClickRight)
									if (func!=NULL)
										func(EachTabLayer[FocusingPos].funcData,FocusingPos,2); 
								stat=Stat_UpFocus;
								Win->StopSolvePosEvent();
								Win->SetPresentArea(CoverLmt&TabBarPosize());
							}
							break;
						case PUI_PosEvent::Pos_Motion:
							if (stat==Stat_NoFocus)
							{
								FocusingPos=GetTabFromPos(event->pos);
								if (FocusingPos!=-1)
								{
									stat=Stat_UpFocus;
									SetNeedLoseFocus();
									Win->StopSolvePosEvent();
									Win->SetPresentArea(CoverLmt&TabBarPosize());
								}
							}
							else
							{
								int p=GetTabFromPos(event->pos);
								if (p==FocusingPos)
									Win->StopSolvePosEvent();
								else
								{
									if (p==-1)
									{
										stat=Stat_NoFocus;
										RemoveNeedLoseFocus();
									}
									FocusingPos=p;
									Win->StopSolvePosEvent();
									Win->SetPresentArea(CoverLmt&TabBarPosize());
								}
							}
							break;
					}
			}
			
			virtual void Show(Posize &lmt)
			{
				Win->RenderFillRect(TabBarPosize()&lmt,ThemeColor(TabBarBackgroundColor));
				if (TabCnt>0)
				{
					Posize TabPs(gPS.x,gPS.y,EachTabLayer[LeftMostShowLayer].TabWidth,TabHeight);
					for (int i=LeftMostShowLayer;i<TabCnt&&TabPs.x<gPS.x2();++i)
					{
						int colorPos=FocusingPos==i?(stat==2||stat==3?2:1):CurrentShowLayerPos==i;
						Win->RenderFillRect(TabPs&lmt,ThemeColor(EachTabLayer[i].TabColor[colorPos]));
						if (EachTabLayer[i].pic()!=NULL)
							Win->RenderCopyWithLmt(EachTabLayer[i].pic(),Posize(TabPs.x,TabPs.y,TabHeight,TabHeight),lmt);
						Win->RenderDrawText(EachTabLayer[i].title,TabPs,lmt,0,ThemeColor(EachTabLayer[i].TextColor));
						Win->Debug_DisplayBorder(TabPs);
						TabPs.x+=TabPs.w+1;
						TabPs.w=EachTabLayer[i].TabWidth;
					}
				}
				Win->Debug_DisplayBorder(gPS);
			}
			
		public:
			void SwitchLayer(int p)
			{
				if (TabCnt==0) return;
				p=EnsureInRange(p,0,TabCnt-1);
				if (CurrentShowLayerPos!=p)
				{
					PUI_DD[0]<<"TabLayer "<<IDName<<" Switch layer to "<<p<<endl;
					if (CurrentShowLayerPos!=-1)
						EachTabLayer[CurrentShowLayerPos].lay->SetEnabled(0);
					CurrentShowLayerPos=p;
					EachTabLayer[p].lay->SetEnabled(1);
					if (func!=NULL)
						func(EachTabLayer[p].funcData,p,1);
					Win->SetNeedUpdatePosize();
					Win->SetPresentArea(CoverLmt&TabBarPosize());
				}
			}
			
			Layer* PushbackLayer(const string &title,const T &_funcdata,const SharedTexturePtr &pic=SharedTexturePtr(NULL))
			{
				Layer *re=new Layer(0,this,new PosizeEX_TabLayer(this));
				EachTabLayer.push_back(TabLayerData(re,title,pic,_funcdata));
				int w=0;
				TTF_SizeUTF8(PUI_Font()(),title.c_str(),&w,NULL);
				EachTabLayer[TabCnt].TabWidth=TabLayerWidthRange.EnsureInRange(w+10);
				++TabCnt;
				SwitchLayer(TabCnt-1);
				return re;
			}
			
			Layer* AddLayer(int pos,const string &title,const T &_funcdata,const SharedTexturePtr &pic=SharedTexturePtr(NULL))
			{
				pos=EnsureInRange(pos,0,TabCnt);
				Layer *re=new Layer(0,this,new PosizeEX_TabLayer(this));
				EachTabLayer.insert(EachTabLayer.begin()+pos,TabLayerData(re,title,pic,_funcdata));
				if (pos<=CurrentShowLayerPos)
					++CurrentShowLayerPos;
				int w=0;
				TTF_SizeUTF8(PUI_Font()(),title.c_str(),&w,NULL);
				EachTabLayer[pos].TabWidth=TabLayerWidthRange.EnsureInRange(w+10);
				++TabCnt;
				SwitchLayer(pos);
				return re;
			}
			
			void DeleteLayer(int pos)
			{
				if (TabCnt==0) return;
				pos=EnsureInRange(pos,0,TabCnt-1);
				if (pos==CurrentShowLayerPos)
					if (pos==0)
						if (TabCnt>=2)
						{
							SwitchLayer(1);
							CurrentShowLayerPos=0;
						}	
						else CurrentShowLayerPos=-1;
					else SwitchLayer(pos-1);
				else
					if (pos<CurrentShowLayerPos)
						--CurrentShowLayerPos;
				delete EachTabLayer[pos].lay;
				EachTabLayer.erase(EachTabLayer.begin()+pos);
				--TabCnt;
			}
			
			inline void SetTabBarBackgroundColor(const RGBA &co)
			{
				TabBarBackgroundColor=!co?ThemeColorBG[0]:co;
				Win->SetPresentArea(TabBarPosize()&CoverLmt);
			}
			
			inline void SetTabColor(int pos,int p,const RGBA &co)
			{
				pos=EnsureInRange(pos,0,TabCnt-1);
				if (InRange(p,0,2))
				{
					EachTabLayer[pos].TabColor[p]=!co?ThemeColorM[p*2+1]:co;
					Win->SetPresentArea(TabBarPosize()&CoverLmt);
				}
				else PUI_DD[1]<<"TabLayer: SetTabColor: p "<<p<<" is not in Range[0,2]"<<endl;
			}
			
			inline void SetTabTextColor(int pos,const RGBA &co)
			{
				EachTabLayer[EnsureInRange(pos,0,TabCnt-1)].TextColor=!co?ThemeColorMT[0]:co;
				Win->SetPresentArea(TabBarPosize()&CoverLmt);
			}
			
			void SetTabTitle(int pos,const string &str)
			{
				pos=EnsureInRange(pos,0,TabCnt-1);
				EachTabLayer[pos].title=str;
				int w=0;
				TTF_SizeUTF8(PUI_Font()(),str.c_str(),&w,NULL);
				EachTabLayer[pos].TabWidth=TabLayerWidthRange.EnsureInRange(w+10);
				Win->SetPresentArea(TabBarPosize()&CoverLmt);
			}
			
			void SetTabPic(int pos,const SharedTexturePtr &pic)
			{
				EachTabLayer[EnsureInRange(pos,0,TabCnt-1)].pic=pic;
				Win->SetPresentArea(TabBarPosize()&CoverLmt);
			}
			
			void SetTabHeight(int h)
			{
				Win->SetPresentArea(Posize(gPS.x,gPS.y,gPS.w,max(h,TabHeight))&CoverLmt);
				TabHeight=h;
				Win->SetNeedUpdatePosize();
			}
			
			void SetTabLayerWidthRange(const Range &ran)
			{
				TabLayerWidthRange=ran;
				for (int i=0,w;i<TabCnt;++i)
				{
					TTF_SizeUTF8(PUI_Font()(),EachTabLayer[i].title.c_str(),&w,NULL);
					EachTabLayer[i].TabWidth=TabLayerWidthRange.EnsureInRange(EachTabLayer[i].TabWidth);
				}
				Win->SetPresentArea(TabBarPosize()&CoverLmt);
			}
			
			inline void SetFunc(void (*_func)(T&,int,int))
			{func=_func;}
			
			inline void SetTabFuncdata(int p,void *_funcdata)
			{EachTabLayer[p].funcData=_funcdata;}
			
			inline int GetTabcnt()
			{return TabCnt;}
			
			inline int GetCurrenShowLayerPos()
			{return CurrentShowLayerPos;}
			
			Layer* Tab(int p)
			{return EachTabLayer[EnsureInRange(p,0,TabCnt-1)].lay;}
			
			TabLayer(const WidgetID &_ID,Widgets *_fa,PosizeEX *psex)
			:Widgets(_ID,WidgetType_TabLayer,_fa,psex) {}
			
			TabLayer(const WidgetID &_ID,Widgets *_fa,const Posize &_rps)
			:Widgets(_ID,WidgetType_TabLayer,_fa,_rps) {}
	};
	
	class BorderRectLayer:public Layer
	{
		protected:
			RGBA BorderColor=ThemeColorM[1];
			int BorderWidth=1;
			
			virtual void Show(Posize &lmt)
			{
				for (int i=1;i<=BorderWidth;++i)
					Win->RenderDrawRectWithLimit(gPS.Shrink(i-1),ThemeColor(BorderColor),lmt);
				Win->RenderFillRect(lmt&gPS.Shrink(BorderWidth-1),ThemeColor(LayerColor));
				Win->Debug_DisplayBorder(gPS);
			}
			
		public:
			void SetBorderColor(const RGBA &co)
			{
				BorderColor=!co?ThemeColorM[0]:co;
				Win->SetPresentArea(CoverLmt);//is it fast?
			}
			
			void SetBorderWidth(int _w)
			{
				if (_w<0) return;
				BorderWidth=_w;
				Win->SetPresentArea(CoverLmt);
			}

			BorderRectLayer(const WidgetID &_ID,Widgets *_fa,const Posize &_rps,const RGBA &_co=ThemeColorM[1])
			:Layer(_ID,WidgetType_BorderRectLayer,_fa,_rps),BorderColor(_co) {}
			
			BorderRectLayer(const WidgetID &_ID,Widgets *_fa,PosizeEX *psex,const RGBA &_co=ThemeColorM[1])
			:Layer(_ID,WidgetType_BorderRectLayer,_fa,psex),BorderColor(_co) {}
	};
	
	class BlurEffectLayer:public Widgets
	{
		protected:
			
			
		public:
			
			
	}; 
	
	template <class T> class Slider:public Widgets
	{
		protected:
			enum
			{
				Stat_NoFocus=0,
				Stat_UpFocus=1,
				Stat_DownSlide=2
			}stat=Stat_NoFocus;
			bool Vertical;
			Posize ckPs={0,0,8,8};
			int barWidth=3;
			double Percent=0;
			void (*func)(T&,double,bool)=NULL;//double: percent bool:IsLooseMouse(set 0 when continuous changing)
			T funcData;
			RGBA BarColor[3]{ThemeColorM[0],ThemeColorM[2],ThemeColor[4]},
				 ChunkColor[3]{ThemeColorM[1],ThemeColorM[3],ThemeColorM[5]};
			
			void SetChunkPs(const Point &pt,bool bo)
			{
				if (Vertical)
					ckPs.y=EnsureInRange(pt.y-ckPs.h/2,gPS.y,gPS.y2()-ckPs.h),
					Percent=(ckPs.y-gPS.y)*1.0/(gPS.h-ckPs.h);
				else 
					ckPs.x=EnsureInRange(pt.x-ckPs.w/2,gPS.x,gPS.x2()-ckPs.w),
					Percent=(ckPs.x-gPS.x)*1.0/(gPS.w-ckPs.w);
				if (func!=NULL)
					func(funcData,Percent,bo);
			}
			
			virtual void CheckPos(const PUI_PosEvent *event,int mode)
			{
				if (mode&PosMode_LoseFocus)
				{
					if (stat!=Stat_NoFocus&&Win->GetOccupyPosWg()!=this)
						if (!Win->IsPosFocused()||!CoverLmt.In(event->pos)||mode&PosMode_ForceLoseFocus)
						{
							stat=Stat_NoFocus;
							RemoveNeedLoseFocus();
							Win->SetPresentArea(CoverLmt);
						}
				}
				else if (mode&(PosMode_Default|PosMode_Occupy))
					switch (event->posType)
					{
						case PUI_PosEvent::Pos_Down:
							if (event->button==PUI_PosEvent::Button_MainClick)
							{
								stat=Stat_DownSlide;
								SetChunkPs(event->pos,0);
								SetNeedLoseFocus();
								Win->SetOccupyPosWg(this);
								Win->StopSolvePosEvent();
								Win->SetPresentArea(CoverLmt);
							}
							break;
						case PUI_PosEvent::Pos_Up:
							if (stat==Stat_DownSlide)
							{
								if (event->type==event->Event_TouchEvent)
								{
									RemoveNeedLoseFocus();
									stat=Stat_NoFocus;
								}
								else stat=Stat_UpFocus;
								SetChunkPs(event->pos,1);
								Win->SetOccupyPosWg(NULL);
								Win->StopSolvePosEvent();
								Win->SetPresentArea(CoverLmt);
							}
							break;
						case PUI_PosEvent::Pos_Motion:
							if (stat==Stat_DownSlide)
							{
								Win->SetPresentArea(ckPs&CoverLmt);
								SetChunkPs(event->pos,0);
								Win->SetPresentArea(ckPs&CoverLmt);
							}
							else if (stat==Stat_NoFocus)
							{
								stat=Stat_UpFocus;
								SetNeedLoseFocus();
								Win->SetPresentArea(CoverLmt);
							}
							Win->StopSolvePosEvent();
							break;
					}
			}
			
			virtual void Show(Posize &lmt)
			{
				if (Vertical)
					Win->RenderFillRect(Posize(gPS.x+(gPS.w-barWidth>>1),gPS.y,barWidth,gPS.h)&lmt,ThemeColor(BarColor[stat]));
				else Win->RenderFillRect(Posize(gPS.x,gPS.y+(gPS.h-barWidth>>1),gPS.w,barWidth)&lmt,ThemeColor(BarColor[stat]));
				Win->RenderFillRect(ckPs&lmt,ThemeColor(ChunkColor[stat]));
				Win->Debug_DisplayBorder(gPS);
			}
			
			virtual void CalcPsEx()
			{
				if (PsEx!=NULL)
					PsEx->GetrPS(rPS);
				Posize lastPs=gPS;
				if (fa!=NULL)
					gPS=rPS+fa->GetgPS();
				else gPS=rPS;
				CoverLmt=gPS&GetFaCoverLmt();
				if (!(lastPs==gPS))
					Win->SetPresentArea(lastPs|gPS);
				
				if (Vertical) 
					ckPs.x=gPS.x,ckPs.y=gPS.y+(gPS.h-ckPs.h)*Percent,ckPs.w=gPS.w;
				else ckPs.x=gPS.x+(gPS.w-ckPs.w)*Percent,ckPs.y=gPS.y,ckPs.h=gPS.h;
			}
			
		public:
			inline void SetBarColor(int p,const RGBA &co)
			{
				if (InRange(p,0,2))
				{
					BarColor[p]=!co?ThemeColorM[p*2]:co;
					Win->SetPresentArea(CoverLmt);
				}
				else PUI_DD[1]<<"Slider: SetBarColor: p "<<p<<" is not in Range[0,2]"<<endl;
			}
			
			inline void SetChunkColor(int p,const RGBA &co)
			{
				if (InRange(p,0,2))
				{
					ChunkColor[p]=!co?ThemeColorM[p*2+1]:co;
					Win->SetPresentArea(CoverLmt);
				}
				else PUI_DD[1]<<"Slider: SetChunkColor: p "<<p<<" is not in Range[0,2]"<<endl;
			}
			
			void SetPercent(double per,bool triggerFunc=1)
			{
				if (stat==2) return;
				Percent=EnsureInRange(per,0,1);
				Win->SetPresentArea(ckPs&CoverLmt);
				if (Vertical) ckPs.y=gPS.y+(gPS.h-ckPs.h)*Percent;
				else ckPs.x=gPS.x+(gPS.w-ckPs.w)*Percent;
				if (triggerFunc)
					if (func!=NULL)
						func(funcData,Percent,1);
				Win->SetPresentArea(ckPs&CoverLmt);
			}
			
			inline void SetPercent_Delta(double per,bool triggerFunc=1)
			{SetPercent(Percent+per,triggerFunc);}
			
			inline double GetPercent() const
			{return Percent;}
			
			inline void SetFunc(void (*_func)(T&,double,bool),const T &_funcdata)
			{
				func=_func;
				funcData=_funcdata;
			}
			
			inline void SetBarWidth(int w)
			{
				barWidth=max(0,w);
				Win->SetPresentArea(CoverLmt);
			}
			
			void SetChunkWidth(int w)
			{
				Win->SetPresentArea(ckPs&CoverLmt);
				if (Vertical)
					ckPs.h=w,ckPs.y=gPS.y+(gPS.h-ckPs.h)*Percent;
				else ckPs.w=w,ckPs.x=gPS.x+(gPS.w-ckPs.w)*Percent;
				Win->SetPresentArea(ckPs&CoverLmt);
			}
			
			Slider(const WidgetID &_ID,Widgets *_fa,const Posize &_rps,bool _vertical,void (*_func)(T&,double,bool),const T &_funcdata)
			:Widgets(_ID,WidgetType_Slider,_fa,_rps),Vertical(_vertical),func(_func),funcData(_funcdata) {}
			
			Slider(const WidgetID &_ID,Widgets *_fa,PosizeEX *psex,bool _vertical,void (*_func)(T&,double,bool),const T &_funcdata)
			:Widgets(_ID,WidgetType_Slider,_fa,psex),Vertical(_vertical),func(_func),funcData(_funcdata) {}
			
			Slider(const WidgetID &_ID,Widgets *_fa,const Posize &_rps,bool _vertical)
			:Widgets(_ID,WidgetType_Slider,_fa,_rps),Vertical(_vertical) {}
			
			Slider(const WidgetID &_ID,Widgets *_fa,PosizeEX *psex,bool _vertical)
			:Widgets(_ID,WidgetType_Slider,_fa,psex),Vertical(_vertical) {}
	};
	
	template <class T> class FullFillSlider:public Widgets
	{
		protected:
			enum
			{
				Stat_NoFocus=0,
				Stat_UpFocus=1,
				Stat_DownSlide=2
			}stat=Stat_NoFocus;
			bool Vertical;
			double Percent=0;
			void (*func)(T&,double,bool)=NULL;//double: percent bool:IsLooseMouse(set 0 when continuous changing)
			T funcData;
			RGBA BarColor[3]{ThemeColorM[0],ThemeColorM[2],ThemeColorM[4]},
				 ChunkColor[3]{ThemeColorM[1],ThemeColorM[3],ThemeColorM[5]};
			
			void SetChunkPs(const Point &pt,bool bo)
			{
				if (Vertical) Percent=(pt.y-gPS.y)*1.0/gPS.h;
				else Percent=(pt.x-gPS.x)*1.0/gPS.w;
				if (func!=NULL)
					func(funcData,EnsureInRange(Percent,0,1),bo);
			}
			
			virtual void CheckPos(const PUI_PosEvent *event,int mode)
			{
				if (mode&PosMode_LoseFocus)
				{
					if (stat!=Stat_NoFocus&&Win->GetOccupyPosWg()!=this)
						if (!Win->IsPosFocused()||!CoverLmt.In(event->pos)||mode&PosMode_ForceLoseFocus)
						{
//							PUI_DD[0]<<"FullFillSlider "<<IDName<<" losefocus"<<endl;
							stat=Stat_NoFocus;
							RemoveNeedLoseFocus();
							Win->SetPresentArea(CoverLmt);
						}
				}
				else if (mode&(PosMode_Default|PosMode_Occupy))
					switch (event->posType)
					{
						case PUI_PosEvent::Pos_Down:
							if (event->button==PUI_PosEvent::Button_MainClick)
							{
//								PUI_DD[0]<<"FullFillSlider "<<IDName<<" click"<<endl;
								stat=Stat_DownSlide;
								SetChunkPs(event->pos,0);
								SetNeedLoseFocus();
								Win->SetOccupyPosWg(this);
								Win->StopSolvePosEvent();
								Win->SetPresentArea(CoverLmt);
							}
							break;
						case PUI_PosEvent::Pos_Up:
							if (stat==Stat_DownSlide)
							{
//								PUI_DD[0]<<"FullFillSlider "<<IDName<<" loose"<<endl;
								if (event->type==event->Event_TouchEvent)
								{
									RemoveNeedLoseFocus();
									stat=Stat_NoFocus;
								}
								else stat=Stat_UpFocus;
								SetChunkPs(event->pos,1);
								Win->SetOccupyPosWg(NULL);
								Win->StopSolvePosEvent();
								Win->SetPresentArea(CoverLmt);
							}
							break;
						case PUI_PosEvent::Pos_Motion:
							if (stat==Stat_DownSlide)
							{
//								PUI_DD[0]<<"FullFillSlider "<<IDName<<" slide"<<endl;
								double per=Percent;
								SetChunkPs(event->pos,0);
								Win->SetPresentArea((Vertical?Posize(0,min(per,Percent)*gPS.h,gPS.w,ceil(fabs(per-Percent)*gPS.h)):Posize(min(per,Percent)*gPS.w,0,ceil(fabs(per-Percent)*gPS.w),gPS.h))+gPS);
							}
							else if (stat==Stat_NoFocus)
							{
//								PUI_DD[0]<<"FullFillSlider "<<IDName<<" focus"<<endl;
								stat=Stat_UpFocus;
								SetNeedLoseFocus();
								Win->SetPresentArea(CoverLmt);
							}
							Win->StopSolvePosEvent();
							break;
					}
			}
			
			virtual void Show(Posize &lmt)
			{
				if (Vertical)
					Win->RenderFillRect(Posize(gPS.x,gPS.y,gPS.w,gPS.h*Percent)&lmt,ThemeColor(ChunkColor[stat])),
					Win->RenderFillRect(Posize(gPS.x,gPS.y+gPS.h*Percent,gPS.w,gPS.h-gPS.h*Percent)&lmt,ThemeColor(BarColor[stat]));
				else
					Win->RenderFillRect(Posize(gPS.x,gPS.y,gPS.w*Percent,gPS.h)&lmt,ThemeColor(ChunkColor[stat])),
					Win->RenderFillRect(Posize(gPS.x+gPS.w*Percent,gPS.y,gPS.w-gPS.w*Percent,gPS.h)&lmt,ThemeColor(BarColor[stat]));
				Win->Debug_DisplayBorder(gPS);
			}
			
		public:
			inline void SetBarColor(int p,const RGBA &co)
			{
				if (InRange(p,0,2))
				{
					BarColor[p]=!co?ThemeColorM[p]:co;
					Win->SetPresentArea(CoverLmt);
				}
				else PUI_DD[1]<<"FullFillSlider: SetBarColor: p "<<p<<" is not in Range[0,2]"<<endl;
			}
			
			inline void SetChunkColor(int p,const RGBA &co)
			{
				if (InRange(p,0,2))
				{
					ChunkColor[p]=!co?ThemeColorM[p*2+1]:co;
					Win->SetPresentArea(CoverLmt);
				}
				else PUI_DD[1]<<"FullFillSlider: SetChunkColor: p "<<p<<" is not in Range[0,2]"<<endl;
			}
			
			void SetPercent(double per,bool triggerFunc=1)
			{
				if (stat==2) return;
				per=EnsureInRange(per,0,1);
				Win->SetPresentArea(((Vertical?Posize(0,min(per,Percent)*gPS.h,gPS.w,ceil(fabs(per-Percent)*gPS.h)):Posize(min(per,Percent)*gPS.w,0,ceil(fabs(per-Percent)*gPS.w),gPS.h))+gPS)&CoverLmt);
				Percent=per;
				if (triggerFunc)
					if (func!=NULL)
						func(funcData,Percent,1);
			}
			
			inline void SetPercent_Delta(double per,bool triggerFunc=1)
			{SetPercent(Percent+per,triggerFunc);}
			
			inline void SetFunc(void (*_func)(T&,double,bool),const T &_funcdata)
			{
				func=_func;
				funcData=_funcdata;
			}
			
			FullFillSlider(const WidgetID &_ID,Widgets *_fa,const Posize &_rps,bool _vertical,void (*_func)(T&,double,bool),const T &_funcdata)
			:Widgets(_ID,WidgetType_FullFillSlider,_fa,_rps),Vertical(_vertical),func(_func),funcData(_funcdata) {}
			
			FullFillSlider(const WidgetID &_ID,Widgets *_fa,PosizeEX *psex,bool _vertical,void (*_func)(T&,double,bool),const T &_funcdata)
			:Widgets(_ID,WidgetType_FullFillSlider,_fa,psex),Vertical(_vertical),func(_func),funcData(_funcdata) {}
			
			FullFillSlider(const WidgetID &_ID,Widgets *_fa,const Posize &_rps,bool _vertical)
			:Widgets(_ID,WidgetType_FullFillSlider,_fa,_rps),Vertical(_vertical) {}
			
			FullFillSlider(const WidgetID &_ID,Widgets *_fa,PosizeEX *psex,bool _vertical)
			:Widgets(_ID,WidgetType_FullFillSlider,_fa,psex),Vertical(_vertical) {}
	};
	
	class ProgressBar:public Widgets//Need Reconstruct...
	{
		protected:
			double Percent=0;
			bool Vertical=0;
			RGBA BackgroundColor=ThemeColorM[0],
				 BarColor=ThemeColorM[4],
				 FullColor=ThemeColorM[7],
				 BorderColor=RGBA_NONE;
			
			virtual void Show(Posize &lmt)
			{
				Win->RenderFillRect(gPS&lmt,ThemeColor(BackgroundColor));
				Win->RenderFillRect((Vertical?Posize(gPS.x,gPS.y+gPS.h*(1-Percent),gPS.w,gPS.h*Percent):Posize(gPS.x,gPS.y,gPS.w*Percent,gPS.h)).Shrink(0)&lmt,
									Percent==1?ThemeColor(FullColor):ThemeColor(BarColor));
				if (BorderColor!=RGBA_NONE)
					Win->RenderDrawRectWithLimit(gPS,ThemeColor(BorderColor),lmt);
				
				Win->Debug_DisplayBorder(gPS);
			}

		public:
			inline void SetBackgroundColor(const RGBA &co)
			{
				BackgroundColor=!co?ThemeColorM[0]:co;
				Win->SetPresentArea(CoverLmt);
			}
			
			inline void SetBarColor(const RGBA &co)
			{
				BarColor=!co?ThemeColorM[4]:co;
				Win->SetPresentArea(CoverLmt);
			}
			
			inline void SetFullColor(const RGBA &co)
			{
				FullColor=!co?ThemeColor[7]:co;
				if (Percent==1)
					Win->SetPresentArea(CoverLmt);
			}
			
			inline void SetBorderColor(const RGBA &co)
			{BorderColor=co;}
			
			inline void SetPercent(double per)
			{
				per=EnsureInRange(per,0,1);
				if (per==Percent) return;
				if (per==1||Percent==1)
					Win->SetPresentArea(CoverLmt);
				if (Vertical)
					Win->SetPresentArea(Posize(gPS.x,gPS.y+gPS.h*(1-min(Percent,per))-1,gPS.w,gPS.h*fabs(per-Percent)+2)&CoverLmt);
				else Win->SetPresentArea(Posize(gPS.x+gPS.w*min(Percent,per)-1,gPS.y,gPS.w*fabs(per-Percent)+2,gPS.h)&CoverLmt);
				Percent=per;  
			}
			
			inline double GetPercent() const
			{return Percent;}
			
			ProgressBar(const WidgetID &_ID,Widgets *_fa,const Posize &_rps,bool _vertical=0)
			:Widgets(_ID,WidgetType_ProgressBar,_fa,_rps),Vertical(_vertical) {}
			
			ProgressBar(const WidgetID &_ID,Widgets *_fa,PosizeEX *psex,bool _vertical=0)
			:Widgets(_ID,WidgetType_ProgressBar,_fa,psex),Vertical(_vertical) {}
	};
	
	template <class T> class PictureBox:public Widgets
	{
		protected:
			enum
			{
				Stat_NoFocus=0,
				Stat_UpFocus=1,
				Stat_DownLeftTwice=2,
				Stat_DownRight=3,
				Stat_DownLeftOnce=4,
//				Stat_DownOther=5//??
			}stat=Stat_NoFocus;
			SharedTexturePtr src;
			SDL_Texture *tex=NULL;//for special use such as easyffmpeg.
			Posize srcPS;
			Layer *ShownPicLayer=NULL;//for special use such as Marking in pic.
			int Mode=0;//0:Fullfill rPS 1:show part that can see in original size,pin centre(MM)
					   //2:like 1,pin LU 3:like 1,pin RU 4:like 1,pin LD 5:like 1,pin RD
					   //6:like 1,pin MU 7:like 1,pin MD 8:like 1,pin ML 9:like 1,pin MR (Mid,Left,Right,Up,Down)
					   //10:show all,pin center,if src is small and can see all,draw in original size(Mode 1),else Mode 11
					   //11:show all that can be seen with fiexd w/h rate,pin centre(MM)
					   //12:like 11,pin LU 13:like 11,pin RU 14:like 11,pin LD 15:like 11,pin RD 
					   //16:like 11,pin MU 17:like 11,pin MD 18:like 11,pin ML 19:like 11,pin MR 
					   //21:fill box and show most pic,pin centre(MM)
					   //22:like 21,pin LU 23:like 21,pin RU 24:like 21,pin LD 25:like 21,pin RD
					   //26:like 21,pin MU 27:like 21,pin MD 28:like 21,pin ML 29:like 21,pin MR
					   //-1: InLarge
			void (*func)(T&,int)=NULL;//int: 0:None 1:Left_Click 2:Left_Double_Click 3:Right_Click
			int (*delayLoadFunc)(T&,PictureBox<T>*)=nullptr;//It will be called when show, if retunrn non-zero, it will be set nullptr;It may be slow if load picture in this func;
			T funcData;
			RGBA BackGroundColor=RGBA_NONE;
			PosizeEX_InLarge *psexInLarge=nullptr;

			class PosizeEX_ForShowPicLayer:public PosizeEX
			{
				protected: 
					PictureBox *tar=NULL;
					
				public:
					virtual void GetrPS(Posize &ps)
					{
						if (nxt!=NULL) nxt->GetrPS(ps);
						ps=tar->GetPictureShownPS()-tar->GetgPS();
					}
					
					PosizeEX_ForShowPicLayer(PictureBox *_tar):tar(_tar) {}
			};

			virtual void CheckPos(const PUI_PosEvent *event,int mode)
			{
				if (func==NULL) return;
				if (mode&PosMode_LoseFocus)
				{
					if (stat!=Stat_NoFocus)
						if (!Win->IsPosFocused()||!CoverLmt.In(event->pos)||mode&PosMode_ForceLoseFocus)
						{
							stat=Stat_NoFocus;
							RemoveNeedLoseFocus();
						}
				}
				else if (mode&PosMode_Default)
					switch (event->posType)
					{
						case PUI_PosEvent::Pos_Down:
							PUI_DD[0]<<"PictureBox "<<IDName<<" click"<<endl;
							if (event->button==PUI_PosEvent::Button_MainClick)
								if (event->clicks==2)
									stat=Stat_DownLeftTwice;
								else stat=Stat_DownLeftOnce;
							else if (event->button==PUI_PosEvent::Button_SubClick)
								stat=Stat_DownRight;
							else stat=Stat_DownLeftOnce;//??
							Win->StopSolvePosEvent();
							break;
						case PUI_PosEvent::Pos_Up:
							if (InRange(stat,Stat_DownLeftTwice,Stat_DownLeftOnce))
							{
								if (event->type==event->Event_TouchEvent)//??
									if (event->button==PUI_PosEvent::Button_MainClick)
										if (event->clicks==2)
											stat=Stat_DownLeftTwice;
										else stat=Stat_DownLeftOnce;
									else if (event->button==PUI_PosEvent::Button_SubClick)
										stat=Stat_DownRight;
								if (func!=NULL)
								{
									PUI_DD[0]<<"PictureBox "<<IDName<<" function "<<(stat==Stat_DownLeftOnce||stat==Stat_DownLeftTwice?"left":"right")<<endl;
									func(funcData,stat==4?1:stat);//??
									Win->StopSolvePosEvent();
								}
								stat=Stat_UpFocus;
							}
							break;
						case PUI_PosEvent::Pos_Motion:
							if (stat==Stat_NoFocus)
							{
								stat=Stat_UpFocus;
								SetNeedLoseFocus();
							}
							Win->StopSolvePosEvent();
							break;
					}
			}
			
			Point GetShownPointFromMode(int mode)
			{
				switch (mode)
				{
					case 1:	return Point(gPS.x+(gPS.w-srcPS.w>>1),gPS.y+(gPS.h-srcPS.h>>1));
					case 2:	return gPS.GetLU();
					case 3:	return Point(gPS.x2()-srcPS.w,gPS.y);
					case 4:	return Point(gPS.x,gPS.y2()-srcPS.h);
					case 5:	return Point(gPS.x2()-srcPS.w,gPS.y2()-srcPS.h);
					case 6:	return Point(gPS.x+(gPS.w-srcPS.w>>1),gPS.y);
					case 7:	return Point(gPS.x+(gPS.w-srcPS.w>>1),gPS.y2()-srcPS.h);
					case 8:	return Point(gPS.x,gPS.y+(gPS.h-srcPS.h>>1));
					case 9:	return Point(gPS.x2()-srcPS.w,gPS.y+(gPS.h-srcPS.h>>1));
					default:return ZERO_POINT;
				}
			}
			
			Posize GetShownPosizeFromMode(int mode)
			{
				switch (mode)
				{
					case 0:		return gPS;
					case 11:	return (srcPS.Slope()>=rPS.Slope()?Posize((gPS.w-gPS.h*srcPS.InverseSlope())/2,0,gPS.h*srcPS.InverseSlope(),gPS.h):Posize(0,(gPS.h-gPS.w*srcPS.Slope())/2,gPS.w,gPS.w*srcPS.Slope()))+gPS;
					case 12:	return (srcPS.Slope()>=rPS.Slope()?Posize(0,0,gPS.h*srcPS.InverseSlope(),gPS.h):Posize(0,0,gPS.w,gPS.w*srcPS.Slope()))+gPS;
					case 13:	return (srcPS.Slope()>=rPS.Slope()?Posize(gPS.w-gPS.h*srcPS.InverseSlope(),0,gPS.h*srcPS.InverseSlope(),gPS.h):Posize(0,0,gPS.w,gPS.w*srcPS.Slope()))+gPS;
					case 14:	return (srcPS.Slope()>=rPS.Slope()?Posize(0,0,gPS.h*srcPS.InverseSlope(),gPS.h):Posize(0,gPS.h-gPS.w*srcPS.Slope(),gPS.w,gPS.w*srcPS.Slope()))+gPS;
					case 15:	return (srcPS.Slope()>=rPS.Slope()?Posize(gPS.w-gPS.h*srcPS.InverseSlope(),0,gPS.h*srcPS.InverseSlope(),gPS.h):Posize(0,gPS.h-gPS.w*srcPS.Slope(),gPS.w,gPS.w*srcPS.Slope()))+gPS;
					case 16:	return (srcPS.Slope()>=rPS.Slope()?Posize((gPS.w-gPS.h*srcPS.InverseSlope())/2,0,gPS.h*srcPS.InverseSlope(),gPS.h):Posize(0,0,gPS.w,gPS.w*srcPS.Slope()))+gPS;
					case 17:	return (srcPS.Slope()>=rPS.Slope()?Posize((gPS.w-gPS.h*srcPS.InverseSlope())/2,0,gPS.h*srcPS.InverseSlope(),gPS.h):Posize(0,gPS.h-gPS.w*srcPS.Slope(),gPS.w,gPS.w*srcPS.Slope()))+gPS;
					case 18:	return (srcPS.Slope()>=rPS.Slope()?Posize(0,0,gPS.h*srcPS.InverseSlope(),gPS.h):Posize(0,(gPS.h-gPS.w*srcPS.Slope())/2,gPS.w,gPS.w*srcPS.Slope()))+gPS;
					case 19:	return (srcPS.Slope()>=rPS.Slope()?Posize(gPS.w-gPS.h*srcPS.InverseSlope(),0,gPS.h*srcPS.InverseSlope(),gPS.h):Posize(0,(gPS.h-gPS.w*srcPS.Slope())/2,gPS.w,gPS.w*srcPS.Slope()))+gPS;
					default:	return ZERO_POSIZE;
				}
			}
			
			Posize GetSourcePosizeFromMode(int mode)
			{
				switch (mode)
				{
					case 21:	return srcPS.Slope()>=rPS.Slope()?Posize(0,(srcPS.h-srcPS.w*gPS.Slope())/2,srcPS.w,ceil(srcPS.w*gPS.Slope())):Posize((srcPS.w-srcPS.h*gPS.InverseSlope())/2,0,ceil(srcPS.h*gPS.InverseSlope()),srcPS.h);
					case 22:	return srcPS.Slope()>=rPS.Slope()?Posize(0,0,srcPS.w,ceil(srcPS.w*gPS.Slope())):Posize(0,0,ceil(srcPS.h*gPS.InverseSlope()),srcPS.h);
					case 23:	return srcPS.Slope()>=rPS.Slope()?Posize(0,0,srcPS.w,ceil(srcPS.w*gPS.Slope())):Posize(srcPS.w-srcPS.h*gPS.InverseSlope(),0,ceil(srcPS.h*gPS.InverseSlope()),srcPS.h);
					case 24:	return srcPS.Slope()>=rPS.Slope()?Posize(0,srcPS.h-srcPS.w*gPS.Slope(),srcPS.w,ceil(srcPS.w*gPS.Slope())):Posize(0,0,ceil(srcPS.h*gPS.InverseSlope()),srcPS.h);
					case 25:	return srcPS.Slope()>=rPS.Slope()?Posize(0,srcPS.h-srcPS.w*gPS.Slope(),srcPS.w,ceil(srcPS.w*gPS.Slope())):Posize(srcPS.w-srcPS.h*gPS.InverseSlope(),0,ceil(srcPS.h*gPS.InverseSlope()),srcPS.h);
					case 26:	return srcPS.Slope()>=rPS.Slope()?Posize(0,0,srcPS.w,ceil(srcPS.w*gPS.Slope())):Posize((srcPS.w-srcPS.h*gPS.InverseSlope())/2,0,ceil(srcPS.h*gPS.InverseSlope()),srcPS.h);
					case 27:	return srcPS.Slope()>=rPS.Slope()?Posize(0,srcPS.h-srcPS.w*gPS.Slope(),srcPS.w,ceil(srcPS.w*gPS.Slope())):Posize((srcPS.w-srcPS.h*gPS.InverseSlope())/2,0,ceil(srcPS.h*gPS.InverseSlope()),srcPS.h);
					case 28:	return srcPS.Slope()>=rPS.Slope()?Posize(0,(srcPS.h-srcPS.w*gPS.Slope())/2,srcPS.w,ceil(srcPS.w*gPS.Slope())):Posize(0,0,ceil(srcPS.h*gPS.InverseSlope()),srcPS.h);
					case 29:	return srcPS.Slope()>=rPS.Slope()?Posize(0,(srcPS.h-srcPS.w*gPS.Slope())/2,srcPS.w,ceil(srcPS.w*gPS.Slope())):Posize(srcPS.w-srcPS.h*gPS.InverseSlope(),0,ceil(srcPS.h*gPS.InverseSlope()),srcPS.h);
					default:	return ZERO_POSIZE;
				}
			}

			virtual void Show(Posize &lmt)
			{
				if (delayLoadFunc!=nullptr)
					if (delayLoadFunc(funcData,this))
						delayLoadFunc=nullptr;
				SDL_Texture *tar=!!src?src():tex;
				Win->RenderFillRect(gPS&lmt,BackGroundColor);
				if (tar!=NULL)
					if (Mode==-1)
						Win->RenderCopyWithLmt(tar,gPS,lmt);
					else if (InRange(Mode,1,9))
						Win->RenderCopyWithLmt(tar,GetShownPointFromMode(Mode),lmt);
					else if (InRange(Mode,11,19)||Mode==0)
						Win->RenderCopyWithLmt(tar,GetShownPosizeFromMode(Mode),lmt);
					else if (InRange(Mode,21,29))
						Win->RenderCopyWithLmt(tar,GetSourcePosizeFromMode(Mode),gPS,lmt);
					else if (Mode==10)
						if (gPS.ToOrigin().In(srcPS))
							Win->RenderCopyWithLmt(tar,GetShownPointFromMode(1),lmt);
						else Win->RenderCopyWithLmt(tar,GetShownPosizeFromMode(11),lmt);
					else
					{
						PUI_DD[2]<<"PictureBox: Mode "<<Mode<<" is not in valid range!"<<endl;
						Win->RenderCopyWithLmt(tar,gPS,lmt);
					}
				Win->Debug_DisplayBorder(gPS);
			}
			
			virtual void CalcPsEx()
			{
				if (PsEx!=NULL)
					PsEx->GetrPS(rPS);
				rPS.h=srcPS.h*1.0/srcPS.w*rPS.w;
				Posize lastPs=CoverLmt;
				if (fa!=NULL)
					gPS=rPS+fa->GetgPS();
				else gPS=rPS;
				CoverLmt=gPS&GetFaCoverLmt();
				if (!(lastPs==CoverLmt))
					Win->SetPresentArea(lastPs|CoverLmt);
			}
			
		public:
			inline void SetBackgroundColor(const RGBA &co)
			{
				BackGroundColor=co;
				Win->SetPresentArea(CoverLmt);
			}
			
			void SetPicture(const SharedTexturePtr &_src,int _mode=0)
			{
				src=_src;
				tex=NULL;
				if (Mode!=-1)
					Mode=_mode;
				else Win->SetNeedUpdatePosize();
				srcPS=GetTexturePosize(src());
				Win->SetPresentArea(CoverLmt);
			}
			
			void SetPicture(SDL_Texture *_tex)
			{
				tex=_tex;
				src=SharedTexturePtr(NULL);
				srcPS=GetTexturePosize(tex);
				if (Mode==-1)
					Win->SetNeedUpdatePosize();
				Win->SetPresentArea(CoverLmt);
			}
			
			void SetShowMode(int _mode)
			{
				if (Mode==_mode||Mode==-1)
					return;
				Mode=_mode;
				Win->SetPresentArea(CoverLmt);
			}
			
			inline int GetShowMode() const
			{return Mode;}
			
			inline SharedTexturePtr GetPicture()
			{return src;}
			
			inline SDL_Texture* GetTexture()
			{return tex;}
			
			inline bool HavePicture()
			{return src()!=nullptr||tex!=nullptr;}
			
			inline Posize GetPictureSize() const
			{return srcPS;}
			
			inline Posize GetPictureShownPS()
			{
				if (InRange(Mode,1,9))
					return srcPS+GetShownPointFromMode();
				else return GetShownPosizeFromMode();
			}
			
			inline Layer* SetEnableShownPicLayer(bool on)//maybe exsit bugs??
			{
				if (on==(ShownPicLayer!=NULL))
					return ShownPicLayer;
				if (on)
					return ShownPicLayer=new Layer(0,this,new PosizeEX_ForShowPicLayer(this));
				else return DelayDeleteToNULL(ShownPicLayer);
			}
			
			inline void SetFunc(void (*_func)(T&,int),const T &_funcdata)
			{
				func=_func;
				funcData=_funcdata;
			}
			
			inline void SetFunc(void (*_func)(T&,int))
			{func=_func;}
			
			inline void SetFuncData(const T &_funcdata)
			{funcData=_funcdata;}
			
			inline T& GetFuncData()
			{return funcData;}
			
			inline void SetDelayLoadFunc(int (*_delayloadfunc)(T&,PictureBox<T>*))
			{delayLoadFunc=_delayloadfunc;}
			
			PictureBox(const WidgetID &_ID,PosizeEX_InLarge *largePsex)
			:Widgets(_ID,WidgetType_PictureBox),psexInLarge(largePsex)
			{
				SetFa(largePsex->GetLargeLayer()->LargeArea());
				AddPsEx(largePsex);
				Mode=-1;
			}
			
			PictureBox(const WidgetID &_ID,Widgets *_fa,const Posize &_rps)
			:Widgets(_ID,WidgetType_PictureBox,_fa,_rps) {}
			
			PictureBox(const WidgetID &_ID,Widgets *_fa,PosizeEX *psex)
			:Widgets(_ID,WidgetType_PictureBox,_fa,psex) {}
	};
	
	class PhotoViewer:public Widgets
	{
		protected:
			enum
			{
				Stat_NoFocus=0,
				Stat_UpFocus=1,
				Stat_DownLeft=2,
				Stat_DownRigth=3 
			}stat=Stat_NoFocus;
			SharedTexturePtr tex;
			SDL_Surface *sur=NULL;
			LargeLayerWithScrollBar *fa2=NULL;
			Posize srcPS=ZERO_POSIZE;
			double lastPer=1,
				   per=1,
				   delta0=0.2,
				   minPer=1,
				   maxPer=16;
//				   fingerDist1;
			bool SurfaceMode=0,
				 AutoFreeSurface=0;
			Point LastPt=ZERO_POINT,
				  ScaleCentre=ZERO_POINT;
			void (*RightClickFunc)(void*)=NULL;
			void *RightClickFuncData=NULL;

			void RecalcPic()
			{
				rPS.w=srcPS.w*per;
				rPS.h=srcPS.h*per;
				if (rPS.w<=fa2->GetrPS().w)
				{
					rPS.x=(fa2->GetrPS().w-rPS.w)/2;
					fa2->ResizeLL(fa2->GetrPS().w,0);
				}
				else
				{
					rPS.x=0;
					fa2->ResizeLL(rPS.w,0);
					fa2->SetViewPort(1,-(ScaleCentre.x-(ScaleCentre.x-gPS.x)*per/lastPer-fa2->GetgPS().x));
				}
				
				if (rPS.h<=fa2->GetrPS().h)
				{
					rPS.y=(fa2->GetrPS().h-rPS.h)/2;
					fa2->ResizeLL(0,fa2->GetrPS().h);
				}
				else
				{
					rPS.y=0;
					fa2->ResizeLL(0,rPS.h);
					fa2->SetViewPort(2,-(ScaleCentre.y-(ScaleCentre.y-gPS.y)*per/lastPer-fa2->GetgPS().y));
				}
			}

			void MovePic(const Point &pt)
			{
				fa2->SetViewPort(5,LastPt.x-pt.x);
				fa2->SetViewPort(6,LastPt.y-pt.y);
				Win->SetPresentArea(CoverLmt);
				LastPt=pt;
			}
			
			void MovePicDelta(const Point &delta)
			{
				fa2->SetViewPort(5,-delta.x);
				fa2->SetViewPort(6,-delta.y);
			}
			
			virtual void CheckEvent(const PUI_Event *event)
			{
				if (event->type==PUI_Event::Event_WheelEvent)
					if (stat!=Stat_NoFocus)
					{
						double delta=per*delta0;
						lastPer=per;
						per+=event->WheelEvent()->dy*delta;
						per=EnsureInRange(per,minPer,maxPer);
						RecalcPic();
						Win->StopSolveEvent();
					}
			}

			virtual void CheckPos(const PUI_PosEvent *event,int mode)
			{
				if (mode&PosMode_LoseFocus)
				{
					if (stat!=Stat_NoFocus&&Win->GetOccupyPosWg()!=this)
						if (!Win->IsPosFocused()||!CoverLmt.In(event->pos)||mode&PosMode_ForceLoseFocus)
						{
							stat=Stat_NoFocus;
							RemoveNeedLoseFocus();
						}
				}
				else if (mode&(PosMode_Default|PosMode_Occupy))
					switch (event->posType)
					{
						case PUI_PosEvent::Pos_Down:
							PUI_DD[0]<<"Button Down In Photoviewer "<<IDName<<endl;
							if (event->button==PUI_PosEvent::Button_MainClick)
							{
								stat=Stat_DownLeft;
								ScaleCentre=LastPt=event->pos;
								Win->SetOccupyPosWg(this);
							}
							if (event->button==PUI_PosEvent::Button_SubClick)
								stat=Stat_DownRigth;
							Win->StopSolvePosEvent();
							break;
						case PUI_PosEvent::Pos_Up:
							PUI_DD[0]<<"Mouse button up in Photoviewer "<<IDName<<endl;
							if (stat==Stat_DownLeft)
								Win->SetOccupyPosWg(NULL);
							else if (stat==Stat_DownRigth)
								if (RightClickFunc!=NULL)
									RightClickFunc(RightClickFuncData);
							stat=Stat_UpFocus;
							if (!NeedLoseFocus)
								SetNeedLoseFocus();
							Win->StopSolvePosEvent();
							break;
						case PUI_PosEvent::Pos_Motion:
							ScaleCentre=event->pos;
							if (event->type==PUI_Event::Event_TouchEvent&&stat==Stat_DownLeft)
							{
								PUI_TouchEvent *touch=event->TouchEvent();
								if (touch->gesture==touch->Ges_MultiMoving)
								{
									lastPer=per;
									per=EnsureInRange(per*touch->dDiameterPercent(),minPer,maxPer);
									RecalcPic();
								}
								MovePicDelta(event->delta);
							}
							else
								if (stat==Stat_NoFocus)
								{
									stat=Stat_UpFocus;
									SetNeedLoseFocus();
								}
								else if (stat==Stat_DownLeft)
									MovePic(event->pos);
							Win->StopSolvePosEvent();
							break;
					}
			}
			
			virtual void Show(Posize &lmt)
			{
				lmt=gPS&lmt;
				Win->RenderCopyWithLmt(tex(),srcPS,gPS,lmt);
				Win->Debug_DisplayBorder(gPS);
			}
			
			virtual void CalcPsEx()
			{
				Posize lastPs=CoverLmt;
				if (PsEx!=NULL)	
					PsEx->GetrPS(rPS);
				static Posize lastFa2Ps=ZERO_POSIZE;
				if (tex()!=NULL&&(fa2->GetrPS().w!=lastFa2Ps.w||fa2->GetrPS().h!=lastFa2Ps.h))
				{
					minPer=min(1.0,min(fa2->GetrPS().w*1.0/srcPS.w,fa2->GetrPS().h*1.0/srcPS.h));
					lastPer=per=EnsureInRange(per,minPer,maxPer);
					RecalcPic();
					lastFa2Ps=fa2->GetrPS();
				}
				if (fa!=NULL)
					gPS=rPS+fa->GetgPS();
				else gPS=rPS;
				CoverLmt=gPS&GetFaCoverLmt();
				if (!(lastPs==CoverLmt))
					Win->SetPresentArea(lastPs|CoverLmt);
			}
			
		public:
			virtual void SetBackGroundColor(const RGBA &co)
			{fa2->SetLargeAreaColor(co);}
			
			void AddPsEx(PosizeEX *psex)
			{fa2->AddPsEx(psex);}
	
			void SetPic(const SharedTexturePtr &_tex)
			{
				if (SurfaceMode)
				{
					if (AutoFreeSurface)
						SDL_FreeSurface(sur);
					sur=NULL;
					AutoFreeSurface=0;
				}
				SurfaceMode=0;
				tex=_tex;
				if (!tex) return;
				srcPS=GetTexturePosize(tex());
				lastPer=per=minPer=min(1.0,min(fa2->GetrPS().w*1.0/srcPS.w,fa2->GetrPS().h*1.0/srcPS.h));
				RecalcPic();
			}
			
			void ReplacePic(const SharedTexturePtr &_tex)//replace picture with same height and width and do not change current scroll
			{
				if (!_tex) return;
				if (GetTexturePosize(tex())!=GetTexturePosize(_tex()))
				{
					PUI_DD[2]<<"PhotoViewer "<<IDName<<" ReplacePic with imcompatible size!"<<endl;
					return;
				}
				if (SurfaceMode)
				{
					if (AutoFreeSurface)
						SDL_FreeSurface(sur);
					sur=NULL;
					AutoFreeSurface=0;
				}
				SurfaceMode=0;
				tex=_tex;
				Win->SetPresentArea(CoverLmt);
			}
			
//			void SetPic(SDL_Surface *_sur,bool _AutoFreeSurface=1)
//			{
//				src=SharedTexturePtr();
//				if (SurfaceMode)
//					if (AutoFreeSurface)
//						SDL_FreeSurface(sur);
//				sur=_sur;
//				AutoFreeSurface=_AutoFreeSurface;
//				SurfaceMode=1;
//				src=
//			}

			void SetRightClickFunc(void (*_rightclickfunc)(void*)=NULL,void *rightclickfuncdata=NULL)
			{
				RightClickFunc=_rightclickfunc;
				RightClickFuncData=rightclickfuncdata;
			}
	
			~PhotoViewer()
			{
				if (SurfaceMode)
					if (AutoFreeSurface)
						SDL_FreeSurface(sur);
			}
			
			PhotoViewer(const WidgetID &_ID,Widgets *_fa,PosizeEX *psex)
			:Widgets(_ID,WidgetType_PhotoViewer)
			{
				fa2=new LargeLayerWithScrollBar(0,_fa,psex,ZERO_POSIZE);
				SetFa(fa2->LargeArea());
				fa2->SetScrollBarWidth(8);
				rPS=ZERO_POSIZE;
			}
			
			PhotoViewer(const WidgetID &_ID,Widgets *_fa,const Posize &_rPS)
			:Widgets(_ID,WidgetType_PhotoViewer)
			{
				fa2=new LargeLayerWithScrollBar(0,_fa,_rPS,_rPS.ToOrigin());
				SetFa(fa2->LargeArea());
				fa2->SetScrollBarWidth(8);
				rPS=ZERO_POSIZE;
			}
	};
	
	class BaseSequenceView:public Widgets
	{
		protected:
			
			
		public:
			
			
	};
	
	template <class T> class SimpleListView:public Widgets
	{
		protected:
			enum
			{
				Stat_NoFocus=0,
				Stat_UpFocusRow=1,
				Stat_DownClickTwice=2,
				Stat_DownClickRight=3,
				Stat_DownClickOnce=4 
			}stat=Stat_NoFocus;
			int	FocusChoose=-1,
				ClickChoose=-1;
			int ListCnt=0;
			vector <string> Text;
			vector <T> FuncData;//It means this widget's FuncData would be deconstructed when deleted
			void (*func)(T&,int,int)=NULL;//int1:Pos(CountFrom 0,if -1,means background)   int2: 0:None 1:Left_Click 2:Left_Double_Click 3:Right_Click
			T BackgroundFuncData;
			PosizeEX_InLarge *psexInLarge=NULL;
			int EachHeight=24,
				Interval=2,
				TextMode=-1;
			RGBA TextColor=ThemeColorMT[0],
				 RowColor[3]{ThemeColorM[0],ThemeColorM[2],ThemeColor[4]};
				 
			inline Posize GetLinePosize(int pos)
			{
				if (pos==-1) return ZERO_POSIZE;
				else return Posize(gPS.x,gPS.y+pos*(EachHeight+Interval),gPS.w,EachHeight);
			}
			
			inline int GetLineFromPos(int y)
			{
				int re=(y-gPS.y)/(EachHeight+Interval);
				if ((y-gPS.y)%(EachHeight+Interval)<EachHeight&&InRange(re,0,ListCnt-1))
					return re;
				else return -1;
			}
			
			virtual void CheckEvent(const PUI_Event *event)
			{
				
			}
			
			virtual void CheckPos(const PUI_PosEvent *event,int mode)
			{
				if (!Win->IsNeedSolvePosEvent()) return;
				if (mode&PosMode_LoseFocus)
				{
					if (stat!=Stat_NoFocus)
						if (!Win->IsPosFocused()||!CoverLmt.In(event->pos)||mode&PosMode_ForceLoseFocus)
						{
							stat=Stat_NoFocus;
							RemoveNeedLoseFocus();
							Win->SetPresentArea(GetLinePosize(FocusChoose)&CoverLmt);
							FocusChoose=-1;
						}
				}
				else if (mode&PosMode_Default)
					switch (event->posType)
					{
						case PUI_PosEvent::Pos_Down:
							if (ClickChoose!=-1)
								Win->SetPresentArea(GetLinePosize(ClickChoose)&CoverLmt);
							if (FocusChoose!=-1)
								Win->SetPresentArea(GetLinePosize(FocusChoose)&CoverLmt);
							ClickChoose=FocusChoose=GetLineFromPos(event->pos.y);
							if (event->button==PUI_PosEvent::Button_MainClick)
								if (event->clicks==2)
									stat=Stat_DownClickTwice;
								else stat=Stat_DownClickOnce;
							else if (event->button==PUI_PosEvent::Button_SubClick)
								stat=Stat_DownClickRight;
							else stat=Stat_DownClickOnce,PUI_DD[1]<<"SimpleListView: Unknown click button,use it as left click once"<<endl;
							Win->StopSolvePosEvent();
							if (ClickChoose!=-1)
								Win->SetPresentArea(GetLinePosize(ClickChoose)&CoverLmt);
							if (FocusChoose!=-1)
								Win->SetPresentArea(GetLinePosize(FocusChoose)&CoverLmt);
							break;
						case PUI_PosEvent::Pos_Up:
							if (stat!=Stat_NoFocus&&stat!=Stat_UpFocusRow)
							{
								PUI_DD[0]<<"SimpleListView "<<IDName<<" func "<<ClickChoose<<" "<<(ClickChoose!=-1?Text[ClickChoose]:"")<<endl;
								if (event->type==event->Event_TouchEvent)//??
									if (event->button==PUI_PosEvent::Button_MainClick)
										if (event->clicks==2)
											stat=Stat_DownClickTwice;
										else stat=Stat_DownClickOnce;
									else if (event->button==PUI_PosEvent::Button_SubClick)
										stat=Stat_DownClickRight;
								if (func!=NULL)
									func(ClickChoose!=-1?FuncData[ClickChoose]:BackgroundFuncData,ClickChoose,stat==4?1:stat);
								stat=Stat_UpFocusRow;
								Win->StopSolvePosEvent();
							}
							break;
						case PUI_PosEvent::Pos_Motion:
							if (stat==Stat_NoFocus)
							{
								stat=Stat_UpFocusRow;
								SetNeedLoseFocus();
							}
							int p=GetLineFromPos(event->pos.y);
							if (InRange(p,0,ListCnt-1))
							{
								if (p!=FocusChoose)
								{
									if (FocusChoose!=-1)
										Win->SetPresentArea(GetLinePosize(FocusChoose)&CoverLmt);
									Win->SetPresentArea(GetLinePosize(p)&CoverLmt);
									FocusChoose=p;
								}
							}
							else if (FocusChoose!=-1)
							{
								Win->SetPresentArea(GetLinePosize(FocusChoose));
								FocusChoose=-1;
							}	
							Win->StopSolvePosEvent();
							break;
					}
			}
			
			Range GetShowRange(Posize ps)
			{
				Range re;
				ps=ps&gPS;
				re.l=EnsureInRange((ps.y-gPS.y)/(EachHeight+Interval),0,ListCnt-1);
				re.r=EnsureInRange((ps.y2()-gPS.y+EachHeight+Interval-1)/(EachHeight+Interval),0,ListCnt-1);
				return re;
			}
			
			virtual void Show(Posize &lmt)
			{
				if (ListCnt==0) return;
				Range ForRange=GetShowRange(lmt);
				Posize RowPs=Posize(gPS.x,gPS.y+ForRange.l*(EachHeight+Interval),gPS.w,EachHeight);
				for (int i=ForRange.l;i<=ForRange.r;++i)
				{
					Win->RenderFillRect(RowPs&lmt,ThemeColor(RowColor[ClickChoose==i?2:FocusChoose==i]));
					Win->RenderDrawText(Text[i],TextMode==-1?RowPs+Point(8,0):(TextMode==1?RowPs-Point(24,0):RowPs),lmt,TextMode,ThemeColor(TextColor));
					Win->Debug_DisplayBorder(RowPs);
					RowPs.y+=EachHeight+Interval; 
				}	
				Win->Debug_DisplayBorder(gPS);
			}
			
			virtual void CalcPsEx()
			{
				if (PsEx!=NULL)
					PsEx->GetrPS(rPS);
				rPS.h=max(0,ListCnt*(EachHeight+Interval)-Interval);
				if (psexInLarge->NeedFullFill())
					rPS.h=max(rPS.h,psexInLarge->GetLargeLayer()->GetrPS().h-rPS.y-(psexInLarge->Fullfill()<0?-psexInLarge->Fullfill():0));
				Posize lastPs=CoverLmt;
				if (fa!=NULL)
					gPS=rPS+fa->GetgPS();
				else gPS=rPS;
				CoverLmt=gPS&GetFaCoverLmt();
				if (!(lastPs==CoverLmt))
					Win->SetPresentArea(lastPs|CoverLmt);
			}
			
			SimpleListView(const WidgetID &_ID,WidgetType _type,PosizeEX_InLarge *largePsex,void (*_func)(T&,int,int)=NULL)
			:Widgets(_ID,_type),func(_func),psexInLarge(largePsex)
			{
				SetFa(largePsex->GetLargeLayer()->LargeArea());
				AddPsEx(largePsex);
			}
			
			SimpleListView(const WidgetID &_ID,WidgetType _type,Widgets *_fa,const Posize &_rps,void (*_func)(T&,int,int)=NULL)
			:Widgets(_ID,_type),func(_func)
			{
				LargeLayerWithScrollBar *larlay=new LargeLayerWithScrollBar(0,_fa,_rps);
				SetFa(larlay->LargeArea());
				AddPsEx(psexInLarge=new PosizeEX_InLarge(larlay,0));
			}
			
			SimpleListView(const WidgetID &_ID,WidgetType _type,Widgets *_fa,PosizeEX *psex,void (*_func)(T&,int,int)=NULL)
			:Widgets(_ID,_type),func(_func)
			{
				LargeLayerWithScrollBar *larlay=new LargeLayerWithScrollBar(0,_fa,psex);
				SetFa(larlay->LargeArea());
				AddPsEx(psexInLarge=new PosizeEX_InLarge(larlay,0));
			}

		public:
			inline void SetTextColor(const RGBA &co)
			{
				TextColor=!co?ThemeColorMT[0]:co;
				Win->SetPresentArea(CoverLmt);
			}

			inline void SetRowColor(int p,const RGBA &co)
			{
				if (InRange(p,0,2))
				{
					RowColor[p]=!co?ThemeColorM[p*2]:co;
					Win->SetPresentArea(CoverLmt);
				}
				else PUI_DD[1]<<"SimpleListView: SetRowColor: p "<<p<<" is not in range[0,2]"<<endl;
			}			
			
			inline void SetRowHeightAndInterval(int _height,int _interval)
			{
				EachHeight=_height;
				Interval=_interval;
				rPS.h=max(0,ListCnt*(EachHeight+Interval)-Interval);
				Win->SetNeedUpdatePosize();
			}
			
			inline void SetTextMode(int mode)
			{
				TextMode=mode;
				Win->SetPresentArea(CoverLmt);
			}
			
			inline void SetListFunc(void (*_func)(T&,int,int))
			{func=_func;}
			
			void SetListContent(int p,const string &str,const T &_funcdata)//p: 0<=p<ListCnt:SetInP >=ListCnt:SetInLast <0:SetInFirst
			{
				p=EnsureInRange(p,0,ListCnt);
				Text.insert(Text.begin()+p,str);
				FuncData.insert(FuncData.begin()+p,_funcdata);
				++ListCnt;
				if (ClickChoose>=p) ++ClickChoose;
				rPS.h=max(0,ListCnt*(EachHeight+Interval)-Interval);
				Win->SetNeedUpdatePosize();
				Win->SetPresentArea(CoverLmt);//?? It don't need fresh such big area
			}
			
			void DeleteListContent(int p)//p: 0<=p<ListCnt:SetInP >=ListCnt:DeleteLast <0:DeleteFirst
			{
				if (!ListCnt||p==-1) return;
				p=EnsureInRange(p,0,ListCnt-1);
				Text.erase(Text.begin()+p);
				FuncData.erase(FuncData.begin()+p);
				--ListCnt;
				if (FocusChoose==ListCnt)
					FocusChoose=-1;
				if (ClickChoose>p) --ClickChoose;
				else if (ClickChoose==p) ClickChoose=-1;
				rPS.h=max(0,ListCnt*(EachHeight+Interval)-Interval);
				Win->SetNeedUpdatePosize();
				Win->SetPresentArea(CoverLmt);
			}
			
			void ClearListContent()
			{
				if (!ListCnt) return;
				PUI_DD[0]<<"Clear SimpleListViewContent: ListCnt:"<<ListCnt<<endl;
				FocusChoose=ClickChoose=-1;
				Text.clear();
				FuncData.clear();
				ListCnt=0;
				rPS.h=0;
				psexInLarge->SetShowPos(rPS.y);
				Win->SetNeedUpdatePosize();
				Win->SetPresentArea(CoverLmt);
			}
			
			SimpleListView* PushbackContent(const string &str,const T &_funcdata)
			{
				SetListContent(1e9,str,_funcdata);
				return this;
			}
			
			void SwapContent(int pos1,int pos2)
			{
				if (!InRange(pos1,0,ListCnt-1)||!InRange(pos2,0,ListCnt-1)||pos1==pos2) return;
				swap(Text[pos1],Text[pos2]);
				swap(FuncData[pos1],FuncData[pos2]);
				Win->SetPresentArea(GetLinePosize(pos1)&CoverLmt);
				Win->SetPresentArea(GetLinePosize(pos2)&CoverLmt);
			}
			
			int Find(const T &_funcdata)
			{
				for (int i=0;i<ListCnt;++i)
					if (FuncData[i]==_funcdata)
						return i;
				return -1;
			}
			
			void SetText(int p,const string &str)
			{
				p=EnsureInRange(p,0,ListCnt-1);
				Text[p]=str;
				Win->SetPresentArea(GetLinePosize(p)&CoverLmt);
			}
			
			void SetSelectLine(int p)
			{
				if (!InRange(p,-1,ListCnt-1))
					return;
				if (p==-1)
					Win->SetPresentArea(GetLinePosize(ClickChoose)&CoverLmt);
				else if (!GetShowRange(psexInLarge->GetLargeLayer()->GetgPS()).InRange(p))
					psexInLarge->SetShowPos(p*(EachHeight+Interval)-rPS.y);
				ClickChoose=p;
			}
			
			inline int GetListCnt()
			{return ListCnt;}
			
			T& GetFuncData(int p)
			{
				p=EnsureInRange(p,0,ListCnt-1);
				return FuncData[p];
			}
			
			void SetBackgroundFuncData(const T &data)
			{BackgroundFuncData=data;}
			
			LargeLayerWithScrollBar* GetLargeLayer()
			{return psexInLarge->GetLargeLayer();}
			
			void ResetLargeLayer(PosizeEX_InLarge *largePsex)//??
			{
				SetFa(largePsex->GetLargeLayer()->LargeArea());
				ReAddPsEx(largePsex);
				psexInLarge=largePsex;
			}
			
			SimpleListView(const WidgetID &_ID,PosizeEX_InLarge *largePsex,void (*_func)(T&,int,int)=NULL)
			:SimpleListView(_ID,WidgetType_SimpleListView,largePsex,_func) {}
			
			SimpleListView(const WidgetID &_ID,Widgets *_fa,const Posize &_rps,void (*_func)(T&,int,int)=NULL)
			:SimpleListView(_ID,WidgetType_SimpleListView,_fa,_rps,_func) {}
			
			SimpleListView(const WidgetID &_ID,Widgets *_fa,PosizeEX *psex,void (*_func)(T&,int,int)=NULL)
			:SimpleListView(_ID,WidgetType_SimpleListView,_fa,psex,_func) {}
	};
	
	template <class T> class SimpleListView_MultiColor:public SimpleListView <T>//maybe need reconstruct
	{
		public:
			struct EachRowColor0
			{
				RGBA row[3],
					 text=RGBA_NONE;
					
				EachRowColor0(const RGBA &row0,const RGBA &row1,const RGBA &row2,const RGBA &_text=ThemeColorMT[0])
				:row{row0,row1,row2},text(_text) {}
				
				EachRowColor0():row{RGBA_NONE,RGBA_NONE,RGBA_NONE} {}
			};
			static EachRowColor0 EachRowColor_NONE;
		
		protected:
			vector <EachRowColor0> EachRowColor;
			
			virtual void Show(Posize &lmt)
			{
				if (this->ListCnt==0) return;//Oh ! Too many this pointers! >_<
				Range ForRange=this->GetShowRange(lmt);
				Posize RowPs=Posize(this->gPS.x,this->gPS.y+ForRange.l*(this->EachHeight+this->Interval),this->gPS.w,this->EachHeight);
				
				for (int i=ForRange.l;i<=ForRange.r;++i)
				{
					this->Win->RenderFillRect(RowPs&lmt,ThemeColor(!this->EachRowColor[i].row[this->ClickChoose==i?2:this->FocusChoose==i]?this->RowColor[this->ClickChoose==i?2:this->FocusChoose==i]:this->EachRowColor[i].row[this->ClickChoose==i?2:this->FocusChoose==i]));//so long! *_*
					this->Win->RenderDrawText(this->Text[i],this->TextMode==-1?RowPs+Point(8,0):(this->TextMode==1?RowPs-Point(24,0):RowPs),lmt,this->TextMode,ThemeColor(!this->EachRowColor[i].text?this->TextColor:this->EachRowColor[i].text));
					this->Win->Debug_DisplayBorder(RowPs);
					RowPs.y+=this->EachHeight+this->Interval;
				}	
				this->Win->Debug_DisplayBorder(this->gPS);
			}
		
		public:
			void SetListContent(int p,const string &str,const T &_funcdata,const EachRowColor0 &co=EachRowColor_NONE)
			{
				p=EnsureInRange(p,0,this->ListCnt);
				EachRowColor.insert(EachRowColor.begin()+p,co);
				SimpleListView <T>::SetListContent(p,str,_funcdata);
			}
			
			SimpleListView_MultiColor* PushbackContent(const string &str,const T &_funcdata,const EachRowColor0 &co=EachRowColor_NONE)
			{
				SetListContent(1e9,str,_funcdata,co);
				return this;
			}
			
			void DeleteListContent(int p)
			{
				if (!this->ListCnt) return;
				p=EnsureInRange(p,0,this->ListCnt-1);
				EachRowColor.erase(EachRowColor.begin()+p);
				SimpleListView <T>::DeleteListContent(p);
			}

			void ClearListContent()
			{
				if (!this->ListCnt) return;
				PUI_DD[0]<<"Clear SimpleListView_MultiColor_Content: ListCnt:"<<this->ListCnt<<endl;
				EachRowColor.clear();
				SimpleListView <T>::ClearListContent();
			}
			
			void SwapContent(int pos1,int pos2)
			{
				if (!InRange(pos1,0,this->ListCnt-1)||!InRange(pos2,0,this->ListCnt-1)||pos1==pos2) return;
				swap(this->Text[pos1],this->Text[pos2]);
				swap(this->FuncData[pos1],this->FuncData[pos2]);
				swap(EachRowColor[pos1],EachRowColor[pos2]);
				this->Win->SetPresentArea((this->GetLinePosize(pos1)|this->GetLinePosize(pos2))&this->CoverLmt);
			}
			
			void SetRowColor(int p,const EachRowColor0 &co=EachRowColor_NONE)
			{
				p=EnsureInRange(p,0,this->ListCnt-1);
				EachRowColor[p]=co;
				this->Win->SetPresentArea(this->CoverLmt&this->GetLinePosize(p));
			}
			
			void SetDefaultColor(const EachRowColor0 &co=EachRowColor_NONE)
			{
				for (int i=0;i<=2;++i)
					SimpleListView <T>::SetRowColor(i,co.row[i]);
				SimpleListView <T>::SetTextColor(co.text);
			}
			
			SimpleListView_MultiColor(const Widgets::WidgetID &_ID,PosizeEX_InLarge *largePsex,void (*_func)(T&,int,int)=NULL)
			:SimpleListView <T>(_ID,Widgets::WidgetType_SimpleListView_MultiColor,largePsex,_func) {}
			
			SimpleListView_MultiColor(const Widgets::WidgetID &_ID,Widgets *_fa,const Posize &_rps,void (*_func)(T&,int,int)=NULL)
			:SimpleListView <T>(_ID,Widgets::WidgetType_SimpleListView_MultiColor,_fa,_rps,_func) {}
			
			SimpleListView_MultiColor(const Widgets::WidgetID &_ID,Widgets *_fa,PosizeEX *psex,void (*_func)(T&,int,int)=NULL)
			:SimpleListView <T>(_ID,Widgets::WidgetType_SimpleListView_MultiColor,_fa,psex,_func) {}
	};
	template <class T> typename SimpleListView_MultiColor<T>::EachRowColor0 SimpleListView_MultiColor<T>::EachRowColor_NONE;
	
	template <class T> class SimpleBlockView:public Widgets
	{
		public:
			struct BlockViewData
			{
				string MainText,
					   SubText1,
					   SubText2;
				T FuncData;
				SharedTexturePtr pic;
				RGBA MainTextColor=RGBA_NONE,
				 	 SubTextColor1=RGBA_NONE,
				 	 SubTextColor2=RGBA_NONE,
					 BlockColor[3]{RGBA_NONE,RGBA_NONE,RGBA_NONE};//NoFocusRow,FocusBlock,ClickBlock
				
				BlockViewData(const T &funcdata,const string &maintext,const string &subtext1="",const string &subtext2="",const SharedTexturePtr &_pic=SharedTexturePtr(NULL))
				:FuncData(funcdata),MainText(maintext),SubText1(subtext1),SubText2(subtext2),pic(_pic) {}
				
				BlockViewData() {}
			};
			
		protected:
			enum
			{
				Stat_NoFocus=0,
				Stat_UpFocusRow=1,
				Stat_DownClickTwice=2,
				Stat_DownClickRight=3,
				Stat_DownClickOnce=4 
			}stat=Stat_NoFocus;
			int BlockCnt=0;
			vector <BlockViewData> BlockData;
			void (*func)(T&,int,int)=NULL;//int1:Pos(CountFrom 0,-1 means background)   int2: 0:None 1:Left_Click 2:Left_Double_Click 3:Right_Click
			T BackgroundFuncData;
			PosizeEX_InLarge *psexInLarge=NULL;
			int	FocusChoose=-1,
				ClickChoose=-1,
				EachLineBlocks=1;
			Posize EachPs={5,5,240,80};//min(w,h) is the pic edge length, if w>=h ,the text will be show on right,else on buttom
			RGBA MainTextColor=ThemeColorMT[0],//These colors are default color
				 SubTextColor1=ThemeColorBG[6],
				 SubTextColor2=ThemeColorBG[6],
				 BlockColor[3]{ThemeColorM[0],ThemeColorM[2],ThemeColorM[4]};//NoFocusRow,FocusBlock,ClickBlock
			bool EnablePic=1,
				 EnableCheckBox=0,
				 EnableDrag=0,
				 DisableKeyboardEvent=0;
			
			Posize GetBlockPosize(int p)//get specific Block Posize
			{
				if (p==-1) return ZERO_POSIZE;
				return Posize(gPS.x+p%EachLineBlocks*(EachPs.x+EachPs.w),gPS.y+p/EachLineBlocks*(EachPs.y+EachPs.h),EachPs.w,EachPs.h);
			}

			int InBlockPosize(const Point &pt)//return In which Posize ,if not exist,return -1
			{
				int re=(pt.y-gPS.y+EachPs.y)/(EachPs.y+EachPs.h)*EachLineBlocks+(pt.x-gPS.x+EachPs.x)/(EachPs.x+EachPs.w);
				if (InRange(re,0,BlockCnt-1)&&(GetBlockPosize(re)&CoverLmt).In(pt))
					return re;
				else return -1;
			}
			
			virtual void CheckEvent(const PUI_Event *event)
			{
				if (BlockCnt==0||DisableKeyboardEvent) return;
				if (event->type==PUI_Event::Event_KeyEvent)
					if (event->KeyEvent()->keyType==PUI_KeyEvent::Key_Down||event->KeyEvent()->keyType==PUI_KeyEvent::Key_Hold)
					{
						int MoveSelectPosDelta=0;
						PUI_KeyEvent *keyevent=event->KeyEvent();
						switch (keyevent->keyCode)
						{
							case SDLK_RETURN:
								if (InRange(ClickChoose,0,BlockCnt-1))
								{
									PUI_DD[0]<<"SimpleBlockView "<<IDName<<": func "<<ClickChoose<<" "<<BlockData[ClickChoose].MainText<<endl;
									if (func!=NULL)
										func(BlockData[ClickChoose].FuncData,ClickChoose,2);
									Win->StopSolveEvent();
								}
								break;
							case SDLK_LEFT:
								if (MoveSelectPosDelta==0)
									MoveSelectPosDelta=-1;
							case SDLK_RIGHT:
								if (MoveSelectPosDelta==0)
									MoveSelectPosDelta=1;
							case SDLK_UP:
								if (MoveSelectPosDelta==0)
									MoveSelectPosDelta=-EachLineBlocks;
							case SDLK_DOWN:
								if (MoveSelectPosDelta==0)
									MoveSelectPosDelta=EachLineBlocks;
								if (ClickChoose!=-1)
								{
									Win->SetPresentArea(GetBlockPosize(ClickChoose)&CoverLmt);
									ClickChoose=EnsureInRange(ClickChoose+MoveSelectPosDelta,0,BlockCnt-1);
									Range showrange=GetShowRange(psexInLarge->GetLargeLayer()->GetgPS());
									if (ClickChoose<showrange.l)
										psexInLarge->SetShowPos(ClickChoose/EachLineBlocks*(EachPs.y+EachPs.h)-rPS.y);
									else if (ClickChoose>showrange.r)
										psexInLarge->SetShowPos((ClickChoose/EachLineBlocks+1)*(EachPs.y+EachPs.h)-psexInLarge->GetLargeLayer()->GetrPS().h-rPS.y);
								}
								else ClickChoose=0;
								Win->SetPresentArea(GetBlockPosize(ClickChoose)&CoverLmt);
								Win->StopSolveEvent();
								break;							
						}
					}

			}
			
			virtual void CheckPos(const PUI_PosEvent *event,int mode)
			{
				if (!Win->IsNeedSolvePosEvent()) return;
				if (mode&PosMode_LoseFocus)
				{
					if (stat!=Stat_NoFocus)
						if (!Win->IsPosFocused()||!CoverLmt.In(event->pos)||mode&PosMode_ForceLoseFocus)
						{
							stat=Stat_NoFocus;
							RemoveNeedLoseFocus();
							Win->SetPresentArea(GetBlockPosize(FocusChoose)&CoverLmt);
							FocusChoose=-1;
						}
				}
				else if (mode&PosMode_Default)
					switch (event->posType)
					{
						case PUI_PosEvent::Pos_Down:
							Win->SetPresentArea(GetBlockPosize(ClickChoose)&CoverLmt);
							Win->SetPresentArea(GetBlockPosize(FocusChoose)&CoverLmt);
							ClickChoose=FocusChoose=InBlockPosize(event->pos);
							Win->SetPresentArea(GetBlockPosize(ClickChoose)&CoverLmt);
							Win->SetPresentArea(GetBlockPosize(FocusChoose)&CoverLmt);
							if (event->button==PUI_PosEvent::Button_MainClick)
								if (event->clicks==2)
									stat=Stat_DownClickTwice;
								else stat=Stat_DownClickOnce;
							else if (event->button==PUI_PosEvent::Button_SubClick)
								stat=Stat_DownClickRight;
							else stat=Stat_DownClickOnce,PUI_DD[1]<<"SimpleBlockView "<<IDName<<": Unknown click button,use it as left click once"<<endl;
							Win->StopSolvePosEvent();
							break;
						case PUI_PosEvent::Pos_Up:
							if (stat!=Stat_NoFocus&&stat!=Stat_UpFocusRow)
							{
								if (ClickChoose>=0)
									PUI_DD[0]<<"SimpleBlockView "<<IDName<<": func "<<ClickChoose<<" "<<BlockData[ClickChoose].MainText<<endl;
								if (event->type==event->Event_TouchEvent)//??
									if (event->button==PUI_PosEvent::Button_MainClick)
										if (event->clicks==2)
											stat=Stat_DownClickTwice;
										else stat=Stat_DownClickOnce;
									else if (event->button==PUI_PosEvent::Button_SubClick)
										stat=Stat_DownClickRight;
								if (func!=NULL)
									func(ClickChoose==-1?BackgroundFuncData:BlockData[ClickChoose].FuncData,ClickChoose,stat==4?1:stat);
								stat=Stat_UpFocusRow;
								Win->StopSolvePosEvent();
							}
							break;
						case PUI_PosEvent::Pos_Motion:
							if (stat==Stat_NoFocus)
							{
								stat=Stat_UpFocusRow;
								SetNeedLoseFocus();
								FocusChoose=InBlockPosize(event->pos);
								Win->SetPresentArea(GetBlockPosize(FocusChoose)&CoverLmt);
							}
							else
							{
								int pos=InBlockPosize(event->pos);
								if (pos!=FocusChoose)
								{
									Win->SetPresentArea(GetBlockPosize(FocusChoose)&CoverLmt);
									Win->SetPresentArea(GetBlockPosize(pos)&CoverLmt);
									FocusChoose=pos;
								}	
							}
							Win->StopSolvePosEvent();
							break;
					}
			}
			
			virtual void Show(Posize &lmt)
			{
				if (BlockCnt==0) return;
				Range ForRange=GetShowRange(lmt);
				Posize BlockPs=GetBlockPosize(ForRange.l);
				for (int i=ForRange.l;i<=ForRange.r;++i)
				{
					Win->RenderFillRect(BlockPs&lmt,ThemeColor(!BlockData[i].BlockColor[ClickChoose==i?2:FocusChoose==i]?BlockColor[ClickChoose==i?2:FocusChoose==i]:BlockData[i].BlockColor[ClickChoose==i?2:FocusChoose==i]));
					
					if (EnablePic&&BlockData[i].pic()!=NULL)
					{
						Win->RenderCopyWithLmt(BlockData[i].pic(),Posize(BlockPs.x,BlockPs.y,min(EachPs.w,EachPs.h),min(BlockPs.w,BlockPs.h)),lmt);
						Win->Debug_DisplayBorder(Posize(BlockPs.x,BlockPs.y,min(EachPs.w,EachPs.h),min(BlockPs.w,BlockPs.h)));
					}
					
					if (EnablePic)
						if (EachPs.w>=EachPs.h)
						{
							Win->RenderDrawText(BlockData[i].MainText,Posize(BlockPs.x+BlockPs.h+8,BlockPs.y,BlockPs.w-BlockPs.h-8,BlockPs.h/3),lmt,-1,ThemeColor(!BlockData[i].MainTextColor?MainTextColor:BlockData[i].MainTextColor));
							Win->RenderDrawText(BlockData[i].SubText1,Posize(BlockPs.x+BlockPs.h+8,BlockPs.y+EachPs.h/3,BlockPs.w-BlockPs.h-8,BlockPs.h/3),lmt,-1,ThemeColor(!BlockData[i].SubTextColor1?SubTextColor1:BlockData[i].SubTextColor1));
							Win->RenderDrawText(BlockData[i].SubText2,Posize(BlockPs.x+BlockPs.h+8,BlockPs.y+EachPs.h/3*2,BlockPs.w-BlockPs.h-8,BlockPs.h/3),lmt,-1,ThemeColor(!BlockData[i].SubTextColor2?SubTextColor2:BlockData[i].SubTextColor2));
						}
						else Win->RenderDrawText(BlockData[i].MainText,Posize(BlockPs.x,BlockPs.y+EachPs.w,BlockPs.w,BlockPs.h-BlockPs.w),lmt,0,ThemeColor(!BlockData[i].MainTextColor?MainTextColor:BlockData[i].MainTextColor));
					else
					{
						Win->RenderDrawText(BlockData[i].MainText,Posize(BlockPs.x+8,BlockPs.y,BlockPs.w-8,BlockPs.h/3),lmt,-1,ThemeColor(!BlockData[i].MainTextColor?MainTextColor:BlockData[i].MainTextColor));
						Win->RenderDrawText(BlockData[i].SubText1,Posize(BlockPs.x+8,BlockPs.y+EachPs.h/3,BlockPs.w-8,BlockPs.h/3),lmt,-1,ThemeColor(!BlockData[i].SubTextColor1?SubTextColor1:BlockData[i].SubTextColor1));
						Win->RenderDrawText(BlockData[i].SubText2,Posize(BlockPs.x+8,BlockPs.y+EachPs.h/3*2,BlockPs.w-8,BlockPs.h/3),lmt,-1,ThemeColor(!BlockData[i].SubTextColor2?SubTextColor2:BlockData[i].SubTextColor2));
					}
					
 					Win->Debug_DisplayBorder(BlockPs);
					
					if ((i+1)%EachLineBlocks==0)
						BlockPs.y+=EachPs.y+EachPs.h,BlockPs.x=gPS.x;
					else BlockPs.x+=EachPs.x+EachPs.w;
				}
				Win->Debug_DisplayBorder(gPS);
			}
			
			virtual void CalcPsEx()
			{
				if (PsEx!=NULL)
					PsEx->GetrPS(rPS);
				EachLineBlocks=max(1,(rPS.w+EachPs.x)/(EachPs.x+EachPs.w));
				rPS.h=ceil(BlockCnt*1.0/EachLineBlocks)*(EachPs.y+EachPs.h)-EachPs.y;
				if (psexInLarge->NeedFullFill())
					rPS.h=max(rPS.h,psexInLarge->GetLargeLayer()->GetrPS().h-rPS.y-(psexInLarge->Fullfill()<0?-psexInLarge->Fullfill():0));//??
				Posize lastPs=CoverLmt;
				if (fa!=NULL)
					gPS=rPS+fa->GetgPS();
				else gPS=rPS;
				CoverLmt=gPS&GetFaCoverLmt();
				if (!(lastPs==CoverLmt))
					Win->SetPresentArea(lastPs|CoverLmt);
			}

		public:
			Range GetShowRange(Posize ps)
			{
				ps=ps&gPS;
				Range re;
				re.l=EnsureInRange((ps.y-gPS.y)/(EachPs.y+EachPs.h)*EachLineBlocks,0,BlockCnt-1);
				re.r=EnsureInRange((ps.y2()-gPS.y+EachPs.y+EachPs.h-1)/(EachPs.y+EachPs.h)*EachLineBlocks-1,0,BlockCnt-1);
				return re;
			}
			
			inline void SetMainTextColor(const RGBA &co)
			{
				MainTextColor=!co?ThemeColorM[0]:co;
				Win->SetPresentArea(CoverLmt);
			}
			
			inline void SetSubTextColor1(const RGBA &co)
			{
				SubTextColor1=!co?ThemeColorBG[6]:co;
				Win->SetPresentArea(CoverLmt);
			}
			
			inline void SetSubTextColor2(const RGBA &co)
			{
				SubTextColor2=!co?ThemeColorBG[6]:co;
				Win->SetPresentArea(CoverLmt);
			}

			inline void SetBlockColor(int p,const RGBA &co)
			{
				if (InRange(p,0,2))
				{
					BlockColor[p]=!co?ThemeColorM[p*2]:co;
					Win->SetPresentArea(CoverLmt);
				}
				else PUI_DD[1]<<"SimpleBlockView "<<IDName<<": SetBlockColor: p "<<p<<" is not in range[0,2]"<<endl;
			}			
			
			void SetEachBlockPosize(const Posize &ps)
			{
				EachPs=ps;
				Win->SetNeedUpdatePosize();
			}

			inline void SetBlockFunc(void (*_func)(T&,int,int))
			{func=_func;}
			
			void SetBlockContent(int p,const BlockViewData &data)//p: 0<=p<ListCnt:SetInP >=ListCnt:SetInLast <0:SetInFirst
			{
				p=EnsureInRange(p,0,BlockCnt);
				BlockData.insert(BlockData.begin()+p,data);
				++BlockCnt;
				if (ClickChoose>=p) ++ClickChoose;
				if (BlockCnt%EachLineBlocks==1)//??
				{
					rPS.h=ceil(BlockCnt*1.0/EachLineBlocks)*(EachPs.y+EachPs.h)-EachPs.y;
					Win->SetNeedUpdatePosize();
				}
				Win->SetPresentArea(CoverLmt);//?? It don't need fresh such big area
			}
			
			void DeleteBlockContent(int p)//p: 0<=p<ListCnt:SetInP >=ListCnt:DeleteLast <0:DeleteFirst
			{
				if (!BlockCnt) return;
				p=EnsureInRange(p,0,BlockCnt-1);
				BlockData.erase(BlockData.begin()+p);
				--BlockCnt;
				if (FocusChoose==BlockCnt) FocusChoose=-1;
				if (ClickChoose>p) --ClickChoose;
				else if (ClickChoose==p) ClickChoose=-1;
				if (BlockCnt%EachLineBlocks==0)
				{
					rPS.h=ceil(BlockCnt*1.0/EachLineBlocks)*(EachPs.y+EachPs.h)-EachPs.y;
					Win->SetNeedUpdatePosize();
				}
				Win->SetPresentArea(CoverLmt);
			}
			
			void ClearBlockContent()
			{
				if (!BlockCnt) return;
				PUI_DD[0]<<"Clear SimpleBlockView "<<IDName<<" content: BlockCnt:"<<BlockCnt<<endl;
				FocusChoose=ClickChoose=-1;
				BlockData.clear();
				BlockCnt=0;
				rPS.h=0;
				psexInLarge->SetShowPos(rPS.y);
				Win->SetNeedUpdatePosize();
				Win->SetPresentArea(CoverLmt);
			}
			
			void SetSelectBlock(int p)
			{
				if (!InRange(p,0,BlockCnt-1))
					return;
				Win->SetPresentArea(GetBlockPosize(ClickChoose)&CoverLmt);
				ClickChoose=p;
				if (!GetShowRange(psexInLarge->GetLargeLayer()->GetgPS()).InRange(p))
					psexInLarge->SetShowPos(p/EachLineBlocks*(EachPs.y+EachPs.h));
				Win->SetPresentArea(GetBlockPosize(ClickChoose)&CoverLmt);
			}
			
			inline int GetCurrentSelectBlock()
			{return ClickChoose;}
			
			SimpleBlockView* PushbackContent(const BlockViewData &data)
			{
				SetBlockContent(1e9,data);
				return this;
			}
			
			inline void SetBackgroundFuncData(const T &data)
			{BackgroundFuncData=data;}
			
			inline int GetBlockCnt()
			{return BlockCnt;}
			
			inline bool Empty()
			{return BlockCnt==0;}
			
			int Find(const T &_funcdata)
			{
				for (int i=0;i<BlockCnt;++i)
					if (BlockData[i].FuncData==_funcdata)
						return i;
				return -1;
			}
			
			BlockViewData& GetBlockData(int p)
			{
				p=EnsureInRange(p,0,BlockCnt);
				return BlockData[p];
			}
			
			T& GetFuncData(int p)
			{
				p=EnsureInRange(p,0,BlockCnt-1);
				return BlockData[p].FuncData;
			}
			
			inline T& GetBackgroundFuncData()
			{return BackgroundFuncData;}
			
			void SetUpdateBlock(int p)
			{
				if (!InRange(p,0,BlockCnt-1))
					return;
				Posize bloPs=GetBlockPosize(p)&CoverLmt;
				Win->SetPresentArea(bloPs);
			}
			
			void SetEnablePic(bool enable)
			{
				EnablePic=enable;
				Win->SetPresentArea(CoverLmt);
			}
			
			inline void SetDisableKeyboardEvent(bool disable)
			{DisableKeyboardEvent=disable;}
			
			LargeLayerWithScrollBar* GetLargeLayer()
			{return psexInLarge->GetLargeLayer();}
			
			void ResetLargeLayer(PosizeEX_InLarge *largePsex)
			{
				SetFa(largePsex->GetLargeLayer()->LargeArea());
				ReAddPsEx(largePsex);
				psexInLarge=largePsex;
			}
			
			SimpleBlockView(const WidgetID &_ID,PosizeEX_InLarge *largePsex,void (*_func)(T&,int,int)=NULL)
			:Widgets(_ID,WidgetType_SimpleBlockView),func(_func),psexInLarge(largePsex)
			{
				SetFa(largePsex->GetLargeLayer()->LargeArea());
				AddPsEx(largePsex);
			}
			
			SimpleBlockView(const WidgetID &_ID,Widgets *_fa,const Posize &_rps,void (*_func)(T&,int,int)=NULL)
			:Widgets(_ID,WidgetType_SimpleBlockView),func(_func)
			{
				LargeLayerWithScrollBar *larlay=new LargeLayerWithScrollBar(0,_fa,_rps);
				SetFa(larlay->LargeArea());
				AddPsEx(psexInLarge=new PosizeEX_InLarge(larlay,0));
			}
			
			SimpleBlockView(const WidgetID &_ID,Widgets *_fa,PosizeEX *psex,void (*_func)(T&,int,int)=NULL)
			:Widgets(_ID,WidgetType_SimpleBlockView),func(_func)
			{
				LargeLayerWithScrollBar *larlay=new LargeLayerWithScrollBar(0,_fa,psex);
				SetFa(larlay->LargeArea());
				AddPsEx(psexInLarge=new PosizeEX_InLarge(larlay,0));
			}
	};
	
	template <class T> class DetailedListView:public Widgets//Maybe need reconstruct...
	{
		/*	Example:
				name  |   size   |    date    |    type    |   ....
			1	xxx			xxx			xxx			xxx
			2	xxx			xxx			xxx			xxx
		*/
		public:
			struct DetailedListViewData
			{
//				friend DetailedListView;
//				protected:
					vector <string> Text;
					vector <RGBA> SpecificTextColor;//if Empty,use the defalut Color
					SharedTexturePtr pic;
					T FuncData;
				
				public:
					DetailedListViewData(const vector <string> &text):Text(text) {}
					
					DetailedListViewData() {}
			};
			
			struct DetailedListViewColumn
			{
//				friend DetailedListView;
				string TagName;
				int TextDisplayMode=0,
					Width=100;
				RGBA ColumnTextColor=RGBA_NONE;
				
				DetailedListViewColumn(const string &tagname,const int &w=100,const int &displaymode=0,const RGBA &co=RGBA_NONE)
				:TagName(tagname),TextDisplayMode(displaymode),Width(w),ColumnTextColor(co) {}
				
				DetailedListViewColumn() {}
			};
		
		protected:
			enum
			{
				Stat_NoFocus=0,
				Stat_FocusRow=1,
				Stat_DownLeftTwice=2,
				Stat_DownRight=3,
				Stat_DownLeftOnce=4,
				Stat_FocusTop=5,
				Stat_DownLeftTop=6,
				Stat_DownRightTop=7,
				Stat_FocusResizeBar=8,
				Stat_ScrollResizeBar=9
			}stat=Stat_NoFocus;
			vector <DetailedListViewColumn> ColumnInfo;
			vector <DetailedListViewData> ListData;
			int ListCnt=0;
			void (*func)(T&,int,int)=NULL;//int1:Pos(CountFrom 0,-1 means background)   int2: 0:None 1:Left_Click 2:Left_Double_Click 3:Right_Click
			T BackgroundFuncData;
//			int (*CompFunc)(const string&,const string&,const string&,int)=NULL;//ItemA,ItemB,TagName,ColumnPos  //<:-1 ==:0 >:1 if ==:Sort by ColumnOrder 
			void (*SortFunc)(vector <DetailedListViewData>&,const string&,int,bool)=NULL;//TagName,ColumnPos,SortDirection
			LargeLayerWithScrollBar *fa2=NULL;
			int	ColumnAndResizeChoose=-1,
				FocusChoose=-1,
				ClickChoose=-1,
				ResizeColumnLeftX=0;
			int TopHeigth=30,
				TopResizeWidth=2,
				EachHeight=24,
				Interval=2,
				MainTextPos=0,
				SortByColumn=0;
			bool SortDirection=0;//0:small to big 1:big to small
			bool EnableAutoMainTextPos=1,
				 EnableCheckBox=0,
				 EnableDrag=0,
				 EnableKeyboardEvent=0,
				 EnableSerialNumber=0;
			RGBA TopResizeBarColor[3]{ThemeColorM[1],ThemeColorM[3],ThemeColorM[5]},
				 TopColumnColor[3]{ThemeColorM[0],ThemeColorM[2],ThemeColorM[4]},
				 EachRowColor[3]{RGBA_NONE,ThemeColorM[1],ThemeColorM[3]},
				 MainTextColor=ThemeColorMT[0],
				 SubTextColor=ThemeColorBG[5],
				 DefaultColumnTextColor=ThemeColorMT[0];
			
			inline int GetLineFromPos(int y)
			{
				int re=(y-gPS.y-TopHeigth)/(EachHeight+Interval);
				if ((y-gPS.y-TopHeigth)%(EachHeight+Interval)<EachHeight&&InRange(re,0,ListCnt-1))
					return re;
				else return -1;
			}
			
			inline Posize GetTopAreaPosize()
			{return Posize(gPS.x,fa2->GetgPS().y,gPS.w,TopHeigth);}
			
			virtual void CheckEvent(const PUI_Event *event)
			{
				//<<Keyboard control...
			}
			
			virtual void CheckPos(const PUI_PosEvent *event,int mode)
			{
				if (mode&PosMode_LoseFocus)
				{
					if (stat!=Stat_NoFocus)
						if (!Win->IsPosFocused()||!CoverLmt.In(event->pos)||mode&PosMode_ForceLoseFocus)
						{
							if (stat<=Stat_DownLeftOnce)
								Win->SetPresentArea(GetLinePosize(FocusChoose)&CoverLmt);
							else Win->SetPresentArea(GetTopAreaPosize()&CoverLmt);
							stat=Stat_NoFocus;
							FocusChoose=-1;
							ColumnAndResizeChoose=-1;
							RemoveNeedLoseFocus();
						}
				}
				else if (mode&PosMode_Default)
					switch (event->posType)
					{
						case PUI_PosEvent::Pos_Down:
							if (event->pos.y-fa2->GetgPS().y<=TopHeigth)
							{
								stat=Stat_FocusTop;
								int x=event->pos.x-gPS.x,s=gPS.x;
								Win->SetPresentArea(GetLinePosize(FocusChoose)&CoverLmt);
								FocusChoose=ColumnAndResizeChoose=-1;
								for (int i=0;i<ColumnInfo.size();x-=ColumnInfo[i].Width,s+=ColumnInfo[i].Width,++i)
									if (x<=ColumnInfo[i].Width-TopResizeWidth)
									{
										ColumnAndResizeChoose=i;
										if (event->button==event->Button_MainClick)
										{
											stat=Stat_DownLeftTop;
											if (SortByColumn==i)
												SortDirection=!SortDirection;
											else SortByColumn=i,SortDirection=0;
											if (EnableAutoMainTextPos)
												MainTextPos=SortByColumn;
										}
										else if (event->button==event->Button_SubClick)
											stat=Stat_DownRightTop;
										break;
									}
									else if (x<=ColumnInfo[i].Width)
									{
										stat=Stat_ScrollResizeBar;
										ResizeColumnLeftX=s;
										ColumnAndResizeChoose=i;
										Win->SetOccupyPosWg(this);
										break;
									}
								Win->SetPresentArea(GetTopAreaPosize()&CoverLmt);
							}
							else
							{
								Win->SetPresentArea(GetLinePosize(ClickChoose)&CoverLmt);
								Win->SetPresentArea(GetLinePosize(FocusChoose)&CoverLmt);
								ClickChoose=FocusChoose=GetLineFromPos(event->pos.y);
								if (event->button==event->Button_MainClick)
									if (event->clicks==2)
										stat=Stat_DownLeftTwice;
									else stat=Stat_DownLeftOnce;
								else if (event->button==event->Button_SubClick)
									stat=Stat_DownRight;
								else stat=Stat_DownLeftOnce,PUI_DD[1]<<"DetailedListView: Unknown click button,use it as left click once"<<endl;
								Win->SetPresentArea(GetLinePosize(ClickChoose)&CoverLmt);
								Win->SetPresentArea(GetLinePosize(FocusChoose)&CoverLmt);
							}
							Win->StopSolvePosEvent();
							break;
						case PUI_PosEvent::Pos_Up:
							if (stat>=Stat_DownLeftTwice)
							{
								if (stat<=Stat_DownLeftOnce)
								{
									PUI_DD[0]<<"DetailedListView "<<IDName<<" func "<<ClickChoose<<" "<<stat<<endl;
									if (event->type==event->Event_TouchEvent)//??
									if (event->button==PUI_PosEvent::Button_MainClick)
										if (event->clicks==2)
											stat=Stat_DownLeftTwice;
										else stat=Stat_DownLeftTwice;
									else if (event->button==PUI_PosEvent::Button_SubClick)
										stat=Stat_DownRight;
									if (func!=NULL)
										func(ClickChoose==-1?BackgroundFuncData:ListData[ClickChoose].FuncData,ClickChoose,stat==4?1:stat);
									stat=Stat_FocusRow;
								}
								else if (stat==Stat_DownLeftTop)
								{
									PUI_DD[0]<<"DetailedListView "<<IDName<<" Sort by "<<ColumnInfo[SortByColumn].TagName<<" "<<SortByColumn<<" "<<SortDirection<<endl;
									if (SortFunc!=NULL)
										SortFunc(ListData,ColumnInfo[SortByColumn].TagName,SortByColumn,SortDirection);
									Win->SetPresentArea(CoverLmt);
									stat=Stat_FocusTop;
								}
								else if (stat==Stat_DownRightTop)
								{
									PUI_DD[2]<<"DetailedListView "<<IDName<<" RightClick ToColumn Cannot be used yet!"<<endl;
									stat=Stat_FocusTop;
									//<<RightClickTop...
								}
								else if (stat==Stat_ScrollResizeBar)
								{
									stat=Stat_FocusResizeBar;
									Win->SetOccupyPosWg(NULL);
									Win->SetPresentArea(GetTopAreaPosize()&CoverLmt);
								}
								Win->StopSolvePosEvent();
							}
							break;
						case PUI_PosEvent::Pos_Motion:
							if (stat==Stat_ScrollResizeBar)
							{
								ColumnInfo[ColumnAndResizeChoose].Width=EnsureInRange(event->pos.x-ResizeColumnLeftX,10,1e9);
								Win->SetPresentArea(CoverLmt);
								Win->SetNeedUpdatePosize();
							}
							else 
							{
								if (stat==Stat_NoFocus)
								{
									stat=Stat_FocusRow;
									SetNeedLoseFocus();
								}
								
								if (InRange(event->pos.y,fa2->GetgPS().y,fa2->GetgPS().y+TopHeigth))
								{
									Win->SetPresentArea(GetLinePosize(FocusChoose)&CoverLmt);
									FocusChoose=-1;
									
									if (stat==Stat_DownLeftTop||stat==Stat_DownRightTop)
									{
										
									}
									else
									{
										int x=event->pos.x-gPS.x,lastChoose=ColumnAndResizeChoose,lastStat=stat;
										ColumnAndResizeChoose=-1;
										stat=Stat_FocusTop;
										for (int i=0;i<ColumnInfo.size();x-=ColumnInfo[i].Width,++i)
											if (x<=ColumnInfo[i].Width-TopResizeWidth)
											{
												ColumnAndResizeChoose=i;
												stat=Stat_FocusTop;
												break;
											}
											else if (x<=ColumnInfo[i].Width)
											{
												ColumnAndResizeChoose=i;
												stat=Stat_FocusResizeBar;
												break;
											}
										if (lastChoose!=ColumnAndResizeChoose||lastStat!=stat)
											Win->SetPresentArea(GetTopAreaPosize()&CoverLmt);
									}
								}
								else
								{
									if (ColumnAndResizeChoose!=-1)
									{
										ColumnAndResizeChoose=-1;
										Win->SetPresentArea(GetTopAreaPosize()&CoverLmt);
									}
									
									int pos=GetLineFromPos(event->pos.y);
									Win->SetPresentArea(GetLinePosize(FocusChoose)&CoverLmt);
									Win->SetPresentArea(GetLinePosize(pos)&CoverLmt);
									FocusChoose=pos;
								}
							}
							Win->StopSolvePosEvent();
							break;	
					}
			}
			
			Range GetShowRange(Posize ps)
			{
				Range re;
				ps=ps&gPS;
				re.l=EnsureInRange((ps.y-gPS.y-TopHeigth)/(EachHeight+Interval),0,ListCnt-1);
				re.r=EnsureInRange((ps.y2()-gPS.y-TopHeigth+EachHeight+Interval-1)/(EachHeight+Interval),0,ListCnt-1);
				return re;
			}
			
			virtual void Show(Posize &lmt)
			{
				if (ListCnt!=0)
				{
					Range ForLR=GetShowRange(lmt);
					Posize ps(gPS.x,gPS.y+TopHeigth+ForLR.l*(EachHeight+Interval),0,EachHeight);
					for (int i=ForLR.l;i<=ForLR.r;++i)
					{
						ps.w=gPS.w;
						Win->RenderFillRect(ps&lmt,ThemeColor(EachRowColor[ClickChoose==i?2:FocusChoose==i]));
						Win->Debug_DisplayBorder(ps);
						for (int j=0;j<ColumnInfo.size()&&j<=ListData[i].Text.size();++j)
						{
							ps.w=ColumnInfo[j].Width;
							Win->RenderDrawText(ListData[i].Text[j],ps,lmt,ColumnInfo[j].TextDisplayMode,ThemeColor(j<ListData[i].SpecificTextColor.size()?ListData[i].SpecificTextColor[j]:(j<ColumnInfo.size()?ColumnInfo[j].ColumnTextColor:(j==MainTextPos?MainTextColor:SubTextColor))));
							Win->Debug_DisplayBorder(ps);
							ps.x+=ps.w;
						}
						if (!!ListData[i].pic)
							Win->RenderCopyWithLmt(ListData[i].pic(),Posize(gPS.x,ps.y,EachHeight,EachHeight),lmt);
						ps.x=gPS.x;
						ps.y+=EachHeight+Interval;
					}
				}

				Posize ps=GetTopAreaPosize();
				Win->Debug_DisplayBorder(ps);
				for (int i=0;i<ColumnInfo.size();++i)
				{
					ps.w=ColumnInfo[i].Width-TopResizeWidth;
					Win->RenderFillRect(lmt&ps,ThemeColor(TopColumnColor[i==ColumnAndResizeChoose&&InRange(stat,5,7)?(stat==5?1:2):0]));
					Win->RenderDrawText(ColumnInfo[i].TagName,ps,lmt,ColumnInfo[i].TextDisplayMode,ThemeColor(!ColumnInfo[i].ColumnTextColor?DefaultColumnTextColor:ColumnInfo[i].ColumnTextColor));
					ps.x+=ps.w+TopResizeWidth;
					Win->RenderFillRect(lmt&Posize(ps.x-TopResizeWidth,ps.y,TopResizeWidth,ps.h),ThemeColor(TopResizeBarColor[i==ColumnAndResizeChoose&&InRange(stat,8,9)?(stat==9?2:1):0]));
				}
				Win->Debug_DisplayBorder(gPS);
			}

			virtual void CalcPsEx()//??
			{
				Posize lastPs=CoverLmt;
				if (PsEx!=NULL)	
					PsEx->GetrPS(rPS);
				rPS.w=0;
				for (auto vp:ColumnInfo)//??
					rPS.w+=vp.Width;
				rPS.h=EnsureInRange(ListCnt*(EachHeight+Interval)-Interval+TopHeigth,fa2->GetrPS().h,1e9);
				if (fa!=NULL)
					gPS=rPS+fa->GetgPS();
				else gPS=rPS;
				CoverLmt=gPS&GetFaCoverLmt();
				fa2->ResizeLL(rPS.w,rPS.h);
				if (!(lastPs==CoverLmt))
					Win->SetPresentArea(lastPs|CoverLmt);
			}

		public:
			inline int GetFocusColumn(int x)//??
			{
				x-=gPS.x;
				if (x<=0)
					return -1;
				for (int i=0;i<ColumnInfo.size();++i)
					if (x<=ColumnInfo[i].Width)
						return i;
					else x-=ColumnInfo[i].Width;
				return -1;
			}
			
			inline Posize GetLinePosize(int pos)
			{
				if (pos==-1) return ZERO_POSIZE;
				else return Posize(gPS.x,gPS.y+TopHeigth+pos*(EachHeight+Interval),gPS.w,EachHeight);
			}
			
			inline void SetNeedUpdateLine(int pos)
			{Win->SetPresentArea(CoverLmt&GetLinePosize(pos));}
			
			inline void SetTopResizeBarColor(int p,const RGBA &co)
			{
				if (InRange(p,0,2))
				{
					TopResizeBarColor[p]=!co?ThemeColorM[p*2+1]:co;
					Win->SetPresentArea(GetTopAreaPosize()&CoverLmt);
				}
				else PUI_DD[1]<<"DetailedListView: SetTopResizeBarColor: p "<<p<<" is not in range[0,2]"<<endl;
			}
			
			inline void SetTopColumnColor(int p,const RGBA &co)
			{
				if (InRange(p,0,2))
				{
					TopColumnColor[p]=!co?ThemeColorM[p*2]:co;
					Win->SetPresentArea(GetTopAreaPosize()&CoverLmt);
				}
				else PUI_DD[1]<<"DetailedListView: SetTopColumnColor: p "<<p<<" is not in range[0,2]"<<endl;
			}
			
			inline void SetEachRowColor(int p,const RGBA &co)
			{
				if (InRange(p,0,2))
				{
					EachRowColor[p]=!co?(p==0?RGBA_NONE:ThemeColorM[p*2-1]):co;
					Win->SetPresentArea(CoverLmt);
				}
				else PUI_DD[2]<<"DetailedListView: SetEachRowColor: p "<<p<<" is not in range[0,2]"<<endl;
			}
			
			inline void SetMainTextColor(const RGBA &co)
			{
				MainTextColor=!co?ThemeColorMT[0]:co;
				Win->SetPresentArea(CoverLmt);
			}
			
			inline void SetSubTextColor(const RGBA &co)
			{
				SubTextColor=!co?ThemeColorBG[5]:co;
				Win->SetPresentArea(CoverLmt);
			}
			
			inline void SetBackgroundColor(const RGBA &co)
			{fa2->SetLargeAreaColor(co);}
			
			void SortBy(int sortbycolumn,bool sortdirection)
			{
				SortByColumn=sortbycolumn;
				SortDirection=sortdirection;
				PUI_DD[0]<<"DetailedListView "<<IDName<<" Sort by "<<ColumnInfo[SortByColumn].TagName<<" "<<SortByColumn<<" "<<SortDirection<<endl;
				if (SortFunc!=NULL)
					SortFunc(ListData,ColumnInfo[SortByColumn].TagName,SortByColumn,SortDirection);
				Win->SetPresentArea(CoverLmt);
			}
			
			inline void SetMainTextPos(int pos)
			{
				if (MainTextPos!=pos)
				{
					MainTextPos=pos;
					Win->SetPresentArea(CoverLmt);
				}
			}
			
			inline void SetEnableAutoMainTextPos(bool en)
			{
				EnableAutoMainTextPos=en;
				SetMainTextPos(SortByColumn);
			}

			inline void SetTopHeightAndResizeWidth(int topheight=30,int topresizewidth=2)
			{
				TopHeigth=topheight;
				TopResizeWidth=topresizewidth;
				Win->SetNeedUpdatePosize();
				Win->SetPresentArea(CoverLmt);
			}

			inline void SetEachHeightAndInterval(int eachheight=24,int interval=2)
			{
				EachHeight=eachheight;
				Interval=interval;
				Win->SetNeedUpdatePosize();
				Win->SetPresentArea(CoverLmt);
			}
			
			inline void SetFunc(void (*_func)(T&,int,int))
			{func=_func;}

			inline void SetBackgroundFuncdata(const T &bgdata)
			{BackgroundFuncData=bgdata;}
			
			void SetSortFunc(void (*_SortFunc)(vector <DetailedListViewData>&,const string&,int,bool))
			{SortFunc=_SortFunc;}

			void SetColumnInfo(int p,const DetailedListViewColumn &column)
			{
				p=EnsureInRange(p,0,ColumnInfo.size());
				ColumnInfo.insert(ColumnInfo.begin()+p,column);
				if (ColumnAndResizeChoose>=p) ++ColumnAndResizeChoose;
				if (MainTextPos>=p) ++MainTextPos; 
				if (SortByColumn>=p) ++SortByColumn;
				Win->SetNeedUpdatePosize();//Is it just OK?
				Win->SetPresentArea(CoverLmt);
			}
			
			void DeleteColumnInfo(int p)
			{
				if (ColumnInfo.emtpy()) return;
				p=EnsureInRange(p,0,ColumnInfo.size()-1);
				ColumnInfo.erase(ColumnInfo.begin()+p);
				if (ColumnAndResizeChoose>p) --ColumnAndResizeChoose;
				else if (ColumnAndResizeChoose==p) ColumnAndResizeChoose=-1;
				if (SortByColumn>p) --SortByColumn;
				else if (SortByColumn==p) SortByColumn=0;
				if (EnableAutoMainTextPos) MainTextPos=SortByColumn;
				else
					if (MainTextPos>p) --MainTextPos;
					else if (MainTextPos==p) MainTextPos=0;
				Win->SetNeedUpdatePosize();
				Win->SetPresentArea(CoverLmt);
			}
			
			void ClearColumnInfo()//??
			{
				if (ColumnInfo.empty()) return;
				ColumnInfo.clear();
				ColumnAndResizeChoose=-1;
				SortByColumn=0;
				MainTextPos=0;
				Win->SetNeedUpdatePosize();
				Win->SetPresentArea(CoverLmt);
			}
			
			DetailedListView* PushbackColumnInfo(const DetailedListViewColumn &column)
			{
				SetColumnInfo(1e9,column);
				return this;
			}
			
			inline int GetColumnCnt()
			{return ColumnInfo.size();}

			void SetListContent(int p,const DetailedListViewData &info)
			{
				p=EnsureInRange(p,0,ListCnt);
				ListData.insert(ListData.begin()+p,info);
				++ListCnt;
				if (ClickChoose>=p) ++ClickChoose;
				Win->SetNeedUpdatePosize();
				Win->SetPresentArea(CoverLmt);
			}
			
			DetailedListView* PushbackListContent(const DetailedListViewData &info)
			{
				SetListContent(1e9,info);
				return this;
			}
			
			void DeleteListContent(int p)
			{
				if (!ListCnt) return;
				p=EnsureInRange(p,0,ListCnt-1);
				ListData.erase(ListData.begin()+p);
				--ListCnt;
				if (ClickChoose>p) --ClickChoose;
				else if (ClickChoose==p) ClickChoose=-1;
				Win->SetNeedUpdatePosize();
				Win->SetPresentArea(CoverLmt);
			}
			
			void ClearListContent()
			{
				if (!ListCnt) return;
				PUI_DD[0]<<"Clear DetailedListView "<<IDName<<" Content: ListCnt "<<ListCnt<<endl;
				FocusChoose=ClickChoose=-1;
				ListData.clear();
				ListCnt=0;
				fa2->SetViewPort(2,0);
				Win->SetNeedUpdatePosize();
				Win->SetPresentArea(CoverLmt);
			}
			
			void UpdateListContent(int p,const DetailedListViewData &info)
			{
				p=EnsureInRange(p,0,ListCnt-1);
				ListData[p]=info;
				if (ClickChoose>=p) ++ClickChoose;
				Win->SetNeedUpdatePosize();
				Win->SetPresentArea(CoverLmt);
			}
			
			inline int GetListCnt()
			{return ListCnt;}
				
			inline T& GetFuncData(int p)
			{return ListData[EnsureInRange(p,0,ListCnt-1)].FuncData;}
			
			int Find(const T &_funcdata)
			{
				for (int i=0;i<ListCnt;++i)
					if (ListData[i]==_funcdata)
						return i;
				return -1;
			}
			
			inline DetailedListViewData& GetListData(int p)//For breakthrough class limit??
			{return ListData[EnsureInRange(p,0,ListCnt-1)];}
			
			inline DetailedListViewColumn& GetColumnData(int p)
			{return ColumnInfo[EnsureInRange(p,0,ColumnInfo.size())];}
			
			void AddPsEx(PosizeEX *psex)
			{fa2->AddPsEx(psex);}
			
			virtual void SetrPS(const Posize &ps)
			{fa2->SetrPS(ps);}
			
			DetailedListView(const WidgetID &_ID,Widgets *_fa,const Posize &_rps,void (*_func)(T&,int,int)=NULL)
			:Widgets(_ID,WidgetType_DetailedListView),func(_func)
			{
				fa2=new LargeLayerWithScrollBar(0,_fa,_rps,ZERO_POSIZE);
				SetFa(fa2->LargeArea());
				rPS={0,0,_rps.w,TopHeigth};
			}
			
			DetailedListView(const WidgetID &_ID,Widgets *_fa,PosizeEX *psex,void (*_func)(T&,int,int)=NULL)
			:Widgets(_ID,WidgetType_DetailedListView),func(_func)
			{
				fa2=new LargeLayerWithScrollBar(0,_fa,psex,ZERO_POSIZE);
				SetFa(fa2->LargeArea());
				rPS=ZERO_POSIZE;
			}
	};
	
	template <class T> class Menu1;
	template <class T> struct MenuData
	{
		friend class Menu1 <T>;
		protected:
			int type=0;//0:none 1:normal_func 2:divide_line 3:sub_menu1
			void (*func)(T&)=NULL;
			T funcData;
			char hotKey=0;//case-insensitive
			SharedTexturePtr pic;
			vector <MenuData<T> > subMenu;
			bool enable=1;
			string text;
			int subMenuW=200;
			
		public:
			bool IsDivideLine() const
			{return type==2;}
			
			MenuData(const string &_text,void (*_func)(T&),const T &_funcdata,char _hotkey=0,const SharedTexturePtr &_pic=SharedTexturePtr(NULL),bool _enable=1)
			:type(1),text(_text),func(_func),funcData(_funcdata),hotKey(_hotkey),pic(_pic),enable(_enable) {}
			
			MenuData(int p)//any num is OK
			:type(2) {}
			
			MenuData(const vector <MenuData> &_submenu,const string &_text,int _submenuWidth=200,char _hotkey=0,const SharedTexturePtr &_pic=SharedTexturePtr(NULL),bool _enable=1)
			:type(3),text(_text),subMenuW(_submenuWidth),subMenu(_submenu),hotKey(_hotkey),pic(_pic),enable(_enable) {}
	};
	
	template <class T> class Menu1:public Widgets//This widget have many differences,it is not so easy to solve it  QAQ
	{
		protected:
			enum
			{
				Stat_NoFocus=0,
				Stat_UpFocus=1,
				Stat_Down=2
			}stat=Stat_NoFocus;
			vector <MenuData<T> > MenuDataArray;
			int	spos1=0,spos2=0,//show pos start(1) and end(2), when not exceed ,spos1 is 0, spos2 is ManudataCnt-1,means all in range
				pos=-1,//-1:no pos -2:ExceedTop -3:ExceedButtom
				subMenupos=-1,//-1 means not have subMenu
				EachHeight=20,//Each/2 is the height of exceed_button
				DivideLineHeight=3,
				BorderWidth=4,
				MenudataCnt=0;
			bool Exceed=0,
				 IsSubMenu=0,
				 BlockKeyEventAndWheelEvent=1,
//				 autoDeleteMenuData=0,
				 autoAdjustMenuPosition=1;
//				 NeedDelete=0;
			Menu1 <T> *rootMenu=NULL;
			SDL_Texture *triTex[2]{NULL,NULL};//enable,disable
			RGBA TextColor[2]{ThemeColorMT[0],ThemeColorMT[1]},//enable,disable
				 DivideLineColor=ThemeColorM[1],
				 MenuColor[3]{ThemeColorBG[0],ThemeColorM[2],ThemeColorM[4]};//no focus(background),focus,click
			
			void SetDelSubMenu()//create or delete subMenu
			{
				if (!InRange(pos,0,MenudataCnt-1))
				{
					if (subMenupos!=-1)
					{
						SetDelayDeleteWidget(childWg);
//						delete childWg;
						subMenupos=-1;
					}
					return;
				}
				
				if (subMenupos==-1||pos!=subMenupos)
				{
					if (pos!=subMenupos)//delete current subMenu
					{
						SetDelayDeleteWidget(childWg);
//						delete childWg;
						subMenupos=-1;
					}
					MenuData <T> &md=MenuDataArray[pos];
					if (md.type==3&&md.enable)
//						if (childWg==NULL)
						{
							Point pt(rPS.w-2*BorderWidth,BorderWidth);
							if (Exceed)
								pt.y+=BorderWidth/2;
							for (int i=spos1;i<pos-1;++i)
								switch (MenuDataArray[i].type)
								{
									case 1:	case 3:	pt.y+=EachHeight;	break;
									case 2:	pt.y+=DivideLineHeight;		break;
								}
							new Menu1 <T> (0,this,pt,md.subMenu,md.subMenuW);
							Win->SetNeedUpdatePosize();
							subMenupos=pos;
						}
				}
			}

			int GetPos(int y)
			{
				if (!InRange(y,BorderWidth,rPS.h-BorderWidth-1))
					return -1;
				if (Exceed)
				{
					if (InRange(y-BorderWidth,0,EachHeight/2-1))//Is top exceed_button
						return -2;
					if (InRange(rPS.h-y-BorderWidth-1,0,EachHeight/2-1))//Is buttom exceed_button
						return -3;
					y-=EachHeight/2+BorderWidth;
				}
				else y-=BorderWidth;
				for (int i=spos1;i<MenudataCnt&&y>BorderWidth&&i<=spos2;++i)
					switch (MenuDataArray[i].type)
					{
						case 1:	case 3:
							if (InRange(y,0,EachHeight-1))
								return i;
							y-=EachHeight;
							break;
						case 2:
							if (InRange(y,0,DivideLineHeight-1))
								return i;
							y-=DivideLineHeight;
							break;
					}
				return -1;
			}
			
			virtual void CheckEvent(const PUI_Event *event)
			{
				if (event->type==PUI_Event::Event_KeyEvent)
				{
					char hotKeyPressed=-1;
					PUI_KeyEvent *keyevent=event->KeyEvent();
					if (keyevent->keyType==PUI_KeyEvent::Key_Down||keyevent->keyType==PUI_KeyEvent::Key_Hold)
						switch (keyevent->keyCode)
						{
							case SDLK_ESCAPE:
								SetDelayDeleteThis();
								Win->StopSolveEvent();
								break;
							
							case SDLK_UP:
								stat=Stat_UpFocus;
								pos=(pos-1+MenudataCnt)%MenudataCnt;
								if (MenuDataArray[pos].type==2)//Only once because normally there are not two divideline near
									pos=(pos-1+MenudataCnt)%MenudataCnt;
								Win->StopSolveEvent();
								Win->SetPresentArea(CoverLmt);
								break;
							
							case SDLK_DOWN:
								stat=Stat_UpFocus;
								pos=(pos+1+MenudataCnt)%MenudataCnt;
								if (MenuDataArray[pos].type==2)
									pos=(pos+1+MenudataCnt)%MenudataCnt;
								Win->StopSolveEvent();
								Win->SetPresentArea(CoverLmt);
								break;
							
							case SDLK_LEFT:
								if (rootMenu!=this)
								{
									SetDelayDeleteThis();
									Win->StopSolveEvent();
								}
								break;
							
							case SDLK_RIGHT:
								if (InRange(pos,0,MenudataCnt-1))
									if (MenuDataArray[pos].type==3)
									{
										SetDelSubMenu();
										Win->StopSolveEvent();
									}
								break;
							
							case SDLK_RETURN:
							case SDLK_SPACE:
								if (InRange(pos,0,MenudataCnt-1))
								{
									MenuData <T> &md=MenuDataArray[pos];
									if (md.enable)
									{
										if (md.type==1)
										{
											if (md.func!=NULL)
												md.func(md.funcData);
											SetDelayDeleteWidget(rootMenu);
										}
										else if (md.type==3)
											SetDelSubMenu();
										Win->SetPresentArea(CoverLmt);
										Win->StopSolveEvent();
									}
								}
								break;
							default:
								if (InRange(keyevent->keyCode,SDLK_a,SDLK_z))
									hotKeyPressed=keyevent->keyCode-SDLK_a+'A';
								else if (InRange(keyevent->keyCode,SDLK_0,SDLK_9))
									hotKeyPressed=keyevent->keyCode-SDLK_0+'0';
								break;
						}
					if (hotKeyPressed!=-1)
					{
						for (int i=0;i<MenudataCnt;++i)
						{
							char ch=MenuDataArray[i].hotKey;
							if (InRange(ch,'a','z'))
								ch+='A'-'a';
							if (ch==hotKeyPressed)
							{
								pos=i;
								MenuData <T> &md=MenuDataArray[i];
								if (md.enable)
									if (md.type==1)
									{
										if (md.func!=NULL)
											md.func(md.funcData);
										SetDelayDeleteWidget(rootMenu);
									}
									else if (md.type==3)
										SetDelSubMenu();
								Win->StopSolveEvent();
								break;
							}
						}
						if (Win->IsNeedSolveEvent())
							SetSystemBeep(SetSystemBeep_Warning);
					}
				}
				if (BlockKeyEventAndWheelEvent&&InThisSet(event->type,PUI_Event::Event_KeyEvent,PUI_Event::Event_WheelEvent))
					Win->StopSolveEvent();	
			}

			virtual void CheckPos(const PUI_PosEvent *event,int mode)//There still exists some bugs I think,I will fix it when found
			{
				if (MenuDataArray.empty()||!(mode&PosMode_Default)) return;
				switch (event->posType)
				{
					case PUI_PosEvent::Pos_Down:
						PUI_DD[0]<<"Menu1 "<<IDName<<" click"<<endl;
						if (gPS.In(event->pos))
						{
							if (event->button==PUI_PosEvent::Button_MainClick)
								if (gPS.Shrink(BorderWidth).In(Win->NowPos()))
								{
									stat=Stat_Down;
									pos=GetPos(event->pos.y-gPS.y);
									Win->SetPresentArea(gPS);
								}
								Win->StopSolvePosEvent();
						}
						else
						{
							stat=Stat_NoFocus;
							Win->SetPresentArea(gPS);
							SetDelayDeleteThis();
						}
						break;
					case PUI_PosEvent::Pos_Up:
						if (stat==Stat_Down&&Win->IsNeedSolvePosEvent())
							if (gPS.Shrink(BorderWidth).In(event->pos))
							{
								PUI_DD[0]<<"Menu1 "<<IDName<<" function"<<endl;
								stat=Stat_UpFocus;
								pos=GetPos(event->pos.y-gPS.y);
								SetDelSubMenu();
								Win->SetPresentArea(gPS);
								Win->StopSolvePosEvent(); 
								if (InRange(pos,0,MenudataCnt-1))
								{
									MenuData <T> &md=MenuDataArray[pos];
									if (md.type==1&&md.enable)
									{
										if (md.func!=NULL)
											md.func(md.funcData);
										SetDelayDeleteWidget(rootMenu);
									}
								}
								else if (pos==-2)
								{
									PUI_DD[2]<<"not usable yet"<<endl;
									//...
								}
								else if (pos==-3)
								{
									PUI_DD[2]<<"not usable yet"<<endl;
									//...
								}
							}
						break;
					case PUI_PosEvent::Pos_Motion:
						if (gPS.In(event->pos))
						{
							int pos_r=pos;
							pos=GetPos(event->pos.y-gPS.y);
							if (pos!=pos_r)
							{
								stat=Stat_UpFocus;
								SetDelSubMenu();
								Win->SetPresentArea(gPS);
							}
							Win->StopSolvePosEvent();
						}
						else
						{
							if (stat==Stat_UpFocus)
								Win->SetPresentArea(gPS);
							stat=Stat_NoFocus;
							pos=-1;
						}
						break;
				}
			}
			
			virtual void _SolvePosEvent(const PUI_PosEvent *event,int mode)
			{
				if (!Win->IsNeedSolvePosEvent())
					return;
				if (Enabled)
				{
					if (Win->IsNeedSolvePosEvent())
						CheckPos(event,mode|PosMode_Pre);
					if (childWg!=NULL)
						SolvePosEventOf(childWg,event,mode);
					if (Win->IsNeedSolvePosEvent())
						CheckPos(event,mode|PosMode_Nxt|PosMode_Default);
				}
				if (nxtBrother!=NULL)
					SolvePosEventOf(nxtBrother,event,mode);
			}
			
			virtual void Show(Posize &lmt)
			{
				Win->RenderFillRect(gPS.Shrink(1)&lmt,ThemeColor(MenuColor[0]));
				Win->RenderDrawRectWithLimit(gPS,ThemeColor(MenuColor[stat]),lmt);
				int y=BorderWidth;
				if (Exceed)
				{
					//<<ExceedButton
				}
				
				for (int i=spos1;i<MenudataCnt;++i)
				{
					if (y+EachHeight>rPS.h-BorderWidth)
						break;
					MenuData <T> &md=MenuDataArray[i];
					switch (md.type)
					{
						case 1:
							if (stat!=0&&pos==i)
								Win->RenderFillRect((Posize(BorderWidth,y,rPS.w-BorderWidth*2,EachHeight)+gPS)&lmt,ThemeColor(MenuColor[stat]));
							if (md.pic()!=NULL)
								Win->RenderCopyWithLmt(md.pic(),Posize(BorderWidth,y,EachHeight,EachHeight).Shrink(2)+gPS,lmt);
							Win->RenderDrawText(md.hotKey!=0?md.text+"("+md.hotKey+")":md.text,Posize(BorderWidth+EachHeight+8,y,rPS.w-EachHeight-8-BorderWidth*2,EachHeight)+gPS,lmt,-1,ThemeColor(TextColor[!md.enable]));
							y+=EachHeight;
							break;
						case 2:
							Win->RenderFillRect((Posize(BorderWidth+EachHeight+4,y+DivideLineHeight/3,rPS.w-BorderWidth*2-EachHeight-4,max(DivideLineHeight/3,1))+gPS)&lmt,ThemeColor(DivideLineColor));
							y+=DivideLineHeight;
							break;
						case 3:
							if (stat!=0&&pos==i)
								Win->RenderFillRect((Posize(BorderWidth,y,rPS.w-BorderWidth*2,EachHeight)+gPS)&lmt,ThemeColor(MenuColor[stat]));
							if (md.pic()!=NULL)
								Win->RenderCopyWithLmt(md.pic(),Posize(BorderWidth,y,EachHeight,EachHeight).Shrink(2)+gPS,lmt);
							Win->RenderDrawText(md.hotKey!=0?md.text+"("+md.hotKey+")":md.text,Posize(BorderWidth+EachHeight+8,y,rPS.w-EachHeight-8-BorderWidth*2-EachHeight/2,EachHeight)+gPS,lmt,-1,ThemeColor(TextColor[!md.enable]));
							if (triTex[!md.enable]==NULL)
								triTex[!md.enable]=CreateTextureFromSurfaceAndDelete(CreateTriangleSurface(EachHeight/4,EachHeight/2,Point(0,0),Point(0,EachHeight/2-1),Point(EachHeight/4-1,EachHeight/4),ThemeColor(TextColor[!md.enable])));
							Win->RenderCopyWithLmt(triTex[!md.enable],Posize(rPS.w-BorderWidth*2-EachHeight/4,y+EachHeight/4,EachHeight/4,EachHeight/2)+gPS,lmt);
							y+=EachHeight;
							break;
					}
					spos2=i;
				}
				
				if (Exceed)
				{
					//<<ExceedButton
				}

				Win->Debug_DisplayBorder(gPS);
			}
			
			virtual void _PresentWidgets(Posize lmt)
			{
				if (nxtBrother!=NULL)
					PresentWidgetsOf(nxtBrother,lmt);
				if (Enabled)
				{
					Show(lmt);
					if (DEBUG_EnableWidgetsShowInTurn)
						SDL_Delay(DEBUG_WidgetsShowInTurnDelayTime),
						SDL_RenderPresent(Win->GetSDLRenderer());
					if (childWg!=NULL)
						PresentWidgetsOf(childWg,lmt);
				}
			}

			int GetHFromData()
			{
				int re=BorderWidth*2;
				for (int i=0;i<MenudataCnt;++i)//??
					switch (MenuDataArray[i].type)
					{
						case 1: case 3:	re+=EachHeight;	break;
						case 2:	re+=DivideLineHeight;	break;
					}
				return re;
			}
			
			virtual void CalcPsEx()
			{
				if (PsEx!=NULL)
					PsEx->GetrPS(rPS);
				Posize lastPs=CoverLmt;
				bool flag=gPS==ZERO_POSIZE;//??
				if (fa!=NULL)
					gPS=rPS+fa->GetgPS();
				else gPS=rPS;
				CoverLmt=gPS;
				if (!(lastPs==CoverLmt)||flag)
					Win->SetPresentArea(lastPs|CoverLmt);
			}
			
			Menu1(const WidgetID &_ID,Menu1 <T> *_faMenu1,const Point &pt,const vector <MenuData<T> > &menudata,int w=200)
			:Widgets(_ID,WidgetType_Menu1)
			{
				SetFa(_faMenu1);
				SetMenuData(menudata);
				SetrPS((Win->GetWinSize()-fa->GetgPS()).Shrink(1).EnsureIn({pt.x,pt.y,w,GetHFromData()}));
				IsSubMenu=1;
				rootMenu=_faMenu1->rootMenu;
			}
			
		public:
			void SetTextColor(bool p,const RGBA &co)
			{
				TextColor[p]=!co?ThemeColorMT[p]:co;
				Win->SetPresentArea(gPS);
				if (triTex[p]!=NULL)
					SDL_DestroyTexture(triTex[p]),triTex[p]=NULL;
			}

			inline void SetDivideLineColor(const RGBA &co)
			{
				DivideLineColor=!co?ThemeColorM[0]:co;
				Win->SetPresentArea(gPS);
			}

			inline void SetMenuColor(int p,const RGBA &co)
			{
				if (InRange(p,0,2))
					MenuColor[p]=!co?(p==0?ThemeColorBG[0]:ThemeColorBG[p*2]):co,Win->SetPresentArea(gPS);
				else PUI_DD[1]<<"Menu1: SetMenuColor: p "<<p<<" is not in Range[0,2]"<<endl;
			}
			
			void SetEachHeight(int h)
			{
				EachHeight=h;
				rPS.h=GetHFromData();
				SetrPS((Win->GetWinSize()-fa->GetgPS()).Shrink(1).EnsureIn(rPS));
				if (triTex[0]!=NULL)
					SDL_DestroyTexture(triTex[0]),triTex[0]=NULL;
				if (triTex[1]!=NULL)
					SDL_DestroyTexture(triTex[1]),triTex[1]=NULL;
				Win->SetNeedUpdatePosize();		
				Win->SetPresentArea(gPS);
			}
			
			void SetDivideLineHeight(int h)
			{
				DivideLineHeight=h;
				rPS.h=GetHFromData();
				SetrPS((Win->GetWinSize()-fa->GetgPS()).Shrink(1).EnsureIn(rPS));
				Win->SetNeedUpdatePosize();
				Win->SetPresentArea(gPS);
			}
			
			void SetBorderWidth(int w)
			{
				BorderWidth=w;
				rPS.h=GetHFromData();
				SetrPS((Win->GetWinSize()-fa->GetgPS()).Shrink(1).EnsureIn(rPS));
				Win->SetNeedUpdatePosize();
				Win->SetPresentArea(gPS);
			}

			void SetMenuData(const vector <MenuData<T> > &data)
			{
				MenuDataArray=data;
				MenudataCnt=MenuDataArray.size();
				rPS.h=GetHFromData();
				SetrPS((Win->GetWinSize()-fa->GetgPS()).Shrink(1).EnsureIn(rPS));
				Win->SetNeedUpdatePosize();
				Win->SetPresentArea(gPS);
			}
			
			inline void SetBlockKeyEventAndWheelEvent(bool on)
			{BlockKeyEventAndWheelEvent=on;}
			
			virtual ~Menu1()
			{
				if (IsSubMenu)
					((Menu1<T>*)fa)->subMenupos=-1;
			}
			
			Menu1(const WidgetID &_ID,const vector <MenuData<T> > &menudata,int w=256,PUI_Window *_win=CurrentWindow)
			:Widgets(_ID,WidgetType_Menu1)
			{
				SetFa(_win->MenuLayer());
				SetMenuData(menudata);
				SetrPS(Win->GetWinSize().EnsureIn({Win->NowPos().x,Win->NowPos().y,w,GetHFromData()}));
				rootMenu=this;
			}
	};
	
	class MessageBoxLayer:public Widgets
	{
		protected:
			enum
			{
				Stat_NoFocus=0,
				Stat_UpFocus=1,
				Stat_DownDrag=2,
				Stat_FocusX=3,
				Stat_DownX=4
			}stat=Stat_NoFocus;
			string MsgBoxTitle;
			bool WarningOn=0,
				 ShowTopAreaColor=0,
				 ShowTopAreaX=1,
				 EnableInterceptControlEvents=0,
				 EnableDragAreaLimit=true;
			int ClickOutsideReaction=3,//0:NONE 1:Close this 2:Warning and not transfer event 3:Warning and ring and not transfer event
				TopAreaHeight=30,//influence the size of X and Title text print area
				DragbleMode=2;//0:Undragble 1:Only top area dragble 2:All blank area dragble
			Point pinPtInLayer=ZERO_POINT;
			RGBA BorderColor[4]={ThemeColorM[0],ThemeColorM[2],ThemeColorM[4],{200,0,0,255}},//no focus(background),focus,drag,warning
				 TitleTextColor=ThemeColorMT[0],
				 TopAreaColor=ThemeColorM[1],
				 BackgroundColor=RGBA_WHITE;
			void (*CloseTriggerFunc)(void *)=NULL;
			void *CloseFuncData=NULL;
			BaseTypeFuncAndData *CloseTriggerFuncAndData=nullptr;
			bool Closing=0;
			
			virtual void CheckEvent(const PUI_Event *event)
			{
				if (event->type==PUI_Event::Event_KeyEvent)
					if (event->KeyEvent()->keyType==PUI_KeyEvent::Key_Down)
						if (event->KeyEvent()->keyCode==SDLK_ESCAPE)
						{
							if (ShowTopAreaX)
								Close();
							Win->StopSolveEvent();
						}
				if (EnableInterceptControlEvents)
					switch (event->type)//??
					{
						case PUI_Event::Event_KeyEvent:
						case PUI_Event::Event_MouseEvent:
						case PUI_Event::Event_TouchEvent:
						case PUI_Event::Event_TextEditEvent:
						case PUI_Event::Event_TextInputEvent:
						case PUI_Event::Event_PenEvent:
						case PUI_Event::Event_VirtualPos:
						case PUI_Event::Event_WheelEvent:
							Win->StopSolveEvent();
					}
					
			}
			
			virtual void CheckPos(const PUI_PosEvent *event,int mode)
			{
				if (!(mode&PosMode_Default)) return;
				switch (event->posType)
				{
					case PUI_PosEvent::Pos_Down:
						if (!gPS.In(event->pos))
						{
							if (!Win->IsNeedSolvePosEvent()) return;
							PUI_DD[0]<<"MessageBox "<<IDName<<" click outside"<<endl;
							stat=Stat_NoFocus;
							if (ClickOutsideReaction==1)
								Close();
							else if (ClickOutsideReaction==2||ClickOutsideReaction==3)
							{
								PUI_DD[0]<<"MessageBox "<<IDName<<" Warning!"<<endl;
								WarningOn=1;
								Win->SetPresentArea(gPS);
								if (ClickOutsideReaction==3)
									SetSystemBeep(SetSystemBeep_Warning);
							}
							else ;
							if (ClickOutsideReaction!=0)
								Win->StopSolvePosEvent();
						}
						else
						{
							if (event->button==PUI_PosEvent::Button_MainClick)
							{
								WarningOn=0;
								if (!Win->IsNeedSolvePosEvent())
									return;
								PUI_DD[0]<<"MessageBox "<<IDName<<" click Layer"<<endl;
								if (ShowTopAreaX&&event->pos.x>=gPS.x2()-TopAreaHeight&&event->pos.y<=gPS.y+TopAreaHeight)
									stat=Stat_DownX;
								else if (DragbleMode==1&&event->pos.y<=gPS.y+TopAreaHeight||DragbleMode==2)
								{
									PUI_DD[0]<<"MessageBox "<<IDName<<" drag"<<endl;
									stat=Stat_DownDrag;
									Win->SetOccupyPosWg(this);
									pinPtInLayer=event->pos-gPS.GetLU();
								}
								Win->SetPresentArea(gPS);
							}
							Win->StopSolvePosEvent();
						}
						break;
					case PUI_PosEvent::Pos_Up:
						if (Win->IsNeedSolvePosEvent())
							if (stat==Stat_DownDrag)
							{
								PUI_DD[0]<<"MessgeBox "<<IDName<<" stop drag"<<endl;
								stat=Stat_UpFocus;
								Win->SetOccupyPosWg(NULL);
								Win->SetPresentArea(gPS);
								Win->StopSolvePosEvent();
							}
							else if (stat==Stat_DownX&&gPS.In(event->pos))
							{
								PUI_DD[0]<<"MessgeBox "<<IDName<<" loose button"<<endl;
								stat=Stat_FocusX;
								Close();
								Win->StopSolvePosEvent();
							}
						break;
					case PUI_PosEvent::Pos_Motion:
						if (!Win->IsNeedSolvePosEvent()) return;
						if (stat==Stat_DownDrag)
						{
							Point newPt=event->pos-pinPtInLayer;
							rPS.x=newPt.x;
							rPS.y=newPt.y;
							if (EnableDragAreaLimit)
								rPS=Win->GetWinSize().EnsureIn(rPS);
							Win->SetPresentArea(rPS&gPS);//??
							Win->StopSolvePosEvent();
							Win->SetNeedUpdatePosize();
						}
						else
							if (gPS.In(event->pos))
							{
								if (ShowTopAreaX&&event->pos.x>=gPS.x2()-TopAreaHeight&&event->pos.y<=gPS.y+TopAreaHeight)
								{
									if (stat!=Stat_FocusX&&stat!=Stat_DownX)
										stat=Stat_FocusX;
								}
								else stat=Stat_UpFocus;
								Win->StopSolvePosEvent();
								Win->SetPresentArea(gPS);
							}
							else
								if (stat!=Stat_NoFocus)
								{
									stat=Stat_NoFocus;
									Win->SetPresentArea(gPS);
								}
								else if (InThisSet(ClickOutsideReaction,2,3)&&event->type==event->Event_TouchEvent)
									Win->StopSolvePosEvent();
						break;
				}
			}
			
			virtual void _SolvePosEvent(const PUI_PosEvent *event,int mode)//??
			{
				if (Enabled)
				{
					CheckPos(event,mode|PosMode_Pre);
					if (childWg!=NULL)
						SolvePosEventOf(childWg,event,mode);
					CheckPos(event,mode|PosMode_Nxt|PosMode_Default);
				}
				if (nxtBrother!=NULL)
					SolvePosEventOf(nxtBrother,event,mode);
			}
			
			virtual void Show(Posize &lmt)
			{
				if (ShowTopAreaColor)
					Win->RenderFillRect(Posize(gPS.x,gPS.y,gPS.w,TopAreaHeight)&lmt,ThemeColor(TopAreaColor)),
					Win->RenderFillRect(Posize(gPS.x,gPS.y+TopAreaHeight,gPS.w,gPS.h-TopAreaHeight)&lmt,ThemeColor(BackgroundColor));
				else Win->RenderFillRect(gPS&lmt,ThemeColor(BackgroundColor));
				Win->RenderDrawText(MsgBoxTitle,Posize(gPS.x+16,gPS.y,gPS.w,TopAreaHeight),lmt,-1,ThemeColor(TitleTextColor));
				//Temp
				if (ShowTopAreaX)
				{
					Win->RenderFillRect(Posize(gPS.x2()-TopAreaHeight,gPS.y,TopAreaHeight,TopAreaHeight).Shrink(3)&lmt,ThemeColor(BorderColor[stat>=3?stat-2:0]));
					Win->RenderDrawText("X",Posize(gPS.x2()-TopAreaHeight,gPS.y,TopAreaHeight,TopAreaHeight).Shrink(3),lmt);
				}
				
				Win->RenderDrawRectWithLimit(gPS,ThemeColor(WarningOn?BorderColor[3]:BorderColor[stat>=3?stat-2:stat]),lmt);
				Win->Debug_DisplayBorder(gPS);
			}
			
			MessageBoxLayer(const WidgetID &_ID,WidgetType _type,const string &title,int _w,int _h,PUI_Window *_win=CurrentWindow)
			:Widgets(_ID,_type),MsgBoxTitle(title)
			{
				SetFa(_win->MenuLayer());
				SetrPS({fa->GetrPS().w-_w>>1,fa->GetrPS().h-_h>>1,_w,_h});
			}
			
			MessageBoxLayer(const WidgetID &_ID,WidgetType _type,const string &title,const Posize &_ps0,PUI_Window *_win=CurrentWindow)
			:Widgets(_ID,_type),MsgBoxTitle(title)
			{
				SetFa(_win->MenuLayer());
				SetrPS(_ps0);
			}
			
		public:
			inline void SetBorderColor(int p,const RGBA &co)
			{
				if (InRange(p,0,3))
				{
					BorderColor[p]=!co?(p==3?RGBA(200,0,0,255):ThemeColorM[p*2]):co;
					Win->SetPresentArea(gPS);
				}
				else PUI_DD[1]<<"MessageBox "<<IDName<<" SetBorderColor error : p "<<p<<" is not in range[0,3]"<<endl;
			}
			
			inline void SetTitleTextColor(const RGBA &co)
			{
				TitleTextColor=!co?ThemeColorMT[0]:co;
				Win->SetPresentArea(gPS);
			}
			
			inline void SetTopAreaColor(const RGBA &co)
			{
				TopAreaColor=!co?ThemeColorM[1]:co;
				Win->SetPresentArea(gPS);
			}
			
			inline void SetBackgroundColor(const RGBA &co)
			{
				BackgroundColor=!co?RGBA_WHITE:co;
				Win->SetPresentArea(gPS);
			}
			
			inline void SetMessageBoxTitle(const string &title)
			{
				MsgBoxTitle=title;
				Win->SetPresentArea(gPS);
			}
			
			inline void SetClickOutsideReaction(int reaction)
			{ClickOutsideReaction=reaction;}
			
			inline void SetEnableInterceptControlEvents(bool on)
			{EnableInterceptControlEvents=on;}
			
			inline void SetTopAreaHeight(int heigth)
			{
				TopAreaHeight=heigth;
				Win->SetPresentArea(gPS);
			}
			
			inline void SetDragbleMode(int dragblemode)
			{DragbleMode=dragblemode;}
			
			inline void EnableShowTopAreaColor(bool on)
			{
				ShowTopAreaColor=on;
				Win->SetPresentArea(gPS);
			}
			
			inline void EnableShowTopAreaX(bool on)
			{
				ShowTopAreaX=on;
				Win->SetPresentArea(gPS);
			}
			
			inline void SetEnableDragAreaLimit(bool on)
			{
				EnableDragAreaLimit=on;
				Win->SetNeedUpdatePosize();
			}
			
			inline void Close(bool notriggerClosefunc=false)
			{
				if (Closing) return;
				Closing=1;
				if (!notriggerClosefunc)
				{
					if (CloseTriggerFunc!=NULL)
						CloseTriggerFunc(CloseFuncData);
					if (CloseTriggerFuncAndData!=nullptr)
						CloseTriggerFuncAndData->CallFunc(0);
				}
				SetDelayDeleteThis();
			}
			
			void SetCloseTriggerFunc(void (*func)(void*),void *funcdata=NULL)
			{
				CloseTriggerFunc=func;
				CloseFuncData=funcdata;
			}
			
			void SetCloseTriggerFuncAndData(BaseTypeFuncAndData *func)
			{
				DeleteToNULL(CloseTriggerFuncAndData);
				CloseTriggerFuncAndData=func;
			}
			
			virtual ~MessageBoxLayer()
			{
				DeleteToNULL(CloseTriggerFuncAndData);
				if (Win->GetOccupyPosWg()==this)
					Win->SetOccupyPosWg(NULL);
			}
			
			MessageBoxLayer(const WidgetID &_ID,const string &title,int _w,int _h,PUI_Window *_win=CurrentWindow)
			:MessageBoxLayer(_ID,WidgetType_MessageBoxLayer,title,_w,_h,_win) {}
			
			MessageBoxLayer(const WidgetID &_ID,const string &title,const Posize &_ps0,PUI_Window *_win=CurrentWindow)
			:MessageBoxLayer(_ID,WidgetType_MessageBoxLayer,title,_ps0,_win) {}
	};
	
	class MessageBoxLayerWindow:public Widgets
	{
		protected:
			
		public:
			
	};
	
	template <class T> class MessageBoxButton:public MessageBoxLayer
	{
		protected:
			struct MessageBoxButtonData
			{
				int p=-1;
				MessageBoxButton *wg=NULL;
				void (*func)(T&)=NULL;
				T funcdata;
				
				MessageBoxButtonData(int _p,MessageBoxButton *_wg,void (*_func)(T&),const T &_funcdata)
				:p(_p),wg(_wg),func(_func),funcdata(_funcdata) {}
				
				MessageBoxButtonData(int _p,MessageBoxButton *_wg)
				:p(_p),wg(_wg) {}
			};
			
			vector <Button<MessageBoxButtonData*>*> Bu;
			vector <MessageBoxButtonData*> BuDa;
			string MsgText;
			RGBA MsgTextColor=RGBA_NONE;
			int ButtonCnt=0,CloseTriggerButton=-1;
			int ButtonWidth=100,ButtonHeigth=30;
			bool ButtonClicked=0;
			TinyText *TT=NULL;
			
			Posize GetButtonPosize(int p)
			{
				switch (ButtonCnt)
				{
					case 1:
						return Posize(gPS.w-ButtonWidth>>1,gPS.h-40,ButtonWidth,ButtonHeigth);
					case 2:
						if (p==1) return Posize(gPS.w/2-ButtonWidth>>1,gPS.h-40,ButtonWidth,ButtonHeigth);
						else return Posize(gPS.w/2*3-ButtonWidth>>1,gPS.h-40,ButtonWidth,ButtonHeigth);
					case 3:
						if (p==1) return Posize(30,gPS.h-40,ButtonWidth,ButtonHeigth);
						else if (p==2) return Posize(gPS.w-ButtonWidth>>1,gPS.h-40,ButtonWidth,ButtonHeigth);
						else return Posize(gPS.w-30-ButtonWidth,gPS.h-40,ButtonWidth,ButtonHeigth);
					default:
						PUI_DD[2]<<"MessageBoxButton "<<IDName<<":There is too many buttons! It is not supported yet!"<<endl;
						return ZERO_POSIZE;
				}
			}
			
			class PosizeEX_ButtonInMsgBox:public PosizeEX
			{
				protected:
					MessageBoxButton *tar=NULL;
					int p;
					
				public:
					virtual void GetrPS(Posize &ps)
					{
						if (nxt!=NULL) nxt->GetrPS(ps);
						ps=tar->GetButtonPosize(p);
					}
					
					PosizeEX_ButtonInMsgBox(MessageBoxButton *_tar,int _p):tar(_tar),p(_p) {}
			};
			
			static void ButtonFunction(MessageBoxButtonData *&data)
			{
				if (data->func!=NULL)
					data->func(data->funcdata);
				data->wg->ButtonClicked=1;
				data->wg->Close();
			}
			
			static void MsgButtonCloseTriggerFunc(void *funcdata)
			{
				MessageBoxButton *p=(MessageBoxButton*)funcdata;
				if (p->ButtonClicked||p->CloseTriggerButton==-1) return;
				if (p->BuDa[p->CloseTriggerButton]->func!=NULL)
					p->BuDa[p->CloseTriggerButton]->func(p->BuDa[p->CloseTriggerButton]->funcdata);
			}
			
		public:
			inline void SetMsgTextColor(const RGBA &co)
			{TT->SetTextColor(MsgTextColor=co);}
			
			inline void SetMsgText(const string &str)
			{TT->SetText(str);}
			
//			Button<T>* GetButton(int p)
//			{
//				if (InRange(p,1,ButtonCnt))
//					return Bu[p-1];
//				else return NULL;
//			}
			
			void AddButton(const string &_text,void (*_func)(T&),const T &_funcdata)
			{
				++ButtonCnt;
				BuDa.push_back(new MessageBoxButtonData(ButtonCnt,this,_func,_funcdata));
				Bu.push_back(new Button<MessageBoxButtonData*>(0,this,new PosizeEX_ButtonInMsgBox(this,ButtonCnt),_text,ButtonFunction,BuDa.back()));
			}
			
			void AddButton(const string &_text)
			{
				++ButtonCnt;
				BuDa.push_back(new MessageBoxButtonData(ButtonCnt,this));
				Bu.push_back(new Button<MessageBoxButtonData*>(0,this,new PosizeEX_ButtonInMsgBox(this,ButtonCnt),_text,ButtonFunction,BuDa.back()));
			}
			
			inline void SetButtonWidth(int width)
			{
				ButtonWidth=width;
				Win->SetNeedUpdatePosize();
			}
			
			inline void SetCloseTriggerButton(int p)
			{CloseTriggerButton=EnsureInRange(p,-1,ButtonCnt-1);}
			
			virtual ~MessageBoxButton()
			{
				for (auto vp:BuDa)
					delete vp;
			}
			
			MessageBoxButton(const WidgetID &_ID,const string &title,const string &msgText,int _w=400,PUI_Window *_win=CurrentWindow)
			:MessageBoxLayer(_ID,WidgetType_MessageBoxButton,title,_w,120,_win),MsgText(msgText)
			{
				TT=new TinyText(0,this,new PosizeEX_Fa6(2,2,20,30,40,50),msgText,-1);//??
				EnableShowTopAreaColor(1);//??
				SetDragbleMode(1);
				MultiWidgets=1;
				CloseTriggerFunc=MsgButtonCloseTriggerFunc;
				CloseFuncData=this;
			}
	};
	
	template <class T> class DropDownButton:public Widgets
	{
		public:
			struct DropDownButtonData
			{
				string Text;
				T funcdata;
				
				DropDownButtonData(const string &_text,const T &_funcdata)
				:Text(_text),funcdata(_funcdata) {}
				
				DropDownButtonData() {}
			};
			
		protected:
			enum
			{
				Stat_NoFocus=0,
				Stat_UpFocus=1,
				Stat_Down=2
			}stat=Stat_NoFocus;
			string CurrentText;
			vector <DropDownButtonData> ChoiceData;
			void (*func)(T&,int)=NULL;//int:pos
			int CurrentChoose=-1;
			int ChoiceListEachHeight=30,//30 is the default height of SingleChoiceList
				TextMode=-1;
			bool AutoChooseWhenOnlyOneChoice=0;
			SimpleListView <DropDownButton<T>*> *SLV=NULL; 
			RGBA TextColor=ThemeColorMT[0],
				 BackgroundColor=ThemeColorBG[0],
				 BorderColor[3]{ThemeColorM[1],ThemeColorM[3],ThemeColorM[5]},
				 ListTextColor=RGBA_NONE,
				 ListColor[3]{RGBA_NONE,RGBA_NONE,RGBA_NONE};
			
			void CloseChoiceList()
			{
				if (SLV==NULL) return;
				SLV->GetLargeLayer()->DelayDelete();
				SLV=NULL;
			}
			
			static void SLV_Func(DropDownButton <T> *&ddb,int pos,int click)
			{
				if (pos==-1||click!=1) return;
				ddb->SetSelectChoice(pos,1);
				ddb->CloseChoiceList();
			}
			
			void OpenChoiceList()
			{
				if (ChoiceData.empty())
					return;
				if (SLV!=NULL)
					CloseChoiceList();
				Posize tarPs;
				if (Win->GetWinPS().h-gPS.y2()>ChoiceData.size()*ChoiceListEachHeight)
					tarPs=Posize(gPS.x,gPS.y2()+1,gPS.w,ChoiceData.size()*ChoiceListEachHeight);
				else if (gPS.y>ChoiceData.size()*ChoiceListEachHeight)
					tarPs=Posize(gPS.x,gPS.y-ChoiceData.size()*ChoiceListEachHeight-1,gPS.w,ChoiceData.size()*ChoiceListEachHeight);
				else if ((Win->GetWinPS().h-gPS.y2())*1.5>=gPS.y)
					tarPs=Posize(gPS.x,gPS.y2()+1,gPS.w,Win->GetWinPS().h-gPS.y2());
				else tarPs=Posize(gPS.x,0,gPS.w,gPS.y-1);
				
				SLV=new SimpleListView <DropDownButton<T>*> (0,Win->MenuLayer(),tarPs,SLV_Func);
				SLV->SetRowHeightAndInterval(ChoiceListEachHeight,0);
				SLV->SetTextMode(TextMode);
				SLV->SetTextColor(ListTextColor);
				for (int i=0;i<=2;++i)
					SLV->SetRowColor(i,ListColor[i]);
				for (auto &vp:ChoiceData)
					SLV->PushbackContent(vp.Text,this);
			}
			
			virtual void CheckEvent(const PUI_Event *event)
			{
				//...
			}
			
			virtual void CheckPos(const PUI_PosEvent *event,int mode)
			{
				if (mode&PosMode_LoseFocus)
				{
					if (SLV!=NULL)
						if (event->posType==PUI_PosEvent::Pos_Down&&!SLV->GetgPS().In(event->pos))//??
							CloseChoiceList();
						else return;
					if (stat!=Stat_NoFocus)
						if (!Win->IsPosFocused()||!CoverLmt.In(event->pos)||NotInSet(Win->GetOccupyPosWg(),this,nullptr)||mode&PosMode_ForceLoseFocus)
						{
							stat=Stat_NoFocus;
							RemoveNeedLoseFocus();
							Win->SetPresentArea(CoverLmt);
						}
				}
				else if (mode&PosMode_Default)
					switch (event->posType)
					{
						case PUI_PosEvent::Pos_Down:
							if (event->button==PUI_PosEvent::Button_MainClick)
							{
								PUI_DD[0]<<"DropDownButton "<<IDName<<" click"<<endl;
								stat=Stat_Down;
								SetNeedLoseFocus();
								Win->StopSolvePosEvent();
								Win->SetPresentArea(CoverLmt);
							}
							break;
						case PUI_PosEvent::Pos_Up:
							if (stat==Stat_Down)
							{
								PUI_DD[0]<<"DropDownButton "<<IDName<<" open choice list"<<endl;
								if (ChoiceData.size()==1&&AutoChooseWhenOnlyOneChoice)
									SetSelectChoice(0,1);
								else OpenChoiceList();
								stat=Stat_UpFocus;
								Win->StopSolvePosEvent();
								Win->SetPresentArea(CoverLmt);
							}
							break;
						case PUI_PosEvent::Pos_Motion:
							if (stat==Stat_NoFocus)
							{
								stat=Stat_UpFocus;
								SetNeedLoseFocus();
								Win->SetPresentArea(CoverLmt);
							}
							Win->StopSolvePosEvent();
							break;
					}
			}
			
			virtual void Show(Posize &lmt)
			{
				Win->RenderFillRect(lmt,ThemeColor(BackgroundColor));
				Win->RenderDrawRectWithLimit(gPS,ThemeColor(BorderColor[stat]),lmt);
				Win->RenderDrawText(CurrentText,gPS+Point(8,0),lmt,-1,ThemeColor(TextColor));
				Win->Debug_DisplayBorder(gPS);
			}
		
		public:
			void SetTextColor(const RGBA &co)
			{
				TextColor=!co?ThemeColorMT[0]:co;
				Win->SetPresentArea(CoverLmt);
			}
			
			void SetBackgroundColor(const RGBA &co)
			{
				BackgroundColor=!co?ThemeColorBG[0]:co;
				Win->SetPresentArea(CoverLmt);
			}
			
			void SetBorderColor(int p,const RGBA &co)
			{
				if (InRange(p,0,2))
				{
					BorderColor[p]=!co?ThemeColorM[p*2+1]:co;
					Win->SetPresentArea(CoverLmt);
				}
				else PUI_DD[1]<<"DropDownButton "<<IDName<<" SetBorderColor error : p "<<p<<" is not in range[0,2]"<<endl;
			}
			
			void SetListTextColor(const RGBA &co)
			{
				ListTextColor=co;
				if (SLV!=NULL)
					SLV->SetTextColor(co);
			}
			
			void SetListColor(int p,const RGBA &co)
			{
				if (InRange(p,0,2))
				{
					ListColor[p]=co;
					if (SLV!=NULL)
						SLV->SetRowColor(p,co);
				}
				else PUI_DD[1]<<"DropDownButton "<<IDName<<" SetListColor error : p "<<p<<" is not in range[0,2]"<<endl;
			}
			
			void SetChoiceData(int p,const DropDownButtonData &data)
			{
				p=EnsureInRange(p,0,ChoiceData.size());
				if (p<=CurrentChoose)
					++CurrentChoose;
				ChoiceData.insert(ChoiceData.begin()+p,data);
				if (SLV!=NULL)
					SLV->SetListContent(p,data.Text,this);
			}
			
			void SetChoiceData(int p,const string &_text,const T &_funcdata)
			{SetChoiceData(p,DropDownButtonData(_text,_funcdata));}
			
			void DeleteChoiceData(int p)
			{
				if (ChoiceData.empty()) return;
				p=EnsureInRange(p,0,ChoiceData.size()-1);
				if (p==CurrentChoose) CurrentChoose=-1;//??not choose,update presentArea and text
				else if (p<CurrentChoose) --CurrentChoose;
				ChoiceData.erase(ChoiceData.begin()+p);
				if (SLV!=NULL)
					SLV->DeleteListContent(p);
			}

			void ClearChoiceData()
			{
				if (ChoiceData.empty()) return;
				ChoiceData.clear();
				CurrentChoose=-1;
				CloseChoiceList();
			}
			
			DropDownButtonData& GetChoiceData(int p)
			{
				if (!ChoiceData.empty())
					return ChoiceData[EnsureInRange(p,0,ChoiceData.size()-1)];
			}

			DropDownButton <T>* PushbackChoiceData(const DropDownButtonData &data)
			{
				SetChoiceData(1e9,data);
				return this;
			}
			
			DropDownButton <T>* PushbackChoiceData(const string &_text,const T &_funcdata)
			{
				SetChoiceData(1e9,DropDownButtonData(_text,_funcdata));
				return this;
			}
			
			inline int GetChoiceCnt()
			{return ChoiceData.size();}

			void SetSelectChoice(int p,bool TrigerFunc)
			{
				if (ChoiceData.empty()) return;
				p=EnsureInRange(p,0,(int)ChoiceData.size()-1);
				CurrentChoose=p;
				CurrentText=ChoiceData[p].Text;
				Win->SetPresentArea(CoverLmt);
				if (TrigerFunc)
					if (func!=NULL)
						func(ChoiceData[p].funcdata,p);
			}
			
			int Find(const T &tar) const
			{
				for (int i=0;i<ChoiceData.size();++i)
					if (ChoiceData[i].funcdata==tar)
						return i;
				return -1;
			}
			
			void SetCurrentText(const string &_text)
			{
				CurrentText=_text;
				Win->SetPresentArea(CoverLmt);
			}

			inline T& GetFuncData(int p)
			{return ChoiceData[EnsureInRange(p,0,(int)ChoiceData.size()-1)].funcdata;}
			
			inline int GetCurrentChoosePos()
			{return CurrentChoose;}
			
			inline DropDownButtonData& GetCurrentChooseData()
			{return ChoiceData[CurrentChoose];}

			inline T& GetCurrentChooseFuncData()
			{return ChoiceData[CurrentChoose].funcdata;}
			
			inline void SetChoiceListEachLineHeight(int h)
			{
				ChoiceListEachHeight=h;
				if (SLV!=NULL)
					SLV->SetRowHeightAndInterval(h,0);
			}
			
			inline void SetTextMode(int mode)
			{
				TextMode=mode;
				if (SLV!=NULL)
					SLV->SetTextMode(mode);
			}
			
			inline void SetAutoChooseWhenOnlyOneChoice(bool en)
			{AutoChooseWhenOnlyOneChoice=en;}
			
			inline void SetFunc(void (*_func)(T&,int))
			{func=_func;}
			
			DropDownButton(const WidgetID &_ID,Widgets *_fa,const Posize &_rps,const string &_currentText="",void (*_func)(T&,int)=NULL)
			:Widgets(_ID,WidgetType_DropDownButton,_fa,_rps),CurrentText(_currentText),func(_func) {}

			DropDownButton(const WidgetID &_ID,Widgets *_fa,PosizeEX *psex,const string &_currentText="",void (*_func)(T&,int)=NULL)
			:Widgets(_ID,WidgetType_DropDownButton,_fa,psex),CurrentText(_currentText),func(_func) {}
	};
	
	class TitleMenu:public Widgets
	{
		protected:
			
		public:
			
	};
	
	template <class T> class AddressSection:public Widgets
	{
		public:
			struct AddressSectionData
			{
				friend class AddressSection <T>;
				protected:
					string text;
					int w=0;
				
				public:
					T data;
					
					void SetText(const string &str)
					{
						text=str;
						TTF_SizeUTF8(PUI_Font()(),text.c_str(),&w,NULL);
						w+=6;
					}
					
					inline const string GetText()
					{return text;}
					
					AddressSectionData(const string &str,const T &da)
					:text(str),data(da)
					{
						TTF_SizeUTF8(PUI_Font()(),text.c_str(),&w,NULL);
						w+=6;
					}
			};
			
		protected:
			//  |>|XXX|<|YYYYYYYYY|<|ZZZZ|
			//Defualt used for FileAddress(Section)
			enum
			{
				Stat_NoFocus=0,
				Stat_UpFocus=1,
				Stat_Down=2
			}stat=Stat_NoFocus;
			vector <AddressSectionData> AddrSecData;
			void (*func)(void*,AddressSection*,T&,int)=NULL;//funcData,this,data,NowFocus
			void *funcData=NULL;
			int SectionCnt=0,
				JointWidth=14,
				NowFocus=0;//0:not in +i:in i -i:in i list     //Do remember +-1 when using AddrSecData
			SharedTexturePtr TriangleTex[3][2];
			RGBA BackgroundColor[6],//0:no_focus_Joint 1:focus_Joint 2:down_Joint 3:no_focus_Section 4:focus_Section 5:down_Section
				 JointTriangleColor[3],//0:no_focus_Joint 1:focus_Joint 2:down_Joint
				 LastJointTriangleColor[3],
				 TextColor=ThemeColorMT[0];

			Doublet <int,int> GetStartShowPosAndWidth()
			{
				int p=SectionCnt-1,w=0;
				while (w==0||p>=0&&w+AddrSecData[p].w+JointWidth<=rPS.w)
					w+=AddrSecData[p--].w+JointWidth;
				return {p+1,w};
			}
			
			void InitDefaultColorAndTex()
			{
				for (int i=0;i<=2;++i)
					BackgroundColor[i]=ThemeColorM[i*2+1],
					BackgroundColor[i+3]=ThemeColorM[(i+1)*2],
					LastJointTriangleColor[i]=RGBA_NONE;
				JointTriangleColor[0]={250,250,250,255};
				JointTriangleColor[1]={240,240,240,255};
				JointTriangleColor[2]={230,230,230,255};
				for (int i=0;i<=2;++i)
					for (int j=0;j<=1;++j)
						TriangleTex[i][j]=SharedTexturePtr(NULL);
			}
			
			int GetChosenSection(int x)
			{
				if (SectionCnt==0) return 0;
				auto p=GetStartShowPosAndWidth().a;
				while (p<SectionCnt&&x>AddrSecData[p].w+JointWidth)
					x-=AddrSecData[p++].w+JointWidth;
				if (p==SectionCnt) return 0;
				else if (x<=JointWidth) return -p-1;
				else return p+1;
			}
			
			virtual void CheckPos(const PUI_PosEvent *event,int mode)
			{
				if (mode&PosMode_LoseFocus)
				{
					if (stat!=Stat_NoFocus)
						if (!Win->IsPosFocused()||!CoverLmt.In(event->pos)||NotInSet(Win->GetOccupyPosWg(),this,nullptr)||mode&PosMode_ForceLoseFocus)
						{
							stat=Stat_NoFocus;
							NowFocus=0;
							RemoveNeedLoseFocus();
							Win->SetPresentArea(CoverLmt);
						}
				}
				else if (mode&PosMode_Default)
					switch (event->posType)
					{
						case PUI_PosEvent::Pos_Down:
							if (event->button==PUI_PosEvent::Button_MainClick)
								if ((NowFocus=GetChosenSection(event->pos.x-gPS.x))!=0)
								{
									PUI_DD[0]<<"AddressSection "<<IDName<<" click "<<NowFocus<<endl;
									stat=Stat_Down;
									SetNeedLoseFocus();
									Win->StopSolvePosEvent();
									Win->SetPresentArea(CoverLmt);
								}
							break;
						case PUI_PosEvent::Pos_Up:
							if (stat==Stat_Down)
							{
								PUI_DD[0]<<"AddressSection "<<IDName<<" function "<<NowFocus<<endl;
								if (func!=NULL)
									func(funcData,this,AddrSecData[abs(NowFocus)-1].data,NowFocus);
								if (event->type==event->Event_TouchEvent)
								{
									stat=Stat_NoFocus;
									RemoveNeedLoseFocus();
								}
								else stat=Stat_UpFocus;
								Win->StopSolvePosEvent();
								Win->SetPresentArea(CoverLmt);
							}
							break;
						case PUI_PosEvent::Pos_Motion:
							int lastFocus=NowFocus;
							if ((NowFocus=GetChosenSection(event->pos.x-gPS.x))!=lastFocus)
							{
								if (NowFocus==0)
								{
									stat=Stat_NoFocus;
									RemoveNeedLoseFocus();
								}
								else
								{
									stat=Stat_UpFocus;
									SetNeedLoseFocus();
								}
								Win->SetPresentArea(CoverLmt);
							}
							if (NowFocus!=0)
								Win->StopSolvePosEvent();
							break;
					}
			}
			
			virtual void Show(Posize &lmt)
			{
				if (SectionCnt!=0)
				{
					auto p=GetStartShowPosAndWidth().a;
					Posize ps(gPS.x,gPS.y,0,gPS.h);
					for (int i=p;i<SectionCnt;++i)
					{
						ps.w=JointWidth;
						int col=stat!=0&&-NowFocus-1==i?stat:0;
						Win->RenderFillRect(ps&lmt,ThemeColor(BackgroundColor[col]));
						ReCreateTriangleTex(1);
						Win->RenderCopyWithLmt(TriangleTex[col][i==0](),ps,lmt);
						ps.x+=ps.w;
						
						ps.w=AddrSecData[i].w;
						col=stat!=0&&NowFocus-1==i?stat:0;
						Win->RenderFillRect(ps&lmt,ThemeColor(BackgroundColor[col+3]));
						Win->RenderDrawText(AddrSecData[i].text,ps,lmt,0,ThemeColor(TextColor));
						ps.x+=ps.w;
					}
				}
				Win->Debug_DisplayBorder(gPS);
			}
			
			void ReCreateTriangleTex(bool checkColorMode)
			{
				if (rPS.h<=0||JointWidth<=0) return;
				for (int i=0;i<=2;++i)
					if (!checkColorMode||LastJointTriangleColor[i]!=ThemeColor(JointTriangleColor[i]))
					{
						TriangleTex[i][0]=TriangleTex[i][1]=SharedTexturePtr(NULL);
						LastJointTriangleColor[i]=ThemeColor(JointTriangleColor[i]);
						TriangleTex[i][0]=SharedTexturePtr(CreateTextureFromSurfaceAndDelete(CreateTriangleSurface(JointWidth,rPS.h,{3,rPS.h>>1},{JointWidth-3,4},{JointWidth-3,rPS.h-4},LastJointTriangleColor[i])));
						TriangleTex[i][1]=SharedTexturePtr(CreateTextureFromSurfaceAndDelete(CreateTriangleSurface(JointWidth,rPS.h,{3,4},{3,rPS.h-4},{JointWidth-3,rPS.h>>1},LastJointTriangleColor[i])));
					}
			}
			
		public:
			inline void SetBackgroundColor(int p,const RGBA &co)
			{
				if (InRange(p,0,5))
				{
					BackgroundColor[p]=!co?ThemeColor[p%3*2+1+int(p>=3)]:co;
					Win->SetPresentArea(CoverLmt);
				}
				else PUI_DD[1]<<"SetBackgroundColor error : p "<<p<<" is not in range[0,5]"<<endl;
			}
			
			void SetJointTriangleColor(int p,const RGBA &co)
			{
				if (InRange(p,0,2))
				{
					JointTriangleColor[p]=co;
					Win->SetPresentArea(CoverLmt);
					ReCreateTriangleTex(1);
				}
				else PUI_DD[1]<<"SetJointTriangleColor error : p "<<p<<" is not in range[0,2]"<<endl;
			}
			
			inline void SetTextColor(const RGBA &co)
			{
				TextColor=!co?ThemeColorMT[0]:co;
				Win->SetPresentArea(CoverLmt);
			}
			
			void Clear()
			{
				AddrSecData.clear();
				SectionCnt=0;
				stat=Stat_NoFocus;
				NowFocus=0;
				Win->SetPresentArea(CoverLmt);
			}
			
			AddressSection* PushbackSection(const string &str,const T &da)
			{
				AddrSecData.push_back(AddressSectionData(str,da));
				SectionCnt++;
				Win->SetPresentArea(CoverLmt);
				return this;
			}
			
			void Popback()
			{
				AddrSecData.pop_back();
				SectionCnt--;
				if (abs(NowFocus)>SectionCnt)
					NowFocus=0;
				Win->SetPresentArea(CoverLmt);
			}
			
			void PopbackMulti(int cnt)
			{
				if (cnt<=0) return;
				cnt=min(cnt,SectionCnt);
				AddrSecData.erase(AddrSecData.begin()+SectionCnt-cnt,AddrSecData.end());
				SectionCnt-=cnt;
				if (abs(NowFocus)>SectionCnt)
					NowFocus=0;
				Win->SetPresentArea(CoverLmt);
			}
			
			T& GetSectionData(int p)
			{return AddrSecData[p].data;}
			
			inline int GetSectionCnt()
			{return SectionCnt;}
			
			inline int GetRealShowWidth()
			{return GetStartShowPosAndWidth().b;}
			
			void SetJointWidth(int w)
			{
				if (JointWidth==w)
					return;
				JointWidth=w;
				ReCreateTriangleTex(0);
				Win->SetPresentArea(CoverLmt);
			}
			
			inline void SetFunc(void (*_func)(void*,AddressSection*,T&,int),void *_funcData=NULL)
			{
				func=_func;
				funcData=_funcData;
			}
			
			AddressSection(const WidgetID &_ID,Widgets *_fa,const Posize &_rPS,void (*_func)(void*,AddressSection*,T&,int)=NULL,void *_funcData=NULL)
			:Widgets(_ID,WidgetType_AddressSection,_fa,_rPS),func(_func),funcData(_funcData)
			{
				InitDefaultColorAndTex();
				ReCreateTriangleTex(0);
			}
			
			AddressSection(const WidgetID &_ID,Widgets *_fa,PosizeEX *psex,void (*_func)(void*,AddressSection*,T&,int)=NULL,void *_funcData=NULL)
			:Widgets(_ID,WidgetType_AddressSection,_fa,psex),func(_func),funcData(_funcData)
			{
				InitDefaultColorAndTex();
				ReCreateTriangleTex(0);
			}
	};
	
	#ifdef PAL_BASICFUNCTIONS_FILE_CPP
	class FileAddressSection:public AddressSection <string>//<<the Menu1 was reconstructed as template that this class should change too...
	{
		protected:
			void (*func0)(void*,const string&)=NULL;
			void *funcdata0=NULL;
			
			struct FileAddrSecFuncForMenuData
			{
				FileAddressSection *FileAddrSec=NULL;
				string path,name;
				int pos;
				
				FileAddrSecFuncForMenuData(FileAddressSection *_FileAddrSec,const string &_path,const string &_name,int _pos)
				:FileAddrSec(_FileAddrSec),path(_path),name(_name),pos(_pos) {}
			};
			
			static void FileAddrSecFuncForMenu(void *&_funcdata)
			{
//				PUI_DD[3]<<"FileAddrSecFuncForMenu"<<endl;
				FileAddrSecFuncForMenuData *data=(FileAddrSecFuncForMenuData*)_funcdata;
				if (data->pos==1)
				{
					data->FileAddrSec->Clear();
					data->FileAddrSec->PushbackSection(data->name,data->name);
					if (data->FileAddrSec->func0!=NULL)
						data->FileAddrSec->func0(data->FileAddrSec->funcdata0,data->name);
				}
				else
				{
					data->FileAddrSec->PopbackMulti(data->FileAddrSec->GetSectionCnt()-data->pos+1);
					data->FileAddrSec->PushbackSection(data->name,data->path+PLATFORM_PATHSEPERATOR+data->name);
					if (data->FileAddrSec->func0!=NULL)
						data->FileAddrSec->func0(data->FileAddrSec->funcdata0,data->path+PLATFORM_PATHSEPERATOR+data->name);
				}
				delete data;
			}
			
			static void FileAddrSecFunc(void *_funcdata,AddressSection <string> *addrSec,string &path,int pos)
			{
//				PUI_DD[3]<<"FileAddrSecFunc. pos "<<pos<<endl;
				FileAddressSection *FileAddrSec=(FileAddressSection*)_funcdata;
				if (pos>0)
				{
					addrSec->PopbackMulti(addrSec->GetSectionCnt()-pos);
					if (FileAddrSec->func0!=NULL)
						FileAddrSec->func0(FileAddrSec->funcdata0,path);
				}
				else
				{
					vector <MenuData<void*> > menudata;
					if (pos==-1)
					{
						for (char panfu[3]="C:";panfu[0]<='Z';++panfu[0])
							menudata.push_back(MenuData<void*>(panfu,FileAddrSecFuncForMenu,new FileAddrSecFuncForMenuData(FileAddrSec,"",panfu,1)));
					}
					else
					{
						string pa=GetPreviousBeforeBackSlash(path);
						vector <string>* allDir=GetAllFile_UTF8(pa,0);
						for (auto vp:*allDir)
							menudata.push_back(MenuData<void*>(vp,FileAddrSecFuncForMenu,new FileAddrSecFuncForMenuData(FileAddrSec,pa,vp,-pos)));
						delete allDir;
					}
					new Menu1<void*>(0,menudata);
				}
			}
			
		public:
			inline void SetFunc(void (*_func0)(void*,const string&),void *_funcdata0=NULL)
			{
				func0=_func0;
				funcdata0=_funcdata0;
			}
			
			void SetAddress(const string &path)
			{
				Clear();
				int p=0,q=0;
				while (q<path.length())
				{
					if (path[q]==PLATFORM_PATHSEPERATOR)
					{
						PushbackSection(path.substr(p,q-p),path.substr(0,q));
						p=q+1;
					}
					++q;
				}
				PushbackSection(path.substr(p,q-p),path);
			}
			
			FileAddressSection(const WidgetID &_ID,Widgets *_fa,const Posize &_rps,const string &initPath,void (*_func0)(void*,const string&)=NULL,void *_funcdata0=NULL)
			:AddressSection(_ID,_fa,_rps),func0(_func0),funcdata0(_funcdata0)
			{
				PUI_DD[0]<<"Create FileAddressSection "<<IDName<<endl;
				Type=WidgetType_FileAddressSection;
				AddressSection<string>::SetFunc(FileAddrSecFunc,this);
				SetAddress(initPath);
			}
			
			FileAddressSection(const WidgetID &_ID,Widgets *_fa,PosizeEX *psex,const string &initPath,void (*_func0)(void*,const string&)=NULL,void *_funcdata0=NULL)
			:AddressSection(_ID,_fa,psex),func0(_func0),funcdata0(_funcdata0)
			{
				PUI_DD[0]<<"Create FileAddressSection "<<IDName<<endl;
				Type=WidgetType_FileAddressSection;
				AddressSection<string>::SetFunc(FileAddrSecFunc,this);
				SetAddress(initPath);
			}
	};
	#endif
	
//	class SpinBox:public Widgets
//	{
//		protected:
//			
//			
//		public:
//			
//			
//			
//	};

	class SimpleTextBox:public Widgets
	{
		public:
			static string SpecialCharVisibleConvert(const string &str)
			{
				if (str=="\t")
					return "<\\t>";
				else if (str=="\n")
					return "<\\n>";
				else if (str=="\r")
					return "<\\r>";
				else if (str=="\e")
					return "<\\e>";
				else if (str.length()==1&&(InRange(str[0],0,31)||str[0]==127))
					return "<"+llTOstr(str[0])+">";
				else return str;
			} 
			
			static string SpecialCharUnusualConvert(const string &str)
			{
				if (str=="\t")
					return "    ";
				else if (str=="\n")
					return "";
				else if (str=="\r")
					return "";
				else if (str=="\e")
					return "<\\e>";
				else if (str.length()==1&&(InRange(str[0],0,31)||str[0]==127))
					return "<"+llTOstr(str[0])+">";
				else return str;
			}
			
			static string SpecialCharNormalConvert(const string &str)
			{
				if (str=="\t")
					return "    ";
				else if (str=="\n")
					return "";
				else if (str=="\r")
					return "";
				else return str;
			}
			
			static string SpecialCharConvert(const string &str,int type)
			{
				switch (type)
				{
					case 0:	return SpecialCharNormalConvert(str);
					case 1:	return SpecialCharVisibleConvert(str);
					case 2:	return SpecialCharUnusualConvert(str);
					default:
						PUI_DD[2]<<"Invalid SpecialCharConvert type!"<<endl;
						return "";
				}
			}
			
		protected:
			struct EachChData
			{
				RGBA co=RGBA_NONE;
				int w=0;
			};
			
			struct EachLineData
			{
				stringUTF8_WithData <EachChData> str;
				vector <int> newLinePos;
				RGBA lineColor=RGBA_NONE;
//					 lineBackgroundColor=RGBA_NONE;
				int sumLineCounts=0;
				
				inline int CalcLineCounts(int last)
				{return sumLineCounts=last+newLinePos.size();}
				
				void UpdateLines(int W,const PUI_Font &font,bool forceUpdate,int specialcharconverttype)
				{
					newLinePos.clear();
					newLinePos.push_back(0);
					int sumw=0;
					for (int i=0,w;i<str.length();sumw+=str(i).w=w,++i)
					{
						if (forceUpdate||(w=str(i).w)==0)
							TTF_SizeUTF8(font(),SpecialCharConvert(str[i],specialcharconverttype).c_str(),&w,NULL);
						if (sumw+w>W&&i!=newLinePos.back())
							newLinePos.push_back(i),sumw=0;
					}
				}
			};
			
			SplayTree <EachLineData> Text;

			RGBA TextColor=RGBA_NONE;
			PUI_Font font;
			PosizeEX_InLarge *psexInLarge=NULL;
			int BorderLWidth=10,
				BorderRWidth=20,
				BorderUWidth=10,
				BorderDWidth=10,
				LineInterval=1,
				EachHeight=0,
				SumLineCounts=0;
			bool NextUpdateIsForced=0,
				 NeedUpdateLineState=0;
			int SpecialCharConvertType=0;
			
			void UpdateLineState()
			{
				for (int i=SumLineCounts=0;i<Text.size();++i)
					Text[i].UpdateLines(rPS.w-BorderLWidth-BorderRWidth,font,NextUpdateIsForced,SpecialCharConvertType),
					SumLineCounts=Text[i].CalcLineCounts(SumLineCounts);
				NextUpdateIsForced=0;
				NeedUpdateLineState=0;
			}
			
			Posize GetLinePosize(int pos)
			{
				if (pos==-1)
					return ZERO_POSIZE;
				int s=Text[pos].sumLineCounts,a=Text[pos].newLinePos.size();
				return Posize(gPS.x+BorderLWidth,gPS.y+BorderUWidth+(s-a)*(EachHeight+LineInterval),gPS.w-BorderLWidth-BorderRWidth,a*(EachHeight+LineInterval)-LineInterval);
			}
			
			int GetShowStartPos()//???
			{
				int l=0,r=Text.size()-1;
				while (l<r)
				{
					int mid=l+r>>1;
					Posize ps=GetLinePosize(mid);
					if (ps.y2()<=CoverLmt.y) l=mid+1;
					else r=mid;
				}
				return l;
			}
			
			Range GetShowRange()//??
			{
				Range re;
				re.r=re.l=GetShowStartPos();
				if (re.l==SumLineCounts) return re;
				int y=GetLinePosize(re.l).y2();
				while (y+LineInterval<CoverLmt.y2()-BorderDWidth&&re.r<SumLineCounts-1)
					y+=GetLinePosize(++re.r).h+LineInterval;
				return re;
			}
			
			virtual void Show(Posize &lmt)
			{
				if (SumLineCounts==0||Text.size()==0) return;
				Range forLR=GetShowRange();
				for (int i=forLR.l;i<=forLR.r;++i)
				{
					Posize linePS=GetLinePosize(i);
					EachLineData &ld=Text[i];
					if (ld.str.empty()) continue;
					int p=0,q=1;
					Posize chPS(linePS.x,linePS.y,ld.str(0).w,EachHeight);
					while (1)
					{
						if ((chPS&lmt).Size()!=0)
						{
							RGBA co=ThemeColor(!ld.str(p).co?(!ld.lineColor?TextColor:ld.lineColor):ld.str(p).co);
							Win->RenderDrawText(SpecialCharConvert(ld.str[p],SpecialCharConvertType),chPS,lmt,0,co,font);//??
						}
						++p;
						if (p>=ld.str.length())
							break;
						if (q<ld.newLinePos.size()&&p==ld.newLinePos[q])
							chPS.x=linePS.x,chPS.y+=EachHeight+LineInterval,++q;
						else chPS.x+=chPS.w;
						chPS.w=ld.str(p).w;
					}
				}
				Win->Debug_DisplayBorder(gPS);
			}
			
			virtual void CalcPsEx()
			{
				if (PsEx!=NULL)
					PsEx->GetrPS(rPS);
				static int LastWidth=0;//??
				if (LastWidth!=rPS.w-BorderLWidth-BorderRWidth||NeedUpdateLineState)
					UpdateLineState();
				LastWidth=rPS.w-BorderLWidth-BorderRWidth;
				rPS.h=max(0,SumLineCounts*(EachHeight+LineInterval)-LineInterval+BorderUWidth+BorderDWidth);
				if (psexInLarge->NeedFullFill())
					rPS.h=max(rPS.h,psexInLarge->GetLargeLayer()->GetrPS().h-rPS.y-max(0,-psexInLarge->Fullfill()));
				Posize lastPs=CoverLmt;
				if (fa!=NULL)
					gPS=rPS+fa->GetgPS();
				else gPS=rPS;
				CoverLmt=gPS&GetFaCoverLmt();
				if (!(lastPs==CoverLmt))
					Win->SetPresentArea(lastPs|CoverLmt);
			}
			
		public:
			inline void SetDefaultTextColor(const RGBA &co)
			{
				TextColor=co;
				Win->SetPresentArea(CoverLmt);
			}
			
			inline void SetLineTextColor(int pos,const RGBA &co)
			{
				if (InRange(pos,0,Text.size()-1))
				{
					Text[pos].lineColor=co;
					Win->SetPresentArea(CoverLmt);
				}
				else PUI_DD[2]<<"SimpleTextBox "<<IDName<<" SetLineTextColor line pos is not in range."<<endl;
			}
			
			inline void SetCharColor(int line,int pos,const RGBA &co)
			{
				if (InRange(line,0,Text.size()-1))
					if (InRange(pos,0,Text[line].str.length()-1))
					{
						Text[line].str(pos).co=co;
						Win->SetPresentArea(CoverLmt);
					}
					else PUI_DD[2]<<"SimpleTextBox "<<IDName<<" SetCharColor pos is not in range!"<<endl;
				else PUI_DD[2]<<"SimpleTextBox "<<IDName<<" SetCharColor line is not in range!"<<endl;
			}
			
			inline void SetBackgroundColor(const RGBA &co)
			{
				if (psexInLarge!=NULL)
					psexInLarge->GetLargeLayer()->SetLargeAreaColor(co);
			}

			inline void SetRowInterval(int itv)
			{
				LineInterval=itv;
				NeedUpdateLineState=1;				
				Win->SetNeedUpdatePosize();
				Win->SetPresentArea(CoverLmt);
			}
			
			inline void SetBorderWidth(char which,int w)//??
			{
				switch (which)
				{
					case 'U':	BorderUWidth=w;	break;
					case 'D':	BorderDWidth=w;	break;
					case 'L':	BorderLWidth=w;	break;
					case 'R':	BorderRWidth=w;	break;
					default:	PUI_DD[2]<<"SimpleTextBox "<<IDName<<" SetBorderWidth wrong parameter: which: "<<(int)which<<endl;	break;
				}
				NeedUpdateLineState=1;
				Win->SetNeedUpdatePosize();
				Win->SetPresentArea(CoverLmt);
			}
			
			inline int GetLines()
			{return Text.size();}
			
			inline int GetLineLength(int p)
			{return Text[p].str.length();}
			
			inline int GetShowLines()//may not be accurate when changed immediately and not updated.
			{return SumLineCounts;}
			
			inline void Clear()
			{
				Text.clear();
				SumLineCounts=0;
				NextUpdateIsForced=0;
				NeedUpdateLineState=1;
				Win->SetNeedUpdatePosize();
				Win->SetPresentArea(CoverLmt);
			}
			
			inline void AddNewLine(int p,const stringUTF8 &strUtf8,const RGBA &co=RGBA_NONE)
			{
				EachLineData eld;
				eld.str=strUtf8;
				eld.lineColor=co;
				Text.insert(Text.size(),eld);
				NeedUpdateLineState=1;
				Win->SetNeedUpdatePosize();
				Win->SetPresentArea(CoverLmt);
			}
			
			inline void AddText(int line,int pos,const stringUTF8 &strUtf8,const RGBA &co=RGBA_NONE)
			{
				EachLineData &eld=Text[line];
				eld.str.insert(pos,strUtf8);
				for (int i=0;i<strUtf8.length();++i)
					eld.str(pos+i).co=co;
				NeedUpdateLineState=1;
				Win->SetNeedUpdatePosize();
				Win->SetPresentArea(CoverLmt);
			}
			
			inline void AppendNewLine(const stringUTF8 &strUtf8,const RGBA &co=RGBA_NONE)
			{
				AddNewLine(Text.size(),strUtf8,co);
				UpdateWidgetsPosizeOf(psexInLarge->GetLargeLayer());//??
				psexInLarge->GetLargeLayer()->SetViewPort(4,1);
			}
			
			inline void AppendText(const stringUTF8 &strUtf8,const RGBA &co=RGBA_NONE)
			{
				if (Text.size()==0) AppendNewLine("");
				AddText(Text.size()-1,Text[Text.size()-1].str.length(),strUtf8,co);
				UpdateWidgetsPosizeOf(psexInLarge->GetLargeLayer());//??
				psexInLarge->GetLargeLayer()->SetViewPort(4,1);
			}
			
			inline void SetText(const stringUTF8 &strUtf8,const RGBA &co=RGBA_NONE)
			{
				Clear();
				int p=0;
				for (int i=0;i<=strUtf8.length();++i)
					if (i==strUtf8.length()&&p<strUtf8.length())
						AppendNewLine(strUtf8.substr(p,i-p),co),p=i+1;
					else if (i<strUtf8.length()&&strUtf8[i]=="\n")
						AppendNewLine(strUtf8.substr(p,i-p+1),co),p=i+1;
				psexInLarge->GetLargeLayer()->SetViewPort(4,0);
			}
			
			inline void AppendLinedText(const stringUTF8 &strUtf8,const RGBA &co=RGBA_NONE)
			{
				bool flag=false;
				if (int n;Text.size()>0&&(n=Text[Text.size()-1].str.length())>0&&Text[Text.size()-1].str[n-1]!="\n")
					flag=true;
				int p=0;
				for (int i=0;i<=strUtf8.length();++i)
					if (i==strUtf8.length()&&p<strUtf8.length())
						if (flag)
							AppendText(strUtf8.substr(p,i-p),co),p=i+1,flag=false;
						else AppendNewLine(strUtf8.substr(p,i-p),co),p=i+1;
					else if (i<strUtf8.length()&&strUtf8[i]=="\n")
						if (flag)
							AppendText(strUtf8.substr(p,i-p+1),co),p=i+1,flag=false;
						else AppendNewLine(strUtf8.substr(p,i-p+1),co),p=i+1;
			}
			
			inline void DeleteLine(int p,int cnt=1)
			{
				Text.erase(p,cnt);
				NeedUpdateLineState=1;
				Win->SetNeedUpdatePosize();
				Win->SetPresentArea(CoverLmt);
			}
			
			inline void DeleteText(int line,int pos,int len)
			{
				Text[line].str.erase(pos,len);
				NeedUpdateLineState=1;
				Win->SetNeedUpdatePosize();
				Win->SetPresentArea(CoverLmt);
			}

			inline stringUTF8 GetLineUTF8(int p)
			{return Text[p].str.StringUTF8();}
			
			inline string GetLine(int p)
			{return Text[p].str.cppString();}
			
			stringUTF8 GetAllTextUTF8()
			{
				stringUTF8 re;
				for (int i=0;i<Text.size();++i)
					re+=Text[i].str.StringUTF8();
				return re;
			}
			
			string GetAllText()
			{
				string re;
				for (int i=0;i<Text.size();++i)
					re+=Text[i].str.cppString();
				return re;
			}
			
			inline void SetFontSize(int size)//size==0:default
			{
				if (InRange(size,0,127))
				{
					font.Size=size;//??
					TTF_SizeUTF8(font()," ",NULL,&EachHeight);
					NextUpdateIsForced=1;
					NeedUpdateLineState=1;
					Win->SetNeedUpdatePosize();
					Win->SetPresentArea(CoverLmt);
				}
				else PUI_DD[2]<<"SimpleTextBox "<<IDName<<" SetFontSize size "<<size<<" is out of range[0,127]!"<<endl;
			}
			
			inline void SetFont(const PUI_Font &tar)
			{
				font=tar;
				TTF_SizeUTF8(font()," ",NULL,&EachHeight);
				NextUpdateIsForced=1;
				NeedUpdateLineState=1;
				Win->SetNeedUpdatePosize();
				Win->SetPresentArea(CoverLmt);
			}
			
			inline void SetSpecialCharConvertType(int type)
			{
				SpecialCharConvertType=type;
				NextUpdateIsForced=1;
				NeedUpdateLineState=1;
				Win->SetNeedUpdatePosize();
				Win->SetPresentArea(CoverLmt);
			}
			
//			inline void SetEnableScrollResize(bool enable)
//			{EnableScrollResize=enable;}
			
			LargeLayerWithScrollBar* GetLargeLayer()
			{return psexInLarge->GetLargeLayer();}
			
			void ResetLargeLayer(PosizeEX_InLarge *largePsex)//??
			{
				SetFa(largePsex->GetLargeLayer()->LargeArea());
				ReAddPsEx(largePsex);
				psexInLarge=largePsex;
			}
			
			SimpleTextBox(const WidgetID &_ID,PosizeEX_InLarge *largePsex)
			:Widgets(_ID,WidgetType_SimpleTextBox),psexInLarge(largePsex)
			{
				SetFa(largePsex->GetLargeLayer()->LargeArea());
				AddPsEx(largePsex);
				SetFontSize(0);
			}
			
			SimpleTextBox(const WidgetID &_ID,Widgets *_fa,const Posize &_rps)
			:Widgets(_ID,WidgetType_SimpleTextBox)
			{
				LargeLayerWithScrollBar *larlay=new LargeLayerWithScrollBar(0,_fa,_rps);
				SetFa(larlay->LargeArea());
				AddPsEx(psexInLarge=new PosizeEX_InLarge(larlay,0));
				SetFontSize(0);
			}
			
			SimpleTextBox(const WidgetID &_ID,Widgets *_fa,PosizeEX *psex)
			:Widgets(_ID,WidgetType_SimpleTextBox)
			{
				LargeLayerWithScrollBar *larlay=new LargeLayerWithScrollBar(0,_fa,psex);
				SetFa(larlay->LargeArea());
				AddPsEx(psexInLarge=new PosizeEX_InLarge(larlay,0));
				SetFontSize(0);
			}
	};
	
	template <class T> class TextEditLine:public Widgets
	{
		public:
			enum
			{
				EE_None=0,
				EE_StartEdit,
				EE_EndEdit,
				EE_StartInput,
				EE_EndInput,
				EE_Editing,
				EE_Enter//Triggerred before enter func.
			};
		
		protected:
			enum
			{
				Stat_NoFocus=0,
				Stat_UpFocus=1,
				Stat_DownLeft=2,
				Stat_DownRight=3
			}stat=Stat_NoFocus;
			stringUTF8 Text,
					   editingText;
			vector <int> ChWidth;
//			vector <RGBA> ChColor;
			bool Editing=0,
				 IntervalTimerOn=0,
				 ShowBorder=1;
			int	StateInput=0,
				ChHeight=0,
				SumWidth=0,
				BorderWidth=3,
				LengthLimit=128,
				EditingTextCursorPos,
				CursorPosX=0;
//				LastEditingTimeStamp=0;
			Range pos={0,0},//this condition l>r is allowed //pos1:pos.l pos2:pos.r(also means current cursor)
				  ShowPos={0,0};
//			Uint32 UpdateSposInterval=300;
//			SDL_TimerID IntervalTimerID;
			string EmptyText;
			void (*EnterFunc)(T&,const stringUTF8&,TextEditLine<T>*,bool)=NULL;//bool:0:change 1:enter
			void (*ExtraFunc)(T&,int,TextEditLine<T>*)=nullptr;//int:EventType
		 	T funcData;
			RGBA TextColor[3]{ThemeColorMT[0],ThemeColorMT[1],ThemeColorM[6]},
				 BackgroundColor[4]{ThemeColorBG[0],ThemeColorM[1],ThemeColorM[3],ThemeColorM[5]},//bg,choosepart stat0,1,2/3
				 BorderColor[4]{ThemeColorM[1],ThemeColorM[3],ThemeColorM[5],RGBA(255,0,0,255)},//stat0,1,2/3 overlimit
				 EmptyTextColor=ThemeColorBG[4];
//			void TurnOnOffIntervalTimer(bool on)
//			{
//				if (IntervalTimerOn==on) return;
//				if (IntervalTimerOn=on)
//					IntervalTimerID=SDL_AddTimer(UpdateSposInterval,PUI_UpdateTimer,new PUI_UpdateTimerData(this,-1));
//				else
//				{
//					SDL_RemoveTimer(IntervalTimerID);
//					IntervalTimerID=0;
//				}
//			}
			
			void SetSposFromAnother(bool fromSposL)//used when one edge of the showPos is set,to calc the other side
			{
				int i,s;
				if (fromSposL)
				{
					for (i=ShowPos.l,s=BorderWidth;i<Text.length()&&s<=rPS.w-BorderWidth;++i)
						s+=ChWidth[i];
					if (s<=rPS.w-BorderWidth) ShowPos.r=Text.length();
					else ShowPos.r=i-1;
				}
				else
				{
					for (i=ShowPos.r-1,s=rPS.w-BorderWidth;i>=0&&s>=BorderWidth;--i)
						s-=ChWidth[i];
					if (s>=BorderWidth) ShowPos.l=0;
					else ShowPos.l=i+2;
				}
			}
		
			void SetSposFromPos2()//adjust ShowPos to ensure pos2 in ShowPos
			{
				if (pos.r<ShowPos.l)
				{
					ShowPos.l=pos.r;
					SetSposFromAnother(1);
					Win->SetPresentArea(CoverLmt);
				}
				else if (ShowPos.r<pos.r)
				{
					ShowPos.r=pos.r;
					SetSposFromAnother(0);
					Win->SetPresentArea(CoverLmt);
				}
			}
			
			int GetPos(int x)//return char interval index:  0,ch0,1,ch1,2,ch2,3
			{
				if (x<BorderWidth)
				{
					for (int s=BorderWidth,i=0,j=ShowPos.l;j>=0;--j)
					{
						if (x>s-ChWidth[j]/2) return i+ShowPos.l;
						s-=ChWidth[j];
						--i;
						if (x>=s) return max(0,i+ShowPos.l);
					}
					return 0;
				}
				else
				{
					for (int s=BorderWidth,i=0,j=ShowPos.l;j<ChWidth.size();++j)
					{
						if (x<s+ChWidth[j]/2) return i+ShowPos.l;
						s+=ChWidth[j];
						++i;
						if (x<=s) return i+ShowPos.l;
					}
					return Text.length();
				}
			}
		public:
			void ClearText(bool triggerFunc=1)
			{
				SumWidth=0;
				ChWidth.clear();
				Text.clear();
				pos=ShowPos=ZERO_RANGE;
				Win->SetPresentArea(CoverLmt);
				if (triggerFunc)
				{
					if (ExtraFunc)
						ExtraFunc(funcData,EE_Editing,this);
					if (EnterFunc!=NULL)
						EnterFunc(funcData,Text,this,0);
				}
			}
			
			void AddText(int p,const stringUTF8 &strUtf8)//p means the interval index,it will be auto Ensured in range[0,Text.length()]
			{
				p=EnsureInRange(p,0,Text.length());
				int len=min(LengthLimit-Text.length(),strUtf8.length());
				Text.insert(p,strUtf8,0,len);
				vector <int> chw;
				for (int i=0,w,h;i<len;++i)
				{
					TTF_SizeUTF8(PUI_Font()(),strUtf8[i].c_str(),&w,&h);
					chw.push_back(w);
					SumWidth+=w;
					ChHeight=max(ChHeight,h);
				}
				ChWidth.insert(ChWidth.begin()+p,chw.begin(),chw.end());
				pos.l=pos.r=p+len;
				SetSposFromPos2();
				if (ExtraFunc)
					ExtraFunc(funcData,EE_Editing,this);
				if (EnterFunc!=NULL)
					EnterFunc(funcData,Text,this,0);
			}
			
			void SetText(const stringUTF8 &strUtf8,bool triggerFunc=1)
			{
				ClearText(0);
				Text=strUtf8;
				if (Text.length()>LengthLimit)
					Text.erase(LengthLimit,Text.length()-LengthLimit);
				for (int i=0,w,h;i<Text.length();++i)
				{
					TTF_SizeUTF8(PUI_Font()(),Text[i].c_str(),&w,&h);
					ChWidth.push_back(w);
					SumWidth+=w;
					ChHeight=max(ChHeight,h);
				}
				pos.l=pos.r=Text.length();
//				SetSposFromPos2();//??
//				if (SumWidth>rPS.w-BorderWidth*2)
//				{
//					ShowPos.r=Text.length();
//					SetSposFromAnother(0);
//				}
//				else 
//				{
					ShowPos.l=0;
					SetSposFromAnother(1);
//				}
				if (triggerFunc)
				{
					if (ExtraFunc)
						ExtraFunc(funcData,EE_Editing,this);
					if (EnterFunc!=NULL)
						EnterFunc(funcData,Text,this,0);
				}
			}
			
			void append(const stringUTF8 &strUtf8)
			{AddText(Text.length(),strUtf8);}
			
			void DeleteText(int p,int len)
			{
				for (int i=p;i<p+len;++i)
					SumWidth-=ChWidth[i];
				ChWidth.erase(ChWidth.begin()+p,ChWidth.begin()+p+len);
				Text.erase(p,len);
				pos.l=pos.r=p;
				if (ShowPos.r>Text.length())//??next 8 lines?
					ShowPos.r=Text.length();
				SetSposFromPos2();
				if (ShowPos.l>Text.length())
				{
					ShowPos.r=Text.length();
					SetSposFromAnother(0);
				}
				if (ExtraFunc)
					ExtraFunc(funcData,EE_Editing,this);
				if (EnterFunc!=NULL)
					EnterFunc(funcData,Text,this,0);
			}
			
			void DeleteTextBack()
			{
				if (Text.length()==0)
					PUI_DD[1]<<"TextEditLine: "<<IDName<<" DeleteTextBack error: Text is empty"<<endl;
				else DeleteText(Text.length()-1,1);
			}
			
			void DeleteTextCursor()
			{
				if (pos.Len0())
					if (pos.r==0) return;
					else DeleteText(pos.r-1,1);
				else DeleteText(min(pos.l,pos.r),abs(pos.Length()));
			}
			
			void AddTextCursor(const stringUTF8 &strUtf8)
			{
				if (!pos.Len0())
					DeleteTextCursor();
				AddText(pos.r,strUtf8);
			}
			
			void SetCursorPos(int p)
			{
				pos.l=pos.r=EnsureInRange(p,0,Text.length());
				Win->SetPresentArea(CoverLmt);
			}

			void SetCursorPos(const Range &rg)
			{
				pos=rg&Range(0,Text.length());
				Win->SetPresentArea(CoverLmt);
			}
			
			stringUTF8 GetSelectedTextUTF8()
			{
				if (pos.Len0()) return stringUTF8();
				else return Text.substrUTF8(min(pos.l,pos.r),abs(pos.Length()));
			}
			
			string GetSelectedText()
			{
				if (pos.Len0()) return "";
				else return Text.substr(min(pos.l,pos.r),abs(pos.Length()));
			}
			
			const stringUTF8& GetTextUTF8()
			{return Text;}
			
			string GetText()
			{return Text.cppString();}

			void StopTextInput()
			{
				if (StateInput)
				{
					if (Editing)
					{
						Editing=0;
						if (ExtraFunc)
							ExtraFunc(funcData,EE_EndEdit,this);
					}
					StateInput=0;
					if (ExtraFunc)
						ExtraFunc(funcData,EE_EndInput,this);
					TurnOnOffIMEWindow(0,Win);
					Win->SetPresentArea(CoverLmt);
					Win->SetKeyboardInputWg(NULL);
					editingText.clear();
				}
			}
			
			void StartTextInput(const Range &rg)
			{
//				DD<<"#"<<rg.l<<" "<<rg.r<<endl;
				SetCursorPos(rg);
				if (!StateInput)
				{
					StateInput=1;
					if (ExtraFunc)
						ExtraFunc(funcData,EE_StartInput,this);
					TurnOnOffIMEWindow(1,Win);
					Win->SetKeyboardInputWg(this);
					Win->SetPresentArea(CoverLmt);
				}
			}
			
			void StartTextInput(int p)
			{
				p=EnsureInRange(p,0,Text.length());
				StartTextInput({p,p});
			}
			
			void StartTextInput()
			{StartTextInput({0,Text.length()});}
			
		protected:
			virtual void ReceiveKeyboardInput(const PUI_TextEvent *event)
			{
				switch (event->type)
				{
					case PUI_Event::Event_TextInputEvent:
					{
						if (!event->text.empty())
							AddTextCursor(stringUTF8(event->text));
						Win->StopSolveEvent();
						break;
					}
					case PUI_Event::Event_TextEditEvent:
					{
						PUI_DD[0]<<"TextEditLine: "<<IDName<<" Editing text: cursor "<<event->cursor<<", length "<<event->length<<endl;
						editingText=event->text;
						EditingTextCursorPos=event->cursor;
						Win->StopSolveEvent();
						if (!pos.Len0())
							DeleteTextCursor();	
						SetCurrentIMEWindowPos({CursorPosX,gPS.y},gPS,Win);
						break;
					}
				}
				if ((editingText.length()!=0)!=Editing)
					if (!Editing)
					{
						Editing=1;
						if (ExtraFunc)
							ExtraFunc(funcData,EE_StartEdit,this);
					}
					else
					{
						Editing=0;
						if (ExtraFunc)
							ExtraFunc(funcData,EE_EndEdit,this);
					}
				Win->SetPresentArea(CoverLmt);
			}
			
			virtual void CheckEvent(const PUI_Event *event)
			{
				switch (event->type)
				{
					case PUI_Event::Event_KeyEvent:
						if (StateInput)
							if (!Editing)
							{
								PUI_KeyEvent *keyevent=event->KeyEvent();
								bool StillNeedSolveEvent=0;
//								if (event.key.timestamp-LastEditingTimeStamp>0)//??
								if (keyevent->keyType==keyevent->Key_Down||keyevent->keyType==keyevent->Key_Hold)
									switch (keyevent->keyCode)
									{
										case SDLK_BACKSPACE:
											PUI_DD[0]<<"TextEditLine:"<<IDName<<" DeleteTextBack"<<endl;
											DeleteTextCursor();
											Win->SetPresentArea(CoverLmt);
											break;
										case SDLK_LEFT:
											if (pos.r>0)
											{
												pos.l=--pos.r;
												SetSposFromPos2();
												Win->SetPresentArea(CoverLmt);
											}
											break;
										case SDLK_RIGHT:
											if (pos.r<Text.length())
											{
												pos.l=++pos.r;
												SetSposFromPos2();
												Win->SetPresentArea(CoverLmt);
											}
											break;
										case SDLK_RETURN:
										case SDLK_KP_ENTER:
										if (ExtraFunc)
											ExtraFunc(funcData,EE_Enter,this);
										if (EnterFunc!=NULL)
											EnterFunc(funcData,Text,this,1);
											break;
										case SDLK_HOME:
											pos.l=pos.r=0;
											SetSposFromPos2();
											Win->SetPresentArea(CoverLmt);
											break;
										case SDLK_END:
											pos.l=pos.r=Text.length();
											SetSposFromPos2();
											Win->SetPresentArea(CoverLmt);
											break;
										case SDLK_v:
											if (keyevent->mod&KMOD_CTRL)
											{
												char *s=SDL_GetClipboardText();
												stringUTF8 str=s;
												SDL_free(s);
												if (str.empty())
													break;
												if (!pos.Len0())
													DeleteTextCursor();
												AddText(pos.r,str);
												SetSposFromPos2();
											}
											break;
										case SDLK_c:
											if (keyevent->mod&KMOD_CTRL)
												if (!pos.Len0())
													SDL_SetClipboardText(GetSelectedText().c_str());
											break;
										case SDLK_x:
											if (keyevent->mod&KMOD_CTRL)
												if (!pos.Len0())
												{
													SDL_SetClipboardText(GetSelectedText().c_str());
													DeleteTextCursor();
													Win->SetPresentArea(CoverLmt);
												}
											break;
										case SDLK_z:
											if (keyevent->mod&KMOD_CTRL)
											{
												PUI_DD[2]<<"TextEditLine: "<<IDName<<" ctrl z cannot use yet"<<endl;
											}
											break;
										case SDLK_a:
											if (keyevent->mod&KMOD_CTRL)
												SetCursorPos({0,Text.length()});
											break;
										case SDLK_ESCAPE:
											StopTextInput();
											break;
										default:
											StillNeedSolveEvent=1;
											break;
									}
								if (!StillNeedSolveEvent)
									Win->StopSolveEvent(); 
							}
							else Win->StopSolveEvent();
					break;
						
					default:
						if (event->type==PUI_EVENT_UpdateTimer)
							if (event->UserEvent()->data1==this)
							{
								SetSposFromPos2();
								pos.r=GetPos(Win->NowPos().y-gPS.y);
							}
						break;
				}
			}
			
			virtual void CheckPos(const PUI_PosEvent *event,int mode)
			{
				if (mode&PosMode_LoseFocus)
				{
					if (stat!=Stat_NoFocus&&Win->GetOccupyPosWg()!=this)
						if (!Win->IsPosFocused()||!CoverLmt.In(event->pos)||mode&PosMode_ForceLoseFocus)
							if (StateInput)
							{
								if (event->posType==event->Pos_Down)
									if (event->button==event->Button_MainClick)
									{
										stat=Stat_NoFocus;
										RemoveNeedLoseFocus();
										StopTextInput();
									}
							}
							else if (stat!=Stat_NoFocus)
							{
								stat=Stat_NoFocus;
								Win->SetPresentArea(CoverLmt);
								RemoveNeedLoseFocus();
							}
				}
				else if (mode&PosMode_Default)
					switch (event->posType)
					{
						case PUI_PosEvent::Pos_Down:
							PUI_DD[0]<<"TextEditLine "<<IDName<<" click"<<endl;
							if (!Editing)
								if (event->button==event->Button_MainClick)
								{
									stat=Stat_DownLeft;
									Win->SetOccupyPosWg(this);
									SetNeedLoseFocus();
									pos.l=pos.r=GetPos(event->pos.x-gPS.x);
									PUI_DD[0]<<"TextEditLine: "<<IDName<<" Start  input"<<endl;
								}
								else if (event->button==event->Button_SubClick)
								{
									stat=Stat_DownRight;
									PUI_DD[2]<<"TextEditLine: "<<IDName<<" Cannot right click yet!"<<endl;
									//<<...Not usable yet
								}
							Win->StopSolvePosEvent();
							Win->SetPresentArea(CoverLmt);
							break;
						case PUI_PosEvent::Pos_Up:
							if (stat==Stat_DownLeft||stat==Stat_DownRight)
							{
								PUI_DD[0]<<"TextEditLine:"<<IDName<<" up"<<endl;
								if (event->type==event->Event_TouchEvent)
									if (event->button==PUI_PosEvent::Button_MainClick)
										stat=Stat_DownLeft;
									else if (event->button==PUI_PosEvent::Button_SubClick)
										stat=Stat_DownRight;
								if (Win->GetOccupyPosWg()==this)
									Win->SetOccupyPosWg(NULL);
								stat=Stat_UpFocus;
								StartTextInput(pos);
								Win->StopSolvePosEvent();
							}
							break;
						case PUI_PosEvent::Pos_Motion:
							if (stat==Stat_DownLeft||stat==Stat_DownRight)
							{
								if (!Editing&&Text.length()!=0)
								{
									pos.r=GetPos(event->pos.x-gPS.x);
									SetSposFromPos2();
	//								if (InRange(event->pos.x,gPS.x+BorderWidth,gPS.x2()-BorderWidth))
	//									TurnOnOffIntervalTimer(0);
	//								else TurnOnOffIntervalTimer(1);
									Win->SetPresentArea(CoverLmt);
								}
							}
							else if (stat==Stat_NoFocus)
							{
								stat=Stat_UpFocus;
								SetNeedLoseFocus();
								Win->SetPresentArea(CoverLmt);
							}
							Win->StopSolvePosEvent();
							break;
					}
			}

			virtual void Show(Posize &lmt)
			{
				int editingTextLen=-Text.length();
				if (StateInput&&Editing)
				{
					AddText(pos.r,editingText);
					editingTextLen+=Text.length();
					SetCursorPos(pos.r-editingTextLen+EditingTextCursorPos);
				}
				else editingTextLen=0;
				
				Win->RenderFillRect(lmt&gPS,ThemeColor(BackgroundColor[0]));
				if (ShowBorder)
					Win->RenderDrawRectWithLimit(gPS,ThemeColor(BorderColor[Text.length()==LengthLimit?3:EnsureInRange((int)stat,0,2)]),lmt);
				int x=BorderWidth,w=0,m=min(pos.l,pos.r),M=max(pos.l,pos.r);
				for (int i=ShowPos.l;i<Text.length()&&i<=ShowPos.r;++i)
					if (i<m) x+=ChWidth[i];
					else if (i>=M) break;
					else w+=ChWidth[i];
				if (StateInput&&pos.Len0())//Draw cursor 
					Win->RenderFillRect(Posize(!Editing?CursorPosX=x+gPS.x:x+gPS.x,BorderWidth+gPS.y,2,rPS.h-BorderWidth*2)&lmt,ThemeColor(BackgroundColor[1+EnsureInRange((int)stat,0,2)]));
				Win->RenderFillRect(Posize(x+gPS.x,BorderWidth+gPS.y,w,rPS.h-BorderWidth*2)&lmt,ThemeColor(BackgroundColor[1+EnsureInRange((int)stat,0,2)]));
				
				int i=0,s;
				if (Text.length()!=0)
					for (i=ShowPos.l,s=BorderWidth;i<Text.length()&&s+ChWidth[i]<=rPS.w-BorderWidth;s+=ChWidth[i++])
					{
						int colorPos=editingTextLen!=0&&InRange(i,pos.r-EditingTextCursorPos,pos.r-EditingTextCursorPos+editingTextLen-1)?2:InRange(i,m,M-1);
						SDL_Texture *tex=Win->CreateRGBATextTexture(Text[i].c_str(),ThemeColor(TextColor[colorPos]));//??
						Win->RenderCopyWithLmt(tex,Posize(s,rPS.h-ChHeight>>1,ChWidth[i],ChHeight)+gPS,lmt&gPS.Shrink(BorderWidth));
						SDL_DestroyTexture(tex);
					}
				else if (EmptyText!="")
					Win->RenderDrawText(EmptyText,gPS+Point(5,0),lmt,-1,ThemeColor(EmptyTextColor));
				ShowPos.r=i;

				if (StateInput&&Editing)
					DeleteText(pos.r-EditingTextCursorPos,editingTextLen);
				
				Win->Debug_DisplayBorder(gPS);
			}
			
		public:
			inline void SetTextColor(int p,const RGBA &co)
			{
				if (InRange(p,0,2))
				{
					TextColor[p]=!co?(p==2?ThemeColorM[6]:ThemeColorMT[p]):co;
					Win->SetPresentArea(CoverLmt);
				}
				else PUI_DD[1]<<"TextEditLine: "<<IDName<<" SetTextColor p "<<p<<" is not in Range[0,2]"<<endl;
			}
			
			inline void SetBackgroundColor(int p,const RGBA &co)
			{
				if (InRange(p,0,3))
				{
					BackgroundColor[p]=!co?(p==0?ThemeColorBG[0]:ThemeColorM[p*2-1]):co;
					Win->SetPresentArea(CoverLmt);
				}
				else PUI_DD[1]<<"TextEditLine: "<<IDName<<" SetBackgroundColor p "<<p<<" is not in Range[0,3]"<<endl;
			}
			
			inline void SetBorderColor(int p,const RGBA &co)
			{
				if (InRange(p,0,3))
				{
					BorderColor[p]=!co?(p==3?RGBA(255,0,0,200):ThemeColorM[p*2+1]):co;
					Win->SetPresentArea(CoverLmt);
				}
				else PUI_DD[1]<<"TextEditLine: "<<IDName<<" SetBorderColor p "<<p<<" is not in Range[0,3]"<<endl;
			}
			
			inline void SetEmptyTextColor(const RGBA &co)
			{
				EmptyTextColor=!co?ThemeColorBG[4]:co;
				if (Text.empty())
					Win->SetPresentArea(CoverLmt);
			}
			
			inline void SetShowBorder(bool bo)
			{
				ShowBorder=bo;
				Win->SetPresentArea(CoverLmt);
			}
			
			inline void SetBorderWidth(int w)
			{
				BorderWidth=w;
				Win->SetPresentArea(CoverLmt);
			}
			
			void SetLengthLimit(int len)
			{
				LengthLimit=len;
				if (len<Text.length())
				{
					PUI_DD[2]<<"TextExitLine: "<<IDName<<" set LengthLimit less than Text.length() cannot use yet"<<endl;
					//<<...
				}				
			}
			
			inline void SetEmptyText(const string &text)
			{
				EmptyText=text;
				if (Text.empty())
					Win->SetPresentArea(CoverLmt);
			}
			
			inline void SetEnterFunc(void (*_enterfunc)(T&,const stringUTF8&,TextEditLine<T>*,bool),const T &_funcdata)
			{
				EnterFunc=_enterfunc;
				funcData=_funcdata;
			}
			
			inline void SetEnterFuncData(const T &_funcdata)
			{funcData=_funcdata;}
			
			inline T& GetEnterFuncData()
			{return funcData;}
			
			inline void SetExtraFunc(void (*_extrafunc)(T&,int,TextEditLine<T>*))
			{ExtraFunc=_extrafunc;}
			
			inline void TriggerFunc(bool isenter=1)
			{
				if (ExtraFunc)
					ExtraFunc(funcData,isenter?EE_Enter:EE_Editing,this);
				if (EnterFunc!=NULL)
					EnterFunc(funcData,Text,this,isenter);
			}

			virtual ~TextEditLine()
			{
//				TurnOnOffIntervalTimer(0);
				StopTextInput();
			}
			
			TextEditLine(const WidgetID &_ID,Widgets *_fa,const Posize &_rps,void (*_enterfunc)(T&,const stringUTF8&,TextEditLine<T>*,bool),const T &_funcdata)
			:Widgets(_ID,WidgetType_TextEditLine,_fa,_rps),EnterFunc(_enterfunc),funcData(_funcdata) {}
			
			TextEditLine(const WidgetID &_ID,Widgets *_fa,PosizeEX *psex,void (*_enterfunc)(T&,const stringUTF8&,TextEditLine<T>*,bool),const T &_funcdata)
			:Widgets(_ID,WidgetType_TextEditLine,_fa,psex),EnterFunc(_enterfunc),funcData(_funcdata) {}
			
			TextEditLine(const WidgetID &_ID,Widgets *_fa,const Posize &_rps)
			:Widgets(_ID,WidgetType_TextEditLine,_fa,_rps) {}
			
			TextEditLine(const WidgetID &_ID,Widgets *_fa,PosizeEX *psex)
			:Widgets(_ID,WidgetType_TextEditLine,_fa,psex) {}
	};
	
	template <class T> class TextEditBox:public Widgets
	{
		protected:
			struct EachChData
			{
				SharedTexturePtr tex[2];
				int timeCode[2]={0,0};
			};
			
			SharedTexturePtr ASCIItex[128][2];
			int timeCodeOfASCIItex[128][2];
			
			enum
			{
				Stat_NoFocus=0,
				Stat_UpFocus=1,
				Stat_DownLeft=2,
				Stat_DownRight=3
			}stat=Stat_NoFocus;
			SplayTree <stringUTF8_WithData <EachChData> > Text;
			stringUTF8 editingText;
			vector <int> TextLengthCount;
			stack <int> TextLengthChange;
			int MaxTextLength=0;
			int EachChWidth=12,//chinese character is twice the english character width
				EachLineHeight=24;
			PUI_Font font;
			string FontPath="";
			LargeLayerWithScrollBar *fa2=NULL;
			bool Editing=0,
				 ShowBorder=1,
				 AutoNextLine=0,//realize it ten years later =_=|| 
				 ShowSpecialChar=0,
				 EnableScrollResize=0,
				 EnableEdit=1;
			int	StateInput=0,
				BorderWidth=3,
				EditingTextCursorPos,
				NewestCode=1;
			void (*EachChangeFunc)(T&,TextEditBox*,bool)=NULL;//bool:Is inner change//It is not so beautiful...
			T EachChangeFuncdata;
			void (*RightClickFunc)(T&,const Point&,TextEditBox*)=NULL;
			T RightClickFuncData;
			Point Pos=ZERO_POINT,Pos_0=ZERO_POINT,//This pos(_0) is not related to pixels
				  EditingPosL=ZERO_POINT,EditingPosR=ZERO_POINT,
				  LastCursorPos=ZERO_POINT;//this is related to pixels
			RGBA TextColor[3],
				 BackgroundColor[4],//bg,choosepart stat0,1,2/3
				 BorderColor[3],//stat0,1,2/3
				 LastTextColor[3];
			
			void InitDefualtColor()
			{
				TextColor[0]=ThemeColorMT[0];
				TextColor[1]=ThemeColorMT[1];
				TextColor[2]=ThemeColorM[6];
				BackgroundColor[0]=ThemeColorBG[0];
				for (int i=0;i<=2;++i)
					BorderColor[i]=BackgroundColor[i+1]=ThemeColorM[i*2+1],
					LastTextColor[i]=RGBA_NONE;
			}
			
			inline bool CheckWidth(const string &str)
			{return str.length()>=2;}
			
			int GetChLenOfEachWidth(const string &str)
			{
				if (str=="\t") return 4;
				return 1+CheckWidth(str);
			}
			
			int GetStrLenOfEachWidth(const stringUTF8_WithData <EachChData> &strUtf8)
			{
				int re=0;
				for (int i=0;i<strUtf8.length();++i)
					re+=GetChLenOfEachWidth(strUtf8[i]);
				return re;
			}
			
			int GetChWidth(const string &str)
			{
				if (str=="\t") return 4*EachChWidth;
				return EachChWidth+CheckWidth(str)*EachChWidth;
			}
			
			inline int StrLenWithoutSlashNR(const stringUTF8_WithData <EachChData> &strUtf8)
			{
				if (strUtf8.length()==0) return 0;
				if (strUtf8[strUtf8.length()-1]=="\n")
					if (strUtf8.length()>=2&&strUtf8[strUtf8.length()-2]=="\r")
						return strUtf8.length()-2;
					else return strUtf8.length()-1;
				else if (strUtf8[strUtf8.length()-1]=="\r")
					if (strUtf8.length()>=2&&strUtf8[strUtf8.length()-2]=="\n")
						return strUtf8.length()-2;
					else return strUtf8.length()-1;
				else return strUtf8.length();
			}
			
			inline Point EnsurePosValid(const Point &pt)
			{return Point(EnsureInRange(pt.x,0,StrLenWithoutSlashNR(Text[pt.y])),EnsureInRange(pt.y,0,Text.size()-1));}
			
			int GetTextLength(int line,int len)
			{
				stringUTF8_WithData <EachChData> &strUtf8=Text[line];
				int w=0;
				for (int i=0;i<len;++i)
					w+=GetChWidth(strUtf8[i]);
				return w;
			}
			
			Point GetPos(const Point &pt)
			{
				int i=0,w,LineY=EnsureInRange((pt.y-gPS.y)/EachLineHeight,0,Text.size()-1),
					x=pt.x-gPS.x-BorderWidth;
				stringUTF8_WithData <EachChData> &strUtf8=Text[LineY];
				int strlen=StrLenWithoutSlashNR(strUtf8);
				while (i<strlen)
				{
					w=GetChWidth(strUtf8[i]);
					if (x<=w/2) return Point(i,LineY);
					++i;
					x-=w;
				}
				return Point(i,LineY);
			}
			
			void SetShowPosFromPos()//??
			{
				if (BorderWidth+EachLineHeight*Pos.y<-fa->GetrPS().y)
					fa2->SetViewPort(2,BorderWidth+EachLineHeight*Pos.y);
				else if (BorderWidth+EachLineHeight*(Pos.y+1)>-fa->GetrPS().y+fa2->GetrPS().h-fa2->GetButtomBarEnableState()*fa2->GetScrollBarWidth())
					fa2->SetViewPort(2,BorderWidth+EachLineHeight*(Pos.y+1)-fa2->GetrPS().h+fa2->GetButtomBarEnableState()*fa2->GetScrollBarWidth());
				int w=GetTextLength(Pos.y,Pos.x);
				if (BorderWidth+w<-fa->GetrPS().x+EachChWidth)
					fa2->SetViewPort(1,BorderWidth+w-EachChWidth);
				else if (BorderWidth+w>-fa->GetrPS().x+fa2->GetrPS().w-fa2->GetRightBarEnableState()*fa2->GetScrollBarWidth()-EachChWidth)
					fa2->SetViewPort(1,BorderWidth+w-fa2->GetrPS().w+fa2->GetRightBarEnableState()*fa2->GetScrollBarWidth()+EachChWidth);
			}
			
			void ResizeTextBoxHeight()
			{
				fa2->ResizeLL(0,EachLineHeight*Text.size()+2*BorderWidth);
				rPS.h=max(fa2->GetrPS().h,EachLineHeight*Text.size()+2*BorderWidth);
			}
			
			void ApplyTextLenChange()
			{
//				PUI_DD[3]<<"TextEditBox "<<IDName<<" ApplyTextLenChange Start"<<endl;
				int newMaxLen=MaxTextLength;
				while (!TextLengthChange.empty())
				{
					int len=TextLengthChange.top();
					TextLengthChange.pop();
					if (len>=0)
					{
						if (len>=TextLengthCount.size())
							TextLengthCount.resize(len+1,0);
						TextLengthCount[len]++;
						newMaxLen=max(newMaxLen,len);
					}
					else TextLengthCount[-len]--;
				}
				
				if (TextLengthCount[newMaxLen]==0)
					for (int i=newMaxLen-1;i>=0;--i)
						if (TextLengthCount[i]>0)
						{
							newMaxLen=i;
							break;
						}
				if (newMaxLen!=MaxTextLength)
				{
					MaxTextLength=newMaxLen;
					fa2->ResizeLL(MaxTextLength*EachChWidth+BorderWidth*2,0);
					rPS.w=max(fa2->GetrPS().w,MaxTextLength*EachChWidth+BorderWidth*2);
				}
//				PUI_DD[3]<<"TextEditBox "<<IDName<<" ApplyTextLenChange End"<<endl;
			}
			
			void AddTextLenCnt(int len)
			{
//				PUI_DD[3]<<"TextEditBox "<<IDName<<" AddTextLenCnt Start "<<len<<endl;
				TextLengthChange.push(len);
//				PUI_DD[3]<<"TextEditBox "<<IDName<<" AddTextLenCnt End"<<endl;
			}
			
			void SubtractTextLenCnt(int len)
			{
//				PUI_DD[3]<<"TextEditBox "<<IDName<<" SubtractTextLenCnt Start "<<len<<endl;
				TextLengthChange.push(-len);
//				PUI_DD[3]<<"TextEditBox "<<IDName<<" SubtractTextLenCnt End"<<endl;
			}

			virtual void CalcPsEx()
			{
				if (PsEx!=NULL)
					PsEx->GetrPS(rPS);
				Posize lastPs=CoverLmt;
				fa2->ResizeLL(MaxTextLength*EachChWidth+BorderWidth*2,0);//??
				rPS.w=max(fa2->GetrPS().w,MaxTextLength*EachChWidth+BorderWidth*2);
				ResizeTextBoxHeight();
				if (fa!=NULL)
					gPS=rPS+fa->GetgPS();
				else gPS=rPS;
				Posize fa2gPS=fa2->GetgPS();//??
				CoverLmt=gPS&GetFaCoverLmt()&fa2gPS;
				if (!(lastPs==CoverLmt))
					Win->SetPresentArea(lastPs|CoverLmt);
				if (fa2->GetRightBarEnableState())
					fa2gPS.w-=fa2->GetScrollBarWidth();
				if (fa2->GetButtomBarEnableState())
					fa2gPS.h-=fa2->GetScrollBarWidth();
			}
			
//			int MaintainNearbyLines(int p,int cnt)
//			{
//				
//			}

			bool InSelectedRange(const Point &pt0,const Point &pt1,const Point &pt2)//[L,R)
			{
				if (pt1==pt2) return 0;
				if (pt1.y==pt2.y)
					return pt0.y==pt1.y&&InRange(pt0.x,min(pt1.x,pt2.x),max(pt1.x,pt2.x)-1);
				else
					if (InRange(pt0.y,min(pt1.y,pt2.y),max(pt1.y,pt2.y)))
						if (pt1.y<pt2.y)
							if (pt0.y==pt1.y)
								return pt0.x>=pt1.x;
							else if (pt0.y==pt2.y)
								return pt0.x<pt2.x;
							else return 1;
						else
							if (pt0.y==pt2.y)
								return pt0.x>=pt2.x;
							else if (pt0.y==pt1.y)
								return pt0.x<pt1.x;
							else return 1;
					else return 0;
			}
		
			void _Clear(bool isInnerChange)
			{
				if (!EnableEdit&&isInnerChange) return;
				Text.clear();
				editingText.clear();
				TextLengthCount.clear();
				MaxTextLength=0;
				Text.insert(0,stringUTF8("\r\n"));
				AddTextLenCnt(2);
				Pos=Pos_0={0,0};
				ApplyTextLenChange();
				ResizeTextBoxHeight();
				SetShowPosFromPos();
				Editing=0;
				Win->SetPresentArea(CoverLmt);
				if (EachChangeFunc!=NULL)
					EachChangeFunc(EachChangeFuncdata,this,isInnerChange);
			}
			
			void _AddNewLine(int p,const stringUTF8 &strUtf8,bool isInnerChange)
			{
				if (strUtf8.length()==0||!EnableEdit&&isInnerChange) return;
//				PUI_DD[3]<<"TextEditBox "<<IDName<<" AddNewLine Start"<<endl;
				int i=0,j=0;
				while (i<strUtf8.length())
				{
					if (strUtf8[i]=="\r")
					{
						if (i<strUtf8.length()-1&&strUtf8[i+1]=="\n")
							++i;
						stringUTF8 substr=strUtf8.substrUTF8(j,i-j+1);
						Text.insert(p++,substr);
						AddTextLenCnt(GetStrLenOfEachWidth(substr));
						j=i+1;
					}
					else if (strUtf8[i]=="\n")
					{
						if (i<strUtf8.length()-1&&strUtf8[i+1]=="\r")
							++i;
						stringUTF8 substr=strUtf8.substrUTF8(j,i-j+1);
						Text.insert(p++,substr);
						AddTextLenCnt(GetStrLenOfEachWidth(substr));
						j=i+1;
					}
					++i;
				}
				if (j<strUtf8.length())
				{
					stringUTF8 substr=strUtf8.substrUTF8(j,strUtf8.length()-j)+stringUTF8("\r\n");
					Text.insert(p++,substr);
					AddTextLenCnt(GetStrLenOfEachWidth(substr));
				}
				Pos=Pos_0={StrLenWithoutSlashNR(Text[p-1]),p-1};
				ApplyTextLenChange();
				ResizeTextBoxHeight();
				SetShowPosFromPos();
				Win->SetPresentArea(CoverLmt);
				if (EachChangeFunc!=NULL)
					EachChangeFunc(EachChangeFuncdata,this,isInnerChange);
//				PUI_DD[3]<<"TextEditBox "<<IDName<<" AddNewLine End"<<endl;
			}
			
			void _AddText(Point pt,const stringUTF8 &strUtf8,bool isInnerChange)
			{
//				PUI_DD[3]<<"TextEditBox "<<IDName<<" AddText Start"<<endl;
				if (strUtf8.length()==0||!EnableEdit&&isInnerChange) return;
				if (InRange(pt.y,0,Text.size()-1))
				{
					stringUTF8_WithData <EachChData> &str2=Text[pt.y];
					stringUTF8_WithData <EachChData> str3=str2.substrUTF8_WithData(pt.x,str2.length()-pt.x);
					SubtractTextLenCnt(GetStrLenOfEachWidth(str2));
					str2.erase(pt.x,str2.length()-pt.x);
					AddTextLenCnt(GetStrLenOfEachWidth(str2));
					
					int i=0,j=0;
					while (i<strUtf8.length())
					{
						if (strUtf8[i]=="\r")
						{
							if (i<strUtf8.length()-1&&strUtf8[i+1]=="\n")
								++i;
							if (j==0)
							{
								SubtractTextLenCnt(GetStrLenOfEachWidth(str2));
								str2.append(strUtf8.substrUTF8(j,i-j+1));
								AddTextLenCnt(GetStrLenOfEachWidth(str2));
								pt.y++;
								pt.x=0;
							}
							else
							{
								stringUTF8 substr=strUtf8.substrUTF8(j,i-j+1);
								Text.insert(pt.y++,substr);
								AddTextLenCnt(GetStrLenOfEachWidth(substr));
							}
							j=i+1;
						}
						else if (strUtf8[i]=="\n")
						{
							if (i<strUtf8.length()-1&&strUtf8[i+1]=="\r")
								++i;
							if (j==0)
							{
								SubtractTextLenCnt(GetStrLenOfEachWidth(str2));
								str2.append(strUtf8.substrUTF8(j,i-j+1));
								AddTextLenCnt(GetStrLenOfEachWidth(str2));
								pt.y++;
								pt.x=0;
							}
							else
							{
								stringUTF8 substr=strUtf8.substrUTF8(j,i-j+1);
								Text.insert(pt.y++,substr);
								AddTextLenCnt(GetStrLenOfEachWidth(substr));
							}
							j=i+1;
						}
						++i;
					}
					if (j<strUtf8.length())
						if (j==0)
						{
							SubtractTextLenCnt(GetStrLenOfEachWidth(str2));
							Pos=Pos_0={str2.length()+strUtf8.length(),pt.y};
							str2.append(strUtf8);
							str2.append(str3);
							AddTextLenCnt(GetStrLenOfEachWidth(str2));
						}
						else
						{
							stringUTF8_WithData <EachChData>  str4=strUtf8.substrUTF8(j,strUtf8.length()-j);
							str4.append(str3);
							Text.insert(pt.y,str4);
							AddTextLenCnt(GetStrLenOfEachWidth(str4));
							Pos=Pos_0={StrLenWithoutSlashNR(Text[pt.y]),pt.y};
							ResizeTextBoxHeight();
						}
					else
					{
						Text.insert(pt.y,str3);
						AddTextLenCnt(GetStrLenOfEachWidth(str3));
						Pos=Pos_0={StrLenWithoutSlashNR(Text[pt.y]),pt.y};
						ResizeTextBoxHeight();
					}
					ApplyTextLenChange();
					SetShowPosFromPos();
					Win->SetPresentArea(CoverLmt);
					if (EachChangeFunc!=NULL)
						EachChangeFunc(EachChangeFuncdata,this,isInnerChange);
				}
				else _AddNewLine(EnsureInRange(pt.y,0,Text.size()),strUtf8,isInnerChange);
//				PUI_DD[3]<<"TextEditBox "<<IDName<<" AddText End"<<endl;
			}
			
			void _SetText(const stringUTF8 &strUtf8,bool isInnerChange)
			{
				if (!EnableEdit&&isInnerChange) return;
				_Clear(isInnerChange);
				_AddText({0,0},strUtf8,isInnerChange);
			}
			
			void _AppendNewLine(const stringUTF8 &strUtf8,bool isInnerChange)
			{_AddNewLine(Text.size(),strUtf8,isInnerChange);}
			
			void _DeleteLine(int p,bool isInnerChange)
			{
//				PUI_DD[3]<<"TextEditBox "<<IDName<<" DeleteLine Start"<<endl;
				if (!EnableEdit&&isInnerChange) return;
				p=EnsureInRange(p,0,Text.size()-1);
				SubtractTextLenCnt(GetStrLenOfEachWidth(Text[p]));
				Text.erase(p);
				Pos=p==0?Point(0,0):Point(p-1,StrLenWithoutSlashNR(Text[p-1]));
				Pos=Pos_0=EnsurePosValid(Pos);
				ResizeTextBoxHeight();
				ApplyTextLenChange();
				SetShowPosFromPos();
				if (EachChangeFunc!=NULL)
					EachChangeFunc(EachChangeFuncdata,this,isInnerChange);
//				PUI_DD[3]<<"TextEditBox "<<IDName<<" DeleteLine End"<<endl;
			}
			
			void _DeleteText(Point pt1,Point pt2,bool isInnerChange)
			{
				if (pt1==pt2||!EnableEdit&&isInnerChange) return;
//				PUI_DD[3]<<"TextEditBox "<<IDName<<" DeleteText Start"<<endl;
				if (pt1.y==pt2.y)
				{
					if (pt1.x>pt2.x)
						swap(pt1,pt2);
					Pos=pt1;
					stringUTF8_WithData <EachChData>  &strUtf8=Text[pt1.y];
					SubtractTextLenCnt(GetStrLenOfEachWidth(strUtf8));
					strUtf8.erase(pt1.x,pt2.x-pt1.x);
					AddTextLenCnt(GetStrLenOfEachWidth(strUtf8));
				}
				else
				{
					if (pt1.y>pt2.y)
						swap(pt1,pt2);
					Pos=pt1;
					if (InRange(pt1.x,0,Text[pt1.y].length()-1))
					{
						stringUTF8_WithData <EachChData>  &strUtf8=Text[pt1.y];
						SubtractTextLenCnt(GetStrLenOfEachWidth(strUtf8));
						strUtf8.erase(pt1.x,strUtf8.length()-pt1.x);
						AddTextLenCnt(GetStrLenOfEachWidth(strUtf8));
					}
					if (InRange(pt2.x,1,StrLenWithoutSlashNR(Text[pt2.y])))
					{
						stringUTF8_WithData <EachChData> &strUtf8=Text[pt2.y];
						SubtractTextLenCnt(GetStrLenOfEachWidth(strUtf8));
						strUtf8.erase(0,pt2.x);
						AddTextLenCnt(GetStrLenOfEachWidth(strUtf8));
					}
					stringUTF8_WithData <EachChData> &strUtf8=Text[pt1.y];
					SubtractTextLenCnt(GetStrLenOfEachWidth(strUtf8));
					strUtf8.append(Text[pt2.y]);
					AddTextLenCnt(GetStrLenOfEachWidth(strUtf8));
					SubtractTextLenCnt(GetStrLenOfEachWidth(Text[pt2.y]));
					Text.erase(pt2.y);
					for (int i=pt2.y-1;i>=pt1.y+1;--i)//It is necessary use --i that the index in SplayTree would change after erase
					{
						SubtractTextLenCnt(GetStrLenOfEachWidth(Text[i]));
						Text.erase(i);
					}
					ResizeTextBoxHeight();
				}
				Pos=Pos_0=EnsurePosValid(Pos);
				ApplyTextLenChange();
				SetShowPosFromPos();
				Win->SetPresentArea(CoverLmt);//...
				if (EachChangeFunc!=NULL)
					EachChangeFunc(EachChangeFuncdata,this,isInnerChange);
//				PUI_DD[3]<<"TextEditBox "<<IDName<<" DeleteText End"<<endl;
			}
			
			void _DeleteTextBack(int len,bool isInnerChange)
			{
//				PUI_DD[3]<<"TextEditBox "<<IDName<<" DeleteTextBack Start"<<endl;
				if (!EnableEdit&&isInnerChange) return;
				Point pt=Pos;
				if (len>Pos.x)
				{
					len-=Pos.x+1;
					pt.y--;
					if (pt.y<0)
						pt={0,0};
					else
					{
						while (len>StrLenWithoutSlashNR(Text[pt.y])&&pt.y>=1)
							len-=StrLenWithoutSlashNR(Text[pt.y--])+1;
						pt.x=max(0,StrLenWithoutSlashNR(Text[pt.y])-len);
					}
				}
				else pt.x-=len;
				_DeleteText(EnsurePosValid(pt),Pos,isInnerChange);
//				PUI_DD[3]<<"TextEditBox "<<IDName<<" DeleteTextBack End"<<endl;
			}
			
			void _DeleteTextCursor(bool isInnerChange)
			{
//				PUI_DD[3]<<"TextEditBox "<<IDName<<" DeleteTextCursor Start"<<endl;
				if (Pos==Pos_0)
					_DeleteTextBack(1,isInnerChange);
				else _DeleteText(Pos,Pos_0,isInnerChange);
//				PUI_DD[3]<<"TextEditBox "<<IDName<<" DeleteTextCursor End"<<endl;
			}
			
			void _AddTextCursor(const stringUTF8 &strUtf8,bool isInnerChange)
			{
//				PUI_DD[3]<<"TextEditBox "<<IDName<<" AddTextCursor Start"<<endl;
				if (!(Pos==Pos_0))
					_DeleteTextCursor(isInnerChange);
				_AddText(Pos,strUtf8,isInnerChange);
//				PUI_DD[3]<<"TextEditBox "<<IDName<<" AddTextCursor End"<<endl;
			}
		
		public:
			inline void Clear()
			{_Clear(0);}
			
			inline void AddNewLine(int p,const stringUTF8 &strUtf8)
			{_AddNewLine(p,strUtf8,0);}
			
			inline void AddText(const Point &pt,const stringUTF8 &strUtf8)
			{_AddText(pt,strUtf8,0);}
			
			inline void SetText(const stringUTF8 &strUtf8)
			{_SetText(strUtf8,0);}
			
			inline void AppendNewLine(const stringUTF8 &strUtf8)
			{_AppendNewLine(strUtf8,0);}
			
			inline void DeleteLine(int p)
			{_DeleteLine(p,0);}
			
			inline void DeleteText(const Point &pt1,const Point &pt2)
			{_DeleteText(pt1,pt2,0);}
			
			inline void DeleteTextBack(int len=1)
			{_DeleteTextBack(len,0);}
			
			inline void DeleteTextCursor()
			{_DeleteTextCursor(0);}
			
			inline void AddTextCursor(const stringUTF8 &strUtf8)
			{_AddTextCursor(strUtf8,0);}
			
			void MoveCursorPos(int delta)//?? 
			{
				if (Text.size()==0) return;
//				PUI_DD[3]<<"TextEditBox "<<IDName<<" MoveCursorPos Start "<<delta<<endl;
				if (delta>0)
					if (delta>StrLenWithoutSlashNR(Text[Pos.y])-Pos.x)
					{
						delta-=StrLenWithoutSlashNR(Text[Pos.y])-Pos.x+1;
						Pos.y++;
						if (Pos.y>=Text.size())
							Pos={StrLenWithoutSlashNR(Text[Text.size()-1]),Text.size()-1};
						else
						{
							while (delta>StrLenWithoutSlashNR(Text[Pos.y])&&Pos.y<Text.size()-1)
								delta-=StrLenWithoutSlashNR(Text[Pos.y++])+1;
							Pos.x=min(delta,StrLenWithoutSlashNR(Text[Pos.y]));
						}
					}
					else Pos.x+=delta;
				else
					if ((delta=-delta)>Pos.x)
					{
						delta-=Pos.x+1;
						Pos.y--;
						if (Pos.y<0)
							Pos={0,0};
						else
						{
							while (delta>StrLenWithoutSlashNR(Text[Pos.y])&&Pos.y>0)
								delta-=StrLenWithoutSlashNR(Text[Pos.y--])+1;
							Pos.x=max(0,StrLenWithoutSlashNR(Text[Pos.y])-delta);
						}
					}
					else Pos.x-=delta;
				Pos_0=Pos;
				SetShowPosFromPos();
				Win->SetPresentArea(CoverLmt);
//				PUI_DD[3]<<"TextEditBox "<<IDName<<" MoveCursorPos End "<<delta<<endl;
			}
			
			void MoveCursorPosUpDown(int downDelta)
			{
				downDelta=EnsureInRange(downDelta,-Pos.y,Text.size()-1-Pos.y);
				if (downDelta==0) return;
				int w=GetTextLength(Pos.y,Pos.x);
				Pos.y+=downDelta;
				Pos.x=0;
				stringUTF8_WithData <EachChData> &strUtf8=Text[Pos.y];
				int strlen=StrLenWithoutSlashNR(strUtf8);
				for (int i=0;i<strlen&&w>0;Pos.x=++i)
					w-=GetChWidth(strUtf8[i]);
				Pos_0=Pos;
				SetShowPosFromPos();
				Win->SetPresentArea(CoverLmt);
			}
			
			void SetCursorPos(const Point &pt)
			{
				Pos=Pos_0=EnsurePosValid(pt);
				SetShowPosFromPos();
				Win->SetPresentArea(CoverLmt);//...
			}
			
			void SetCursorPos(const Point &pt1,const Point &pt2)
			{
				Pos=pt1;
				Pos_0=pt2;
				SetShowPosFromPos();
				Win->SetPresentArea(CoverLmt);
			}
			
			stringUTF8 GetSelectedTextUTF8()
			{
				stringUTF8 re;
				if (Pos==Pos_0)
					return re;
				Point pt1=Pos,pt2=Pos_0;
				if (pt1.y>pt2.y||pt1.y==pt2.y&&pt1.x>pt2.x)
					swap(pt1,pt2);
				if (pt1.y==pt2.y)
					re.append(Text[pt1.y].substrUTF8(pt1.x,pt2.x-pt1.x));
				else
				{
					re.append(Text[pt1.y].substrUTF8(pt1.x,Text[pt1.y].length()-pt1.x));
					for (int i=pt1.y+1;i<=pt2.y-1;++i)
						re.append(Text[i].StringUTF8());
					re.append(Text[pt2.y].substrUTF8(0,pt2.x));
				}
				return re;	
			}
			
			string GetSelectedText()
			{return GetSelectedTextUTF8().cppString();}
			
			stringUTF8 GetLineUTF8(int p)
			{return Text[EnsureInRange(p,0,Text.size()-1)].StringUTF8();}
			
			string GetLine(int p)
			{return Text[EnsureInRange(p,0,Text.size()-1)].cppString();}
			
			stringUTF8 GetAllTextUTF8()
			{
				stringUTF8 re;
				for (int i=0;i<Text.size();++i)
					re.append(Text[i].StringUTF8());
				return re;
			}
			
			string GetAllText()
			{return GetAllTextUTF8().cppString();}
			
			void SetEachCharSize(int w,int h)
			{
				EachChWidth=w;
				EachLineHeight=h;
				fa2->ResizeLL(MaxTextLength*EachChWidth+BorderWidth*2,EachLineHeight*Text.size()+2*BorderWidth);
				rPS.w=max(fa2->GetrPS().w,MaxTextLength*EachChWidth+BorderWidth*2);
				rPS.h=max(fa2->GetrPS().h,EachLineHeight*Text.size()+2*BorderWidth);
				SetShowPosFromPos();
			}
			
			inline void SetFontSize(int size)
			{
				if (font.Size==size) return;
				font.Size=size;
				++NewestCode;
				Win->SetPresentArea(CoverLmt);
			}
			
			inline void SetFont(const PUI_Font &tar)
			{
				font=tar;
				++NewestCode;
				Win->SetPresentArea(CoverLmt);
			}
			
			inline int GetLinesCount()
			{return Text.size();}
			
			inline Point GetCursorPos()
			{return Pos;}
			
		protected:
			virtual void ReceiveKeyboardInput(const PUI_TextEvent *event)
			{
				switch (event->type)
				{
					case PUI_Event::Event_TextInputEvent:
					{
						if (!event->text.empty())
						{
							if (!(EditingPosL==EditingPosR)) 
							{
								Point oldPos=EditingPosL;//It's very ugly =_=|| 
								_DeleteText(EditingPosL,EditingPosR,1);
								SetCursorPos(oldPos);
								EditingPosL=EditingPosR=ZERO_POINT;
							}
							_AddTextCursor(stringUTF8(event->text),1);
						}
						Win->StopSolveEvent();
						break;
					}
					case PUI_Event::Event_TextEditEvent:
					{
//						PUI_DD[0]<<"TextEditBox: "<<IDName<<" Editing text: start "<<event.edit.start<<", len "<<event.edit.length<<endl;
						editingText=event->text;
						EditingTextCursorPos=event->cursor;
						Win->StopSolveEvent();
						
						Editing=editingText.length()!=0;
						if (Editing)
						{
							if (!(Pos==Pos_0))
								_DeleteTextCursor(1);
							if (!(EditingPosL==EditingPosR))
								_DeleteText(EditingPosL,EditingPosR,1);
							EditingPosL=Pos;
							_AddTextCursor(editingText,1);
							EditingPosR=Pos;
							MoveCursorPos(EditingTextCursorPos-editingText.length());
						}
						else if (!(EditingPosL==EditingPosR)) 
						{
							Point oldPos=EditingPosL;//It's very ugly =_=|| 
							_DeleteText(EditingPosL,EditingPosR,1);
							SetCursorPos(oldPos);
							EditingPosL=EditingPosR=ZERO_POINT;
						}
						SetCurrentIMEWindowPos(LastCursorPos,Posize(gPS.x,LastCursorPos.y,gPS.w,EachLineHeight),Win);
						break;
					}
				}
				Win->SetPresentArea(CoverLmt);
			}
			
			virtual void CheckEvent(const PUI_Event *event)
			{
				switch (event->type)
				{
					case PUI_Event::Event_KeyEvent:
						if (StateInput)
						{
							PUI_KeyEvent *keyevent=event->KeyEvent();
							if ((keyevent->keyType==keyevent->Key_Down||keyevent->keyType==keyevent->Key_Hold)&&!Editing)
							{
								bool flag=1;
								switch (keyevent->keyCode)
								{
									case SDLK_BACKSPACE: 	_DeleteTextCursor(1);		break;
									case SDLK_LEFT:			MoveCursorPos(-1);			break;
									case SDLK_RIGHT: 		MoveCursorPos(1);			break;
									case SDLK_UP: 			MoveCursorPosUpDown(-1);	break;
									case SDLK_DOWN: 		MoveCursorPosUpDown(1);		break;
									case SDLK_PAGEUP: 		MoveCursorPosUpDown(-fa2->GetrPS().h/EachLineHeight);	break;
									case SDLK_PAGEDOWN:		MoveCursorPosUpDown(fa2->GetrPS().h/EachLineHeight);	break;
									case SDLK_TAB: 			_AddTextCursor("\t",1);		break;
									case SDLK_RETURN:
										if (Pos.x>=StrLenWithoutSlashNR(Text[Pos.y]))
											_AddNewLine(Pos.y+1,"\r\n",1);
										else 
										{
											Point pt=Pos;
											_AddNewLine(Pos.y+1,Text[Pos.y].substr(Pos.x,StrLenWithoutSlashNR(Text[Pos.y])-Pos.x),1);
											_DeleteText(pt,Point(StrLenWithoutSlashNR(Text[pt.y]),pt.y),1);
											Pos_0=Pos={0,pt.y+1};
											Win->SetPresentArea(CoverLmt);
										}
										break;
									case SDLK_v:
										if (keyevent->mod&KMOD_CTRL)
										{
											char *s=SDL_GetClipboardText();
											stringUTF8 str=s;
											SDL_free(s);
											if (str.empty())
												break;
											_AddTextCursor(str,1);
										}
										else flag=0;
										break;
									case SDLK_c:
										if (keyevent->mod&KMOD_CTRL)
											if (!(Pos==Pos_0))
												SDL_SetClipboardText(GetSelectedText().c_str());
										break;
									case SDLK_x:
										if (keyevent->mod&KMOD_CTRL)
										{
											if (!(Pos==Pos_0))
											{
												SDL_SetClipboardText(GetSelectedText().c_str());
												_DeleteTextCursor(1);
											}
										}
										else flag=0;
										break;
									case SDLK_z:
										if (keyevent->mod&KMOD_CTRL)
										{
											PUI_DD[2]<<"TextEditBox: "<<IDName<<" ctrl z cannot use yet"<<endl;
											//<<...
										}
										else flag=0;
										break;
									
									case SDLK_a:
										if (keyevent->mod&KMOD_CTRL)
											SetCursorPos({StrLenWithoutSlashNR(Text[Text.size()-1]),Text.size()-1},{0,0});
										else flag=0;
										break;
									
									case SDLK_ESCAPE:
										StateInput=0;
										TurnOnOffIMEWindow(0,Win);
										Editing=0;
										Win->SetPresentArea(CoverLmt);
										Win->SetKeyboardInputWg(NULL);
										editingText.clear();
										PUI_DD[0]<<"TextEditBox:"<<IDName<<" Stop input"<<endl;
										break;
									default:
										flag=0;
								}
								if (flag)
									Win->StopSolveEvent();
							}
							else Win->StopSolveEvent();
						}
						break;
					case PUI_Event::Event_WheelEvent:
						if (EnableScrollResize)
							if (SDL_GetModState()&KMOD_CTRL)//??
							{
								double lambda=1+event->WheelEvent()->dy*0.1;
								int h=EnsureInRange(EachLineHeight*lambda,10,160),w=h/2;
								SetEachCharSize(w,h);
								SetFontSize(h*0.75);
								Win->StopSolveEvent();
							}
						break;
				}
			}
		
			virtual void CheckPos(const PUI_PosEvent *event,int mode)
			{
				if ((mode&PosMode_LoseFocus)&&Win->GetOccupyPosWg()!=this)
				{
					if (!Win->IsPosFocused()||!CoverLmt.In(event->pos)||mode&PosMode_ForceLoseFocus)
						if (StateInput)
						{
							if (event->posType==event->Pos_Down)
								if (event->button==event->Button_MainClick)
								{
									stat=Stat_NoFocus;
									StateInput=0;
									TurnOnOffIMEWindow(0,Win);
									Editing=0;
									Win->SetPresentArea(CoverLmt);
									Win->SetKeyboardInputWg(NULL);
									editingText.clear();
									RemoveNeedLoseFocus();
									PUI_DD[0]<<"TextEditBox:"<<IDName<<" Stop input"<<endl;
								}
						}
						else if (stat!=Stat_NoFocus)
						{
							stat=Stat_NoFocus;
							Win->SetPresentArea(CoverLmt);
							RemoveNeedLoseFocus();
						}
				}
				else if (mode&PosMode_Default)
					switch (event->posType)
					{
						case PUI_PosEvent::Pos_Down:
							PUI_DD[0]<<"TextEditBox "<<IDName<<" click"<<endl;
							if (!Editing)
								if (event->button==event->Button_MainClick)
								{
									stat=Stat_DownLeft;
									Pos=Pos_0=GetPos(event->pos);
									SetShowPosFromPos();
									Win->SetOccupyPosWg(this);
									SetNeedLoseFocus();
									PUI_DD[0]<<"TextEditBox "<<IDName<<": Start  input"<<endl;
								}
								else if (event->button==event->Button_SubClick)
								{
									stat=Stat_DownRight;
									SetNeedLoseFocus();
								}
							Win->StopSolvePosEvent();
							Win->SetPresentArea(CoverLmt);
							break;
						case PUI_PosEvent::Pos_Up:
							if (stat==Stat_DownLeft||stat==Stat_DownRight)
							{
								PUI_DD[0]<<"TextEditBox "<<IDName<<": up"<<endl;
								if (event->type==event->Event_TouchEvent)
									if (event->button==PUI_PosEvent::Button_MainClick)
										stat=Stat_DownLeft;
									else if (event->button==PUI_PosEvent::Button_SubClick)
										stat=Stat_DownRight;
								if (Win->GetOccupyPosWg()==this)
									Win->SetOccupyPosWg(NULL);
								if (stat==Stat_DownRight)
									if (RightClickFunc!=NULL)
									{
										RightClickFunc(RightClickFuncData,GetPos(event->pos),this);
										PUI_DD[0]<<"TextEditBox "<<IDName<<": RightClickFunc"<<endl;
									}
								stat=Stat_UpFocus;
								StateInput=1;
								TurnOnOffIMEWindow(1,Win);
								SDL_Rect rct=PosizeToSDLRect(gPS);
								SDL_SetTextInputRect(&rct);
								Win->SetKeyboardInputWg(this);
								Win->StopSolvePosEvent();
								Win->SetPresentArea(CoverLmt);
							}
							break;
						case PUI_PosEvent::Pos_Motion:
							if (stat==Stat_DownLeft||stat==Stat_DownRight)
							{
								if (!Editing)
								{
									Pos=GetPos(event->pos);
									SetShowPosFromPos();
									Win->SetPresentArea(CoverLmt);
								}
							}
							else if (stat==Stat_NoFocus)
							{
								stat=Stat_UpFocus;
								SetNeedLoseFocus();
								Win->SetPresentArea(CoverLmt);
							}
							Win->StopSolvePosEvent();
							break;
					}
			}
			
			virtual void Show(Posize &lmt)
			{
//				Point EditingPosL,EditingPosR;
//				
//				if (StateInput&&Editing)
//				{
//					EditingPosL=Pos;
//					AddText(Pos,editingText);
//					EditingPosR=Pos;
//					MoveCursorPos(EditingTextCursorPos-editingText.length());
//				}
				
				Win->RenderFillRect(lmt&gPS,ThemeColor(BackgroundColor[0]));
				if (ShowBorder)
					Win->RenderDrawRectWithLimit(fa2->GetgPS(),ThemeColor(BorderColor[EnsureInRange((int)stat,0,2)]),lmt);
				
				int ForL=-fa->GetrPS().y/EachLineHeight,
					ForR=ForL+fa2->GetrPS().h/EachLineHeight+1;
				ForL=EnsureInRange(ForL,0,Text.size()-1);
				ForR=EnsureInRange(ForR,0,Text.size()-1);
				
				RGBA NowTextColor[2]={ThemeColor(TextColor[0]),ThemeColor(TextColor[1])};
				if (NowTextColor[0]!=LastTextColor[0]||NowTextColor[1]!=LastTextColor[1])
				{
					++NewestCode;
					LastTextColor[0]=NowTextColor[0];
					LastTextColor[1]=NowTextColor[1];
				}
				
				Posize CharPs=Posize(gPS.x+BorderWidth,gPS.y+BorderWidth+ForL*EachLineHeight,EachChWidth,EachLineHeight);
				for (int i=ForL,flag=0;i<=ForR;++i)
				{
					stringUTF8_WithData <EachChData> &strUtf8=Text[i];
					for (int j=0;j<strUtf8.length();++j)
					{
						string str=strUtf8[j];
						CharPs.w=GetChWidth(str);
						if ((CharPs&lmt).Size()>0)
						{
							if (Editing)
							{
								if (Pos==Point(j,i)&&StateInput)
									Win->RenderFillRect(Posize(CharPs.x-1,CharPs.y,2,CharPs.h)&lmt,ThemeColor(BackgroundColor[1+EnsureInRange((int)stat,0,2)]));
								flag=InSelectedRange({j,i},EditingPosL,EditingPosR);
							}
							else if (Pos==Pos_0)
							{
								if (Pos==Point(j,i)&&StateInput)
									Win->RenderFillRect(Posize(LastCursorPos.x=CharPs.x-1,LastCursorPos.y=CharPs.y,2,CharPs.h)&lmt,ThemeColor(BackgroundColor[1+EnsureInRange((int)stat,0,2)]));
							}
							else flag=InSelectedRange({j,i},Pos,Pos_0);
							
							if (!Editing&&flag)
								Win->RenderFillRect(CharPs&lmt,ThemeColor(BackgroundColor[1+EnsureInRange((int)stat,0,2)]));
							
							SDL_Texture *tex=NULL;
							int chCol=flag?(Editing?2:1):0;
							if (chCol==2)
								Win->RenderDrawText(str,CharPs,lmt,0,ThemeColor(TextColor[2]),font);
							else
							{
								RGBA &co=NowTextColor[chCol];
								if (str=="\r")
									if (ShowSpecialChar)
									{
										if (timeCodeOfASCIItex['\r'][chCol]!=NewestCode||!ASCIItex['\r'][chCol])
										{
											ASCIItex['\r'][chCol]=SharedTexturePtr(Win->CreateRGBATextTexture("\\r",co,font));
											timeCodeOfASCIItex['\r'][chCol]=NewestCode;
										}
										tex=ASCIItex['\r'][chCol]();
									}
									else tex=NULL;
								else if (str=="\n")
									if (ShowSpecialChar)
									{
										if (timeCodeOfASCIItex['\n'][chCol]!=NewestCode||!ASCIItex['\n'][chCol])
										{
											ASCIItex['\n'][chCol]=SharedTexturePtr(Win->CreateRGBATextTexture("\\n",co,font));
											timeCodeOfASCIItex['\n'][chCol]=NewestCode;
										}
										tex=ASCIItex['\n'][chCol]();
									}
									else tex=NULL;
								else if (str=="\t")
									if (ShowSpecialChar)
									{
										if (timeCodeOfASCIItex['\t'][chCol]!=NewestCode||!ASCIItex['\t'][chCol])
										{
											ASCIItex['\t'][chCol]=SharedTexturePtr(Win->CreateRGBATextTexture("\\t",co,font));
											timeCodeOfASCIItex['\t'][chCol]=NewestCode;
										}
										tex=ASCIItex['\t'][chCol]();
									}
									else tex=NULL;
								else
								{
									if (strUtf8(j).timeCode[chCol]!=NewestCode||!strUtf8(j).tex[chCol])
										if (str.length()==1&&str[0]>0)
										{
											if (timeCodeOfASCIItex[str[0]][chCol]!=NewestCode||!ASCIItex[str[0]][chCol])
											{
												ASCIItex[str[0]][chCol]=SharedTexturePtr(Win->CreateRGBATextTexture(str.c_str(),co,font));
												timeCodeOfASCIItex[str[0]][chCol]=NewestCode;
											}
											strUtf8(j).tex[chCol]=ASCIItex[str[0]][chCol];
											strUtf8(j).timeCode[chCol]=NewestCode;
										}
										else
										{
											strUtf8(j).tex[chCol]=SharedTexturePtr(Win->CreateRGBATextTexture(str.c_str(),co,font));
											strUtf8(j).timeCode[chCol]=NewestCode;
										}
									tex=strUtf8(j).tex[chCol]();
								}
								if (tex!=NULL)
									Win->RenderCopyWithLmt_Centre(tex,CharPs,lmt);
							}
						}
						CharPs.x+=CharPs.w;
					}
					
					if (Editing)
					{
						if (Pos==Point(strUtf8.length(),i))
							Win->RenderFillRect(Posize(CharPs.x-1,CharPs.y,2,CharPs.h)&lmt,ThemeColor(BackgroundColor[1+EnsureInRange((int)stat,0,2)]));
					}
					else if (Pos==Pos_0)
						if (Pos==Point(strUtf8.length(),i))
							Win->RenderFillRect(Posize(CharPs.x-1,CharPs.y,2,CharPs.h)&lmt,ThemeColor(BackgroundColor[1+EnsureInRange((int)stat,0,2)]));
					CharPs.x=gPS.x+BorderWidth;
					CharPs.y+=EachLineHeight;
				}
				
//				if (StateInput&&Editing)
//				{
//					MoveCursorPos(editingText.length()-EditingTextCursorPos+editingText.length());
//					DeleteTextBack(editingText.length());
//				}

				Win->Debug_DisplayBorder(gPS);
			}
		
		public:
			inline void SetTextColor(int p,const RGBA &co)
			{
				if (InRange(p,0,2))
				{
					TextColor[p]=!co?(p==2?ThemeColorM[6]:ThemeColorMT[p]):co;
					Win->SetPresentArea(CoverLmt);
				}
				else PUI_DD[1]<<"TextEditBox "<<IDName<<": SetTextColor p "<<p<<" is not in Range[0,2]"<<endl;
			}
			
			inline void SetBackgroundColor(int p,const RGBA &co)
			{
				if (InRange(p,0,3))
				{
					BackgroundColor[p]=!co?(p==0?ThemeColorBG[0]:ThemeColorM[p*2-1]):co;
					Win->SetPresentArea(CoverLmt);
				}
				else PUI_DD[1]<<"TextEditBox "<<IDName<<": SetBackgroundColor p "<<p<<" is not in Range[0,3]"<<endl;
			}
			
			inline void SetBorderColor(int p,const RGBA &co)
			{
				if (InRange(p,0,2))
				{
					BorderColor[p]=!co?ThemeColorM[p*2+1]:co;
					Win->SetPresentArea(CoverLmt);
				}
				else PUI_DD[1]<<"TextEditBox "<<IDName<<": SetBorderColor p "<<p<<" is not in Range[0,2]"<<endl;
			}
			
			inline void EnableShowBorder(bool en)
			{
				if (ShowBorder==en) return;
				ShowBorder=en;
				Win->SetPresentArea(CoverLmt);
			}
			
			void SetBorderWidth(int w)
			{
				BorderWidth=w;
				fa2->ResizeLL(MaxTextLength*EachChWidth+BorderWidth*2,EachLineHeight*Text.size()+2*BorderWidth);
				rPS.w=max(fa2->GetrPS().w,MaxTextLength*EachChWidth+BorderWidth*2);
				rPS.h=max(fa2->GetrPS().h,EachLineHeight*Text.size()+2*BorderWidth);
				SetShowPosFromPos();
			}
			
			inline bool GetShowSpecialChar() const
			{return ShowSpecialChar;}
			
			inline void SetShowSpecialChar(bool en)
			{
				if (en==ShowSpecialChar) return;
				ShowSpecialChar=en;
				Win->SetPresentArea(CoverLmt);
			}
			
			inline void SetEachChangeFunc(void (*_eachchangefunc)(T&,TextEditBox*,bool))
			{EachChangeFunc=_eachchangefunc;}
			
			inline void SetEachChangeFuncData(const T &eachchangefuncdata)
			{EachChangeFuncdata=eachchangefuncdata;}
			
			inline void SetRightClickFunc(void (*_rightClickFunc)(T&,const Point&,TextEditBox*))
			{RightClickFunc=_rightClickFunc;}
			
			inline void SetRightClickFuncData(const T &rightclickfuncdata)
			{RightClickFuncData=rightclickfuncdata;}
			
			inline void SetEnableScrollResize(bool enable)
			{EnableScrollResize=enable;}
			
			inline void SetEnableEdit(bool enable)
			{EnableEdit=enable;}
			
			void AddPsEx(PosizeEX *psex)
			{fa2->AddPsEx(psex);}
			
			virtual void SetrPS(const Posize &ps)
			{fa2->SetrPS(ps);}
			
			inline LargeLayerWithScrollBar* GetFa2()
			{return fa2;}
			
			TextEditBox(const WidgetID &_ID,Widgets *_fa,PosizeEX *psex)
			:Widgets(_ID,WidgetType_TextEditBox)
			{
				fa2=new LargeLayerWithScrollBar(0,_fa,psex,ZERO_POSIZE);
				SetFa(fa2->LargeArea());
				rPS=ZERO_POSIZE;
				TextLengthCount.resize(10,0);
				memset(timeCodeOfASCIItex,0,sizeof timeCodeOfASCIItex);
				_Clear(1);
				InitDefualtColor();
				font.Size=18;//??
			}
			
			TextEditBox(const WidgetID &_ID,Widgets *_fa,const Posize &_rps)
			:Widgets(_ID,WidgetType_TextEditBox)
			{
				fa2=new LargeLayerWithScrollBar(0,_fa,_rps,ZERO_POSIZE);
				SetFa(fa2->LargeArea());
				rPS={0,0,_rps.w,_rps.h};
				TextLengthCount.resize(10,0);
				memset(timeCodeOfASCIItex,0,sizeof timeCodeOfASCIItex);
				_Clear(1);
				InitDefualtColor();
				font.Size=18;
			}
	};
	
	class TextBox:public Widgets
	{
		protected:
			string text;
			int ExtendMode=1;//0: Not change 1:Show previous most(cannot show part show...) 2:Show all as possible(auto change font size) 3:InLargeLayerMode(auto change LargeLayer size)
			PUI_Font userfont,font;
			RGBA TextColor=ThemeColorMT[0];
			PosizeEX_InLarge *psexInLarge=NULL;
			
			virtual void Show(Posize &lmt)
			{
				
			}
			
			virtual void CalcPsEx()
			{
				
			}
			
		public:

	};
	
	class TerminalTextBox:public Widgets
	{
		protected:
			
			
		public:
			
	};
	
	class LayerForListViewTemplate:public Widgets
	{
		template <class T> friend class ListViewTemplate;
		protected:
			Widgets *ActualFather=NULL;
			
			virtual void CalcPsEx()
			{
				Posize lastPs=gPS;
				gPS=rPS;
				CoverLmt=gPS&ActualFather->GetCoverLmt();
				if (!(lastPs==gPS))
					Win->SetPresentArea(lastPs|gPS);
			}
		
			LayerForListViewTemplate(const WidgetID &_ID,Widgets *actFa)
			:Widgets(_ID,WidgetType_LayerForListViewTemplate),ActualFather(actFa)
			{
				Win=actFa->GetWin();
				Depth=actFa->GetDepth()+1;
			}
	};
	
	template <class T> class ListViewTemplate:public Widgets
	{
		public:
			struct EachRowColor
			{
				RGBA co[3];
				
				RGBA& operator [] (int p)
				{return co[p];}
				
				EachRowColor(const RGBA &co0,const RGBA &co1,const RGBA &co2)
				{co[0]=co0;co[1]=co1;co[2]=co2;} 
				
				EachRowColor()
				{co[0]=co[1]=co[2]=RGBA_NONE;}
			};
		
		protected:
			enum
			{
				Stat_NoFocus=0,
				Stat_UpFocusRow=1,
				Stat_DownClickTwice=2,
				Stat_DownClickRight=3,
				Stat_DownClickOnce=4 
			}stat=Stat_NoFocus;
			int	FocusChoose=-1,
				ClickChoose=-1;
			int ListCnt=0;
			vector <LayerForListViewTemplate*> LineLayer;
			vector <T> FuncData;
			void (*func)(T&,int,int)=NULL;//int1:Pos(CountFrom 0)   int2: 0:None 1:Left_Click 2:Left_Double_Click 3:Right_Click
			T BackgroundFuncData;
			PosizeEX_InLarge *psexInLarge=NULL;
			int EachHeight=30,
				Interval=2;
			bool EnableKeyboardEvent=0,
				 EnablePosEvent=1;
			vector <EachRowColor> RowColor;
			EachRowColor DefaultColor;
			
			inline Posize GetLinePosize(int pos)
			{
				if (pos==-1) return ZERO_POSIZE;
				else return Posize(gPS.x,gPS.y+pos*(EachHeight+Interval),gPS.w,EachHeight);
			}
			
			inline int GetLineFromPos(int y)
			{
				int re=(Win->NowPos().y-gPS.y)/(EachHeight+Interval);
				if ((y-gPS.y)%(EachHeight+Interval)<EachHeight&&InRange(re,0,ListCnt-1))
					return re;
				else return -1;
			}
				 
			virtual void CheckEvent(const PUI_Event *event)
			{
				if (EnableKeyboardEvent)
				{
					
				}
			}

			virtual void CheckPos(const PUI_PosEvent *event,int mode)
			{
				if (!Win->IsNeedSolvePosEvent()||!EnablePosEvent) return;
				if (mode&PosMode_LoseFocus)
				{
					if (stat!=Stat_NoFocus)
						if (!Win->IsPosFocused()||!CoverLmt.In(event->pos)||mode&PosMode_ForceLoseFocus)
						{
							stat=Stat_NoFocus;
							if (FocusChoose!=-1)
							{
								Win->SetPresentArea(GetLinePosize(FocusChoose)&CoverLmt);
								FocusChoose=-1;
							}
							RemoveNeedLoseFocus();
						}
				}
				else if (mode&PosMode_Default)
					switch (event->posType)
					{
						case PUI_PosEvent::Pos_Down:
							if (ClickChoose!=-1)
								Win->SetPresentArea(GetLinePosize(ClickChoose)&CoverLmt);
							if (FocusChoose!=-1)
								Win->SetPresentArea(GetLinePosize(FocusChoose)&CoverLmt);
							ClickChoose=FocusChoose=GetLineFromPos(event->pos.y);
							if (event->button==PUI_PosEvent::Button_MainClick)
								if (event->clicks==2)
									stat=Stat_DownClickTwice;
								else stat=Stat_DownClickOnce;
							else if (event->button==PUI_PosEvent::Button_SubClick)
								stat=Stat_DownClickRight;
							else stat=Stat_DownClickOnce,PUI_DD[1]<<"ListViewTemplate "<<IDName<<": Unknown click button,use it as left click once"<<endl;
							Win->StopSolvePosEvent();
							if (ClickChoose!=-1)
								Win->SetPresentArea(GetLinePosize(ClickChoose)&CoverLmt);
							if (FocusChoose!=-1)
								Win->SetPresentArea(GetLinePosize(FocusChoose)&CoverLmt);
							break;
						case PUI_PosEvent::Pos_Up:
							if (stat!=Stat_NoFocus&&stat!=Stat_UpFocusRow)
							{
								PUI_DD[0]<<"ListViewTemplate "<<IDName<<" func "<<ClickChoose<<endl;
								if (event->type==event->Event_TouchEvent)//??
									if (event->button==PUI_PosEvent::Button_MainClick)
										if (event->clicks==2)
											stat=Stat_DownClickTwice;
										else stat=Stat_DownClickOnce;
									else if (event->button==PUI_PosEvent::Button_SubClick)
										stat=Stat_DownClickRight;
								if (func!=NULL)
									func(ClickChoose!=-1?FuncData[ClickChoose]:BackgroundFuncData,ClickChoose,stat==4?1:stat);
								stat=Stat_UpFocusRow;
								Win->StopSolvePosEvent();
							}
							break;
						case PUI_PosEvent::Pos_Motion:
							if (stat==Stat_NoFocus)
							{
								stat=Stat_UpFocusRow;
								SetNeedLoseFocus();
							}
							int p=GetLineFromPos(event->pos.y);
							if (InRange(p,0,ListCnt-1))
							{
								if (p!=FocusChoose)
								{
									if (FocusChoose!=-1)
										Win->SetPresentArea(GetLinePosize(FocusChoose)&CoverLmt);
									Win->SetPresentArea(GetLinePosize(p)&CoverLmt);
									FocusChoose=p;
								}
							}
							else if (FocusChoose!=-1)
							{
								Win->SetPresentArea(GetLinePosize(FocusChoose));
								FocusChoose=-1;
							}	
							Win->StopSolvePosEvent();
							break;
					}
			}

			virtual void Show(Posize &lmt)
			{
				if (ListCnt==0) return;
				Range ForLR=GetShowRange(lmt);
				Posize RowPs=Posize(gPS.x,gPS.y+ForLR.l*(EachHeight+Interval),gPS.w,EachHeight);
				for (int i=ForLR.l;i<=ForLR.r;RowPs.y+=EachHeight+Interval,++i)
					Win->RenderFillRect(RowPs&lmt,ThemeColor(!RowColor[i][ClickChoose==i?2:FocusChoose==i]?DefaultColor[ClickChoose==i?2:FocusChoose==i]:RowColor[i][ClickChoose==i?2:FocusChoose==i])),
					Win->Debug_DisplayBorder(RowPs);
				Win->Debug_DisplayBorder(gPS);
			}

			virtual void _SolveEvent(const PUI_Event *event)
			{
				if (!Win->IsNeedSolveEvent())
					return;
				if (nxtBrother!=NULL)
					SolveEventOf(nxtBrother,event);
				if (!Enabled) return;
				if (childWg!=NULL)
					SolveEventOf(childWg,event);
				if (ListCnt>0)
				{
					Range ForLR=GetShowRange();
					for (int i=ForLR.l;i<=ForLR.r;++i)
						SolveEventOf(LineLayer[i],event);
				}
				if (Win->IsNeedSolveEvent())
					CheckEvent(event);
			}
			
			virtual void _SolvePosEvent(const PUI_PosEvent *event,int mode)
			{
				if (!Win->IsNeedSolvePosEvent())
					return;
				if (Enabled)
					if (gPS.In(event->pos))
					{
						if (Win->IsNeedSolvePosEvent())
							CheckPos(event,mode|PosMode_Pre);
						if (childWg!=NULL)
							SolvePosEventOf(childWg,event,mode);
						if (ListCnt>0)
						{
							Range ForLR=GetShowRange();
							for (int i=ForLR.l;i<=ForLR.r;++i)
								SolvePosEventOf(LineLayer[i],event,mode);
						}
						if (Win->IsNeedSolvePosEvent())
							CheckPos(event,mode|PosMode_Nxt|PosMode_Default);
					}
				if (nxtBrother!=NULL)
					SolvePosEventOf(nxtBrother,event,mode);
			}
			
			virtual void _PresentWidgets(Posize lmt)
			{
				if (nxtBrother!=NULL)
					PresentWidgetsOf(nxtBrother,lmt);
				if (Enabled)
				{
					lmt=lmt&gPS;
					Show(lmt);
					if (DEBUG_EnableWidgetsShowInTurn)
						SDL_Delay(DEBUG_WidgetsShowInTurnDelayTime),
						SDL_RenderPresent(Win->GetSDLRenderer());
					if (ListCnt>0)
					{
						Range ForLR=GetShowRange();
						for (int i=ForLR.l;i<=ForLR.r;++i)
							PresentWidgetsOf(LineLayer[i],lmt);
					}
					if (childWg!=NULL)
						PresentWidgetsOf(childWg,lmt);
				}
			}

			virtual void _UpdateWidgetsPosize()
			{
				if (nxtBrother!=NULL)
					UpdateWidgetsPosizeOf(nxtBrother);
				if (!Enabled) return;
				CalcPsEx();
				if (childWg!=NULL)
					UpdateWidgetsPosizeOf(childWg);
				if (ListCnt>0)
				{
					Range ForLR=GetShowRange();
					Posize RowPs=Posize(gPS.x,gPS.y+ForLR.l*(EachHeight+Interval),gPS.w,EachHeight);
					for (int i=ForLR.l;i<=ForLR.r;RowPs.y+=EachHeight+Interval,++i)
					{
						LineLayer[i]->SetrPS(RowPs);
						UpdateWidgetsPosizeOf(LineLayer[i]);
					}
				}
			}
			
			virtual void CalcPsEx()
			{
				if (PsEx!=NULL)
					PsEx->GetrPS(rPS);
				rPS.h=max(0,ListCnt*(EachHeight+Interval)-Interval)+1;
				if (psexInLarge->NeedFullFill())
					rPS.h=max(rPS.h,psexInLarge->GetLargeLayer()->GetrPS().h-rPS.y-max(0,-psexInLarge->Fullfill()));
				Posize lastPs=CoverLmt;
				if (fa!=NULL)
					gPS=rPS+fa->GetgPS();
				else gPS=rPS;
				CoverLmt=gPS&GetFaCoverLmt();
				if (!(lastPs==CoverLmt))
					Win->SetPresentArea(lastPs|CoverLmt);
			}
			
		public:
			inline void SetRowColor(int pos,int p,const RGBA &co)
			{
				if (InRange(pos,0,ListCnt-1)&&InRange(p,0,2))
				{
					RowColor[pos][p]=co;
					Win->SetPresentArea(GetLinePosize(pos)&CoverLmt);
				}
				else PUI_DD[1]<<"ListViewTemplate "<<IDName<<": SetRowColor: pos or p is not in range"<<endl;
			}
			
			inline void SetDefaultRowColor(int p,const RGBA &co)
			{
				if (InRange(p,0,2))
				{
					DefaultColor[p]=!co?ThemeColorM[p*2]:co;
					Win->SetPresentArea(CoverLmt);
				}
				else PUI_DD[1]<<"ListViewTemplate "<<IDName<<": SetDefaultRowColor: p "<<p<<" is not in Range[0,2]"<<endl;
			}
			
			inline void SetRowHeightAndInterval(int _height,int _interval)
			{
				EachHeight=_height;
				Interval=_interval;
				rPS.h=max(0,ListCnt*(EachHeight+Interval)-Interval);
				Win->SetNeedUpdatePosize();
			}
			
			inline void SetListFunc(void (*_func)(T&,int,int))
			{func=_func;}
			
			void SetBackgroundFuncData(const T &data)
			{BackgroundFuncData=data;}
			
			Widgets* SetListContent(int p,const T &_funcdata,const EachRowColor &co)
			{
				p=EnsureInRange(p,0,ListCnt);
				FuncData.insert(FuncData.begin()+p,_funcdata);
				RowColor.insert(RowColor.begin()+p,co);
				LayerForListViewTemplate *re=new LayerForListViewTemplate(0,this);
				LineLayer.insert(LineLayer.begin()+p,re);
				++ListCnt;
				if (ClickChoose>=p) ++ClickChoose;
				rPS.h=max(0,ListCnt*(EachHeight+Interval)-Interval);
				Win->SetNeedUpdatePosize();
				Win->SetPresentArea(CoverLmt);//?? It don't need fresh such big area
				return re;
			}
			
			Widgets* SetListContent(int p,const T &_funcdata)
			{return SetListContent(p,_funcdata,EachRowColor());}
			
			void DeleteListContent(int p)
			{
				if (!ListCnt) return;
				p=EnsureInRange(p,0,ListCnt-1);
				FuncData.erase(FuncData.begin()+p);
				RowColor.erase(RowColor.begin()+p);
				delete LineLayer[p];
				LineLayer.erase(LineLayer.begin()+p);
				--ListCnt;
				if (FocusChoose==ListCnt)
					FocusChoose=-1;
				if (ClickChoose>p) --ClickChoose;
				else if (ClickChoose==p) ClickChoose=-1;
				rPS.h=max(0,ListCnt*(EachHeight+Interval)-Interval);
				Win->SetNeedUpdatePosize();
				Win->SetPresentArea(CoverLmt);
			}
			
			void ClearListContent()
			{
				if (!ListCnt) return;
				FocusChoose=ClickChoose=-1;
				FuncData.clear();
				RowColor.clear();
				for (auto vp:LineLayer)
					delete vp;
				LineLayer.clear();
				ListCnt=0;
				rPS.h=0;
				psexInLarge->SetShowPos(rPS.y);
				Win->SetNeedUpdatePosize();
				Win->SetPresentArea(CoverLmt);
			}
			
			Widgets* PushbackContent(const T &_funcdata,const EachRowColor &co)
			{return SetListContent(1e9,_funcdata,co);}
			
			Widgets* PushbackContent(const T &_funcdata)
			{return SetListContent(1e9,_funcdata,EachRowColor());}
			
			void SwapContent(int pos1,int pos2)
			{
				if (!InRange(pos1,0,ListCnt-1)||!InRange(pos2,0,ListCnt-1)||pos1==pos2) return;
				swap(LineLayer[pos1],LineLayer[pos2]);
				swap(FuncData[pos1],FuncData[pos2]);
				swap(RowColor[pos1],RowColor[pos2]);
				Win->SetPresentArea(GetLinePosize(pos1)&CoverLmt);
				Win->SetPresentArea(GetLinePosize(pos2)&CoverLmt);
			}
			
			int Find(const T &_funcdata)
			{
				for (int i=0;i<ListCnt;++i)
					if (FuncData[i]==_funcdata)
						return i;
				return -1;
			}
			
			void SetSelectLine(int p,bool TriggerFunc=0)
			{
				p=EnsureInRange(p,-1,ListCnt-1);
				if (p==ClickChoose&&!TriggerFunc==0) return;
				if (p==-1)
					Win->SetPresentArea(GetLinePosize(ClickChoose)&CoverLmt);
				else if (!GetShowRange(CoverLmt).InRange(p))
					psexInLarge->SetShowPos(p*(EachHeight+Interval)-rPS.y);
				ClickChoose=p;
				Win->SetPresentArea(GetLinePosize(ClickChoose)&CoverLmt);
				if (TriggerFunc&&func!=NULL)
					func(p==-1?BackgroundFuncData:FuncData[p],p,1);
			}
			
			inline int GetCurrentSelectLine()
			{return ClickChoose;}
			
			inline int GetListCnt()
			{return ListCnt;}
			
			Range GetShowRange(Posize ps)
			{
				Range re;
				ps=ps&gPS;
				re.l=EnsureInRange((ps.y-gPS.y)/(EachHeight+Interval),0,ListCnt-1);
				re.r=EnsureInRange((ps.y2()-gPS.y+EachHeight+Interval-1)/(EachHeight+Interval),0,ListCnt-1);
				return re;
			}
			
			Range GetShowRange()//??
			{return GetShowRange(psexInLarge->GetLargeLayer()->GetgPS());}
			
			T& GetFuncData(int p)
			{
				p=EnsureInRange(p,0,ListCnt-1);
				return FuncData[p];
			}
			
			Widgets* GetLineLayer(int p)
			{
				if (ListCnt==0) 
					return NULL;
				p=EnsureInRange(p,0,ListCnt-1);
				return LineLayer[p];
			}
			
			LargeLayerWithScrollBar* GetLargeLayer()
			{return psexInLarge->GetLargeLayer();}
			
			void ResetLargeLayer(PosizeEX_InLarge *largePsex)
			{
				SetFa(largePsex->GetLargeLayer()->LargeArea());
				ReAddPsEx(largePsex);
				psexInLarge=largePsex;
			}
			
			inline void SetEnablePosEvent(bool enable)
			{EnablePosEvent=enable;}
			
			~ListViewTemplate()
			{ClearListContent();}
			
			ListViewTemplate(const WidgetID &_ID,PosizeEX_InLarge *largePsex,void (*_func)(T&,int,int)=NULL)
			:Widgets(_ID,WidgetType_ListViewTemplate),func(_func),psexInLarge(largePsex)
			{
				SetFa(largePsex->GetLargeLayer()->LargeArea());
				AddPsEx(largePsex);
				for (int i=0;i<=2;++i)
					DefaultColor.co[i]=ThemeColorM[i*2];
			}
			
			ListViewTemplate(const WidgetID &_ID,Widgets *_fa,const Posize &_rps,void (*_func)(T&,int,int)=NULL)
			:Widgets(_ID,WidgetType_ListViewTemplate),func(_func)
			{
				LargeLayerWithScrollBar *larlay=new LargeLayerWithScrollBar(0,_fa,_rps);
				SetFa(larlay->LargeArea());
				AddPsEx(psexInLarge=new PosizeEX_InLarge(larlay,0));
				for (int i=0;i<=2;++i)
					DefaultColor.co[i]=ThemeColorM[i*2];
			}
			
			ListViewTemplate(const WidgetID &_ID,Widgets *_fa,PosizeEX *psex,void (*_func)(T&,int,int)=NULL)
			:Widgets(_ID,WidgetType_ListViewTemplate),func(_func)
			{
				LargeLayerWithScrollBar *larlay=new LargeLayerWithScrollBar(0,_fa,psex);
				SetFa(larlay->LargeArea());
				AddPsEx(psexInLarge=new PosizeEX_InLarge(larlay,0));
				for (int i=0;i<=2;++i)
					DefaultColor.co[i]=ThemeColorM[i*2];
			}
	};
	
	class LayerForBlockViewTemplate:public Widgets
	{
		template <class T> friend class BlockViewTemplate;
		protected:
			Widgets *ActualFather=NULL;
			
			virtual void CalcPsEx()
			{
				Posize lastPs=gPS;
				gPS=rPS;
				CoverLmt=gPS&ActualFather->GetCoverLmt();
				if (!(lastPs==gPS))
					Win->SetPresentArea(lastPs|gPS);
			}
		
			LayerForBlockViewTemplate(const WidgetID &_ID,Widgets *actFa)
			:Widgets(_ID,WidgetType_LayerForBlockViewTemplate),ActualFather(actFa)
			{
				Win=actFa->GetWin();
				Depth=actFa->GetDepth()+1;
			}
	};
	
	template <class T> class BlockViewTemplate:public Widgets
	{
		public:
			struct BlockViewData
			{
				friend class BlockViewTemplate;
				protected:
					LayerForBlockViewTemplate *lay=NULL;
					bool OnOff=0;
					RGBA co[3];
					
				public:
					T FuncData;
					
					inline LayerForBlockViewTemplate* operator () ()
					{return lay;}
					
					inline RGBA operator [] (int p)
					{return co[p];}
					
					BlockViewData(const T &funcdata,const RGBA &co0,const RGBA &co1,const RGBA &co2)
					:FuncData(funcdata)
					{
						co[0]=co0;
						co[1]=co1;
						co[2]=co2;
					}
					
					BlockViewData(const T &funcdata):FuncData(funcdata)
					{co[0]=co[1]=co[2]=RGBA_NONE;}
					
					BlockViewData()
					{co[0]=co[1]=co[2]=RGBA_NONE;}
			};
		
		protected:
			enum
			{
				Stat_NoFocus=0,
				Stat_UpFocusRow=1,
				Stat_DownClickTwice=2,
				Stat_DownClickRight=3,
				Stat_DownClickOnce=4
			}stat=Stat_NoFocus;
			vector <BlockViewData> BlockData;
			int BlockCnt=0;
			void (*func)(T&,int,int)=NULL;//int1:Pos(CountFrom 0)   int2: 0:None 1:Left_Click 2:Left_Double_Click 3:Right_Click
			void (*onoffFunc)(T&,int,bool)=NULL;
			T BackgroundFuncData;
			PosizeEX_InLarge *psexInLarge=NULL;
			int	FocusChoose=-1,
				ClickChoose=-1,
				EachLineBlocks=1;//update this after changing rPS or EachPs
			Posize EachPs={5,5,240,80};//min(w,h) is the pic edge length, if w>=h ,the text will be show on right,else on buttom
			bool EnableKeyboardEvent=0,
				 EnablePosEvent=1;
			RGBA DefaultBlockColor[3];//NoFocusRow,FocusBlock,ClickBlock
			
			void InitDefaultBlockColor()
			{
				for (int i=0;i<=2;++i)
					DefaultBlockColor[i]=ThemeColorM[i*2];
			}
			
			Posize GetBlockPosize(int p)//get specific Block Posize
			{
				if (p==-1) return ZERO_POSIZE;
				return Posize(gPS.x+p%EachLineBlocks*(EachPs.x+EachPs.w),gPS.y+p/EachLineBlocks*(EachPs.y+EachPs.h),EachPs.w,EachPs.h);
			}

			int InBlockPosize(const Point &pt)//return In which Posize ,if not exist,return -1
			{
				int re=(pt.y-gPS.y+EachPs.y)/(EachPs.y+EachPs.h)*EachLineBlocks+(pt.x-gPS.x+EachPs.x)/(EachPs.x+EachPs.w);
				if (InRange(re,0,BlockCnt-1)&&(GetBlockPosize(re)&CoverLmt).In(pt))
					return re;
				else return -1;
			}

			virtual void CheckEvent(const PUI_Event *event)
			{
				//<<Keyevent control...
			}
			
			virtual void CheckPos(const PUI_PosEvent *event,int mode)
			{
				if (!Win->IsNeedSolvePosEvent()||!EnablePosEvent) return;
				if (mode&PosMode_LoseFocus)
				{
					if (stat!=Stat_NoFocus)
						if (!Win->IsPosFocused()||!CoverLmt.In(event->pos)||mode&PosMode_ForceLoseFocus)
						{
							stat=Stat_NoFocus;
							if (FocusChoose!=-1)
							{
								Win->SetPresentArea(GetBlockPosize(FocusChoose)&CoverLmt);
								FocusChoose=-1;
							}
							RemoveNeedLoseFocus();
						}
				}
				else if (mode&PosMode_Default)
					switch (event->posType)
					{
						case PUI_PosEvent::Pos_Down:
							Win->SetPresentArea(GetBlockPosize(ClickChoose)&CoverLmt);
							Win->SetPresentArea(GetBlockPosize(FocusChoose)&CoverLmt);
							ClickChoose=FocusChoose=InBlockPosize(event->pos);
							Win->SetPresentArea(GetBlockPosize(ClickChoose)&CoverLmt);
							Win->SetPresentArea(GetBlockPosize(FocusChoose)&CoverLmt);
							if (event->button==PUI_PosEvent::Button_MainClick)
								if (event->clicks==2)
									stat=Stat_DownClickTwice;
								else stat=Stat_DownClickOnce;
							else if (event->button==PUI_PosEvent::Button_SubClick)
								stat=Stat_DownClickRight;
							else stat=Stat_DownClickOnce,PUI_DD[1]<<"BlockViewTemplate "<<IDName<<": Unknown click button,use it as left click once"<<endl;
							Win->StopSolvePosEvent();
							break;
						case PUI_PosEvent::Pos_Up:
							if (stat!=Stat_NoFocus&&stat!=Stat_UpFocusRow)
							{
								PUI_DD[0]<<"BlockViewTemplate function "<<ClickChoose<<endl;
								if (event->type==event->Event_TouchEvent)//??
									if (event->button==PUI_PosEvent::Button_MainClick)
										if (event->clicks==2)
											stat=Stat_DownClickTwice;
										else stat=Stat_DownClickOnce;
									else if (event->button==PUI_PosEvent::Button_SubClick)
										stat=Stat_DownClickRight;
								if (func!=NULL)
									func(ClickChoose==-1?BackgroundFuncData:BlockData[ClickChoose].FuncData,ClickChoose,stat==4?1:stat);
								stat=Stat_UpFocusRow;
								Win->StopSolvePosEvent();
							}
							break;
						case PUI_PosEvent::Pos_Motion:
							if (stat==Stat_NoFocus)
							{
								stat=Stat_UpFocusRow;
								SetNeedLoseFocus();
								FocusChoose=InBlockPosize(event->pos);
								Win->SetPresentArea(GetBlockPosize(FocusChoose)&CoverLmt);
							}
							else
							{
								int pos=InBlockPosize(event->pos);
								if (pos!=FocusChoose)
								{
									Win->SetPresentArea(GetBlockPosize(FocusChoose)&CoverLmt);
									Win->SetPresentArea(GetBlockPosize(pos)&CoverLmt);
									FocusChoose=pos;
								}	
							}
							Win->StopSolvePosEvent();
							break;
					}
			}
			
			virtual void Show(Posize &lmt)
			{
				if (BlockCnt==0) return;
				Range ForLR=GetShowRange(lmt);
				Posize BlockPs=GetBlockPosize(ForLR.l);
				for (int i=ForLR.l;i<=ForLR.r;++i)
				{
					int p=ClickChoose==i?2:FocusChoose==i;
					Win->RenderFillRect(BlockPs&lmt,ThemeColor(!BlockData[i].co[p]?DefaultBlockColor[p]:BlockData[i].co[p]));
 					Win->Debug_DisplayBorder(BlockPs);
					
					if ((i+1)%EachLineBlocks==0)
						BlockPs.y+=EachPs.y+EachPs.h,BlockPs.x=gPS.x;
					else BlockPs.x+=EachPs.x+EachPs.w;
				}
				Win->Debug_DisplayBorder(gPS);
			}
			
			virtual void CalcPsEx()
			{
				if (PsEx!=NULL)
					PsEx->GetrPS(rPS);
				EachLineBlocks=max(1,(rPS.w+EachPs.x)/(EachPs.x+EachPs.w));
				rPS.h=ceil(BlockCnt*1.0/EachLineBlocks)*(EachPs.y+EachPs.h)-EachPs.y+1;
				if (psexInLarge->NeedFullFill())
					rPS.h=max(rPS.h,psexInLarge->GetLargeLayer()->GetrPS().h-rPS.y-max(0,-psexInLarge->Fullfill()));//??
				Posize lastPs=CoverLmt;
				if (fa!=NULL)
					gPS=rPS+fa->GetgPS();
				else gPS=rPS;
				CoverLmt=gPS&GetFaCoverLmt();
				if (!(lastPs==CoverLmt))
					Win->SetPresentArea(lastPs|CoverLmt);
			}

			virtual void _SolveEvent(const PUI_Event *event)
			{
				if (!Win->IsNeedSolveEvent())
					return;
				if (nxtBrother!=NULL)
					SolveEventOf(nxtBrother,event);
				if (!Enabled) return;
				if (childWg!=NULL)
					SolveEventOf(childWg,event);
				if (BlockCnt>0)
				{
					Range ForLR=GetShowRange();
					for (int i=ForLR.l;i<=ForLR.r;++i)
						SolveEventOf(BlockData[i].lay,event);
				}
				if (Win->IsNeedSolveEvent())
					CheckEvent(event);
			}
			
			virtual void _SolvePosEvent(const PUI_PosEvent *event,int mode)
			{
				if (!Win->IsNeedSolvePosEvent())
					return;
				if (Enabled)
					if (gPS.In(event->pos))
					{
						if (Win->IsNeedSolvePosEvent())
							CheckPos(event,mode|PosMode_Pre);
						if (childWg!=NULL)
							SolvePosEventOf(childWg,event,mode);
						if (BlockCnt>0)
						{
							Range ForLR=GetShowRange();
							for (int i=ForLR.l;i<=ForLR.r;++i)
								SolvePosEventOf(BlockData[i].lay,event,mode);
						}
						if (Win->IsNeedSolvePosEvent())
							CheckPos(event,mode|PosMode_Nxt|PosMode_Default);
					}
				if (nxtBrother!=NULL)
					SolvePosEventOf(nxtBrother,event,mode);
			}
			
			virtual void _PresentWidgets(Posize lmt)
			{
				if (nxtBrother!=NULL)
					PresentWidgetsOf(nxtBrother,lmt);
				if (Enabled)
				{
					lmt=lmt&gPS;
					Show(lmt);
					if (DEBUG_EnableWidgetsShowInTurn)
						SDL_Delay(DEBUG_WidgetsShowInTurnDelayTime),
						SDL_RenderPresent(Win->GetSDLRenderer());
					if (BlockCnt>0)
					{
						Range ForLR=GetShowRange();
						for (int i=ForLR.l;i<=ForLR.r;++i)
							PresentWidgetsOf(BlockData[i].lay,lmt);
					}
					if (childWg!=NULL)
						PresentWidgetsOf(childWg,lmt);
				}
			}
			
			virtual void _UpdateWidgetsPosize()
			{
				if (nxtBrother!=NULL)
					UpdateWidgetsPosizeOf(nxtBrother);
				if (!Enabled) return;
				CalcPsEx();
				if (childWg!=NULL)
					UpdateWidgetsPosizeOf(childWg);
				if (BlockCnt>0)
				{
					Range ForLR=GetShowRange();
					Posize BlockPs=GetBlockPosize(ForLR.l);
					for (int i=ForLR.l;i<=ForLR.r;++i)
					{
						BlockData[i].lay->SetrPS(BlockPs);
						UpdateWidgetsPosizeOf(BlockData[i].lay);
						if ((i+1)%EachLineBlocks==0)
							BlockPs.y+=EachPs.y+EachPs.h,BlockPs.x=gPS.x;
						else BlockPs.x+=EachPs.x+EachPs.w;
					}
				}
			}
			
		public:
			inline void SetBlockColor(int pos,int p,const RGBA &co)
			{
				if (InRange(pos,0,BlockCnt-1)&&InRange(p,0,2))
				{
					BlockData[pos].co[p]=co;
					Win->SetPresentArea(GetBlockPosize(pos)&CoverLmt);
				}
				else PUI_DD[1]<<"BlockViewTemplate "<<IDName<<": SetBlockColor: pos or p is not in range"<<endl;
			}
			
			inline void SetDefaultBlockColor(int p,const RGBA &co)
			{
				if (InRange(p,0,2))
				{
					DefaultBlockColor[p]=!co?ThemeColorM[p*2]:co;
					Win->SetPresentArea(CoverLmt);
				}
				else PUI_DD[1]<<"BlockViewTemplate "<<IDName<<": SetDefaultBlockColor: p "<<p<<" is not in range[0,2]"<<endl;
			}			
			
			inline void SetEachBlockPosize(const Posize &ps)
			{
				EachPs=ps;
				Win->SetNeedUpdatePosize();
			}

			inline void SetBlockFunc(void (*_func)(T&,int,int))
			{func=_func;}
			
			void SetBackgroundFuncData(const T &data)
			{BackgroundFuncData=data;}
			
			Widgets* SetBlockContent(int p,const BlockViewData &data)
			{
				p=EnsureInRange(p,0,BlockCnt);
				BlockData.insert(BlockData.begin()+p,data);
				BlockData[p].lay=new LayerForBlockViewTemplate(0,this);
				++BlockCnt;
				if (ClickChoose>=p) ++ClickChoose;
				if (BlockCnt%EachLineBlocks==1)
				{
					rPS.h=ceil(BlockCnt*1.0/EachLineBlocks)*(EachPs.y+EachPs.h)-EachPs.y+1;
					Win->SetNeedUpdatePosize();
				}
				Win->SetPresentArea(CoverLmt);
				return BlockData[p].lay;
			}
			
			Widgets* SetBlockContent(int p,const T &funcdata)
			{return SetBlockContent(p,BlockViewData(funcdata));}
			
			Widgets* SetBlockContent(int p,const T &funcdata,const RGBA &co0,const RGBA &co1,const RGBA &co2)
			{return SetBlockContent(p,BlockViewData(funcdata,co0,co1,co2));}
			
			void DeleteBlockContent(int p)//p: 0<=p<ListCnt:SetInP >=ListCnt:DeleteLast <0:DeleteFirst
			{
				if (!BlockCnt) return;
				p=EnsureInRange(p,0,BlockCnt-1);
				delete BlockData[p].lay;
				BlockData.erase(BlockData.begin()+p);
				--BlockCnt;
				if (FocusChoose==BlockCnt) FocusChoose=-1;
				if (ClickChoose>p) --ClickChoose;
				else if (ClickChoose==p) ClickChoose=-1;
				if (BlockCnt%EachLineBlocks==0)
				{
					rPS.h=ceil(BlockCnt*1.0/EachLineBlocks)*(EachPs.y+EachPs.h)-EachPs.y+1;
					Win->SetNeedUpdatePosize();
				}
				Win->SetPresentArea(CoverLmt);
			}
			
			void ClearBlockContent()
			{
				if (!BlockCnt) return;
				PUI_DD[0]<<"Clear BlockViewTemplate "<<IDName<<" content: BlockCnt:"<<BlockCnt<<endl;
				FocusChoose=ClickChoose=-1;
				for (auto &vp:BlockData)
					delete vp.lay;
				BlockData.clear();
				BlockCnt=0;
				rPS.h=0;
				psexInLarge->SetShowPos(rPS.y);
				Win->SetNeedUpdatePosize();
				Win->SetPresentArea(CoverLmt);
			}
			
			Widgets* PushbackContent(const BlockViewData &data)
			{return SetBlockContent(1e9,data);}
			
			Widgets* PushbackContent(const T &funcdata)
			{return SetBlockContent(1e9,funcdata);}
			
			Widgets* PushbackContent(const T &funcdata,const RGBA &co0,const RGBA &co1,const RGBA &co2)
			{return SetBlockContent(funcdata,co0,co1,co2);}

			void SetSelectBlock(int p,bool TriggerFunc=0)
			{
				p=EnsureInRange(p,-1,BlockCnt-1);
				if (p==ClickChoose&&!TriggerFunc) return;
				Win->SetPresentArea(GetBlockPosize(ClickChoose)&CoverLmt);
				ClickChoose=p;
				if (!GetShowRange(psexInLarge->GetLargeLayer()->GetgPS()).InRange(p))
					psexInLarge->SetShowPos(p/EachLineBlocks*(EachPs.y+EachPs.h));
				Win->SetPresentArea(GetBlockPosize(ClickChoose)&CoverLmt);
				if (TriggerFunc&&func!=NULL)
					func(p==-1?BackgroundFuncData:BlockData[p].FuncData,p,1);
			}

			inline int GetCurrentSelectBlock()//-1 means none
			{return ClickChoose;}
			
			inline int GetBlockCnt()
			{return BlockCnt;}
			
			inline bool Empty()
			{return BlockCnt==0;}
			
			int Find(const T &_funcdata)
			{
				for (int i=0;i<BlockCnt;++i)
					if (BlockData[i].FuncData==_funcdata)
						return i;
				return -1;
			}
			
			inline BlockViewData& GetBlockData(int p)
			{return BlockData[EnsureInRange(p,0,BlockCnt-1)];}
			
			T& GetFuncData(int p)
			{
				if (p==-1) return BackgroundFuncData;
				else return BlockData[EnsureInRange(p,0,BlockCnt-1)].FuncData;
			}
			
			inline Widgets* GetBlockLayer(int p)
			{return BlockData[EnsureInRange(p,0,BlockCnt-1)].lay;}
			
			inline T& GetBackgroundFuncData()
			{return BackgroundFuncData;}
			
			inline void SetEnablePosEvent(bool enable)
			{EnablePosEvent=enable;}
			
			void SetUpdateBlock(int p)
			{
				if (!InRange(p,0,BlockCnt-1))
					return;
				Posize bloPs=GetBlockPosize(p)&CoverLmt;
				if (bloPs.Size()>0)
					Win->SetPresentArea(bloPs);
			}
			
			inline void SetEnableKeyboardEvent(bool en)
			{EnableKeyboardEvent=en;}

			Range GetShowRange(Posize ps)
			{
				ps=ps&gPS;
				Range re;
				re.l=EnsureInRange((ps.y-gPS.y)/(EachPs.y+EachPs.h)*EachLineBlocks,0,BlockCnt-1);
				re.r=EnsureInRange((ps.y2()-gPS.y+EachPs.y+EachPs.h-1)/(EachPs.y+EachPs.h)*EachLineBlocks-1,0,BlockCnt-1);
				return re;
			}

			inline Range GetShowRange()
			{return GetShowRange(CoverLmt);}
			
			inline int GetEachLineBlocks()
			{return EachLineBlocks;}
			
			LargeLayerWithScrollBar* GetLargeLayer()
			{return psexInLarge->GetLargeLayer();}
			
			void ResetLargeLayer(PosizeEX_InLarge *largePsex)
			{
				SetFa(largePsex->GetLargeLayer()->LargeArea());
				ReAddPsEx(largePsex);
				psexInLarge=largePsex;
			}
			
			~BlockViewTemplate()
			{ClearBlockContent();}
			
			BlockViewTemplate(const WidgetID &_ID,PosizeEX_InLarge *largePsex,void (*_func)(T&,int,int)=NULL)
			:Widgets(_ID,WidgetType_BlockViewTemplate),func(_func),psexInLarge(largePsex)
			{
				SetFa(largePsex->GetLargeLayer()->LargeArea());
				AddPsEx(largePsex);
				InitDefaultBlockColor();
			}
			
			BlockViewTemplate(const WidgetID &_ID,Widgets *_fa,const Posize &_rps,void (*_func)(T&,int,int)=NULL)
			:Widgets(_ID,WidgetType_BlockViewTemplate),func(_func)
			{
				LargeLayerWithScrollBar *larlay=new LargeLayerWithScrollBar(0,_fa,_rps);
				SetFa(larlay->LargeArea());
				AddPsEx(psexInLarge=new PosizeEX_InLarge(larlay,0));
				InitDefaultBlockColor();
			}
			
			BlockViewTemplate(const WidgetID &_ID,Widgets *_fa,PosizeEX *psex,void (*_func)(T&,int,int)=NULL)
			:Widgets(_ID,WidgetType_BlockViewTemplate),func(_func)
			{
				LargeLayerWithScrollBar *larlay=new LargeLayerWithScrollBar(0,_fa,psex);
				SetFa(larlay->LargeArea());
				AddPsEx(psexInLarge=new PosizeEX_InLarge(larlay,0));
				InitDefaultBlockColor();
			}
	};
	
	template <class T> class TreeViewTemplate:public Widgets
	{
		protected:
			
			
		public:
			
			
			
	};
	
	//Special:
	//..
	
	class MessageBox2:public Widgets
	{
		protected:
			
			
		public:
			
			
	};
	
	#if PAL_CurrentPlatform == PAL_Platform_Windows
	class SimpleFileSelectBox:public MessageBoxLayer
	{
		protected:
			AddressSection <string> *PathSection=NULL;
			SimpleListView_MultiColor <Triplet<SimpleFileSelectBox*,string,bool> > *FileList=NULL;
			Button <SimpleFileSelectBox*> *ButtonOK=NULL,
										  *ButtonBack=NULL,
										  *ButtonForward=NULL,
										  *ButtonUp=NULL;
			TinyText *SelectedPathText=NULL,
					 *TT_ListInfo=NULL;
			BorderRectLayer *TextBorderLayer=NULL;
			TextEditLine <SimpleFileSelectBox*> *TEL_SaveName=NULL,
												*TEL_SaveFormat=NULL;
			int (*func)(const string&)=NULL;//return 0:OK 1:Invalid target
			set <string> FormatFilter;//.XXX(specificFormatFile),dir(Directory),all(any),file(any file)
			stack <string> PathHistory,PopHistory;
			string CurrentPath,SelectedTarget;
			bool SaveMode=0;//Not usable yet...
			
			vector <Triplet<string,string,bool> > GetAllInPath(const string &path)
			{
				vector <Triplet<string,string,bool> > re;
				if (path=="")
				{
					vector <string> diskDrive=GetAllLogicalDrive();
					for (auto vp:diskDrive)
						if (GetDriveTypeA(vp.c_str())!=DRIVE_REMOTE)
							re.push_back({vp,GetLogicalDriveInfo(vp).name+"("+vp+")",0});
						else re.push_back({vp,vp,0});
				}
				else
				{
					auto *vec=GetAllFile_UTF8(path);
					sort(vec->begin(),vec->end(),[](const pair<string,int> &x,const pair <string,int> &y)->bool
					{
						if (x.second==y.second)
							return SortComp_WithNum(x.first,y.first);
						else return x.second<y.second;
					});
					for (auto vp:*vec)
						re.push_back({path+PLATFORM_PATHSEPERATOR+vp.first,vp.first,bool(vp.second)});
					delete vec;
				}
				return re;
			}
			
			bool ValidTarget(const string &path,bool isFile)
			{
				if (path=="") return 0;
				if (FormatFilter.find("all")!=FormatFilter.end())
					return 1;
				if (!isFile)
					return FormatFilter.find("dir")!=FormatFilter.end();
				else if (FormatFilter.find("file")!=FormatFilter.end()) return 1;
				else return FormatFilter.find(Atoa(GetAftername(path)))!=FormatFilter.end();
			}
			
			void SetCurrentPath(const string &path,int from)//0:common 1:back 2:forward 3:Init
			{
				if (path==CurrentPath&&from!=3) return;
				if (from==1)
				{
					PopHistory.push(CurrentPath);
					PathHistory.pop();
				}
				else if (from==2)
				{
					PathHistory.push(CurrentPath);
					PopHistory.pop();
				}
				else if (from!=3)
				{
					PathHistory.push(CurrentPath);
					PopHistory=stack <string> ();
				}
				CurrentPath=path;
				
				vector <Doublet<string,string> > addrVec;
				string pathstr=path;
				while (pathstr!="")
				{
					addrVec.push_back({GetLastAfterBackSlash(pathstr),pathstr});
					pathstr=GetPreviousBeforeBackSlash(pathstr);
				}
				PathSection->Clear();
				PathSection->PushbackSection(PUIT("´ËµçÄÔ"),"");
				for (int i=(int)addrVec.size()-1;i>=0;--i)
					PathSection->PushbackSection(addrVec[i].a,addrVec[i].b);
				
				TT_ListInfo->SetText("");
				using SLVType=SimpleListView_MultiColor <Triplet<SimpleFileSelectBox*,string,bool> >;
				FileList->ClearListContent();
				auto vec=GetAllInPath(CurrentPath);
				bool emptyFlag=1;
				for (auto vp:vec)
					if (vp.c==0)
						emptyFlag=0,FileList->PushbackContent(vp.b,{this,vp.a,vp.c},SLVType::EachRowColor0(RGBA(255,240,203,255),RGBA(255,223,182,255),RGBA(255,197,146,255)));
					else if (ValidTarget(vp.b,vp.c))
						emptyFlag=0,FileList->PushbackContent(vp.b,{this,vp.a,vp.c},SLVType::EachRowColor0(ThemeColorM[0],ThemeColorM[1],ThemeColorM[2]));
				if (emptyFlag)
					TT_ListInfo->SetText(PUIT("µ±Ç°ÎÄ¼þ¼ÐÎª¿Õ"));
			}
			
			void SetSelectTarget(const string &path)
			{
				SelectedTarget=path;
				SelectedPathText->SetText(PUIT("ÒÑÑ¡Ôñ£º ")+GetLastAfterBackSlash(path));
			}
			
			void NotSelectWarning(const string &info="")
			{
				using namespace Charset;
				SetSystemBeep(SetSystemBeep_Error);
				(new MessageBoxButton<int>(0,PUIT("´íÎó"),info==""?PUIT("Ñ¡Ôñ¸ÃÄ¿±êÊ§°Ü!"):info))->AddButton(PUIT("È·¶¨"),NULL,0);
			}
			
			int SetSelectOK()
			{
				if (func!=NULL)
					if (SelectedTarget=="")
						if (ValidTarget(CurrentPath,0))
							if (func(CurrentPath)!=0)
								return NotSelectWarning(),1;
							else 0;
						else return NotSelectWarning(PUIT("ÉÐÎ´Ñ¡ÔñÈÎºÎÄ¿±ê!")),1;
					else if (func(SelectedTarget)!=0)
						return NotSelectWarning(),1;
				DelayDelete();
				return 0;
			}
			
			static void PathSectionFunc(void *funcdata,AddressSection <string> *addrsec,string &path,int nowfocus)
			{
				auto *This=(SimpleFileSelectBox*)funcdata;
				if (nowfocus==addrsec->GetSectionCnt())
					return;
				if (nowfocus>=1)
					This->SetCurrentPath(path,0);
				else if (nowfocus<=-2)
				{
					auto vec=This->GetAllInPath(addrsec->GetSectionData(-nowfocus-2));
					int realcnt=0;
					for (auto vp:vec)
						if (!vp.c)
							++realcnt;
					PUI_Window *win=This->GetWin(); 
					Posize winps=win->GetWinPS();
					Posize ps(win->NowPos().x,win->NowPos().y,300,min3(800,realcnt*26+2,winps.h));
					ps=winps.ToOrigin().EnsureIn(ps);
					auto mbl=new MessageBoxLayer(0,"",ps,win);
					mbl->EnableShowTopAreaColor(0);
					mbl->EnableShowTopAreaX(0);
					mbl->SetClickOutsideReaction(1);
					auto slv=new SimpleListView <Triplet <string,MessageBoxLayer*,SimpleFileSelectBox*> > (0,mbl,new PosizeEX_Fa6(2,2,2,2,2,2),
						[](Triplet <string,MessageBoxLayer*,SimpleFileSelectBox*> &funcdata,int pos,int click)->void
						{
							if (pos==-1) return;
							funcdata.c->SetCurrentPath(funcdata.a,0);
							funcdata.b->Close();
						});
					for (auto vp:vec)
						if (!vp.c)
							slv->PushbackContent(vp.b,{vp.a,mbl,This});
				}
			}
			
			static void FileListFunc(Triplet<SimpleFileSelectBox*,string,bool> &funcdata,int pos,int click)
			{
				if (pos==-1) return;
				auto This=funcdata.a;
				if (click==1)
					if (This->ValidTarget(funcdata.b,funcdata.c))
						This->SetSelectTarget(funcdata.b);
					else 0;
				else if (click==2)
					if (funcdata.c==0) This->SetCurrentPath(funcdata.b,0);
					else if (This->ValidTarget(funcdata.b,funcdata.c))
					{
						This->SetSelectTarget(funcdata.b);
						This->SetSelectOK();
					}
					else 0;
				else if (click==3)
				{
					//...
				}
			}
			
			inline static void ButtonOKFunc(SimpleFileSelectBox *&This)
			{This->SetSelectOK();}
			
			static void ButtonBackFunc(SimpleFileSelectBox *&This)
			{
				if (!This->PathHistory.empty())
					This->SetCurrentPath(This->PathHistory.top(),1);
			}
			
			static void ButtonForwardFunc(SimpleFileSelectBox *&This)
			{
				if (!This->PopHistory.empty())
					This->SetCurrentPath(This->PopHistory.top(),2);
			}
			
			static void ButtonUpFunc(SimpleFileSelectBox *&This)
			{
				if (This->PathSection->GetSectionCnt()>=2)
					This->SetCurrentPath(This->PathSection->GetSectionData(This->PathSection->GetSectionCnt()-2),0);
			}
			
			void InitWidgetsGUI()
			{
				ButtonBack=new Button <SimpleFileSelectBox*> (0,this,new PosizeEX_Fa6(3,3,30,30,40,30),"<",ButtonBackFunc,this);
				ButtonForward=new Button <SimpleFileSelectBox*> (0,this,new PosizeEX_Fa6(3,3,60,30,40,30),">",ButtonForwardFunc,this);
				ButtonUp=new Button <SimpleFileSelectBox*> (0,this,new PosizeEX_Fa6(3,3,90,30,40,30),"^",ButtonUpFunc,this);
				PathSection=new AddressSection <string> (0,this,new PosizeEX_Fa6(2,3,130,30,40,30),PathSectionFunc,this);
				FileList=new SimpleListView_MultiColor <Triplet<SimpleFileSelectBox*,string,bool> > (0,this,new PosizeEX_Fa6(2,2,30,30,80,50),FileListFunc);
				ButtonOK=new Button <SimpleFileSelectBox*> (0,this,new PosizeEX_Fa6(1,1,60,30,30,10),PUIT("È·¶¨"),ButtonOKFunc,this);
				TextBorderLayer=new BorderRectLayer(0,this,new PosizeEX_Fa6(2,1,30,120,30,10));
				SelectedPathText=new TinyText(0,TextBorderLayer,new PosizeEX_Fa6(2,2,5,5,0,0),PUIT("ÉÐÎ´Ñ¡ÔñÎÄ¼þ"),-1);
				TT_ListInfo=new TinyText(0,this,new PosizeEX_Fa6(2,3,30,30,120,30),"");
				EnableShowTopAreaColor(1);
				SetEnableInterceptControlEvents(1);
			}
			
		public:
			
			virtual ~SimpleFileSelectBox()
			{
				
			}
			
			SimpleFileSelectBox(const WidgetID &_ID,int (*_func)(const string&),const string &basepath,set <string> formatFilter,const Posize &_ps0,const string &title="",PUI_Window *_win=CurrentWindow)
			:MessageBoxLayer(_ID,WidgetType_SimpleFileSelectBox,title==""?PUIT("Ñ¡ÔñÎÄ¼þ"):title,_ps0,_win),func(_func),FormatFilter(formatFilter)
			{
				MultiWidgets=1;
				InitWidgetsGUI();
				SetCurrentPath(basepath,3);
			}
			
			SimpleFileSelectBox(const WidgetID &_ID,int (*_func)(const string&),const string &basepath="",set <string> formatFilter=set <string> {"file"},int _w=600,int _h=400,const string &title="",PUI_Window *_win=CurrentWindow)
			:SimpleFileSelectBox(_ID,_func,basepath,formatFilter,Posize(_win->GetWinPS().w-_w>>1,_win->GetWinPS().h-_h>>1,_w,_h)) {}
	};
	#endif
	
	template <class T> class SimpleDateTimeBox:public MessageBoxLayer
	{
		protected:
			TextEditLine <SimpleDateTimeBox*> *Tels[5]{0,0,0,0,0};
			Button <SimpleDateTimeBox*> *SetButton=NULL;
			EventLayer <SimpleDateTimeBox*> *TabEnterEventLayer=NULL;
			int (*func)(T&,const PAL_TimeP&)=NULL;//return 0:OK 1:Invalid Date(Box not disappear)
			PAL_TimeP t;
			T funcdata;
			
			void InitWidgetsGUI()
			{
				using namespace Charset;
				TabEnterEventLayer=new EventLayer <SimpleDateTimeBox*> (0,this,
					[](SimpleDateTimeBox *&This,const PUI_Event *event)->int
					{
						PUI_KeyEvent *keyevent=NULL;
						if (event->type==PUI_Event::Event_KeyEvent&&(keyevent=event->KeyEvent())&&InThisSet(keyevent->keyType,PUI_KeyEvent::Key_Down,PUI_KeyEvent::Key_Hold))
							if (keyevent->keyCode==SDLK_TAB)
							{
								for (int i=0;i<=4;++i)
									if (This->Win->GetKeyboardInputWg()==This->Tels[i])
									{
										This->Tels[i]->SetCursorPos(1e9);
										This->Tels[i]->StopTextInput();
										if ((keyevent->mod&KMOD_CTRL)||(keyevent->mod&KMOD_SHIFT))
											This->Tels[(i-1+5)%5]->StartTextInput();
										else This->Tels[(i+1)%5]->StartTextInput();
										return 1;
									}
								if (This->Win->GetKeyboardInputWg()==NULL)
									This->Tels[0]->StartTextInput();
								return 1;
							}
							else if (keyevent->keyCode==SDLK_RETURN)
								return This->SetButton->TriggerFunc(),1;
						return 0;
					},this);
				
				Tels[0]=new TextEditLine <SimpleDateTimeBox*> (0,this,{30,40,40,30},NULL,this);
				new TinyText(0,this,{80,40,20,30},AnsiToUtf8("Äê"));
				Tels[1]=new TextEditLine <SimpleDateTimeBox*> (0,this,{110,40,40,30},NULL,this);
				new TinyText(0,this,{160,40,20,30},AnsiToUtf8("ÔÂ"));
				Tels[2]=new TextEditLine <SimpleDateTimeBox*> (0,this,{190,40,40,30},NULL,this);
				new TinyText(0,this,{240,40,20,30},AnsiToUtf8("ÈÕ"));
				Tels[3]=new TextEditLine <SimpleDateTimeBox*> (0,this,{30,80,40,30},NULL,this);
				new TinyText(0,this,{80,80,20,30},AnsiToUtf8("Ê±"));
				Tels[4]=new TextEditLine <SimpleDateTimeBox*> (0,this,{110,80,40,30},NULL,this);
				new TinyText(0,this,{160,80,20,30},AnsiToUtf8("·Ö"));
				if (t!=PAL_TimeP())
				{
					Tels[0]->SetText(llTOstr(t.year));
					Tels[1]->SetText(llTOstr(t.month));
					Tels[2]->SetText(llTOstr(t.day));
					Tels[3]->SetText(llTOstr(t.hour));
					Tels[4]->SetText(llTOstr(t.minute,2));
				}
				
				SetButton=new Button <SimpleDateTimeBox*> (0,this,{190,80,70,30},AnsiToUtf8("ÉèÖÃ"),
					[](SimpleDateTimeBox *&This)->void
					{
						int y=GetAndCheckStringNaturalNumber(DeleteSideBlank(This->Tels[0]->GetText())),
							mo=GetAndCheckStringNaturalNumber(DeleteSideBlank(This->Tels[1]->GetText())),
							d=GetAndCheckStringNaturalNumber(DeleteSideBlank(This->Tels[2]->GetText())),
							h=GetAndCheckStringNaturalNumber(DeleteSideBlank(This->Tels[3]->GetText())),
							mi=GetAndCheckStringNaturalNumber(DeleteSideBlank(This->Tels[4]->GetText()));
						if (InRange(y,1,9999)&&InRange(mo,1,12)&&InRange(d,1,GetMonthDaysCount(mo,IsLeapYear(y)))&&InRange(h,0,23)&&InRange(mi,0,59))
						{
							This->t=PAL_TimeP(y,mo,d,h,mi,0);
							if (This->func==NULL||This->func(This->funcdata,This->t)==0)
								This->Close();
						}
						else SetSystemBeep(SetSystemBeep_Error),(new MessageBoxButton<int>(0,AnsiToUtf8("´íÎó"),AnsiToUtf8("ÊäÈëÊý¾Ý´æÔÚ²»¹æ·¶Ïî£¡")))->AddButton(AnsiToUtf8("È·¶¨"),NULL,0);
					},this);
				
				EnableShowTopAreaColor(1);
				SetEnableInterceptControlEvents(1);
			}
			
		public:
			inline void SetFunc(int (*_func)(T&,const PAL_TimeP&),const T &_funcdata)
			{
				func=_func;
				funcdata=_funcdata;
			}
			
			virtual ~SimpleDateTimeBox()
			{
				
			}
			
			SimpleDateTimeBox(const WidgetID &_ID,int (*_func)(T&,const PAL_TimeP&),const T &_funcdata,const PAL_TimeP &initTime=PAL_TimeP(),const string &title="",const Point &pos=Point(-1,-1),PUI_Window *_win=CurrentWindow)
			:MessageBoxLayer(_ID,WidgetType_SimpleDateTimeBox,title==""?Charset::AnsiToUtf8("ÉèÖÃÈÕÆÚÊ±¼ä"):title,Posize(pos.x==-1?_win->GetWinPS().w-300>>1:pos.x,pos.y==-1?_win->GetWinPS().h-120>>1:pos.y,300,120),_win),func(_func),funcdata(_funcdata),t(initTime)
			{
				MultiWidgets=1;
				InitWidgetsGUI();
			}
	};
	
	//Template widgets of int
	using ButtonI					=Button						<int>;
	using CheckBoxI					=CheckBox					<int>;
	using SwitchButtonI				=SwitchButton				<int>;
	using SingleChoiceButtonI		=SingleChoiceButton			<int>;
	using Button2I					=Button2					<int,int>;
	using ShapedPictureButtonI		=ShapedPictureButton		<int>;
	using DrawerLayerI				=DrawerLayer				<int>;
	using EventLayerI				=EventLayer					<int>;
	using PosEventLayerI			=PosEventLayer				<int>;
	using ShowLayerI				=ShowLayer					<int>;
	using GestureSenseLayerI		=GestureSenseLayer			<int>;
	using TabLayerI					=TabLayer					<int>;
	using SliderI					=Slider						<int>;
	using FullFillSliderI			=FullFillSlider				<int>;
	using PictureBoxI				=PictureBox					<int>;
	using SimpleListViewI			=SimpleListView				<int>;
	using SimpleListView_MultiColorI=SimpleListView_MultiColor	<int>;
	using SimpleBlockViewI			=SimpleBlockView			<int>;
	using DetailedListViewI			=DetailedListView			<int>;
	using Menu1I					=Menu1						<int>;
	using MenuDataI					=MenuData					<int>;
	using MessageBoxButtonI			=MessageBoxButton			<int>;
	using DropDownButtonI			=DropDownButton				<int>;
	using AddressSectionI			=AddressSection				<int>;
	using TextEditLineI				=TextEditLine				<int>;
	using TextEditBoxI				=TextEditBox				<int>;
	using ListViewTemplateI			=ListViewTemplate			<int>;
	using BlockViewTemplateI		=BlockViewTemplate			<int>;
	using TreeViewTemplateI			=TreeViewTemplate			<int>;
	using SimpleDateTimeBoxI		=SimpleDateTimeBox			<int>;
	
	//Driver:
	//.. 
	
//	void PUI_UpdateWidgetsPosize(Widgets *wg)
//	{
//		if (wg==NULL) return;
//		UpdateWidgetsPosize(wg->nxtBrother);
//		if (!wg->Enabled) return;
//		wg->CalcPsEx();
//		UpdateWidgetsPosize(wg->childWg);
//	}
	
	void PUI_UpdateWidgetsPosize(PUI_Window *win=NULL)
	{
		if (win!=NULL)
		{
			CurrentWindow=win;
			if (!(win->_BackGroundLayer->gPS==win->GetWinSize()))
			{
				win->PresentLimit=win->GetWinSize();
				win->_BackGroundLayer->gPS=win->_BackGroundLayer->rPS=win->_BackGroundLayer->CoverLmt=win->GetWinSize();
				win->_MenuLayer->gPS=win->_MenuLayer->rPS=win->_MenuLayer->CoverLmt=win->GetWinSize();
			}
			if (win->_BackGroundLayer->childWg!=NULL)
				win->_BackGroundLayer->childWg->_UpdateWidgetsPosize();
			if (win->_MenuLayer->childWg!=NULL)
				win->_MenuLayer->childWg->_UpdateWidgetsPosize();
			win->NeedUpdatePosize=0;
			win->NeedFreshScreen=1;
		}
		else
			for (auto sp:PUI_Window::AllWindow)
				if (sp->NeedUpdatePosize)
					PUI_UpdateWidgetsPosize(sp);
	}
	
	bool PUI_PresentWidgets()
	{
		if (IsSynchronizeRefreshTimerOn()&&!PUI_Window::CurrentRefreshReady)
			return 0;
		bool re=0;
		for (auto sp:PUI_Window::AllWindow)
			if ((sp->NeedFreshScreen||PUI_Window::NeedFreshScreenAll)&&!sp->Hidden)
			{
				CurrentWindow=sp;
				if (PUI_Window::NeedFreshScreenAll)
					sp->PresentLimit=sp->GetWinSize();
				Posize lmt=sp->PresentLimit;
				if (CurrentWindow->NeedRefreshFullScreen||DEBUG_DisplayPresentLimitFlag)
					lmt=sp->GetWinSize();
				if (CurrentWindow->NeedRefreshFullScreen)
					sp->RenderClear();
				sp->_BackGroundLayer->_PresentWidgets(lmt);
				sp->_MenuLayer->_PresentWidgets(lmt);
				sp->NeedFreshScreen=0;
				if (DEBUG_DisplayPresentLimitFlag)
				{
					sp->DEBUG_DisplayPresentLimit();
					PUI_DD[3]<<"PresentLimit "<<sp->PresentLimit.x<<" "<<sp->PresentLimit.y<<" "<<sp->PresentLimit.w<<" "<<sp->PresentLimit.h<<endl;
				}
				SDL_RenderPresent(sp->ren);
				sp->PresentLimit=ZERO_POSIZE;
				re=1;
			}
		PUI_Window::NeedFreshScreenAll=0;
		PUI_Window::CurrentRefreshReady=0;
		return re;
	}

	
	enum
	{
		PUI_SolveEventSucceed=0,
		PUI_SolveEventUnknown=1,
		PUI_SolveEventNotSolved=2
	};
	
	int PUI_SolveEvent(PUI_Event *event)//0:Solve event successfully 1:Unknown event to solve 2:Known event but not solved
	{
		int re=PUI_SolveEventUnknown,posMode=0;
		PUI_Event::_NowSolvingEvent=event;
		switch (event->type)
		{
			case PUI_Event::Event_WindowEvent:
				CurrentWindow=event->WindowEvent()->win;//?? whether need to check win is not NULL?
				CurrentWindow->NeedSolveEvent=1;
				switch (event->WindowEvent()->eventType)
				{
					case PUI_WindowEvent::Window_Resized:
						PUI_DD[0]<<"Window "<<CurrentWindow->WindowTitle<<" Resize to "<<event->WindowEvent()->data1<<" "<<event->WindowEvent()->data2<<endl;
						CurrentWindow->WinPS.w=event->WindowEvent()->data1;
						CurrentWindow->WinPS.h=event->WindowEvent()->data2;
						CurrentWindow->SetPresentArea(CurrentWindow->GetWinSize());
						CurrentWindow->SetNeedUpdatePosize();
						CurrentWindow->NeedSolveEvent=0;
						re=PUI_SolveEventSucceed;
						break;
			        case PUI_WindowEvent::Window_Shown:
			            PUI_DD[0]<<"Window "<<CurrentWindow->WindowTitle<<" shown"<<endl;
			            CurrentWindow->SetPresentArea(CurrentWindow->GetWinSize());
			            break;
			        case PUI_WindowEvent::Window_Hidden:
			            PUI_DD[0]<<"Window "<<CurrentWindow->WindowTitle<<" hidden"<<endl;
			            break;
			        case PUI_WindowEvent::Window_Exposed:
			            PUI_DD[0]<<"Window "<<CurrentWindow->WindowTitle<<" exposed"<<endl;
			            CurrentWindow->SetPresentArea(CurrentWindow->GetWinSize());
						re=PUI_SolveEventSucceed;
			            break;
					case PUI_WindowEvent::Window_Moved:
			            PUI_DD[0]<<"Window "<<CurrentWindow->WindowTitle<<" moved to "<<event->WindowEvent()->data1<<","<<event->WindowEvent()->data2<<endl;
			            CurrentWindow->SetPresentArea(CurrentWindow->GetWinSize());
						CurrentWindow->WinPS.x=event->WindowEvent()->data1;
			            CurrentWindow->WinPS.y=event->WindowEvent()->data2;
			            re=PUI_SolveEventSucceed;
			            break;
			        case PUI_WindowEvent::Window_SizeChange:
			            PUI_DD[0]<<"Window "<<CurrentWindow->WindowTitle<<" size changed to "<<event->WindowEvent()->data1<<"x"<<event->WindowEvent()->data2<<endl;
			            break;
			        case PUI_WindowEvent::Window_Minimized:
			            PUI_DD[0]<<"Window "<<CurrentWindow->WindowTitle<<" minimized"<<endl;
			            break;
			        case PUI_WindowEvent::Window_Maximized:
			            PUI_DD[0]<<"Window "<<CurrentWindow->WindowTitle<<" maximized"<<endl;
			            break;
			        case PUI_WindowEvent::Window_Restored:
			            PUI_DD[0]<<"Window "<<CurrentWindow->WindowTitle<<" restored"<<endl;
			            CurrentWindow->SetPresentArea(CurrentWindow->GetWinSize());
			            break;
			        case PUI_WindowEvent::Window_Enter:
			            PUI_DD[0]<<"Mouse entered window "<<CurrentWindow->WindowTitle<<endl;
			            CurrentWindow->PosFocused=1;
						break;
			        case PUI_WindowEvent::Window_Leave:
			            PUI_DD[0]<<"Mouse left window "<<CurrentWindow->WindowTitle<<endl;
			            CurrentWindow->PosFocused=0;
			            if (CurrentWindow->ScalePercent==1)
							CurrentWindow->_NowPos=PUI_GetGlobalMousePoint()-CurrentWindow->WinPS.GetLU();
						else CurrentWindow->_NowPos=(PUI_GetGlobalMousePoint()-CurrentWindow->WinPS.GetLU())/CurrentWindow->ScalePercent;
						if (CurrentWindow->OccupyPosWg==NULL)
			            {
//							for (PUI_Window::LoseFocusLinkTable *p=CurrentWindow->LoseFocusWgHead;p;p=p->nxt)
//								if (p->wg!=NULL)
//									if (p->wg->Enabled)
//									{
//										PUI_PosEvent *eve=new PUI_PosEvent(PUI_Event::Event_VirtualPos,event->timeStamp,CurrentWindow->NowPos(),ZERO_POINT,CurrentWindow,PUI_PosEvent::Pos_Motion,PUI_PosEvent::Button_None,0);
//										p->wg->CheckPos(eve,Widgets::PosMode_LoseFocus|Widgets::PosMode_Direct);
//										delete eve;
//									}
//									else PUI_DD[2]<<"NeedLoseFocusWg "<<p->wg->IDName<<" is disabled!"<<endl;
//								else PUI_DD[2]<<"NeedLoseFocusWidgets is NULL"<<endl;
							for (set<Widgets*>::iterator sp=CurrentWindow->LoseFocusTargets.begin(),nsp;sp!=CurrentWindow->LoseFocusTargets.end();sp=nsp)
							{
								nsp=sp;
								++nsp;
								if ((*sp)!=nullptr)
									if ((*sp)->Enabled)
									{
										PUI_PosEvent *eve=new PUI_PosEvent(PUI_Event::Event_VirtualPos,event->timeStamp,CurrentWindow->NowPos(),ZERO_POINT,CurrentWindow,PUI_PosEvent::Pos_Motion,PUI_PosEvent::Button_None,0);
										(*sp)->CheckPos(eve,Widgets::PosMode_LoseFocus|Widgets::PosMode_Direct);
										delete eve;
									}
									else PUI_DD[2]<<"NeedLoseFocusWg "<<(*sp)->IDName<<" is disabled!"<<endl;
							}
						}
						else
						{
							//<<Start getting mouse global position timer or callback for OccupyPos widgets.
						}
			            break;
			        case PUI_WindowEvent::Window_GainFocus:
			            PUI_DD[0]<<"Window "<<CurrentWindow->WindowTitle<<" gained keyboard focus"<<endl;
			            CurrentWindow->KeyboardFocused=1;
			            if (CurrentWindow->IsTransparentBackground)
			            	CurrentWindow->SetBackgroundTransparentTimerOnOff(1);
			            break;
			        case PUI_WindowEvent::Window_LoseFocus:
			            PUI_DD[0]<<"Window "<<CurrentWindow->WindowTitle<<" lost keyboard focus"<<endl;
			            CurrentWindow->KeyboardFocused=0;
			            if (CurrentWindow->IsTransparentBackground)
			            	CurrentWindow->SetBackgroundTransparentTimerOnOff(0);
			            break;
			        case PUI_WindowEvent::Window_Close:
			            PUI_DD[0]<<"Window "<<CurrentWindow->WindowTitle<<" closed"<<endl;
			            if (CurrentWindow->CloseFunc!=NULL)
			           		CurrentWindow->CloseFunc->CallFunc(0);//??
			           	re=PUI_SolveEventSucceed;
			            break;
			        case PUI_WindowEvent::Window_TakeFocus:
			            PUI_DD[0]<<"Window "<<CurrentWindow->WindowTitle<<" is offered a focus"<<endl;
			            break;
			        case PUI_WindowEvent::Window_HitTest:
			            PUI_DD[0]<<"Window "<<CurrentWindow->WindowTitle<<" has a special hit test"<<endl;
			            break;
			        default:
			            PUI_DD[2]<<"Window "<<CurrentWindow->WindowTitle<<" got unknown event "<<event->WindowEvent()->eventType<<endl;
			            break;
				}
				CurrentWindow->NeedSolveEvent=0;
				break;
			
			case PUI_Event::Event_WindowTBEvent:
				CurrentWindow=event->WindowTBEvent()->win;
				CurrentWindow->TransparentBackgroundPic=SharedTexturePtr(CurrentWindow->CreateTextureFromSurfaceAndDelete(event->WindowTBEvent()->sur));
				CurrentWindow->TransparentBackgroundPicPS=GetTexturePosize(CurrentWindow->TransparentBackgroundPic());
				CurrentWindow->SetPresentArea(event->WindowTBEvent()->updatedPs);
				break;
			
			case PUI_Event::Event_TextInputEvent:
			case PUI_Event::Event_TextEditEvent:
				CurrentWindow=event->TextEvent()->win;
				CurrentWindow->NeedSolveEvent=1;
				if (CurrentWindow->KeyboardInputWg!=NULL)
					CurrentWindow->KeyboardInputWg->ReceiveKeyboardInput(event->TextEvent());
				if (CurrentWindow->NeedSolveEvent)
				{
					CurrentWindow->_MenuLayer->_SolveEvent(event);
					CurrentWindow->_BackGroundLayer->_SolveEvent(event);
				}
				re=CurrentWindow->NeedSolveEvent?PUI_SolveEventNotSolved:PUI_SolveEventSucceed;
				CurrentWindow->NeedSolveEvent=0;
				break;
			
			case PUI_Event::Event_MouseEvent:
			case PUI_Event::Event_TouchEvent:
			case PUI_Event::Event_PenEvent:
			case PUI_Event::Event_VirtualPos:
				CurrentWindow=event->PosEvent()->win;
				CurrentWindow->_NowPos=event->PosEvent()->pos;
				CurrentWindow->NeedSolvePosEvent=1;
				
//				for (PUI_Window::LoseFocusLinkTable *p=CurrentWindow->LoseFocusWgHead;p;p=p->nxt)
//					if (p->wg!=NULL)
//						if (p->wg->Enabled)
//							p->wg->CheckPos(event->PosEvent(),posMode|Widgets::PosMode_LoseFocus|Widgets::PosMode_Direct);
//						else
//						{
//							PUI_DD[1]<<"NeedLoseFocusWg "<<p->wg->IDName<<" is disabled, remove it's property!"<<endl;
//							p->wg->CheckPos(event->PosEvent(),posMode|Widgets::PosMode_ForceLoseFocus|Widgets::PosMode_LoseFocus|Widgets::PosMode_Direct);//??
//							p->wg->RemoveNeedLoseFocus();
//						}
//					else PUI_DD[2]<<"NeedLoseFocusWidgets is NULL!"<<endl;
				for (set<Widgets*>::iterator sp=CurrentWindow->LoseFocusTargets.begin(),nsp;sp!=CurrentWindow->LoseFocusTargets.end();sp=nsp)
				{
					nsp=sp;
					++nsp;
					if ((*sp)!=nullptr)
						if ((*sp)->Enabled)
							(*sp)->CheckPos(event->PosEvent(),posMode|Widgets::PosMode_LoseFocus|Widgets::PosMode_Direct);
						else
						{
							PUI_DD[1]<<"NeedLoseFocusWg "<<(*sp)->IDName<<" is disabled, remove it's property!"<<endl;
							(*sp)->CheckPos(event->PosEvent(),posMode|Widgets::PosMode_ForceLoseFocus|Widgets::PosMode_LoseFocus|Widgets::PosMode_Direct);//??
							(*sp)->RemoveNeedLoseFocus();
						}
					else PUI_DD[2]<<"NeedLoseFocusWidgets is NULL!"<<endl;
				}

				if (CurrentWindow->OccupyPosWg!=NULL)
					if (CurrentWindow->OccupyPosWg->Enabled)
						CurrentWindow->OccupyPosWg->CheckPos(event->PosEvent(),posMode|Widgets::PosMode_Occupy|Widgets::PosMode_Default/*??*/|Widgets::PosMode_Direct);
					else
					{
						PUI_DD[1]<<"OccupyPosWg "<<CurrentWindow->OccupyPosWg->IDName<<" id disabled, remove it's property!"<<endl; 
						CurrentWindow->OccupyPosWg->CheckPos(event->PosEvent(),posMode|Widgets::PosMode_Occupy|Widgets::PosMode_ForceLoseFocus|Widgets::PosMode_LoseFocus|Widgets::PosMode_Direct);
						CurrentWindow->SetOccupyPosWg(NULL);//??
					}
				else
				{
					CurrentWindow->_MenuLayer->_SolvePosEvent(event->PosEvent(),posMode);
					CurrentWindow->_BackGroundLayer->_SolvePosEvent(event->PosEvent(),posMode);
				}
				re=CurrentWindow->NeedSolvePosEvent?PUI_SolveEventNotSolved:PUI_SolveEventSucceed;
				CurrentWindow->NeedSolvePosEvent=0;
				break;
			
			case PUI_Event::Event_KeyEvent:
				CurrentWindow=event->KeyEvent()->win;//??
				goto GOTO_StartSolveEvent;
			case PUI_Event::Event_WheelEvent:
				CurrentWindow=event->WheelEvent()->win;
				
			GOTO_StartSolveEvent:
				CurrentWindow->NeedSolveEvent=1;
				CurrentWindow->_MenuLayer->_SolveEvent(event);
				CurrentWindow->_BackGroundLayer->_SolveEvent(event);
				re=CurrentWindow->NeedSolveEvent?PUI_SolveEventNotSolved:PUI_SolveEventSucceed;
				CurrentWindow->NeedSolveEvent=0;
				break;
			
			case PUI_Event::Event_ForceRenderEvent:
			{
				PUI_ForceRenderEvent *FRevent=event->ForceRenderEvent();
				if (FRevent->win==NULL)
					PUI_Window::SetNeedFreshScreenAll();
				else if (FRevent->Area==ZERO_POSIZE)
					FRevent->win->SetPresentArea(FRevent->win->GetWinPS().ToOrigin());
				break;
			}
			
			case PUI_Event::Event_RefreshEvent:
				if (event->RefreshEvent()->CurrentIsNewest())
					PUI_Window::CurrentRefreshReady=1;
				else PUI_DD[3]<<"CurrentRefreshEvent is not newest! "<<event->RefreshEvent()->CurrentRefreshCode<<endl;
				re=PUI_SolveEventSucceed;
				break;
			
			case PUI_Event::Event_FunctionEvent:
				event->FunctionEvent()->Trigger();
				break;
				
			default:
				if (event->type==PUI_EVENT_UpdateTimer)
				{
					int *tarID=(int*)event->UserEvent()->data1;
					Widgets *tar=Widgets::GetWidgetsByID(*tarID);//??
					delete tarID;
					if (tar!=NULL)
					{
						event->UserEvent()->data1=tar;//Dangerous??
						CurrentWindow=tar->Win;
						CurrentWindow->NeedSolveEvent=1;
						tar->CheckEvent(event);
						CurrentWindow->NeedSolveEvent=0;
						re=PUI_SolveEventSucceed;
					}
				}
				break;
		}
		PUI_Event::_NowSolvingEvent=nullptr;
		while (!Widgets::WidgetsToDeleteAfterEvent.empty())
		{
			delete Widgets::WidgetsToDeleteAfterEvent.front();
			Widgets::WidgetsToDeleteAfterEvent.pop();
		}
		return re;
	}
	
	void PUI_DebugEventSolve(PUI_Event *event)
	{
		if (event->type==PUI_Event::Event_KeyEvent&&event->KeyEvent()->keyType==PUI_KeyEvent::Key_Down)
			if (event->KeyEvent()->mod&KMOD_CTRL)
				switch (event->KeyEvent()->keyCode)
				{
					case SDLK_F1:
						if (DEBUG_EnableForceQuitShortKey)
							switch (DEBUG_ForceQuitType)
							{
								case 0:	exit(0);
								case 1:	exit(1);
								case 2:	abort();
							}
					
					case SDLK_F2:
						DEBUG_DisplayBorderFlag=!DEBUG_DisplayBorderFlag;
						PUI_Window::SetNeedFreshScreenAll();
						break;
					
					case SDLK_F3:
						PUI_DD.SwitchToContrary();
						break;
						
					case SDLK_F4:
						DEBUG_DisplayPresentLimitFlag=!DEBUG_DisplayPresentLimitFlag;
						break;
					
//					case SDLK_F5:
//						DEBUG_EnableWidgetsShowInTurn=!DEBUG_EnableWidgetsShowInTurn;
//						break;
					case SDLK_F6:
						if (DEBUG_EnableDebugThemeColorChange)
						{
							static int ThemeColorCode=0;
							++ThemeColorCode;
							if (ThemeColorCode==PUI_ThemeColor::PUI_ThemeColor_UserDefined)
								ThemeColorCode=PUI_ThemeColor::PUI_ThemeColor_Blue;
							ThemeColor=PUI_ThemeColor(ThemeColorCode);
							PUI_Window::SetNeedFreshScreenAll();
						}
						break;
				}
	}
	
	PUI_Event* SDLEventToPUIEvent(const SDL_Event &SDLevent)//if NULL,means Unknown SDL_Event
	{
		switch (SDLevent.type)
		{
		    case SDL_QUIT:
				return new PUI_Event(PUI_Event::Event_Quit,SDLevent.common.timestamp);
		    case SDL_APP_TERMINATING:	
				/**< The application is being terminated by the OS
		            Called on iOS in applicationWillTerminate()
		            Called on Android in onDestroy()
				*/
			case SDL_APP_LOWMEMORY:
				/**< The application is low on memory, free memory if possible.
		            Called on iOS in applicationDidReceiveMemoryWarning()
		            Called on Android in onLowMemory()
		        */
		    case SDL_APP_WILLENTERBACKGROUND:
				/**< The application is about to enter the background
		            Called on iOS in applicationWillResignActive()
		            Called on Android in onPause()
				*/
		    case SDL_APP_DIDENTERBACKGROUND:
				/**< The application did enter the background and may not get CPU for some time
                    Called on iOS in applicationDidEnterBackground()
                    Called on Android in onPause()
		        */
			case SDL_APP_WILLENTERFOREGROUND:
			 	/**< The application is about to enter the foreground
		            Called on iOS in applicationWillEnterForeground()
		            Called on Android in onResume()
		        */
		    case SDL_APP_DIDENTERFOREGROUND:
				/**< The application is now interactive
					Called on iOS in applicationDidBecomeActive()
					Called on Android in onResume()
		        */
//		    case SDL_DISPLAYEVENT:	/**< Display state change */
		    case SDL_WINDOWEVENT:	/**< Window state change */
		    {
		    	PUI_WindowEvent *event=new PUI_WindowEvent;
		    	event->type=PUI_Event::Event_WindowEvent;
		    	event->timeStamp=SDLevent.common.timestamp;
		    	event->win=PUI_Window::GetWindowBySDLID(SDLevent.window.windowID);
				switch (SDLevent.window.event)
		    	{
					case SDL_WINDOWEVENT_SHOWN:			event->eventType=PUI_WindowEvent::Window_Shown;		break;
					case SDL_WINDOWEVENT_HIDDEN:		event->eventType=PUI_WindowEvent::Window_Hidden;	break;
					case SDL_WINDOWEVENT_EXPOSED:		event->eventType=PUI_WindowEvent::Window_Exposed;	break;
					case SDL_WINDOWEVENT_MOVED:			event->eventType=PUI_WindowEvent::Window_Moved;		break;
					case SDL_WINDOWEVENT_RESIZED:		event->eventType=PUI_WindowEvent::Window_Resized;	break;
					case SDL_WINDOWEVENT_SIZE_CHANGED:	event->eventType=PUI_WindowEvent::Window_SizeChange;break;
					case SDL_WINDOWEVENT_MINIMIZED:		event->eventType=PUI_WindowEvent::Window_Minimized;	break;
					case SDL_WINDOWEVENT_MAXIMIZED:		event->eventType=PUI_WindowEvent::Window_Maximized;	break;
					case SDL_WINDOWEVENT_RESTORED:		event->eventType=PUI_WindowEvent::Window_Restored;	break;
					case SDL_WINDOWEVENT_ENTER:			event->eventType=PUI_WindowEvent::Window_Enter;		break;
					case SDL_WINDOWEVENT_LEAVE:			event->eventType=PUI_WindowEvent::Window_Leave;		break;
					case SDL_WINDOWEVENT_FOCUS_GAINED:	event->eventType=PUI_WindowEvent::Window_GainFocus;	break;
					case SDL_WINDOWEVENT_FOCUS_LOST:	event->eventType=PUI_WindowEvent::Window_LoseFocus;break;
					case SDL_WINDOWEVENT_CLOSE:			event->eventType=PUI_WindowEvent::Window_Close;		break;
					case SDL_WINDOWEVENT_TAKE_FOCUS:	event->eventType=PUI_WindowEvent::Window_TakeFocus;	break;
					case SDL_WINDOWEVENT_HIT_TEST:		event->eventType=PUI_WindowEvent::Window_HitTest;	break;
					default:							event->eventType=PUI_WindowEvent::Window_None;		break;
				}
				event->data1=SDLevent.window.data1;//??
				event->data2=SDLevent.window.data2;
				if (event->win==NULL)
					DeleteToNULL(event);//??
				return event;
			}
			case SDL_KEYDOWN:
			case SDL_KEYUP:
			{
				PUI_KeyEvent *event=new PUI_KeyEvent;
				event->type=PUI_Event::Event_KeyEvent;
				event->timeStamp=SDLevent.key.timestamp;
				event->win=PUI_Window::GetWindowBySDLID(SDLevent.key.windowID);
				event->keyCode=SDLevent.key.keysym.sym;//??
				event->mod=SDLevent.key.keysym.mod;//??
				event->keyType=SDLevent.type==SDL_KEYDOWN?(SDLevent.key.repeat?PUI_KeyEvent::Key_Hold:PUI_KeyEvent::Key_Down)
														 :(SDLevent.type==SDL_KEYUP?PUI_KeyEvent::Key_Up:PUI_KeyEvent::Key_NONE);
				if (event->win==NULL)
					DeleteToNULL(event);//??
				return event;
			}
			case SDL_TEXTEDITING:
			{
				PUI_TextEvent *event=new PUI_TextEvent;
				event->type=PUI_Event::Event_TextEditEvent;
				event->timeStamp=SDLevent.common.timestamp;
				event->win=PUI_Window::GetWindowBySDLID(SDLevent.edit.windowID);
				event->text=SDLevent.edit.text;
				event->cursor=SDLevent.edit.start;
				event->length=SDLevent.edit.length;
				if (event->win==NULL)
					DeleteToNULL(event);
				return event;
			}
		    case SDL_TEXTINPUT:
			{
				PUI_TextEvent *event=new PUI_TextEvent;
				event->type=PUI_Event::Event_TextInputEvent;
				event->timeStamp=SDLevent.common.timestamp;
				event->win=PUI_Window::GetWindowBySDLID(SDLevent.text.windowID);
				event->text=SDLevent.text.text;
				event->length=event->text.length();
				if (event->win==NULL)
					DeleteToNULL(event);
				return event;
			}
		    case SDL_KEYMAPCHANGED:
				/**< Keymap changed due to a system event such as an
		    		input language or keyboard layout change.
				*/
				return nullptr;
			case SDL_MOUSEMOTION:
			case SDL_MOUSEBUTTONDOWN:
			case SDL_MOUSEBUTTONUP:
			{
				PUI_MouseEvent *event=new PUI_MouseEvent;
				event->type=PUI_Event::Event_MouseEvent;
				event->timeStamp=SDLevent.common.timestamp;
				if (SDLevent.type==SDL_MOUSEMOTION)
				{
					if (!PUI_EnableTouchSimulateMouse&&SDLevent.motion.which==SDL_TOUCH_MOUSEID)
					{
						delete event;
						return NULL;
					}
					event->win=PUI_Window::GetWindowBySDLID(SDLevent.motion.windowID);
					event->pos={SDLevent.motion.x,SDLevent.motion.y};
					event->delta={SDLevent.motion.xrel,SDLevent.motion.yrel};
					event->posType=PUI_PosEvent::Pos_Motion;
					event->button=PUI_PosEvent::Button_None;
					event->clicks=0;
					event->which=PUI_MouseEvent::Mouse_NONE;
				}
				else
				{
					if (!PUI_EnableTouchSimulateMouse&&SDLevent.button.which==SDL_TOUCH_MOUSEID)
					{
						delete event;
						return NULL;
					}
					event->win=PUI_Window::GetWindowBySDLID(SDLevent.button.windowID);
					event->pos={SDLevent.button.x,SDLevent.button.y};
					event->delta=ZERO_POINT;
					event->posType=SDLevent.type==SDL_MOUSEBUTTONUP?PUI_PosEvent::Pos_Up:PUI_PosEvent::Pos_Down;
					if (SDLevent.button.button==SDL_BUTTON_LEFT)
						event->button=PUI_PosEvent::Button_MainClick;
					else if (SDLevent.button.button==SDL_BUTTON_RIGHT)
						event->button=PUI_PosEvent::Button_SubClick;
					else event->button=PUI_PosEvent::Button_OtherClick;
					event->clicks=SDLevent.button.clicks;
					if (SDLevent.button.button==SDL_BUTTON_LEFT) event->which=PUI_MouseEvent::Mouse_Left;
					else if (SDLevent.button.button==SDL_BUTTON_RIGHT) event->which=PUI_MouseEvent::Mouse_Right;
					else if (SDLevent.button.button==SDL_BUTTON_MIDDLE) event->which=PUI_MouseEvent::Mouse_Middle;
					else if (SDLevent.button.button==SDL_BUTTON_X1) event->which=PUI_MouseEvent::Mouse_X1;
					else if (SDLevent.button.button==SDL_BUTTON_X2) event->which=PUI_MouseEvent::Mouse_X2;
					else event->which=PUI_MouseEvent::Mouse_NONE;
				}
				event->state=SDL_GetGlobalMouseState(NULL,NULL);//??
				if (event->win==NULL)
					DeleteToNULL(event);//??
				else if (event->win->GetScalePercent()!=1)
					 event->pos=event->pos/event->win->GetScalePercent();
				return event;
			}
			case SDL_MOUSEWHEEL:
			{
				PUI_WheelEvent *event=new PUI_WheelEvent;
				event->type=PUI_Event::Event_WheelEvent;
				event->timeStamp=SDLevent.common.timestamp;
				event->win=PUI_Window::GetWindowBySDLID(SDLevent.wheel.windowID);
				event->dx=SDLevent.wheel.x;
				event->dy=SDLevent.wheel.y;
				if (event->win==NULL)
					DeleteToNULL(event);//??
				else if (event->win->GetScalePercent()!=1)
					event->dx/=event->win->GetScalePercent(),
					event->dy/=event->win->GetScalePercent();
				return event;
			}
//			case SDL_JOYAXISMOTION:/**< Joystick axis motion */
//		    case SDL_JOYBALLMOTION:/**< Joystick trackball motion */
//		    case SDL_JOYHATMOTION:/**< Joystick hat position change */
//		    case SDL_JOYBUTTONDOWN:/**< Joystick button pressed */
//		    case SDL_JOYBUTTONUP:/**< Joystick button released */
//		    case SDL_JOYDEVICEADDED:/**< A new joystick has been inserted into the system */
//		    case SDL_JOYDEVICEREMOVED:/**< An opened joystick has been removed */
//			case SDL_CONTROLLERAXISMOTION:/**< Game controller axis motion */
//		    case SDL_CONTROLLERBUTTONDOWN:/**< Game controller button pressed */
//		    case SDL_CONTROLLERBUTTONUP:/**< Game controller button released */
//		    case SDL_CONTROLLERDEVICEADDED:/**< A new Game controller has been inserted into the system */
//		    case SDL_CONTROLLERDEVICEREMOVED:/**< An opened Game controller has been removed */
//		    case SDL_CONTROLLERDEVICEREMAPPED:/**< The controller mapping was updated */
//				return nullptr;
			case SDL_FINGERDOWN:
			case SDL_FINGERUP:
			case SDL_FINGERMOTION:
			{
				if (PUI_EnableTouchSimulateMouse)
					return nullptr;
//				if (SDLevent.tfinger.touchId==SDL_MOUSE_TOUCHID)
//					return NULL;
			#if PAL_CurrentPlatform==PAL_Platform_Windows//??
				PUI_TouchEvent *event=new PUI_TouchEvent;
				event->type=PUI_Event::Event_TouchEvent;
				event->timeStamp=SDLevent.common.timestamp;
				event->win=PUI_Window::GetWindowBySDLID(SDLevent.tfinger.windowID);//??
				if (event->win==NULL)
				{
					delete event;
					return nullptr;
				}
				Posize winps=event->win->GetWinPS();
				
				static unsigned int lastGestureDiameter=0,
									maxCountInThisGesture=0,
									lastMainclickDownTime[2]{0,0};
				static Point lastMainClickPoint[2];
				int lastCount=PUI_TouchEvent::Finger::count;
				int replacePos=0,samePos=-1,zeroPos=-1,acceptPos;//find right position to solve
				for (int i=0;i<10;++i)
				{
					PUI_TouchEvent::Finger &fin=PUI_TouchEvent::fingers[i];
					if (fin.fingerID!=0&&fin.downTime<PUI_TouchEvent::fingers[replacePos].downTime)
						replacePos=i;
					if (fin.fingerID==SDLevent.tfinger.fingerId&&samePos==-1)
						samePos=i;
					if (fin.fingerID==0&&zeroPos==-1)
						zeroPos=i;
				}
				
				if (SDLevent.type!=SDL_FINGERUP)//update place
				{
					if (samePos==-1&&zeroPos==-1)
						acceptPos=replacePos,PUI_DD[1]<<"Finger count over 10, replace oldest!"<<endl;
					else if (samePos!=-1)
						acceptPos=samePos;
					else//???
					{
						acceptPos=zeroPos;
						++PUI_TouchEvent::Finger::count;
						event->countChanged=1;
					}
					PUI_TouchEvent::Finger &fin=PUI_TouchEvent::fingers[acceptPos];
					event->count=PUI_TouchEvent::Finger::count;
					fin.pos=Point(SDLevent.tfinger.x*winps.w,SDLevent.tfinger.y*winps.h);
					if (SDLevent.type==SDL_FINGERDOWN)
					{
						fin.downTime=event->timeStamp;
						if (fin.count==1)
							PUI_TouchEvent::Finger::gestureStartTime=event->timeStamp,
							event->duration=0;
					}
					else event->duration=event->timeStamp-fin.gestureStartTime;
					fin.win=event->win;
					fin.pressure=SDLevent.tfinger.pressure;
					fin.fingerID=SDLevent.tfinger.fingerId;
				}
				else
					if (samePos==-1)
					{
						PUI_DD[1]<<"Finger up however not exsit, do nothing!"<<endl;
						delete event;
						return nullptr;
					}
					else//give back place
					{
						acceptPos=samePos;
						PUI_TouchEvent::Finger &fin=PUI_TouchEvent::fingers[samePos];
						--PUI_TouchEvent::Finger::count;
						event->count=PUI_TouchEvent::Finger::count;
						event->countChanged=1;
						event->duration=event->timeStamp-fin.gestureStartTime;
						fin.pos=ZERO_POINT;
						fin.downTime=0;
						fin.win=NULL;
						fin.pressure=0;
						fin.fingerID=0;
					}
				PUI_TouchEvent::Finger::lastUpdateTime=event->timeStamp;
				maxCountInThisGesture=max(maxCountInThisGesture,(unsigned int)event->count);
				
				auto getdist=[](const Point &p1,const Point &p2)->float
				{return sqrt((p1.x-p2.x)*(p1.x-p2.x)+(p1.y-p2.y)*(p1.y-p2.y));};
				
				if (SDLevent.type==SDL_FINGERMOTION)//??
				{
					event->posType=PUI_PosEvent::Pos_Motion;
					event->button=PUI_PosEvent::Button_None;
					event->clicks=0;
				}
				else if (SDLevent.type==SDL_FINGERDOWN)
				{
					if (event->count==1)
						event->posType=PUI_PosEvent::Pos_Down;
					else event->posType=PUI_PosEvent::Pos_Motion;
					event->clicks=1;
					if (event->count==1&&maxCountInThisGesture==1)
					{
						event->button=PUI_PosEvent::Button_MainClick;
						lastMainclickDownTime[0]=lastMainclickDownTime[1];
						lastMainclickDownTime[1]=event->timeStamp;
						lastMainClickPoint[0]=lastMainClickPoint[1];
						lastMainClickPoint[1]=Point(SDLevent.tfinger.x*winps.w,SDLevent.tfinger.y*winps.h);
						if (lastMainclickDownTime[1]-lastMainclickDownTime[0]<=PUI_Parameter_DoubleClickGaptime&&getdist(lastMainClickPoint[0],lastMainClickPoint[1])<=PUI_Parameter_DoubleClickTouchPointBias)
							event->clicks=2;
					}
					else event->button=PUI_PosEvent::Button_OtherClick;
				}
				else
				{
					if (event->count==0)
						event->posType=PUI_PosEvent::Pos_Up;
					else event->posType=PUI_PosEvent::Pos_Motion;
					event->clicks=1;
					if (event->count==0&&maxCountInThisGesture==1)
					{
//						DD[3]<<"duration "<<event->duration<<endl;
//						DD[3]<<"gaptime "<<lastMainclickDownTime[1]-lastMainclickDownTime[0]<<endl;
						if (event->duration>PUI_Parameter_LongClickOuttime)
							event->button=PUI_PosEvent::Button_SubClick;
						else if (lastMainclickDownTime[1]-lastMainclickDownTime[0]<=PUI_Parameter_DoubleClickGaptime&&getdist(lastMainClickPoint[0],lastMainClickPoint[1])<=PUI_Parameter_DoubleClickTouchPointBias)
						{
							lastMainclickDownTime[0]=lastMainclickDownTime[1]=0;
							event->button=PUI_PosEvent::Button_MainClick,
							event->clicks=2;
						}
					}
					else event->button=PUI_PosEvent::Button_OtherClick;
				}
				if (event->count==0)
					maxCountInThisGesture=0;

				static Point lastCenter=ZERO_POINT;
				PUI_TouchEvent::Finger *fins=PUI_TouchEvent::fingers;
				deque <Doublet<unsigned,Point> > &track=PUI_TouchEvent::Finger::CenterPosTrack;
				vector <int> validPos;
				float sumPressure=0,sumRadius=0;
				for (int i=0;i<=9;++i)
					if (fins[i].fingerID!=0)
						validPos.push_back(i),
						event->pos+=fins[i].pos,
						sumPressure+=fins[i].pressure;
				if (event->count==0)
					event->pos=Point(SDLevent.tfinger.x*winps.w,SDLevent.tfinger.y*winps.h),
					event->pressure=SDLevent.tfinger.pressure;
				else
					event->pos=Point(event->pos.x/event->count,event->pos.y/event->count),
					event->pressure=sumPressure/event->count;
				if (!event->countChanged&&lastCount!=0)
					event->delta=event->pos-lastCenter;
				else event->delta=ZERO_POINT;
				lastCenter=event->pos;
				
				static float lastSpeedX=0,lastSpeedY=0;
				if (event->countChanged)//?? Calc speed
				{
					track.clear();
					track.push_back({event->timeStamp,event->pos});
					event->speedX=lastSpeedX;
					event->speedY=lastSpeedY;
					if (event->count==0)
						lastSpeedX=lastSpeedY=0;
				}
				else
				{
					track.push_back({event->timeStamp,event->pos});
					if (event->timeStamp-track.front().a>PUI_Parameter_TouchSpeedSampleTimeLength)
						track.pop_front();
					float sumDistX=0,sumDistY=0;
					for (int i=1;i<track.size();++i)
						sumDistX+=track[i].b.x-track[i-1].b.x,
						sumDistY+=track[i].b.y-track[i-1].b.y;
					if (track.back().a-track.front().a==0)
						event->speedX=lastSpeedX,event->speedY=lastSpeedY;
					else event->speedX=sumDistX/(track.back().a-track.front().a),event->speedY=sumDistY/(track.back().a-track.front().a);
					if (fabs(event->speedX)<=PUI_Parameter_TouchSpeedDiscardLimit)
						event->speedX=0;
					if (fabs(event->speedY)<=PUI_Parameter_TouchSpeedDiscardLimit)
						event->speedY=0;
					if (event->count==0)
						lastSpeedX=lastSpeedY=0;
					else lastSpeedX=event->speedX,lastSpeedY=event->speedY;
				}
//				PUI_DD[3]<<":"<<event->speedX<<" "<<event->speedY<<endl;
				
				if (event->count>=2)
				{
					for (int i=0;i<validPos.size();++i)//?? Calc diameter
						for (int j=i+1;j<validPos.size();++j)
							event->diameter=max(event->diameter,(unsigned int)getdist(fins[validPos[i]].pos,fins[validPos[j]].pos));
					if (!event->countChanged)
						event->dDiameter=(int)event->diameter-(int)lastGestureDiameter;
				}
				lastGestureDiameter=event->diameter;
				
				event->gesture=PUI_TouchEvent::Finger::gesture;
				if (event->count==0)
					event->gesture=PUI_TouchEvent::Ges_End;
				else switch (PUI_TouchEvent::Finger::gesture)
				{
					case PUI_TouchEvent::Ges_NONE:
					case PUI_TouchEvent::Ges_End:
						if (event->count>=2)
							event->gesture=PUI_TouchEvent::Ges_MultiClick;
						else event->gesture=PUI_TouchEvent::Ges_ShortClick;
						event->gesChanged=1;
						break;
					case PUI_TouchEvent::Ges_ShortClick:
						if (event->count>=2)
							event->gesture=PUI_TouchEvent::Ges_MultiClick;
						else if (event->speed()>PUI_Parameter_MovingTriggerSpeed)
							event->gesture=PUI_TouchEvent::Ges_Moving;
						else if (event->duration>=PUI_Parameter_LongClickOuttime)
							event->gesture=PUI_TouchEvent::Ges_LongClick;
						else break;
						event->gesChanged=1;
						break;
					case PUI_TouchEvent::Ges_LongClick:
						if (event->count>=2)
							event->gesture=PUI_TouchEvent::Ges_MultiClick;
						else if (event->speed()>PUI_Parameter_MovingTriggerSpeed)
							event->gesture=PUI_TouchEvent::Ges_Moving;
						else break;
						event->gesChanged=1;
						break;
					case PUI_TouchEvent::Ges_Moving:
						if (event->count>=2)
							event->gesture=PUI_TouchEvent::Ges_MultiMoving,
							event->gesChanged=1;
						break;
					case PUI_TouchEvent::Ges_MultiClick:
						if (event->speed()>PUI_Parameter_MovingTriggerSpeed)
							event->gesture=PUI_TouchEvent::Ges_MultiMoving;
						else if (abs(event->dDiameter)>PUI_Parameter_MovingTriggerPinch)
							event->gesture=PUI_TouchEvent::Ges_MultiMoving;
						else if (event->count==1)
							event->gesture=PUI_TouchEvent::Ges_Moving;
						else break;
						event->gesChanged=1;
						break;
					case PUI_TouchEvent::Ges_MultiMoving:
						if (event->count==1)
							event->gesture=PUI_TouchEvent::Ges_Moving,
							event->gesChanged=1;
						break;
				}
				PUI_TouchEvent::Finger::gesture=event->gesture;
//				DD[3]<<"CurrentSpeed "<<event->speed()<<" "<<event->speedX<<" "<<event->speedY<<endl;
//				DD[3]<<"diameter "<<event->diameter<<" "<<event->dDiameter<<endl;
				
				return event;
			#else
				return nullptr;
			#endif
			}
			case SDL_DOLLARGESTURE:
		    case SDL_DOLLARRECORD:
		    case SDL_MULTIGESTURE:
			case SDL_CLIPBOARDUPDATE:
				return nullptr;
			case SDL_DROPFILE:
		    case SDL_DROPTEXT:
		    case SDL_DROPBEGIN:/**< A new set of drops is beginning (NULL filename) */
			case SDL_DROPCOMPLETE:/**< Current set of drops is now complete (NULL filename) */
		    {
		    	PUI_DropEvent *event=new PUI_DropEvent;
		    	event->type=PUI_Event::Event_DropEvent;
		    	event->timeStamp=SDLevent.common.timestamp;
		    	event->win=PUI_Window::GetWindowBySDLID(SDLevent.drop.windowID);//??
				switch (SDLevent.type)
				{
					case SDL_DROPFILE:		event->drop=PUI_DropEvent::Drop_File;		break;
					case SDL_DROPTEXT:		event->drop=PUI_DropEvent::Drop_Text;		break;
					case SDL_DROPBEGIN:		event->drop=PUI_DropEvent::Drop_Begin;		break;
					case SDL_DROPCOMPLETE:	event->drop=PUI_DropEvent::Drop_Complete;	break;
				}
				if (SDLevent.drop.file!=NULL)
					event->str=SDLevent.drop.file;
				if (event->win==NULL)//??
					DeleteToNULL(event);
				return event;
			}
			case SDL_AUDIODEVICEADDED:/**< A new audio device is available */
		    case SDL_AUDIODEVICEREMOVED:/**< An audio device has been removed. */
//			case SDL_SENSORUPDATE:/**< A sensor was updated */
		    case SDL_RENDER_TARGETS_RESET:/**< The render targets have been reset and their contents need to be updated */
		    case SDL_RENDER_DEVICE_RESET:/**< The device has been reset and all textures need to be recreated */
		    	return nullptr;
		    case SDL_SYSWMEVENT:	/**< System specific event */
		    {
//		    	#if PAL_CurrentPlatform == PAL_Platform_Windows
//	#if PAL_SDL_IncludeSyswmHeader == 1 && PAL_GUI_EnablePenEvent == 1
			#if PAL_SDL_IncludeSyswmHeader == 1
				#if PAL_CurrentPlatform == PAL_Platform_Windows
				if (SDLevent.syswm.msg==nullptr)
					return nullptr;
				SDL_SysWMmsg &winmsg=*SDLevent.syswm.msg;
				if (winmsg.subsystem!=SDL_SYSWM_WINDOWS)
					return nullptr;
				HWND hWnd=winmsg.msg.win.hwnd;
				UINT msg=winmsg.msg.win.msg;
				WPARAM wParam=winmsg.msg.win.wParam;
				LPARAM lParam=winmsg.msg.win.lParam;
				switch (msg)
				{
					#if PAL_GUI_EnablePenEvent == 1
					case WM_POINTERUPDATE:
					{
						UINT32 pointerId=GET_POINTERID_WPARAM(wParam);
						POINTER_INPUT_TYPE pointerType=PT_POINTER;
						POINTER_PEN_INFO penInfo;
						if (GetPointerType(pointerId,&pointerType)&&pointerType==PT_PEN&&GetPointerPenInfo(pointerId,&penInfo))
						{
							static Point LastPenPos=ZERO_POINT;
							PUI_PenEvent *event=new PUI_PenEvent();
							event->type=PUI_Event::Event_PenEvent;
							event->timeStamp=SDLevent.syswm.timestamp;
							event->win=PUI_Window::GetPUIWindowFromWinHWND(hWnd);//??
							if (event->win==NULL)
								return DeleteToNULL(event);
							event->pos=Point(penInfo.pointerInfo.ptPixelLocation.x,penInfo.pointerInfo.ptPixelLocation.y)-event->win->GetWinPS().GetLU();
							if (!(penInfo.pointerInfo.pointerFlags&POINTER_FLAG_NEW))
								event->delta=event->pos-LastPenPos;
							else PUI_DD[3]<<"New pen pointer"<<endl;
							LastPenPos=event->pos;
							event->posType=PUI_PosEvent::Pos_Motion;//?? Because the click will be send as mouse again?
							event->posType=PUI_PosEvent::Button_None;//??
							event->clicks=0;//??
							if (penInfo.penMask&PEN_MASK_PRESSURE)
								event->pressure=penInfo.pressure/1024.0;
							if (penInfo.penMask&PEN_MASK_ROTATION)
								event->theta=penInfo.rotation;
							if (penInfo.penMask&PEN_MASK_TILT_X)
								event->theta=penInfo.tiltX;
							if (penInfo.penMask&PEN_MASK_TILT_Y)
								event->theta=penInfo.tiltY;
							if (penInfo.penFlags&PEN_FLAG_BARREL)
								event->stat|=PUI_PenEvent::Stat_Barrel;
							if (penInfo.penFlags&PEN_FLAG_INVERTED)
								event->stat|=PUI_PenEvent::Stat_Inverted;
							if (penInfo.penFlags&PEN_FLAG_ERASER)
								event->stat|=PUI_PenEvent::Stat_Eraser;
							if (penInfo.pointerInfo.pointerFlags&POINTER_FLAG_INCONTACT)
								event->contactted=1;
							else event->contactted=0;
							return event;
						}
						break;
					}
					#endif
					default:
						break;
				}
				#endif
			#endif
				return nullptr;
			}
			default:
				if (SDLevent.type==PUI_SDL_USEREVENT)
					return (PUI_Event*)SDLevent.user.data1;
				else if (SDLevent.type>=SDL_USEREVENT)
				{
					PUI_SDLUserEvent *event=new PUI_SDLUserEvent;
					event->type=PUI_Event::Event_SDLUserEvent;
					event->timeStamp=SDLevent.user.timestamp;
					event->SDLtype=SDLevent.type;
					event->code=SDLevent.user.code;
					event->data1=SDLevent.user.data1;
					event->data2=SDLevent.user.data2;
					return event;
				}
		}
		return nullptr;
	}
	
	PUI_Event* PUI_WaitEvent()
	{
		SDL_Event event;
		SDL_WaitEvent(&event);
		return SDLEventToPUIEvent(event);
	}
	
	PUI_Event* PUI_PollEvent()
	{
		SDL_Event event;
		if (SDL_PollEvent(&event))
			return SDLEventToPUIEvent(event);
		else return nullptr;
	}
		
	void PUI_EasyEventLoop(bool (*func)(const PUI_Event*,bool,int&))
	{
		int QuitFlag=0;
		while (QuitFlag!=1)
		{
			PUI_Event *mainEvent=PUI_WaitEvent();
			if (mainEvent==nullptr)
				continue;
			if (!func(mainEvent,0,QuitFlag))
				if (PUI_SolveEvent(mainEvent)!=0)
				{
					if (mainEvent->type==PUI_Event::Event_Quit)
						QuitFlag=1;
					else if (mainEvent->type==PUI_PosEvent::Event_KeyEvent&&mainEvent->KeyEvent()->keyType==PUI_KeyEvent::Key_Down)
						if (mainEvent->KeyEvent()->keyCode==SDLK_ESCAPE)
							QuitFlag=1;
					PUI_DebugEventSolve(mainEvent);
					func(mainEvent,1,QuitFlag);
				}
			PUI_UpdateWidgetsPosize();
			PUI_PresentWidgets();
			delete mainEvent;
		}
	}
	
	void PUI_EasyEventLoop(void (*func)(const PUI_Event*,int&)=NULL)
	{
		int QuitFlag=0;
		while (QuitFlag!=1)
		{
			PUI_Event *mainEvent=PUI_WaitEvent();
			if (mainEvent==nullptr)
				continue;
			if (PUI_SolveEvent(mainEvent)!=0)
			{
				if (mainEvent->type==PUI_Event::Event_Quit)
					QuitFlag=1;
				else if (mainEvent->type==PUI_PosEvent::Event_KeyEvent&&mainEvent->KeyEvent()->keyType==PUI_KeyEvent::Key_Down)
					if (mainEvent->KeyEvent()->keyCode==SDLK_ESCAPE)
						QuitFlag=1;
				PUI_DebugEventSolve(mainEvent);
				if (func!=NULL)
					func(mainEvent,QuitFlag);
			}
			PUI_UpdateWidgetsPosize();
			PUI_PresentWidgets();
			delete mainEvent;
		}
	}
	
	int PAL_GUI_Init_NoInitWindow()
	{
		PUI_DD.SetDebugType(0,"PUI_Info");
		PUI_DD.SetDebugType(1,"PUI_Warning");
		PUI_DD.SetDebugType(2,"PUI_Error");
		PUI_DD.SetDebugType(3,"PUI_Debug");
		
		PUI_DD[0]<<"PAL_GUI_Init"<<endl;
		
		SDL_Init(SDL_INIT_EVERYTHING);
		string resourcepath;
		char *basepath=SDL_GetBasePath();//??
		if (basepath!=nullptr)
		{
			resourcepath=string(basepath);
			SDL_free(basepath);
		}
		
		#if PAL_SDL_IncludeSyswmHeader == 1
		SDL_EventState(SDL_SYSWMEVENT,SDL_ENABLE);
		#endif
		
		SDL_SetHint(SDL_HINT_MOUSE_FOCUS_CLICKTHROUGH,"1");
		
		if (!PUI_DefaultFonts)
		{
			#if PAL_GUI_UseSystemFontFile == 1
				#if PAL_CurrentPlatform == PAL_Platform_Windows
				string winpath=GetWindowsDirectoryPath();
				if (winpath!="")
				{
					PUI_DefaultFonts.SetDefaultSize(PUI_DefaultFontSize);
					PUI_DefaultFonts.SetFontFile(winpath+"\\Fonts\\msyhl.ttc");
				}
				#endif
			#endif
			if (!PUI_DefaultFonts)
			{
				PUI_DefaultFonts.SetDefaultSize(PUI_DefaultFontSize);
				PUI_DefaultFonts.SetFontFile(resourcepath+"msyhl.ttc");
			}
		}
		
		if (!PUI_DefaultFonts)
			return PUI_DD[2]<<"Failed to load default font!"<<endl,1; 
		
		PUI_SDL_USEREVENT=SDL_RegisterEvents(1);
		PUI_EVENT_UpdateTimer=PUI_Event::RegisterEvent(1);
		PUI_Event_TimerPerSecond=PUI_Event::RegisterEvent(1);
		
		if (PAL_SDL_IncludeSyswmHeader)
			SDL_EventState(SDL_SYSWMEVENT,SDL_ENABLE); 
		
		if (PUI_Parameter_RefreshRate!=0)
			SetSynchronizeRefreshTimerOnOff(1);
		PUI_DD[0]<<"PAL_GUI_Init: OK"<<endl;
		return 0;
	}
	
	int PAL_GUI_Init(const Posize &winps=PUI_WINPS_DEFAULT,const string &title="",unsigned int flag=PUI_Window::PUI_FLAG_RESIZEABLE)
	{
		int re=PAL_GUI_Init_NoInitWindow();
		if (re) return re;
		MainWindow=CurrentWindow=new PUI_Window(winps,title,flag);
		if (MainWindow==nullptr)
			return 2;
		return 0;
	}
	
	int PAL_GUI_Quit()
	{
		PUI_DD[0]<<"PAL_GUI_Quit"<<endl;
		while (!PUI_Window::GetAllWindowSet().empty())
			delete *PUI_Window::GetAllWindowSet().begin();
		PUI_DD[3]<<"PAL_GUI_Quit Delete windows OK, then delete other widgets."<<endl;
		while (!Widgets::GetWidgetsIDmap().empty())
		{
			PUI_DD[1]<<"Widgets is nonempty! delete "<<Widgets::GetWidgetsIDmap().begin()->second;
			PUI_DD[1]<<"ID "<<Widgets::GetWidgetsIDmap().begin()->second->GetID()<<endl;
			delete Widgets::GetWidgetsIDmap().begin()->second;
		}
		PUI_DD[0]<<"SDL_Quit"<<endl;
		SDL_Quit();
		PUI_DD[0]<<"SDL_Quit OK"<<endl;
		PUI_DD[0]<<"PAL_GUI_Quit OK"<<endl;
		return 0;
	}
	
	int PUI_EasyEventLoopAndQuit(bool (*func)(const PUI_Event*,bool,int&))
	{
		PUI_EasyEventLoop(func);
		return PAL_GUI_Quit();
	}
	
	int PUI_EasyEventLoopAndQuit(void (*func)(const PUI_Event*,int&)=nullptr)
	{
		PUI_EasyEventLoop(func);
		return PAL_GUI_Quit();
	}
}
#endif
