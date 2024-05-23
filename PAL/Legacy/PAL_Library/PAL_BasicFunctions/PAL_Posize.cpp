#ifndef PAL_BASICFUNCTIONS_POSIZE_CPP
#define PAL_BASICFUNCTIONS_POSIZE_CPP 1

#include "../PAL_BuildConfig.h"

#include <algorithm>

namespace PAL::Legacy
{
	struct Range
	{
		int l,r;
		
		inline bool Len0() const
		{return l==r;}
		
		inline bool IsEmpty() const
		{return l>r;}
		
		inline int Length() const
		{return r-l;}
		
		inline int mid() const
		{return l+r>>1;}
		
		template <typename T> bool InRange(const T &x) const
		{return l<=x&&x<=r;}
		
		template <typename T> T EnsureInRange(const T &x) const
		{
			if (x<l) return l;
			else if (x>r) return r;
			else return x;
		}
		
		inline void Shift(const int &x)
		{l+=x;r+=x;}
		
		inline Range operator & (const Range &rg) const
		{return Range(std::max(l,rg.l),std::min(r,rg.r));}
		
		Range() {}
		Range(const int &_l,const int &_r):l(_l),r(_r) {}
	};

	struct Point
	{
		int x,y;
		
		inline double Mo2() const
		{return x*x+y*y;}
		
		inline double Mo() const
		{return sqrt(Mo2());}
		
		inline Point operator + (const Point &_pt) const
		{return Point(x+_pt.x,y+_pt.y);}

		inline Point operator - (const Point &_pt) const
		{return Point(x-_pt.x,y-_pt.y);}
		
		inline Point& operator += (const Point &_pt)
		{return *this=operator +(_pt);}
		
		inline Point& operator -= (const Point &_pt)
		{return *this=operator -(_pt);}
		
		inline bool operator == (const Point &_pt) const
		{return x==_pt.x&&y==_pt.y;}
		
		inline bool operator != (const Point &_pt) const
		{return x!=_pt.x||y!=_pt.y;}
		
		inline Point operator * (double lambda) const
		{return Point(x*lambda+0.5,y*lambda+0.5);}
		
		inline Point operator * (float lambda) const
		{return Point(x*lambda+0.5,y*lambda+0.5);}
		
		inline Point operator / (double lambda) const
		{return Point(x/lambda,y/lambda);}
		
		inline Point operator / (float lambda) const
		{return Point(x/lambda,y/lambda);}
		
		inline int operator * (const Point &_pt) const
		{return x*_pt.x+y*_pt.y;}
		
		inline int operator % (const Point &_pt) const
		{return x*_pt.y-y*_pt.x;}
		
		Point() {}
		Point(const int &_x,const int &_y):x(_x),y(_y) {}
	};

	struct Posize
	{
		int x,y,w,h;
		
		inline int x2() const
		{return x+w-1;}
		
		inline int y2() const
		{return y+h-1;}
		
		inline int midX() const
		{return (x+x2())>>1;}
		
		inline int midY() const
		{return (y+y2())>>1;}
		
		inline Point GetCentre() const
		{return Point(midX(),midY());}
		
		inline void SetCentre(const Point &pt)
		{
			x=pt.x-((w+1)>>1)+1;
			y=pt.y-((h+1)>>1)+1;
		}
		
		inline Point GetLU() const
		{return Point(x,y);}
		
		inline Point GetLD() const
		{return Point(x,y2());}
		
		inline Point GetRU() const
		{return Point(x2(),y);}
		
		inline Point GetRD() const
		{return Point(x2(),y2());}
		
		inline Posize ToOrigin() const
		{return Posize(0,0,w,h);}
		
		inline int Size() const
		{return w*h;}
		
		inline double Slope() const
		{return w==0?1e100:h*1.0/w;}
		
		inline double InverseSlope() const
		{return h==0?1e100:w*1.0/h;}
		
		inline bool In(const int &xx,const int &yy) const
		{return x<=xx&&xx<x+w&&y<=yy&&yy<y+h;}
		
		inline bool In(const Point &pt) const
		{return x<=pt.x&&pt.x<x+w&&y<=pt.y&&pt.y<y+h;}
		
		inline bool In(const Posize &ps) const
		{return x<=ps.x&&ps.x+ps.w<=x+w&&y<=ps.y&&ps.y+ps.h<=y+h;}
		
		inline void SetX_ChangeW(const int &_x)
		{w=x2()-_x+1;x=_x;}
		
		inline void SetY_ChangeH(const int &_y)
		{h=y2()-_y+1;y=_y;}
		
		inline void SetX2(const int &_x2)
		{w=_x2-x+1;}
		
		inline void SetY2(const int &_y2)
		{h=_y2-y+1;}
		
		inline void SetX2_ChangeX(const int &_x2)
		{x=_x2-w+1;}
		
		inline void SetY2_ChangeY(const int &_y2)
		{y=_y2-h+1;}
		
		inline void SetW_ChangeX(const int &_w)
		{x=x2()-_w+1;w=_w;}
		
		inline void SetH_ChangeY(const int &_h)
		{y=y2()-_h+1;h=_h;}
		
		inline Point EnsureIn(const Point &_pt) const
		{
			using namespace std;
			return Point(max(min(_pt.x,x2()),x),max(min(_pt.y,y2()),y));
		}
			
		inline Posize EnsureIn(Posize _ps)
		{
			if (_ps.w>w) _ps.w=w;
			if (_ps.h>h) _ps.h=h;
			if (_ps.x2()>x2()) _ps.SetX2_ChangeX(x2());
			if (_ps.y2()>y2()) _ps.SetY2_ChangeY(y2());
			if (_ps.x<x) _ps.x=x;
			if (_ps.y<y) _ps.y=y;
			return _ps;	
		}
		
		inline Posize operator & (const Posize &_ps) const
		{
			using namespace std;
			Posize re;
			re.x=max(x,_ps.x);
			re.y=max(y,_ps.y);
			re.w=max(0,min(x2(),_ps.x2())-re.x+1);
			re.h=max(0,min(y2(),_ps.y2())-re.y+1);
			return re;
		}
		
		inline Posize operator | (const Posize &_ps) const
		{
			using namespace std;
			if (_ps.w*_ps.h==0)
				return *this;
			if (w*h==0)
				return _ps;
			Posize re;
			re.x=min(x,_ps.x);
			re.y=min(y,_ps.y);
			re.w=max(0,max(x2(),_ps.x2())-re.x+1);
			re.h=max(0,max(y2(),_ps.y2())-re.y+1);
			return re;
		}
		
		inline Posize& operator |= (const Posize &_ps)
		{
			using namespace std;
			if (_ps.w*_ps.h==0)
				return *this;
			Posize re;
			if (w*h==0)
				re=_ps;
			else
			{
				re.x=min(x,_ps.x);
				re.y=min(y,_ps.y);
				re.w=max(0,max(x2(),_ps.x2())-re.x+1);
				re.h=max(0,max(y2(),_ps.y2())-re.y+1);
			}
			x=re.x;y=re.y;w=re.w;h=re.h;
			return *this;
		}
		
		inline Posize operator + (const Posize &_ps) const
		{return Posize(x+_ps.x,y+_ps.y,w,h);}
		
		inline Posize operator + (const Point &_pt) const
		{return Posize(x+_pt.x,y+_pt.y,w,h);}
		
		inline Posize operator - (const Posize &_ps) const
		{return Posize(x-_ps.x,y-_ps.y,w,h);}
		
		inline Posize operator - (const Point &_pt) const
		{return Posize(x-_pt.x,y-_pt.y,w,h);}
		
		inline Posize& operator += (const Posize &_ps)
		{
			x+=_ps.x;y+=_ps.y;
			return *this;
		}
		
		inline Posize& operator += (const Point &_pt)
		{
			x+=_pt.x;y+=_pt.y;
			return *this;
		}
		
		inline Posize& operator -= (const Posize &_ps)
		{
			x-=_ps.x;y-=_ps.y;
			return *this;
		}
		
		inline Posize& operator -= (const Point &_pt)
		{
			x-=_pt.x;y-=_pt.y;
			return *this;
		}
		
		inline bool operator == (const Posize &_ps) const
		{return x==_ps.x&&y==_ps.y&&w==_ps.w&&h==_ps.h;}
		
		inline bool operator != (const Posize &_ps) const
		{return x!=_ps.x||y!=_ps.y||w!=_ps.w||h!=_ps.h;}
		
		inline Posize operator * (double lambda) const
		{return Posize(x*lambda+0.5,y*lambda+0.5,w*lambda+0.5,h*lambda+0.5);}
		
		inline Posize operator * (float lambda) const
		{return Posize(x*lambda+0.5,y*lambda+0.5,w*lambda+0.5,h*lambda+0.5);}
		
		inline Posize operator / (double lambda) const
		{return Posize(x/lambda,y/lambda,w/lambda,h/lambda);}
		
		inline Posize operator / (float lambda) const
		{return Posize(x/lambda,y/lambda,w/lambda,h/lambda);}
		
		inline Posize Expand(const int &d) const
		{return Posize(x-d,y-d,w+2*d,h+2*d);}
		
		inline Posize Shrink(const int &d) const
		{return Posize(x+d,y+d,w-2*d,h-2*d);}
		
		inline Posize Flexible(const double &px,const double &py) const
		{return Posize(x*px+0.5,y*py+0.5,w*px+0.5,h*py+0.5);}
		
		Posize() {}
		Posize(const int &_x,const int &_y,const int &_w,const int &_h):x(_x),y(_y),w(_w),h(_h) {}
		Posize(const Point &pt,int _w,int _h):x(pt.x),y(pt.y),w(_w),h(_h) {}
	};

	const Range ZERO_RANGE={0,0};
	const Point ZERO_POINT={0,0};
	const Posize ZERO_POSIZE={0,0,0,0};
	const Posize LARGE_POSIZE={-100000000,-100000000,200000000,200000000};

	Point MakePoint(int x,int y)//NotRecommended
	{return Point(x,y);}

	Posize MakePosize(int x,int y,int w,int h)//NotRecommended
	{return Posize(x,y,w,h);}
}
#endif

#if PAL_LibraryUseSDL == 1
#ifndef PAL_BASICFUNCTIONS_POSIZE_CPP_WITHSDL
#define PAL_BASICFUNCTIONS_POSIZE_CPP_WITHSDL 1
#include PAL_SDL_HeaderPath

namespace PAL::Legacy
{
	inline void PosizeToSDLRect(const Posize &src,SDL_Rect &tar)
	{
		tar.x=src.x;
		tar.y=src.y;
		tar.w=src.w;
		tar.h=src.h;
	}

	inline const SDL_Rect PosizeToSDLRect(const Posize &src)
	{return {src.x,src.y,src.w,src.h};}

	inline const SDL_Point PointToSDLPoint(const Point &src)
	{return {src.x,src.y};}

	inline void SDLRectToPosize(const SDL_Rect &src,Posize &tar)
	{
		tar.x=src.x;
		tar.y=src.y;
		tar.w=src.w;
		tar.h=src.h;
	}

	inline Posize SDLRectToPosize(const SDL_Rect &src)
	{return Posize(src.x,src.y,src.w,src.h);}

	inline Point SDLPointToPoint(const SDL_Point &src)
	{return Point(src.x,src.y);}
}
#endif
#endif
