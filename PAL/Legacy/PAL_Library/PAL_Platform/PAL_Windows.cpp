#ifndef PAL_PLATFORM_WINDOWS_CPP
#define PAL_PLATFORM_WINDOWS_CPP 1

#include "PAL_PlatformCommon.cpp"
#include "../PAL_BasicFunctions/PAL_Charset.cpp"
#include "../PAL_BasicFunctions/PAL_StringEX.cpp"

#include <vector>
#include <string>

#include <Windows.h>
#include <shlobj.h>
#include <shellapi.h>

#include <commctrl.h>
#include <commoncontrols.h>

namespace PAL::Legacy::PAL_Platform
{
	#if PAL_LibraryUseSDL == 1
		#include PAL_SDL_HeaderPath
		
		SDL_Surface* HBITMAPToSDLSurface(HBITMAP hbmp,bool autoDelete=0,bool verticalReverse=0)
		{
			if (hbmp==NULL)
				return nullptr;
			SDL_Surface *re=NULL;
			BITMAP bmp{0};
			GetObject(hbmp,sizeof(BITMAP),&bmp);
			if (bmp.bmBits!=NULL)
			{
				re=SDL_CreateRGBSurfaceWithFormat(0,bmp.bmWidth,bmp.bmHeight,32,SDL_PIXELFORMAT_BGRA32);
				SDL_SetSurfaceBlendMode(re,SDL_BLENDMODE_BLEND);
				for (int y=0;y<re->h;++y)
					memcpy((Uint32*)re->pixels+y*re->pitch/4,(Uint32*)bmp.bmBits+(verticalReverse?y:re->h-1-y)*re->pitch/4,re->pitch);
			}
			if (autoDelete)
				DeleteObject(hbmp);
			return re;
		}
		
		SDL_Surface* HICONToSDLSurface(HICON hicon,bool autoDestroy=0)
		{
			if (hicon==NULL)
				return nullptr;
			SDL_Surface *re=NULL;
			ICONINFO info{0};
		    if (GetIconInfo(hicon,&info))
		    	re=HBITMAPToSDLSurface((HBITMAP)CopyImage(info.hbmColor,IMAGE_BITMAP,0,0,LR_CREATEDIBSECTION),1);
			if (autoDestroy)
				DestroyIcon(hicon);
			return re;
		}
		
		SDL_Surface*GetFileItemThumbnail(const std::string &path,int size)
		{
			SDL_Surface *re=nullptr;
			IShellItemImageFactory *isiif=nullptr;
			if (SHCreateItemFromParsingName(Charset::Utf8ToUnicode(path).c_str(),NULL,IID_IShellItemImageFactory,(void**)&isiif)==S_OK)
			{
				HBITMAP hbmp;
				if (isiif->GetImage({size,size},SIIGBF_THUMBNAILONLY,&hbmp)==S_OK)
					re=HBITMAPToSDLSurface(hbmp,1,1);
				isiif->Release();
			}
			return re;
		}
		
		SDL_Surface* GetFileItemIcon(const std::string &path,int size)//Need CoInitialize(0);
		{
			SDL_Surface *re=nullptr;
			IShellItemImageFactory *isiif=nullptr;
			if (SHCreateItemFromParsingName(Charset::Utf8ToUnicode(path).c_str(),NULL,IID_IShellItemImageFactory,(void**)&isiif)==S_OK)
			{
				HBITMAP hbmp;
				if (isiif->GetImage({size,size},SIIGBF_ICONONLY,&hbmp)==S_OK)
					re=HBITMAPToSDLSurface(hbmp,1);
				isiif->Release();
			}
			return re;
		}
		
		SDL_Surface* GetFileIcon_SHGetFileInfo(const std::string &str,unsigned flag,unsigned attribute,unsigned sizeLevel)//0:Small 1:Large 2:ExtraLarge 3:Jumbo ; It is not safe to call in multitread at a time, should be mutex-protected
		{
			SHFILEINFOA sfi{0};
			
			if (sizeLevel==0) flag|=SHGFI_SMALLICON;
			else if (sizeLevel==1) flag|=SHGFI_LARGEICON;
			else flag|=SHGFI_SYSICONINDEX;

			if(SUCCEEDED(SHGetFileInfoA(str.c_str(),attribute,&sfi,sizeof(sfi),flag)))
				if(sizeLevel>=2)
				{
					HICON hIcon=0;
					HIMAGELIST* imageList;
					if (SUCCEEDED(SHGetImageList((sizeLevel!=2?SHIL_JUMBO:SHIL_EXTRALARGE),IID_IImageList,(void**)&imageList)))
					{
						((IImageList*)imageList)->GetIcon(sfi.iIcon,ILD_TRANSPARENT,&hIcon);
						return HICONToSDLSurface(hIcon,1);
					}
					else return nullptr;
				}
				else return HICONToSDLSurface(sfi.hIcon,1);
			else return nullptr;
		}
		
		inline SDL_Surface* GetFileTypeIcon(const std::string &aftername,unsigned sizeLevel)
		{
			if (aftername.length()==0||aftername[0]!='.')
				return nullptr;
			else return GetFileIcon_SHGetFileInfo(aftername,SHGFI_ICON|SHGFI_USEFILEATTRIBUTES,FILE_ATTRIBUTE_NORMAL,sizeLevel);
		}
		
		inline SDL_Surface* GetExeFileIcon(const std::string &aftername,unsigned sizeLevel)
		{return GetFileIcon_SHGetFileInfo(aftername,SHGFI_ICON|SHGFI_EXETYPE,FILE_ATTRIBUTE_NORMAL,sizeLevel);}
		
		SDL_Surface* GetCommonIcon_SHGetStockIconInfo(unsigned id,unsigned sizeLevel)//0:Small 1:Large 2:ExtraLarge 3:Jumbo ; It is not safe to call in multitread at a time, should be mutex-protected
		{
			SHSTOCKICONINFO info{0};
			info.cbSize=sizeof(info);
			
			unsigned flag=SHGSI_ICON;
			if (sizeLevel==0) flag|=SHGSI_SMALLICON;
			else if (sizeLevel==1) flag|=SHGSI_LARGEICON;
			else flag|=SHGSI_SYSICONINDEX;

			if(SUCCEEDED(SHGetStockIconInfo((SHSTOCKICONID)id,flag,&info)))
				if(sizeLevel>=2)
				{
					HICON hIcon=0;
					HIMAGELIST* imageList;
					if (SUCCEEDED(SHGetImageList((sizeLevel!=2?SHIL_JUMBO:SHIL_EXTRALARGE),IID_IImageList,(void**)&imageList)))
					{
						((IImageList*)imageList)->GetIcon(info.iSysImageIndex,ILD_TRANSPARENT,&hIcon);
						return HICONToSDLSurface(hIcon,1);
					}
					else return nullptr;
				}
				else return HICONToSDLSurface(info.hIcon,1);
			else return nullptr;
		}
		
//		inline SDL_Surface* GetCommonDirIcon(int type,unsigned sizeLevel)//Type:0:Normal 1:Empty
//		{return GetCommonIcon_SHGetStockIconInfo(,sizeLevel);}	
	#endif
	
	int GetClipboardFiles(HWND hwnd,std::vector <std::string> &dst,bool clearDst=1)//return 0:Copy 1:Move -1:failed
	{
		if (clearDst)
			dst.clear();
		DWORD dwEffect=0;
		if (OpenClipboard(hwnd))
		{
		    HDROP hDrop=(HDROP)GetClipboardData(CF_HDROP);
		    HANDLE h=GetClipboardData(RegisterClipboardFormat(CFSTR_PREFERREDDROPEFFECT));
		    if (h)
		    	dwEffect=*(DWORD*)h;
		    if (hDrop!=NULL) 
			{
		        int nCount=DragQueryFileW(hDrop,(UINT)-1,NULL,0);
		        if (nCount)
				{
		            wchar_t szFile[MAX_PATH];
		            for (int i=0;i<nCount;++i)
					{
		                DragQueryFileW(hDrop,i,szFile,MAX_PATH);
		                dst.push_back(DeleteEndBlank(Charset::UnicodeToUtf8(szFile)));
		            }
		        }
		    }
		    CloseClipboard();
		}
		return dwEffect&DROPEFFECT_COPY?0:(dwEffect&DROPEFFECT_MOVE?1:-1);
	}
	
	int SetClipboardFiles(HWND hwnd,const std::vector <std::string> &paths,bool IsMove)
	{
		if (!OpenClipboard(hwnd))
			return -1;
		EmptyClipboard();
		
		if (paths.empty())
			return 0;
		
		std::wstring allpathW;
		for (const auto &vp:paths)
			allpathW+=DeleteEndBlank(Charset::Utf8ToUnicode(vp))+L'\0';
		allpathW+=L'\0';
		
		int size=sizeof(DROPFILES)+allpathW.length()*sizeof(wchar_t);
		HDROP hdrop=(HDROP)GlobalAlloc(GMEM_MOVEABLE,size);
		DROPFILES *df=(DROPFILES*)GlobalLock(hdrop);
		
		df->pFiles=sizeof(DROPFILES);
		df->fWide=TRUE;
		CopyMemory((char*)df+sizeof(DROPFILES),allpathW.c_str(),allpathW.length()*sizeof(wchar_t));
		GlobalUnlock(hdrop);
		SetClipboardData(CF_HDROP,hdrop);
	
		DWORD preferred=IsMove?DROPEFFECT_MOVE:DROPEFFECT_COPY;
		HGLOBAL hData=GlobalAlloc(GMEM_MOVEABLE,sizeof(DWORD));
		DWORD *pData=(DWORD*)GlobalLock(hData);
		*pData=preferred;
		GlobalUnlock(hData);
		SetClipboardData(RegisterClipboardFormat(CFSTR_PREFERREDDROPEFFECT),hData);
	
		size=allpathW.length()*sizeof(wchar_t);
		hData=GlobalAlloc(GMEM_MOVEABLE,size);
		char *psData=(char*)GlobalLock(hData);
		CopyMemory(psData,allpathW.c_str(),allpathW.length()*sizeof(wchar_t));
		GlobalUnlock(hData);
		SetClipboardData(RegisterClipboardFormat(CFSTR_FILENAMEW),hData);
	
		CloseClipboard();
		return 0;
	}

	std::string GetAppDataLocalDirectoryPath()
	{
		wchar_t buffer[MAX_PATH];
		if(SUCCEEDED(SHGetFolderPathW(NULL,CSIDL_LOCAL_APPDATA,NULL,0,buffer))) 
		    return DeleteEndBlank(Charset::UnicodeToUtf8(buffer));
		else return "";
	}
	
//    void AdjustPrivilege()
//    {
//        HANDLE hToken;
//        if (OpenProcessToken(GetCurrentProcess(),TOKEN_ADJUST_PRIVILEGES,&hToken))
//        {
//            TOKEN_PRIVILEGES tp;
//            tp.PrivilegeCount=1;
//            tp.Privileges[0].Attributes=SE_PRIVILEGE_ENABLED;
//            if (LookupPrivilegeValue(NULL,SE_DEBUG_NAME,&tp.Privileges[0].Luid))
//                AdjustTokenPrivileges(hToken,FALSE,&tp,sizeof(tp),NULL,NULL);
//            CloseHandle(hToken);
//        }
//    }
};

#endif
