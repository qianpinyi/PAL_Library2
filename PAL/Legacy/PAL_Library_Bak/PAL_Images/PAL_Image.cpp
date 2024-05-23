#ifndef PAL_IMAGE_CPP
#define PAL_IMAGE_CPP 1

#include "../PAL_BuildConfig.h"

#include "../PAL_BasicFunctions/PAL_BasicFunctions_0.cpp"
#include "../PAL_BasicFunctions/PAL_Color.cpp"

#include <string>

#if PAL_LibraryUseSDL == 1
	#include PAL_SDL_headerPath
	#include PAL_SDL_imageHeaderPath
#endif

namespace PAL_Image
{
	class RGBAImage
	{
		friend RGBAImage LoadRGBAImage(const std::string&);
		protected:
			RGBA *data=nullptr;
			unsigned w=0,h=0;
			
			inline void Deconstruct()
			{
				if (data!=nullptr)
					DELETEtoNULL(data);
				w=h=0;
			}
			
			void CopyFrom(const RGBAImage &tar)
			{
				Deconstruct();
				w=tar.h;
				h=tar.h;
				data=new RGBA[w*h];
				memcpy(data,tar.data,sizeof(RGBA)*w*h);
			}
			
		public:
			inline bool operator ! () const
			{return data==nullptr;}
			
			inline bool Valid() const
			{return data!=nullptr;}
			
			inline RGBA* operator [] (int row)
			{return data+row*w;}
			
			inline RGBA& operator () (int row,int col)
			{return data[row*w+col]}
			
//			inline RGBA* Data()
//			{return data;}
			
			inline unsigned W() const
			{return w;}
			
			inline unsigned H() const
			{return h;}
			
			inline unsigned Size() const
			{return w*h;}
			
			inline unsigned Bytes() const
			{return sizeof(RGBA)*w*h;}
	
			RGBAImage& operator = (const RGBAImage &tar) const
			{
				if (&tar!=this)
					CopyFrom(tar);
				return *this;
			}
			
			RGBAImage(const RGBAImage &tar)
			{CopyFrom(tar);}
			
			RGBAImage(unsigned _w,unsigned _h,const RGBA &initColor=RGBA_BLACK):w(_w),h(_h)
			{
				data=new RGBA[w*h];
				MemsetT(data,initColor,w*h);
			}
			
			RGBAImage() {}
			
			~RGBAImage()
			{Deconstruct();}
	};

	#if PAL_LibraryUseSDL == 1
	RGBAImage LoadRGBAImage(const std::string &path)//It may be slow...
	{
		RGBAImage img;
		SDL_Surface *sur=IMG_Load(path.c_str());
		if (sur!=NULL)
		{
			img=RGBAImage(sur->w,sur->h);
			SDL_PixelFormat *format=SDL_AllocFormat(SDL_PIXELFORMAT_RGBA32);
			SDL_Surface *sur2=SDL_ConvertSurface(tmp,format,0);
			memcpy(img.data,sur2->data,img.Bytes());
			SDL_FreeFormat(format);
			SDL_FreeSurface(sur);
			SDL_FreeSurface(sur2);
		}
		return img;
	}
	#endif

};

#endif
