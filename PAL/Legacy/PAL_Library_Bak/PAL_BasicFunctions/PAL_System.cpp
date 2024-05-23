#ifndef PAL_BASICFUNCTIONS_SYSTEM_CPP
#define PAL_BASICFUNCTIONS_SYSTEM_CPP 1

#include "../PAL_BuildConfig.h"

#include "PAL_BasicFunctions_0.cpp"
#include "PAL_Charset.cpp"
#include "PAL_Color.cpp"
#include "PAL_StringEX.cpp"

#include <cstdlib>
#include <string>
#include <vector>
#include <iostream>
#include <thread>

enum
{
	SetSystemBeep_None=0,
	SetSystemBeep_Default,
	SetSystemBeep_Error,
	SetSystemBeep_Warning,
	SetSystemBeep_Info,
	SetSystemBeep_Notification,
};

#if PAL_CurrentPlatform == PAL_Platform_Windows
#include <Windows.h>

class PAL_SingleProcessController
{
	public:
		enum
		{
			EventCode_None=0,
			EventCode_Exsit,
			EventCode_SomeOneRun
		};
	
	protected:
		std::thread Th_EventFunc;
		BaseTypeFuncAndData *EventFunc=NULL;
		HANDLE SingleEvent=NULL,
			   CloseEvent=NULL;
		bool Exsit=0;
		
		static void Thread_Func(void *thdata)
		{
			PAL_SingleProcessController *This=(PAL_SingleProcessController*)thdata;
			HANDLE handles[]={This->SingleEvent,This->CloseEvent};
			while (1)
				switch (WaitForMultipleObjects(sizeof(handles)/sizeof(HANDLE),handles,FALSE,INFINITE))
				{
					case WAIT_OBJECT_0:
						if (This->EventFunc!=NULL)
							This->EventFunc->CallFunc(EventCode_SomeOneRun);
						break;
					case WAIT_OBJECT_0+1:
						CloseHandle(This->CloseEvent);
						return;
				}
		}
		
	public:
		inline bool IsExsit()
		{return Exsit;}
		
		~PAL_SingleProcessController()
		{
			if (CloseEvent)
				SetEvent(CloseEvent);
			if (Th_EventFunc.joinable())
				Th_EventFunc.join();
			CloseHandle(SingleEvent);
			if (EventFunc!=NULL)
				delete EventFunc;
		}
		
		PAL_SingleProcessController(const std::string &name,BaseTypeFuncAndData *eventfunc=NULL)
		:EventFunc(eventfunc)
		{
			if (!name.empty())
			{
	//			assert(!name.empty());
				std::wstring wstr=DeleteEndBlank(Charset::Utf8ToUnicode(name));
	//			std::wstring singleName = L"Global\\" + exeName;
				SingleEvent=CreateEventW(NULL,0,0,wstr.c_str());
				if (!SingleEvent||GetLastError()==ERROR_ALREADY_EXISTS)
				{
					if (EventFunc!=NULL)
						EventFunc->CallFunc(EventCode_Exsit);
					Exsit=1;
					SetEvent(SingleEvent);
				}
				if (EventFunc!=NULL)
				{
					CloseEvent=CreateEvent(NULL,1,0,NULL);
					Th_EventFunc=std::thread(Thread_Func,this);
				}
			}
		}
}; 

//class SubprocessManager
//{
//	public:
//		enum
//		{
//			ERR_None=0,
//			
//		};
//		
//	protected:
//		bool Running=0;
//		int SubProcessRetureCode=0;
//		std::string SubProcessPath;
//		
//	public:
//		int Start(const std::string &cmdArgs="")
//		{
//			
//			return 0;
//		}
//		
//		int Stop(unsigned int timeOut=0) 
//		{
//			
//			return 0;
//		}
//		
//		int Terminate()
//		{
//			
//			return 0;
//		}
//		
//		char ReadChar()
//		{
//			
//			return 0;
//		}
//		
//		std::string Read(unsigned int cnt)//0: no limit
//		{
//			
//			return "";
//		}
//		
//		std::string Readline()
//		{
//			
//			return "";
//		}
//
//		int Write(const std::string &str)
//		{
//			
//			return 0;
//		}
//		
//		int Writeline(const std::string &str)
//		{return Write(str+"\n");}
//		
//		int SetReadFunc()
//		{
//			
//			return 0;
//		}
//		
//		int SetReadlineFunc()
//		{
//			
//			return 0;
//		}
//		
//		int SetEndFunc()
//		{
//			
//			return 0;
//		}
//		
//		bool IsRunning() const
//		{
//			
//			return 0;
//		}
//		
//		bool IsEnd() const
//		{
//			
//			return 0;
//		}
//		
//		int GetReturnCode() const
//		{
//			
//			return 0;
//		}
//		
//		inline int SetSubprocess(const std::string &path)
//		{
//			
//			return 0;
//		}
//		
//		inline int SetWorkPath(const std::string &path)
//		{
//			
//			return 0;
//		}
//		
//		~SubprocessManager()
//		{
//			
//		}
//		
//		SubprocessManager(const std::string &path,const std::string &cmdArgs,const std::string &workPath)
//		{
//			
//		}
//		
//		SubprocessManager(const std::string &path=""):SubProcessPath(path) {}
//};

void SetSystemBeep(unsigned int type)
{
	switch (type)
	{
		case SetSystemBeep_None:	break;
		case SetSystemBeep_Error:			MessageBeep(MB_ICONHAND);	break;
		case SetSystemBeep_Warning:			MessageBeep(0xFFFFFFFF);	break;
		case SetSystemBeep_Info:			MessageBeep(0xFFFFFFFF);	break;
		case SetSystemBeep_Notification:	MessageBeep(0xFFFFFFFF);	break;
		case SetSystemBeep_Default:			MessageBeep(0xFFFFFFFF);	break;
	}
}

std::string GetSystem32DirectoryPath()
{
	wchar_t buffer[1024];
	if (GetSystemDirectoryW(buffer,1024))
		return DeleteEndBlank(Charset::UnicodeToUtf8(buffer));
	else return "";
}

std::string GetWindowsDirectoryPath()
{
	wchar_t buffer[1024];
	if (GetWindowsDirectoryW(buffer,1024))
		return DeleteEndBlank(Charset::UnicodeToUtf8(buffer));
	else return "";
}

int systemUTF8(const std::string &str)
{return _wsystem(Charset::Utf8ToUnicode(str).c_str());}

void SetCmdUTF8AndPrintInfo(const std::string &appname,const std::string &version,const std::string &author)
{
	system("chcp 65001");
	system("cls");
	std::cout<<appname<<std::endl
			 <<"Version: "<<version<<std::endl
			 <<"Author:  "<<author<<std::endl;
}

void OpenCMDStartAt(const std::string &path)
{
	ShellExecuteW(0,L"open",L"cmd",L"",Charset::Utf8ToUnicode(path).c_str(),SW_SHOWNORMAL);
}

void OpenPowerShellStartAt(const std::string &path)
{
	ShellExecuteW(0,L"open",L"powershell",L"",Charset::Utf8ToUnicode(path).c_str(),SW_SHOWNORMAL);
}

void OpenWSLBashStartAt(const std::string &path)
{
	ShellExecuteW(0,L"open",L"bash",L"",Charset::Utf8ToUnicode(path).c_str(),SW_SHOWNORMAL);
}

void OpenWindowsTerminalStartAt(const std::string &path)
{
	ShellExecuteW(0,L"open",L"wt",(L"-d \""+Charset::Utf8ToUnicode(path)+L"\"").c_str(),Charset::Utf8ToUnicode(path).c_str(),SW_SHOWNORMAL);
}

void SelectInWinExplorer(const std::string &path)
{systemUTF8("explorer /select, \""+path+"\"");}

int ShowPropertiesOfExplorer(const std::string &pathUTF8)
{
	std::wstring path=DeleteEndBlank(Charset::Utf8ToUnicode("\""+pathUTF8+"\""));
	SHELLEXECUTEINFOW ShExecInfo{0};
	ShExecInfo.cbSize=sizeof(SHELLEXECUTEINFO);
	ShExecInfo.fMask=SEE_MASK_INVOKEIDLIST ;
	ShExecInfo.hwnd=NULL;
	ShExecInfo.lpVerb=L"properties";
	ShExecInfo.lpFile=path.c_str();
	ShExecInfo.lpParameters=L"";
	ShExecInfo.lpDirectory=NULL;
	ShExecInfo.nShow=SW_SHOW;
	ShExecInfo.hInstApp=NULL;
	return ShellExecuteExW(&ShExecInfo);
}

int GetLogicalDriveCount()
{
	unsigned int x=GetLogicalDrives();
	int re=0;
	while (x)
	{
		if (x&1)
			++re;
		x>>=1;
	}
	return re;
}

std::vector <std::string> GetAllLogicalDrive()
{
	unsigned int bufferSize=GetLogicalDriveStrings(0,NULL);
	char *buffer=new char[bufferSize];
	memset(buffer,0,bufferSize*sizeof(char));
	GetLogicalDriveStringsA(bufferSize,buffer);
	std::string str;
	std::vector <std::string> re;
	for (unsigned int i=0;i<bufferSize;++i)
		if (buffer[i]==0)
		{
			if (!str.empty())
			{
				str.erase(str.length()-1,1);
				re.push_back(str);
			}
			str.clear();
		}
		else str+=buffer[i];
	delete buffer;
	return re;
}

struct LogicalDriveInfo
{
	std::string path,
				name,
				fileSystem;
	unsigned long long FreeBytesAvailable=0,
					   TotalNumberOfBytes=0,
					   TotalNumberOfFreeBytes=0;
	long unsigned int VolumeSerialNumber=0,
				 	  MaxComponentLength=0,
				 	  FSflags=0;
	bool Succeed=0;
};

LogicalDriveInfo GetLogicalDriveInfo(const std::string &rootPath)
{
	std::wstring rootPathW=DeleteEndBlank(Charset::Utf8ToUnicode(rootPath+"\\"));
	LogicalDriveInfo re;
	wchar_t nameBuffer[32]={0},
			FSnameBuffer[16]={0};
	ULARGE_INTEGER FreeBytesAvailable,
				   TotalNumberOfBytes,
				   TotalNumberOfFreeBytes;
	unsigned int x=GetVolumeInformationW(rootPathW.c_str(),nameBuffer,32,&re.VolumeSerialNumber,&re.MaxComponentLength,&re.FSflags,FSnameBuffer,16),
				 y=GetDiskFreeSpaceExW(rootPathW.c_str(),&FreeBytesAvailable,&TotalNumberOfBytes,&TotalNumberOfFreeBytes);
	re.Succeed=x&&y;
//	std::cout<<x<<" "<<y<<std::endl;
	re.path=rootPath;
	re.name=DeleteEndBlank(Charset::UnicodeToUtf8(nameBuffer));
	re.fileSystem=DeleteEndBlank(Charset::UnicodeToUtf8(FSnameBuffer));
	re.TotalNumberOfBytes=TotalNumberOfBytes.QuadPart;
	re.TotalNumberOfFreeBytes=TotalNumberOfFreeBytes.QuadPart;
	re.FreeBytesAvailable=FreeBytesAvailable.QuadPart;
	return re;
}

struct WinThemeColorFromRegistry
{
	RGBA accentColorMenu,
		 accentColorInactive,
		 accentPalette[8];
	int stat=0;
};

int GetWinThemeColor(WinThemeColorFromRegistry &re)
{
	memset(&re,0,sizeof(re));
	HKEY hKey;
	DWORD type=4,size=4;
	if (RegOpenKeyExA(HKEY_CURRENT_USER,"Software\\Microsoft\\Windows\\DWM",0,KEY_READ,&hKey)==ERROR_SUCCESS)
		if (RegQueryValueExA(hKey,"AccentColorInactive",NULL,&type,(BYTE*)&re.accentColorInactive,&size)==ERROR_SUCCESS)
			re.stat|=0x0200;
	RegCloseKey(hKey);
	
	if (RegOpenKeyExA(HKEY_CURRENT_USER,"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Accent",0,KEY_READ,&hKey)==ERROR_SUCCESS)
	{
		type=4,size=4;
		if (RegQueryValueExA(hKey,"AccentColorMenu",NULL,&type,(BYTE*)&re.accentColorMenu,&size)==ERROR_SUCCESS)
			re.stat|=0x0100;
		type=3,size=32;
		if (RegQueryValueExA(hKey,"AccentPalette",NULL,&type,(BYTE*)&re.accentPalette,&size)==ERROR_SUCCESS)
			re.stat|=0xFF;
	}
	RegCloseKey(hKey);
	return re.stat;
}

int SetWinThemeColor(WinThemeColorFromRegistry &co,bool delayFlag)
{
	//...
	return 0;
}

inline bool IsFileExsit(const std::wstring &path,int filter=-1)//filter:-1:no filter 0:file 1:dir
{
	int x=GetFileAttributesW(path.c_str());
	if (x==-1) return 0;
	else if (filter==-1||(filter!=0)==bool(x&FILE_ATTRIBUTE_DIRECTORY))
		return 1;
	else return 0;
}

inline bool IsFileExsit(const std::string &path,int filter=-1)
{return IsFileExsit(DeleteEndBlank(Charset::Utf8ToUnicode(path)),filter);}

#endif
#if PAL_CurrentPlatform != PAL_Platform_Windows


void SetSystemBeep(unsigned int type)
{
	switch (type)
	{
		default:	break;
	}
}
#endif

#endif
