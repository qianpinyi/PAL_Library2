#ifndef PAL_WINDOWS_SHELLCONTEXTMENU_CPP
#define PAL_WINDOWS_SHELLCONTEXTMENU_CPP 1

//-lshlwapi -lole32 -lshell32; pre define INITGUID
#include <cstdlib>
#include <string>
#include <vector>

#include <windows.h>
#include <Shlobj.h>
#include <Shobjidl.h>
#include <shlwapi.h>
#include <unknwn.h>

#include <PAL_BasicFunctions/PAL_Charset.cpp>

namespace PAL_Platform
{
	class ShellContextMenu
	{
		protected:
			IShellFolder *m_pDesktopFolder=nullptr,
						 *m_pParentFolder=nullptr;
			IContextMenu  *m_pContextMenu=nullptr;
			IContextMenu2 *m_pContextMenu2=nullptr;
			IContextMenu3 *m_pContextMenu3=nullptr;
			std::vector <LPCITEMIDLIST> m_idls;
			std::wstring m_strParentFolder;
			
			void ReleaseIdls()
			{
				for (auto &vp:m_idls)
					if (vp!=nullptr)
						CoTaskMemFree((void*)vp);
				m_idls.clear();
			}
			
			void ReleaseAll()
			{
				ReleaseIdls();
			#define _ReleaseComPtr(p) \
				if ((p)!=nullptr){(p)->Release(); (p) = nullptr;}
			
				_ReleaseComPtr(m_pContextMenu3);
				_ReleaseComPtr(m_pContextMenu2);
				_ReleaseComPtr(m_pContextMenu);
				_ReleaseComPtr(m_pParentFolder);
				_ReleaseComPtr(m_pDesktopFolder);
			#undef _ReleaseComPtr
			}
	
			IContextMenu* GetContextMenuInterfaces(IShellFolder *pShellFolder,std::vector <LPCITEMIDLIST> &idls)
			{
				IContextMenu* pResult=nullptr;
				UINT refReversed=0;
				auto hr=pShellFolder->GetUIObjectOf(nullptr,idls.size(),idls.data(),IID_IContextMenu,&refReversed,(void**)&pResult);
				if (hr!=S_OK)
					return nullptr;
				return pResult;
			}
			
			bool InvokeCmd(IContextMenu* pContext,int nCmdSelection,LPCWSTR szFolder,LPPOINT pt)
			{
				try
				{
//					cout<<"Z"<<endl;
	//				USES_CONVERSION;
					std::string szFolderA=Charset::UnicodeToUtf8(szFolder);
					CMINVOKECOMMANDINFOEX invoke;
					memset(&invoke,0,sizeof(CMINVOKECOMMANDINFOEX));
					invoke.cbSize=sizeof(CMINVOKECOMMANDINFOEX);
					invoke.lpVerb=(LPCSTR)(nCmdSelection-CDM_FIRST);
					invoke.lpDirectory=szFolderA.c_str();//C2CA(szFolder);
					invoke.lpVerbW=(LPCWSTR)(nCmdSelection-CDM_FIRST);
					invoke.lpDirectoryW=szFolder;
					invoke.fMask=CMIC_MASK_UNICODE;
					//invoke.ptInvoke.x=pt->x;
					//invoke.ptInvoke.y=pt->y;
					invoke.nShow=SW_SHOWNORMAL;
					
					auto hr=pContext->InvokeCommand((CMINVOKECOMMANDINFO*)&invoke);
					return S_OK==hr;
				}
				catch (...)
				{
					return false;
				}
			}
			
			bool InvokeCmd(IContextMenu* pContext,LPCWSTR szCmd,LPCWSTR szFolder)
			{
				CMINVOKECOMMANDINFOEX info;
				info.cbSize=sizeof(CMINVOKECOMMANDINFOEX);
				info.lpVerbW=szCmd;
				info.lpDirectoryW=szFolder;
				info.fMask=CMIC_MASK_UNICODE;
				return S_OK==pContext->InvokeCommand((CMINVOKECOMMANDINFO*)&info);
			}
			
			bool ShowContextMenu(HWND hWnd,LPPOINT pt)
			{
//				cout<<"D"<<endl;
				if (m_idls.empty())
				{
					ReleaseAll();
					return false;
				}
//				cout<<"E"<<endl;
				m_pContextMenu=GetContextMenuInterfaces(m_pParentFolder,m_idls);
				if (m_pContextMenu==nullptr)
				{
					ReleaseAll();
					return false;
				}
				
//				cout<<"F"<<endl;
				auto hMenu=::CreatePopupMenu();
				auto hr=m_pContextMenu->QueryContextMenu(hMenu,0,CDM_FIRST,CDM_LAST,CMF_EXPLORE|CMF_NORMAL);
				if (HRESULT_SEVERITY(hr)!=SEVERITY_SUCCESS)
				{
					ReleaseAll();
					return false;
				}
				
//				cout<<"G"<<endl;
				m_pContextMenu->QueryInterface(IID_IContextMenu2,(void**)&m_pContextMenu2);
				m_pContextMenu->QueryInterface(IID_IContextMenu3,(void**)&m_pContextMenu3);
				
//				cout<<"H"<<endl;
				auto nSel=TrackPopupMenuEx(hMenu,TPM_RETURNCMD,pt->x,pt->y,hWnd,nullptr);
//				cout<<"nSel "<<nSel<<endl;
				if (nSel==0)
				{
					auto error=::GetLastError();
			
					wchar_t buffer[1024];
					::FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_FROM_SYSTEM|FORMAT_MESSAGE_IGNORE_INSERTS,NULL,error,MAKELANGID(LANG_NEUTRAL,SUBLANG_DEFAULT),buffer,1024,NULL);
				}
				DestroyMenu(hMenu);
				
//				cout<<"I"<<endl;
		
		
				
//				wchar_t strCmd[1024];
//				hr=m_pContextMenu->GetCommandString(nSel,GCS_VALIDATEW|GCS_VERBW|GCS_UNICODE,nullptr,(char*)strCmd,1024);
//			
//				wchar_t strHelpr[1024];
//				hr=m_pContextMenu->GetCommandString(nSel,GCS_VALIDATEW|GCS_HELPTEXTW|GCS_UNICODE,nullptr,(char*)strHelpr,1024);
////				cout<<"hr "<<hr<<endl;
//				if (hr==S_OK)
//					InvokeCmd(m_pContextMenu,strCmd,m_strParentFolder.c_str());
////				else cout<<"LE "<<GetLastError()<<endl;
//		
		
				if (nSel>0)
					InvokeCmd(m_pContextMenu,nSel,m_strParentFolder.c_str(),pt);
			
				ReleaseAll();
				return true;
			}
			
		public:
			IShellFolder* GetDesktopFolder()
			{
				if (m_pDesktopFolder==nullptr)
					if (SHGetDesktopFolder(&m_pDesktopFolder)!=S_OK)
						return nullptr;
				return m_pDesktopFolder;
			}
			
			IShellFolder* GetParentFolder(LPCWSTR szFolder)
			{
				if (m_pParentFolder==nullptr)
				{
					auto pDesktop=GetDesktopFolder();
					if (pDesktop==nullptr)
						return nullptr;
	
	//				ULONG pchEaten=0;
					LPITEMIDLIST pidl=nullptr;
	//				DWORD dwAttributes=0;
					auto hr=pDesktop->ParseDisplayName(nullptr,nullptr,(LPWSTR)szFolder,nullptr,&pidl,nullptr);
					if (hr!=S_OK)
						return nullptr;
			
					STRRET sRetName;
					hr=pDesktop->GetDisplayNameOf(pidl,SHGDN_FORPARSING,&sRetName);
					if (hr!=S_OK)
					{
						CoTaskMemFree(pidl);
						return nullptr;
					}
					
					wchar_t buffer[_MAX_PATH];
					hr=StrRetToBufW(&sRetName,pidl,buffer,_MAX_PATH);
					m_strParentFolder=buffer;
					if (hr!=S_OK)
					{
						CoTaskMemFree(pidl);
						return nullptr;
					}
			
					IShellFolder *pParentFolder=nullptr;
					hr=pDesktop->BindToObject(pidl,nullptr,IID_IShellFolder,(void**)&pParentFolder);
					if (hr!=S_OK)
					{
						CoTaskMemFree(pidl);
						return nullptr;
					}
			
					CoTaskMemFree(pidl);
					m_pParentFolder=pParentFolder;
				}
				return m_pParentFolder;
			}
		
			std::wstring GetDirectory(LPCWSTR szFile)
			{
				WCHAR szDrive[_MAX_DRIVE];
				WCHAR szDir[_MAX_DIR];
				WCHAR szFName[_MAX_FNAME];
				WCHAR szExt[_MAX_EXT];
				_wsplitpath_s(szFile,szDrive,szDir,szFName,szExt);
			
				std::wstring strResult=szDrive;
				strResult+=szDir;
				return strResult;
			}
			
			std::wstring GetFileNameWithExt(LPCWSTR szFile)
			{
				WCHAR szDrive[_MAX_DRIVE];
				WCHAR szDir[_MAX_DIR];
				WCHAR szFName[_MAX_FNAME];
				WCHAR szExt[_MAX_EXT];
				_wsplitpath_s(szFile,szDrive,szDir,szFName,szExt);
			
				std::wstring strResult=szFName;
				strResult+=szExt;
				return strResult;
			}
			
			bool GetPidls(const std::vector <std::wstring> &files)
			{
				ReleaseIdls();
				if (files.empty())
					return false;
			
				auto pParentFolder=GetParentFolder(GetDirectory(files[0].c_str()).c_str());
				if (pParentFolder==nullptr)
					return false;
				
				for (auto &vp:files)
				{
					std::wstring strFile=GetFileNameWithExt(vp.c_str());
			
					ULONG pchEaten=0;
					LPITEMIDLIST pidl=nullptr;
					DWORD dwAttributes=0;
					auto hr=pParentFolder->ParseDisplayName(nullptr,nullptr,(LPWSTR)strFile.c_str(),&pchEaten,&pidl,&dwAttributes);
					if (hr!=S_OK)
						continue;
					m_idls.push_back(pidl);
				}
				return !m_idls.empty();
			}
			
			bool ShowContextMenu(const std::vector <std::wstring> &files,HWND hWnd,LPPOINT pt)
			{
//				cout<<"J"<<endl;
				ReleaseAll();
//				cout<<"K"<<endl;
				if (!GetPidls(files))
					return false;
//				cout<<"L"<<endl;
				return ShowContextMenu(hWnd,pt);
			}
			
			~ShellContextMenu()
			{
				ReleaseAll();
			}
			
			ShellContextMenu() {}
	};
};

#endif
