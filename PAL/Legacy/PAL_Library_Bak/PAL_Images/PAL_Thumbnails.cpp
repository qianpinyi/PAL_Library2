#ifndef PAL_THUMBNAILS_CPP
#define PAL_THUMBNAILS_CPP

/*
	...
	22.1.23: Update Thumbnail file operation from fstream to BasicIO(BasicBinaryIO)
	22.3.13: ThumbnailFileV2;
*/
#include "../PAL_BasicFunctions/PAL_BasicFunctions_0.cpp"
#include "../PAL_BasicFunctions/PAL_Debug.cpp"
#include "../PAL_BasicFunctions/PAL_Color.cpp"
#include "../PAL_BasicFunctions/PAL_StringEX.cpp"
#include "../PAL_BasicFunctions/PAL_File.cpp"
#include "../PAL_DataStructure/PAL_Tuple.cpp"

#include PAL_SDL_imageHeaderPath

#include <fstream>
#include <cmath>
#include <vector>
#include <atomic>

class ThumbnailPic
{
	protected:
		std::string path;
		bool IsRelativePath=1;
		int w=0,h=0;
		RGBA *pic=NULL;
		SDL_Surface *sur=NULL;//SDL_PIXELFORMAT_RGBA32; Freed when deconstructed;
		int *count=NULL;
		int version=1;
		
		void Deconstruct()
		{
			if (count!=NULL)
			{
				if (--*count==0)
				{
					SDL_FreeSurface(sur);
					delete[] pic;
					delete count;
				}
				sur=NULL;
				pic=NULL;
				count=NULL;
				w=h=0;
			}
		}
		
		void CopyFrom(const ThumbnailPic &tar)
		{
			path=tar.path;
			IsRelativePath=tar.IsRelativePath;
			w=tar.w;
			h=tar.h;
			pic=tar.pic;
			sur=tar.sur;
			count=tar.count;
			if (count!=NULL)
				++*count;
		}
		
		void Construct()
		{
			count=new int(1);
			#if SDL_BYTEORDER == SDL_BIG_ENDIAN
			Uint32 Rmask=0xff000000,
				   Gmask=0x00ff0000,
				   Bmask=0x0000ff00,
				   Amask=0x000000ff;
			#else
			Uint32 Rmask=0x000000ff,
				   Gmask=0x0000ff00,
				   Bmask=0x00ff0000,
				   Amask=0xff000000;
			#endif
			sur=SDL_CreateRGBSurfaceFrom(pic,w,h,4*8,w*4,Rmask,Gmask,Bmask,Amask);
			SDL_SetSurfaceBlendMode(sur,SDL_BLENDMODE_BLEND);
		}
		
	public:
		int Read(BasicBinIO &bio)
		{
			bio>>version;
			if (version==1)
			{
				Deconstruct();
				bio>>path>>IsRelativePath>>w>>h;
				pic=new RGBA[h*w];
				bio.ReadN(pic,w*h*sizeof(RGBA));
				Construct();
				if (bio.LastError())
					return 2;
				else return 0;
			}
			else return 1;
		}
		
		int Write(BasicBinIO &bio)
		{
			bio<<version<<path<<IsRelativePath<<w<<h<<DataWithSize(pic,w*h*sizeof(RGBA));
			return bio.LastError()?2:0;
		}
		
		inline SDL_Surface* operator () ()
		{return sur;}
		
		inline SDL_Surface* GetPicSurface()
		{return sur;}
		
		inline bool operator ! ()
		{return count==NULL;}
		
		inline std::string GetPath()
		{return path;}
		
		inline int GetW()
		{return w;}
		
		inline int GetH()
		{return h;}
		
		inline int GetIsRelativePath()
		{return IsRelativePath;}
		
		inline void SetPath(const std::string &_path,bool isRelativePath)
		{
			path=_path;
			IsRelativePath=isRelativePath;
		}
		
		void SetPic(int _w,int _h,RGBA *_pic)
		{
			Deconstruct();
			w=_w;
			h=_h;
			pic=_pic;
			Construct();
		}
		
		~ThumbnailPic()
		{Deconstruct();}
		
		ThumbnailPic& operator = (const ThumbnailPic &tar)
		{
			if (&tar==this)
				return *this;
			Deconstruct();
			CopyFrom(tar);
			return *this;
		}
		
		ThumbnailPic(const ThumbnailPic &tar)
		{CopyFrom(tar);}
		
		ThumbnailPic(const std::string &_path,bool isRelativePath,int _w,int _h,RGBA *_pic)
		:path(_path),IsRelativePath(isRelativePath),w(_w),h(_h),pic(_pic) {Construct();}
		
		ThumbnailPic(const std::string &_path,int _w,int _h,RGBA *_pic)
		:path(_path),w(_w),h(_h),pic(_pic) {Construct();}
		
		ThumbnailPic(int _w,int _h,RGBA *_pic)
		:w(_w),h(_h),pic(_pic) {Construct();}
		
		ThumbnailPic() {}
};

struct ThumbnailFile
{
	//Version(int)|FatherPathString|TotCnt(int)|ThumbnailData
	int Version=1;
	std::string faPath;
	std::vector <ThumbnailPic> PicData;
	
	ThumbnailPic FindFirstByName(const std::string &name)
	{
		for (auto vp:PicData)
			if (GetLastAfterBackSlash(vp.GetPath())==name)
				return vp;
		return ThumbnailPic();
	}
	
	ThumbnailPic FindByPath(const std::string &path)
	{
		for (auto &vp:PicData)
			if (vp.GetPath()==path)
				return vp;
		return ThumbnailPic();
	}
	
	void ClearAll()
	{
		faPath.clear();
		PicData.clear();
	}
	
	void ClearThumb()
	{PicData.clear();}
	
	int Read(BasicBinIO &bio)
	{
		bio>>Version;
		if (Version==1)
		{
			int cnt=0;
			bio>>faPath>>cnt;
			for (int i=1,returnCode;i<=cnt;++i)
			{
				ThumbnailPic pic;
				if (returnCode=pic.Read(bio))
					return returnCode;
				PicData.push_back(pic);
			}
			return 0;
		}
		else return 1;
	}
	
	int Write(BasicBinIO &bio)
	{
		bio<<Version<<faPath<<(int)PicData.size();
		int returnCode;
		for (auto &vp:PicData)
			if (returnCode=vp.Write(bio))
				return returnCode;
		return 0;
	}
};

ThumbnailPic GetThumbnail(SDL_Surface *sur0,int w,int h)
{
	ThumbnailPic re;
//	re.SetPath(picPath,0);
	RGBA *pic=new RGBA[h*w];
	RGBA co0;

	SDL_Surface *sur=SDL_ConvertSurfaceFormat(sur0,SDL_PIXELFORMAT_RGBA32,0);//Bad in efficiency?
//	SDL_FreeSurface(sur0);
	for (int i=0;i<h;++i)
		for (int j=0;j<w;++j)
		{
			double per=std::min(1.0,std::min(w*1.0/sur->w,h*1.0/sur->h));
			int sumr=0,sumg=0,sumb=0,suma=0,siz=0;
			double sx=(j-w/2.0)/per,tx=(j+1-w/2.0)/per,sy=(i-h/2.0)/per,ty=(i+1-h/2.0)/per;
			
			for (int y=sy+sur->h/2.0;y<ty+sur->h/2.0;++y)
				for (int x=sx+sur->w/2.0;x<tx+sur->w/2.0;++x)
				{
					if (InRange(x,0,sur->w-1)&&InRange(y,0,sur->h-1))
						SDL_GetRGBA(*(Uint32*)(sur->pixels+y*sur->pitch+x*sur->pitch/sur->w),sur->format,&co0.r,&co0.g,&co0.b,&co0.a);
					else co0={255,255,255,0};
					sumr+=co0.r;
					sumg+=co0.g;
					sumb+=co0.b;
					suma+=co0.a;
					siz+=1;
				}
			
			if (siz!=0)
				pic[i*w+j]=RGBA(sumr/siz,sumg/siz,sumb/siz,suma/siz);
		}
	SDL_FreeSurface(sur);
	re.SetPic(w,h,pic);
	return re;
}

ThumbnailPic GetThumbnail(const std::string &picPath,int w,int h)
{
	using namespace PAL_Lib1;
	DD[0]<<"GetThumbnail of "<<picPath<<endl;
	SDL_Surface *sur0=IMG_Load(picPath.c_str());
	if (sur0!=nullptr)
	{
		ThumbnailPic re=GetThumbnail(sur0,w,h);
		re.SetPath(picPath,0);
		SDL_FreeSurface(sur0);
		return re;
	}
	else return ThumbnailPic();
}

PAL_DS::Doublet<ThumbnailPic,SDL_Surface*> GetThumbnailAndRawpic(const std::string &picPath,int w,int h)
{
	using namespace PAL_Lib1;
	DD[0]<<"GetThumbnailAndRawpic of "<<picPath<<endl;
	SDL_Surface *sur0=IMG_Load(picPath.c_str());
	if (sur0!=nullptr)
	{
		ThumbnailPic re=GetThumbnail(sur0,w,h);
		re.SetPath(picPath,0);
		return {re,sur0};
	}
	else return {ThumbnailPic(),nullptr};
}

//Version2
class ThumbnailFileV2
{
	public:
		enum
		{
			Attribute_None=0,
			Attribute_RGBA=1,
//			Attribute_PNG=2,
//			Attribute_JPG=4
		};
		
		struct ThumbnailPicture
		{
			unsigned Version=2;
			std::string name;
			unsigned long long timecode=0;//0 means unknown...
			unsigned short w,h;
			unsigned attribute;
			unsigned size=0;//Size of data in bytes
			
			RGBA *pic=nullptr;
			SDL_Surface *sur=nullptr;
			
			void GetSurFromPic()
			{
				if (pic==nullptr||sur!=nullptr) return;
				sur=SDL_CreateRGBSurfaceWithFormatFrom(pic,w,h,32,w*4,SDL_PIXELFORMAT_RGBA32);
				SDL_SetSurfaceBlendMode(sur,SDL_BLENDMODE_BLEND);
			}
			
			inline SDL_Surface* CopySurface()
			{
				if (sur==nullptr) return nullptr;
				else return SDL_ConvertSurfaceFormat(sur,SDL_PIXELFORMAT_RGBA32,0);//??
			}
			
			int Read(BasicBinIO &io)
			{
				io>>Version>>name>>timecode>>w>>h>>attribute>>size;
				if (attribute&Attribute_RGBA)
				{
					pic=new RGBA[size/sizeof(RGBA)];
					io.ReadN(pic,size);
					GetSurFromPic();
				}
				else ;
				return io.LastError()!=0;
			}
			
			int Write(BasicBinIO &io)
			{
				io<<Version<<name<<timecode<<w<<h<<attribute<<size;
				if (attribute&Attribute_RGBA)
					io.WriteN(pic,size);
				else ;
				return io.LastError()!=0;
			}
			
			~ThumbnailPicture()
			{
				if (sur!=nullptr)
					SDL_FreeSurface(sur),
					sur=nullptr;
				if (attribute&Attribute_RGBA)
					DELETEtoNULL(pic);
			}
		};
		
	protected:
		std::map <std::string,ThumbnailPicture*> ma;
		std::string BasePath,RepresentivePicName;
		unsigned Version=2,Attribute=0;
		
	public:
		static ThumbnailPicture* GetThumbnail(SDL_Surface *src,int w,int h,const std::string &name,bool deleteSrc=0,const std::atomic_int *quitflag=nullptr)
		{
			if (src==nullptr)
				return nullptr;
			
			ThumbnailPicture *thumb=new ThumbnailPicture();
			thumb->name=name;
			thumb->w=w;
			thumb->h=h;
			thumb->size=w*h*sizeof(RGBA);
			thumb->attribute=Attribute_RGBA;
			RGBA *pic=thumb->pic=new RGBA[h*w];
			
			RGBA co0;
			SDL_Surface *sur=SDL_ConvertSurfaceFormat(src,SDL_PIXELFORMAT_RGBA32,0);//Bad in efficiency??
			for (int i=0;i<h&&quitflag!=nullptr&&*quitflag!=1;++i)
				for (int j=0;j<w;++j)
				{
					double per=std::min(1.0,std::min(w*1.0/sur->w,h*1.0/sur->h));
					int sumr=0,sumg=0,sumb=0,suma=0,siz=0;
					double sx=(j-w/2.0)/per,tx=(j+1-w/2.0)/per,sy=(i-h/2.0)/per,ty=(i+1-h/2.0)/per;
					
					for (int y=sy+sur->h/2.0;y<ty+sur->h/2.0;++y)
						for (int x=sx+sur->w/2.0;x<tx+sur->w/2.0;++x)
						{
							if (InRange(x,0,sur->w-1)&&InRange(y,0,sur->h-1))
								SDL_GetRGBA(*(Uint32*)(sur->pixels+y*sur->pitch+x*sur->pitch/sur->w),sur->format,&co0.r,&co0.g,&co0.b,&co0.a);
							else co0={255,255,255,0};
							sumr+=co0.r;
							sumg+=co0.g;
							sumb+=co0.b;
							suma+=co0.a;
							siz+=1;
						}
					
					if (siz!=0)
						pic[i*w+j]=RGBA(sumr/siz,sumg/siz,sumb/siz,suma/siz);
				}
			SDL_FreeSurface(sur);
			
			if (quitflag!=nullptr&&*quitflag==1)
				DeleteToNULL(thumb);
			if (thumb!=nullptr)
				thumb->GetSurFromPic();
			if (deleteSrc)
				SDL_FreeSurface(src);
			return thumb;
		}
		
		inline static ThumbnailPicture* GetThumbnail(const std::string &picPath,int w,int h,const std::atomic_int *quitflag=nullptr)
		{
			SDL_Surface *src=IMG_Load(picPath.c_str());
			return GetThumbnail(src,w,h,GetLastAfterBackSlash(picPath),1,quitflag);
		}
		
		inline static PAL_DS::Doublet <ThumbnailPicture*,SDL_Surface*> GetThumbnailAndRawPic(const std::string &picPath,int w,int h,const std::atomic_int *quitflag=nullptr)
		{
			SDL_Surface *src=IMG_Load(picPath.c_str());
			return {GetThumbnail(src,w,h,GetLastAfterBackSlash(picPath),0,quitflag),src};
		}
		
		static ThumbnailPicture* ReadRepresentiveThumb(BasicBinIO &io)
		{
			unsigned version=2,Attribute=0,cnt=0;
			io>>version;
			if (version==2)
			{
				std::string str;
				io>>str>>str>>Attribute>>cnt;
				if (str!=""&&cnt!=0)
				{
					ThumbnailPicture *thumb=new ThumbnailPicture();
					if (thumb->Read(io))
						delete thumb;
					else return thumb;
				}
			}
			return nullptr;
		}
		
		inline const std::string& GetBasePath() const
		{return BasePath;}
		
		inline void SetBasePath(const std::string &basepath)
		{BasePath=basepath;}
		
		ThumbnailPicture* Find(const std::string &name)
		{
			auto mp=ma.find(name);
			if (mp!=ma.end())
				return mp->second;
			else return nullptr;
		}
		
		int Insert(const std::string &name,ThumbnailPicture *thumb,bool replaceOld=0)
		{
			if (thumb==nullptr) return 1;
			auto mp=ma.find(name!=""?name:thumb->name);
			if (mp!=ma.end())
				if (replaceOld)
				{
					delete mp->second;
					mp->second=thumb;
				}
				else return 2;
			else ma[name!=""?name:thumb->name]=thumb;
			return 0;
		}
		
		inline int Insert(ThumbnailPicture *thumb,bool replaceOld=0)
		{return Insert("",thumb,replaceOld);}
		
		int Erase(const std::string &name)
		{
			auto mp=ma.find(name);
			if (mp!=ma.end())
			{
				delete mp->second;
				ma.erase(mp);
				return 0;
			}
			else return 1;
		}
		
		void Clear(bool ClearThumbOnly=1)
		{
			for (auto &mp:ma)
				delete mp.second;
			ma.clear();
			if (!ClearThumbOnly)
				BasePath=RepresentivePicName="",
				Attribute=0,
				Version=2;
		}
		
		inline SDL_Surface* GetSurface(const std::string &name)
		{
			auto thumb=Find(name);
			if (thumb!=nullptr)
				return thumb->sur;
			else return nullptr;
		}
		
		inline SDL_Surface* CopySurface(const std::string &name)
		{
			auto thumb=Find(name);
			if (thumb!=nullptr)
				return thumb->CopySurface();
			else return nullptr;
		}
		
		int Read(BasicBinIO &io)
		{
			io>>Version;
			unsigned cnt=0;
			if (Version==2)
			{
				io>>BasePath>>RepresentivePicName>>Attribute>>cnt;
				for (int i=1;i<=cnt;++i)
				{
					ThumbnailPicture *thumb=new ThumbnailPicture();
					if (thumb->Read(io))
					{
						delete thumb;
						return 3;
					}
					else ma[thumb->name]=thumb;
				}
				return io.LastError()==0?0:2;
			}
			else if (Version==1)
			{
				unsigned cnt=0;
				io>>BasePath>>cnt;
				for (int i=1;i<=cnt;++i)
				{
					int ver,w,h;
					bool rela;
					std::string path;
					io>>ver;
					if (ver==1)
					{
						io>>path>>rela>>w>>h;
						if (rela&&ma.find(path)==ma.end())
						{
							ThumbnailPicture *thumb=new ThumbnailPicture();
							thumb->name=path;
							thumb->timecode=0;
							thumb->w=w;
							thumb->h=h;
							thumb->attribute=Attribute_RGBA;
							thumb->size=w*h*4;
							thumb->pic=new RGBA[h*w];
							io.ReadN(thumb->pic,thumb->size);
							thumb->GetSurFromPic();
							if (io.LastError()!=0)
							{
								delete thumb;
								return 4;
							}
							else ma[thumb->name]=thumb;
						}
						else return 7;
					}
					else return 6;
				}
				Version=2;
				return io.LastError()==0?0:5;
			}
			else return 1;
		}
		
		int Write(BasicBinIO &io)
		{
			io<<2<<BasePath<<RepresentivePicName<<Attribute<<(unsigned)ma.size();
			if (RepresentivePicName!="")
			{
				auto mp=ma.find(RepresentivePicName);
				if (mp!=ma.end())
					if (mp->second->Write(io))
						return 1;
			}
			for (auto &mp:ma)
				if (mp.first!=RepresentivePicName)
					if (mp.second->Write(io))
						return 2;
			return io.LastError()==0?0:3;
		}
		
		~ThumbnailFileV2()
		{
			Clear();
		}
};

#endif
