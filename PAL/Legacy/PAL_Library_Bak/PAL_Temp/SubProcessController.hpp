#ifndef PAL_TEMP_SUBPROCESSCONTROLLER_HPP
#define PAL_TEMP_SUBPROCESSCONTROLLER_HPP

#include <string>
#include <cassert>
#include <Windows.h>

#include "../PAL_BasicFunctions/PAL_Charset.cpp"

namespace PAL
{
	class SubProcessController
	{
		public:
			enum
			{
				Type_None=0,
				Type_Auto,
				Type_Basic,
				Type_PseCon
			};
			
			enum
			{
				Stat_None=0,
				Stat_Initing,
				Stat_Running,
				Stat_Stoped
			};
			
			enum
			{
				Stop_Wait,
				Stop_Terminate,
			};
			
			enum
			{
				ERR_None=0,
				ERR_InitFailed=-1,
				ERR_WriteFileFailed=-2,
				ERR_ReadFileFailed=-3,
				ERR_GetFileSizeExFailed=-4,
				ERR_ParameterError=-5,
				ERR_CreatePipeFailed=-6,
				ERR_SetHandleInformationFailed=-7,
				ERR_CreateProcessFailed=-8,
				ERR_CreatePseudoConsoleFailed=-9,
				ERR_PrepareStartupInformationFailed=-10,
				ERR_OutOfMemory=-11,
				ERR_ProgramAndArgsIsNULL=-12,
				ERR_FailedToCreatePseudoConsole=-13,
				ERR_FailedToInitializeProcThreadAttributeList=-14,
				ERR_FailedToUpdateProcThreadAttribute=-15
			};
			
		protected:
			int type=Type_None;
			int stat=Stat_None;
			
		public:
			virtual int Write(const char *src,int size)=0;
			virtual int Read(char *dst,int size)=0;
			virtual int Stop(int stopMode)=0;
			virtual int Measure()=0;//How many characters unread.
			virtual int GetState()=0;
			virtual int Close()=0;
			virtual int Start(const std::wstring &program,const std::wstring &args=L"")=0;
			
			int Start(const std::string &program_u8,const std::string &args_u8="")
			{return Start(Charset::Utf8ToUnicode(program_u8),Charset::Utf8ToUnicode(args_u8));}
			
//			bool HaveData()
//			{return Count!=0;}
//			
//			std::string Getline()
//			{
//				
//			}
			
			template <int N> std::string GetN()//get with buffersize N
			{
				static_assert(N>=8);
				char buffer[N];
				int s=Read(buffer,N-1);
				if (s>=0)
				{
					buffer[s]=0;
					return buffer; 
				}
				else return "";
			}
			
			std::string Get128()
			{return GetN<128>();}
			
			std::string Get256()
			{return GetN<256>();}
			
			std::string Get512()
			{return GetN<512>();}
			
			std::string Get()
			{return GetN<512>();}
			
			int Send(const std::string &str)
			{return Write(str.c_str(),str.length());}
			
			virtual ~SubProcessController()
			{
//				if (int e=Stop2(Stop_Wait))
//					assert(false);//??
			}
	};
	
	class BasicSubProcessController:public SubProcessController
	{
		protected:
			HANDLE ReadClient=NULL,
				   WriteClient=NULL,
				   ClientRead=NULL,
				   ClientWrite=NULL;
			STARTUPINFOEXW Si{0};
			PROCESS_INFORMATION Pi{0};
			wchar_t *ProgramWptr=nullptr,
					*ArgsWptr=nullptr;
			bool Cfg_BindStderrWithStdout=true;
			
			static void CloseHandleToNULL(HANDLE &h)
			{
				if (h==NULL||h==INVALID_HANDLE_VALUE)
					return;
				CloseHandle(h);
				h=NULL;
			}
			
			void ClosePipes()
			{
				CloseHandleToNULL(ReadClient);
				CloseHandleToNULL(WriteClient);
				CloseHandleToNULL(ClientRead);
				CloseHandleToNULL(ClientWrite);
			}
			
			void ResetStartupInfo()
			{
				if (Si.lpAttributeList)
				{
					DeleteProcThreadAttributeList(Si.lpAttributeList);
					free(Si.lpAttributeList);
				}
				ZeroMemory(&Si,sizeof(STARTUPINFOEXW));
			}
			
			void ResetProcessInfo()
			{
				ZeroMemory(&Pi,sizeof(PROCESS_INFORMATION));
			}
			
			void CleanProgramAndArgswptr()
			{
				DELETEtoNULL(ProgramWptr);
				DELETEtoNULL(ArgsWptr);
			}
			
			virtual int PreparePipes(bool inherit=true)
			{
				ClosePipes();
				SECURITY_ATTRIBUTES saAttr;
				saAttr.nLength=sizeof(saAttr);
				saAttr.lpSecurityDescriptor=NULL;
				saAttr.bInheritHandle=inherit?TRUE:FALSE;
				
				if (!CreatePipe(&ClientRead,&WriteClient,&saAttr,0)
				  ||!CreatePipe(&ReadClient,&ClientWrite,&saAttr,0))  
				{
					ClosePipes();
					return ERR_CreatePipeFailed;
				}
				
				if (inherit)
					if (!SetHandleInformation(ReadClient,HANDLE_FLAG_INHERIT,0)
					  ||!SetHandleInformation(WriteClient,HANDLE_FLAG_INHERIT,0))
					{
						ClosePipes();
						return ERR_SetHandleInformationFailed;
					}
				return ERR_None;
			}
			
			virtual int PrepareStartupInformation()
			{
				ResetStartupInfo();
				Si.StartupInfo.cb=sizeof(STARTUPINFO);
				Si.StartupInfo.dwFlags|=STARTF_USESHOWWINDOW;
				Si.StartupInfo.dwFlags|=STARTF_USESTDHANDLES;
				Si.StartupInfo.hStdInput=ClientRead;
				Si.StartupInfo.hStdOutput=ClientWrite;
				if (Cfg_BindStderrWithStdout)
					Si.StartupInfo.hStdError=ClientWrite;
				else Si.StartupInfo.hStdError=GetStdHandle(STD_ERROR_HANDLE);
				return ERR_None;
			}
			
			virtual int DumpProgramAndArgs(const std::wstring &program,const std::wstring &args)
			{
				CleanProgramAndArgswptr();
				if (program!=L"")
				{
					ProgramWptr=new wchar_t[program.length()+1];
					wcscpy(ProgramWptr,program.c_str());
				}
				if (args!=L"")
				{
					ArgsWptr=new wchar_t[args.length()+1];
					wcscpy(ArgsWptr,args.c_str());
				}
				return ERR_None;
			}
			
			virtual int StartProgram()
			{
				ResetProcessInfo();
				if (!CreateProcessW(ProgramWptr,ArgsWptr,NULL,NULL,TRUE,0,NULL,NULL,&Si.StartupInfo,&Pi))
					return ERR_CreateProcessFailed;
				return ERR_None;
			}
			
		public:
			virtual int Write(const char *src,int size)
			{
				DWORD dwWritten;
				BOOL bSuccess=WriteFile(WriteClient,src,size,&dwWritten,NULL);
				if (!bSuccess)
					return ERR_WriteFileFailed;
				else return dwWritten;
			}
			
			virtual int Read(char *dst,int size)
			{
				DWORD dwRead;
				BOOL bSuccess=ReadFile(ReadClient,dst,size,&dwRead,NULL);
				if (!bSuccess)
					return ERR_ReadFileFailed;
				else return dwRead;
			}
			
			virtual int Stop(int stopMode)
			{
				//...
			}
			
			virtual int Measure()
			{
				LARGE_INTEGER sz;
				if (GetFileSizeEx(ReadClient,&sz))
					return sz.QuadPart;
				else return ERR_GetFileSizeExFailed;
			}
			
			virtual int GetState()
			{
				//Check current state and update...
			}
			
			virtual int Close()
			{
				ClosePipes();
				Stop(Stop_Terminate);
				//...
			}
			
			virtual int Start(const std::wstring &program,const std::wstring &args)
			{
				stat=Stat_Initing;
				if (int err=PreparePipes())
					return err;
				if (int err=PrepareStartupInformation())
					return err;
				if (int err=DumpProgramAndArgs(program,args))
					return err;
				if (ProgramWptr==nullptr&&ArgsWptr==nullptr)
					return ERR_ProgramAndArgsIsNULL;
				if (int err=StartProgram())
					return err;
				CloseHandleToNULL(ClientRead);
				CloseHandleToNULL(ClientWrite);
				stat=Stat_Running;//??
				return ERR_None;
			}
			
			void Config_BindStderrWithStdout(bool onoff)//Configure this before start.
			{Cfg_BindStderrWithStdout=onoff;}
			
			BasicSubProcessController() {}
	};
	
	#if NTDDI_VERSION>=0x0A000006
	class PseudoConsoleController:public BasicSubProcessController//NTDDI_VERSION>=0x0A000006 needed
	{
		protected:
			HPCON hPseudoConsole=NULL;
			int ConWidth=80,
				ConHeight=40;
			
			void ClosePseudoConsole()
			{
				CloseHandleToNULL(hPseudoConsole);
			}
			
			int PreparePseudoConsole()
			{
				HRESULT hr=CreatePseudoConsole({ConWidth,ConHeight},ClientRead,ClientWrite,0,&hPseudoConsole);
				if (FAILED(hr))
					return ERR_FailedToCreatePseudoConsole;
				return ERR_None;
			}
			
			virtual int PreparePipes(bool ignore=true)
			{
				if (int err=BasicSubProcessController::PreparePipes())
					return err;
				if (int err=PreparePseudoConsole())
				{
					ClosePipes();
					return err;
				}
				return ERR_None;
			}
	
			virtual int PrepareStartupInformation()
			{
				ResetStartupInfo();
				size_t bytesRequired=0; 
				Si.StartupInfo.cb=sizeof(STARTUPINFOEXW);
				InitializeProcThreadAttributeList(NULL,1,0,&bytesRequired);
				Si.lpAttributeList=(LPPROC_THREAD_ATTRIBUTE_LIST)malloc(bytesRequired);//Remember free this.
				if (!Si.lpAttributeList)
					return ERR_OutOfMemory;
				if (!InitializeProcThreadAttributeList(Si.lpAttributeList,1,0,&bytesRequired))
				{
					ResetStartupInfo();
					return ERR_FailedToInitializeProcThreadAttributeList;
				}
				if (!UpdateProcThreadAttribute(Si.lpAttributeList,0,PROC_THREAD_ATTRIBUTE_PSEUDOCONSOLE,hPseudoConsole,sizeof(HPCON),NULL,NULL))
				{
					ResetStartupInfo();
					return ERR_FailedToUpdateProcThreadAttribute;
				}
				return ERR_None;
			}

			virtual int StartProgram()
			{
				ResetProcessInfo();
				if (!CreateProcessW(ProgramWptr,ArgsWptr,NULL,NULL,FALSE,EXTENDED_STARTUPINFO_PRESENT,NULL,NULL,&Si.StartupInfo,&Pi))
					return ERR_CreateProcessFailed;
				return ERR_None;
			}
			
		public:
			
			virtual int Close()
			{
				//...
				ClosePseudoConsole();
				//...
			}
	};
	#endif
	
	class SubProcessReader
	{
		protected:
			SubProcessController *subproc=nullptr;
			
		public:
			
			
	};
};

#endif
