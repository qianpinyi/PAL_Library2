#ifndef PAL_SCREENCAPTURE_CPP
#define PAL_SCREENCAPTURE_CPP 1

#include "../PAL_BuildConfig.h"

#include PAL_SDL_HeaderPath

#include "../PAL_BasicFunctions/PAL_Posize.cpp"
#include "../PAL_BasicFunctions/PAL_Debug.cpp"

#if PAL_CurrentPlatform == PAL_Platform_Windows
	#include <Windows.h>
	#include <MissingHeader/Magnification.h>
#endif

class ScreenCapture//??
{
	public:
		enum
		{
			ERR_NONE=0,
			ERR_SurfaceIsNULL,
			ERR_InitFailed,
			ERR_HaveBeenInited,
			ERR_NotInited,
			ERR_CannotExcludeNULL,
			ERR_CannotExclude,
			ERR_CaptureFailed
		};
		
	protected:
		#if PAL_CurrentPlatform == PAL_Platform_Windows
	    HWND hostWindow=NULL;
	    HWND magnifierWindow=NULL;
		#endif
		SDL_Surface *sur=NULL;
		void (*capOKfunc)(void*,const SDL_Surface*,const Posize&)=NULL;//Be carefull,it is called in other thread! And sur may be NULL!
		void *funcdata=NULL;
		Posize LastCaptureArea;
		bool Inited=0;
		
		#if PAL_CurrentPlatform == PAL_Platform_Windows
		static DWORD GetTlsIndex()
		{
			static const DWORD tls_index=TlsAlloc();
		    return tls_index;
		}
		
		static BOOL WINAPI ImageScalingCallback(HWND hwnd,void* srcdata,MAGIMAGEHEADER srcheader,void* destdata,MAGIMAGEHEADER destheader,RECT unclipped,RECT clipped,HRGN dirty)
		{
		    ScreenCapture *p=reinterpret_cast<ScreenCapture*>(TlsGetValue(GetTlsIndex()));
		    TlsSetValue(GetTlsIndex(),nullptr);
			if (srcheader.cbSize==4*srcheader.width*srcheader.height)
			{
				SDL_Surface *tmp=SDL_CreateRGBSurfaceFrom(srcdata,srcheader.width,srcheader.height,32,srcheader.cbSize/srcheader.height,0,0,0,0);//??
				SDL_PixelFormat *format = SDL_AllocFormat(SDL_PIXELFORMAT_RGBA32);
				p->sur=SDL_ConvertSurface(tmp,format,0);
				SDL_FreeFormat(format);
				SDL_FreeSurface(tmp);
			}
			if (p->capOKfunc!=NULL)
		    	p->capOKfunc(p->funcdata,p->sur,p->LastCaptureArea);
			return TRUE;
		}
		#endif
		
	public:
		inline Posize GetLastCaptureArea() const
		{return LastCaptureArea;}
		
		inline SDL_Surface* GetLastCaptureResult()
		{return sur;}
		
		inline int FreeLastCaptureResult()
		{
			if (sur==NULL)
				return ERR_SurfaceIsNULL;
			SDL_FreeSurface(sur);
			sur=NULL;
			return 0;
		}
		
		inline SDL_Surface* DeliverCaptureResult()
		{
			if (sur==NULL)
				return NULL;
			SDL_Surface *re=sur;
			sur=NULL;
			return re;
		}
		
		int Capture(const Posize &screenArea=ZERO_POSIZE)//default fullScreen
		{
			if (!Inited)
				return ERR_NotInited;
			FreeLastCaptureResult();
			#if PAL_CurrentPlatform == PAL_Platform_Windows
			Posize screenPs=Posize(GetSystemMetrics(SM_XVIRTUALSCREEN),GetSystemMetrics(SM_YVIRTUALSCREEN),GetSystemMetrics(SM_CXVIRTUALSCREEN),GetSystemMetrics(SM_CYVIRTUALSCREEN));
			Posize &ps=LastCaptureArea;
			if (screenArea==ZERO_POSIZE)
				ps=screenPs;
		    else ps=screenArea;
		    if (ps.Size()<=0)
				return ERR_CaptureFailed;
//			DD[3]<<"Capture "<<ps.x<<" "<<ps.y<<" "<<ps.w<<" "<<ps.h<<endl;
			if (!SetWindowPos(magnifierWindow,NULL,ps.x,ps.y,ps.w,ps.h,0))
				return DD[2]<<"Failed to set magnifierWindow's pos! Error code: "<<GetLastError()<<endl,ERR_CaptureFailed;
		    RECT rect={ps.x,ps.y,ps.x+ps.w,ps.y+ps.h};
		    TlsSetValue(GetTlsIndex(),this);
			if (!MagSetWindowSource(magnifierWindow,rect))
				return DD[2]<<"MagSetWindowSource failed! ErrorCode: "<<GetLastError()<<endl,ERR_CaptureFailed;
//			DD[3]<<"CaptureOK"<<endl;
			return 0;
			#endif
			return ERR_CaptureFailed;
		}
		
		inline SDL_Surface* CaptureSurface(const Posize &screenArea=ZERO_POSIZE)
		{
			if (Capture()==0)
				return sur;
			return NULL;
		}
		
		#if PAL_CurrentPlatform == PAL_Platform_Windows
		int SetExcludeWindows(HWND hs[],int cnt)
		{
	        if (!MagSetWindowFilterList(magnifierWindow,MW_FILTERMODE_EXCLUDE,cnt,hs))
	            return DD[2]<<"Failed to set excluded window! Error code: "<<GetLastError()<<endl,ERR_CannotExclude;
	        return 0;
		}
		
		inline int SetExcludeWindow(HWND h)
		{
			if (!h) return ERR_CannotExcludeNULL;
			return SetExcludeWindows(&h,1);
		}
		#endif
		
//		void SetCaptureOKFunc(void (*_func)(void*,const SDL_Surface*,const Posize&),void *_funcdata)//??
//		{
//			capOKfunc=_func;
//			funcdata=_funcdata;
//		}
		
		int Init()
		{
			if (Inited)
				return ERR_HaveBeenInited;
			#if PAL_CurrentPlatform == PAL_Platform_Windows
		    if (!MagInitialize())
		    	return DD[2]<<"MagInitialize faild! Error code: "<<GetLastError()<<endl,ERR_InitFailed;
		 
		    HMODULE hInstance=nullptr;
		    if (!GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,reinterpret_cast<char*>(&DefWindowProc),&hInstance))
			{
				MagUninitialize();
				DD[2]<<"Failed to GetModuleHandleEX! Error code "<<GetLastError()<<endl;
				return ERR_InitFailed;
		    }

		    WNDCLASSEXW wcex={};//??
		    wcex.cbSize=sizeof(WNDCLASSEX);
		    wcex.lpfnWndProc=&DefWindowProc;
		    wcex.hInstance=hInstance;
		    wcex.hCursor=LoadCursor(nullptr,IDC_ARROW);
		    wcex.lpszClassName=L"ScreenCaptureMagnifierHost";
		    RegisterClassExW(&wcex);

		    hostWindow=CreateWindowExW(WS_EX_LAYERED,L"Magnifier",L"MagnifierHost",0,0,0,0,0,nullptr,nullptr,hInstance,nullptr);
		    if (!hostWindow)
			{
				MagUninitialize();
				DD[2]<<"Failed to create window MagnifierHost! Error code:"<<GetLastError()<<endl;
		        return ERR_InitFailed;
		    }
			
		    magnifierWindow=CreateWindowW(L"Magnifier",L"MagnifierWindow",WS_CHILD|WS_VISIBLE,0,0,0,0,hostWindow,nullptr,hInstance,nullptr);
		    if (!magnifierWindow)
			{
				MagUninitialize();
				DD[2]<<"Failed to create window MagnifierHost! Error code:"<<GetLastError()<<endl;
		        return ERR_InitFailed;
		    }
		 
		    ShowWindow(hostWindow,SW_HIDE);
			
		    if (!MagSetImageScalingCallback(magnifierWindow,ImageScalingCallback))
			{
				MagUninitialize();
				DD[2]<<"Failed to set image scaling callback! Error code: "<<GetLastError()<<endl;
				return ERR_InitFailed;
		    }
		    
		    //Is there the error that the created window was not released when error happened??
		    Inited=1;
			return 0;
			#endif
			return ERR_InitFailed;
		}
		
		ScreenCapture() {}
};

#endif
