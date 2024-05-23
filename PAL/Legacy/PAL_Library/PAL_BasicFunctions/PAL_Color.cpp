#ifndef PAL_BASICFUNCTIONS_COLOR_CPP
#define PAL_BASICFUNCTIONS_COLOR_CPP 1

#include "../PAL_BuildConfig.h"
#include "../PAL_BasicFunctions/PAL_BasicFunctions_0.cpp"

namespace PAL::Legacy
{
	struct HSVA;
	struct RGBA
	{
		#define Uint8 unsigned char
		Uint8 r,g,b,a=255;
		
		inline float Opacity() const
		{return a/255.0;}
		
		inline bool TransParent() const
		{return a!=255;}
		
		inline bool HaveColor() const
		{return a!=0/*r!=0||g!=0||b!=0||a!=0*/;}
		
		inline bool operator ! () const
		{return r==0&&g==0&&b==0&&a==0;}
		
		inline void SetColor(const Uint8 &_r,const Uint8 &_g,const Uint8 &_b)
		{r=_r;g=_g;b=_b;}
		
		inline void SetColor(const Uint8 &_r,const Uint8 &_g,const Uint8 &_b,const Uint8 &_a)
		{r=_r;g=_g;b=_b;a=_a;}
		
		inline RGBA operator << (const RGBA &src) const
		{
			unsigned int sr=src.r,sg=src.g,sb=src.b,tr=r,tg=g,tb=b;
			float sa=src.a/255.0,ta=a/255.0;
			return RGBA(sr*sa+tr*(1-sa),sg*sa+tg*(1-sa),sb*sa+tb*(1-sa),(sa+ta*(1-sa))*255);
		}
		
		inline RGBA operator >> (const RGBA &tar) const
		{
			unsigned int sr=r,sg=g,sb=b,tr=tar.r,tg=tar.g,tb=tar.b;
			float sa=a/255.0,ta=tar.a/255.0;
			return RGBA(sr*sa+tr*(1-sa),sg*sa+tg*(1-sa),sb*sa+tb*(1-sa),(sa+ta*(1-sa))*255);
		}
		
		inline RGBA operator * (float per) const
		{
			per=per>1?1:(per<0?0:per);
			if (per==0) return {0,0,0,0};//In case of conflict with DynamicUserColor
			else return RGBA(r,g,b,a*per);
		}
		
		inline RGBA AnotherA(Uint8 _a) const
		{return RGBA(r,g,b,_a);}
		
	//	inline RGBA operator + (const RGBA &co) const
	//	{
	//		//...
	//	}
	//	
	//	inline RGBA operator % (const RGBA &co) const
	//	{
	//		//...
	//	}
		
		inline bool operator == (const RGBA &co) const
		{return {r==co.r&&g==co.g&&b==co.b&&a==co.a};}
		
		inline bool operator != (const RGBA &co) const
		{return r!=co.r||g!=co.g||b!=co.b||a!=co.a;}
		
		Uint8 ToGray() const
		{return (r*0.299+g*0.587+b*0.114)*a/255.0;}
		
		inline unsigned ToIntRGBA() const
		{return (unsigned int)r<<24|(unsigned int)g<<16|(unsigned int)b<<8|(unsigned int)a;}
		
		RGBA() {}
		RGBA(const Uint8 &gray):r(gray),g(gray),b(gray) {}
		RGBA(const Uint8 &gray,const Uint8 &_a):r(gray),g(gray),b(gray),a(_a) {}
		RGBA(const Uint8 &_r,const Uint8 &_g,const Uint8 &_b):r(_r),g(_g),b(_b) {} 
		RGBA(const Uint8 &_r,const Uint8 &_g,const Uint8 &_b,const Uint8 &_a):r(_r),g(_g),b(_b),a(_a) {} 
		RGBA(const HSVA &co);
		#undef Uint8
	};

	const RGBA RGBA_WHITE={255,255,255,255},
		  RGBA_BLACK={0,0,0,255},
		  RGBA_NONE={0,0,0,0},
		  RGBA_TRANSPARENT={255,255,255,0},
		  RGBA_RED={255,0,0,255},
		  RGBA_GREEN={0,255,0,255},
		  RGBA_BLUE={0,0,255,255},
		  RGBA_BLUE_8A[8]={{0,100,255,31},{0,100,255,63},{0,100,255,95},{0,100,255,127},{0,100,255,159},{0,100,255,191},{0,100,255,223},{0,100,255,255}},
		  RGBA_GRAY_B8A[8]={{0,0,0,31},{0,0,0,63},{0,0,0,95},{0,0,0,127},{0,0,0,159},{0,0,0,191},{0,0,0,223},{0,0,0,255}},
		  RGBA_GRAY_W8A[8]={{255,255,255,31},{255,255,255,63},{255,255,255,95},{255,255,255,127},{255,255,255,159},{255,255,255,191},{255,255,255,223},{255,255,255,255}},
		  RGBA_BLUE_8[8]={{224,236,255,255},{192,217,255,255},{160,197,255,255},{128,178,255,255},{96,158,255,255},{64,139,255,255},{32,119,255,255},{0,100,255,255}},
		  RGBA_GRAY_8[8]={{224,224,224,255},{192,192,192,255},{160,160,160,255},{128,128,128,255},{96,96,96,255},{64,64,64,255},{32,32,32,255},{0,0,0,255}},
		  RGBA_GREEN_8[8]={{224,255,224,255},{192,255,192,255},{160,255,160,255},{128,255,128,255},{96,255,96,255},{64,255,64,255},{32,255,32,255},{0,255,0,255}},
		  RGBA_Black={0,0,0,255},
		  RGBA_DarkRed={128,0,0,255},
		  RGBA_DarkGreen={0,128,0,255},
		  RGBA_DarkYellow={128,128,0,255},
		  RGBA_DarkBlue={0,0,128,255},
		  RGBA_DarkMagenta={128,0,128,255},
		  RGBA_DarkCyan={0,128,128,255},
		  RGBA_LightGray={192,192,192,255},
		  RGBA_DarkGray={128,128,128,255},
		  RGBA_LightRed={255,0,0,255},
		  RGBA_LightGreen={0,255,0,255},
		  RGBA_LightYellow={255,255,0,255},
		  RGBA_LightBlue={0,0,255,255},
		  RGBA_LightMagenta={255,0,255,255},
		  RGBA_LightCyan={0,255,255,255},
		  RGBA_White={255,255,255,255};
	
	struct HSVA
	{
		double h,s,v,a=1;
		
		HSVA() {}
		HSVA(double _h,double _s,double _v):h(_h),s(_s),v(_v) {}
		HSVA(double _h,double _s,double _v,double _a):h(_h),s(_s),v(_v),a(_a) {}
		HSVA(const RGBA &co);
	};
	
	RGBA::RGBA(const HSVA &co)
	{
		double h=co.h*6,s=co.s,v=co.v;
		double f=h-int(h),p=v*(1-s),q=v*(1-f*s),t=v*(1-(1-f)*s);
		double r,g,b;
		switch ((int)h)
		{
			case 0:	r=v;g=t;b=p; break;
			case 1:	r=q;g=v;b=p; break;
			case 2:	r=p;g=v;b=t; break;
			case 3:	r=p;g=q;b=v; break;
			case 4: r=t;g=p;b=v; break;
			case 5: r=v;g=p;b=q; break;
		}
		this->r=r*255;
		this->g=g*255;
		this->b=b*255;
		this->a=co.a*255;
	}
	
	HSVA::HSVA(const RGBA &co)
	{
		double r=co.r/255.0,
			   g=co.g/255.0,
			   b=co.b/255.0;
		double cmax=maxN(r,g,b),
			   cmin=minN(r,g,b);
		double delta=cmax-cmin;
		if (delta==0)
			h=0;
		else if (cmax==r)
			h=((g-b)/delta+0)/6.0;
		else if (cmax==g)
			h=((b-r)/delta+2)/6.0;
		else h=((r-g)/delta+4)/6.0;
		if (h<0)
			h+=1.0;
		if (cmax==0)
			s=0;
		else s=delta/cmax;
		v=cmax;
		a=co.a/255.0;
	}
}
#endif

#if PAL_LibraryUseSDL == 1
#ifndef PAL_BASICFUNCTIONS_COLOR_CPP_COLORWITHSDL
#define PAL_BASICFUNCTIONS_COLOR_CPP_COLORWITHSDL 1

#include PAL_SDL_HeaderPath//??
namespace PAL::Legacy
{
	inline void SDLColorToRGBA(const SDL_Color &src,RGBA &tar)
	{
		tar.r=src.r;
		tar.g=src.g;
		tar.b=src.b;
		tar.a=src.a;
	}

	inline RGBA SDLColorToRGBA(const SDL_Color &src)
	{return {src.r,src.g,src.b,src.a};}

	inline void RGBAToSDLColor(const RGBA &src,SDL_Color &tar)
	{
		tar.r=src.r;
		tar.g=src.g;
		tar.b=src.b;
		tar.a=src.a;
	}

	inline SDL_Color RGBAToSDLColor(const RGBA &src)
	{return {src.r,src.g,src.b,src.a};}
}
#endif
#endif
