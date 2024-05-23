#ifndef PAL_SDLNET_EX_WITHPUI_CPP
#define PAL_SDLNET_EX_WITHPUI_CPP 1

#include "../PAL_BuildConfig.h"

#include "../PAL_GUI/PAL_GUI_0.cpp"
#include "../PAL_BasicFunctions/PAL_StringEX.cpp"

#include <atomic>

#include PAL_SDL_netHeaderPath

namespace PAL::Legacy
{
	class EasySDLNetWithPUI//Please init PUI first;ThisClass provide lock free network
	{
		public:
			enum
			{
				ERR_None=0,
				ERR_ThreadIsRunning,
				ERR_TargetError,
				ERR_ResolveError,
				ERR_TCPOpenError,
				ERR_SendError,
				ERR_RecvError,//maybe connection is lost
				ERR_NewError,
				ERR_DataIsNULL
			};
			static const unsigned int EventType;
			
			class Event:public PAL_GUI::PUI_Event
			{
				friend class EasySDLNetWithPUI;
				protected:
					unsigned int mode=0,len=0;//if mode==-1(0xFFFFFFFF),means Error(or Msg),len means code,data means this
					void *data=nullptr;//new [len+1]
					
				public:
					inline bool IsERR() const
					{return mode==-1;}
					
					inline unsigned int ERR() const
					{return len;}
					
					inline EasySDLNetWithPUI* Src() const
					{return (EasySDLNetWithPUI*)data;}
					
					inline unsigned int Mode() const
					{return mode;}
					
					inline unsigned int Size() const
					{return len;}
					
					inline unsigned char operator [] (unsigned int p) const
					{return ((char*)data)[p];}
					
					inline bool Valid() const
					{return mode!=-1&&data!=nullptr;}
					
					inline const void* Data() const
					{return data;}
					
					inline char* Handover()
					{
						if (mode==-1)
							return nullptr;
						char *re=(char*)data;
						data=nullptr;
						return re;
					}
					
					inline std::string Str() const
					{
						if (mode!=-1&&data==nullptr) return "";
						else return std::string((char*)data);
					}
					
					virtual ~Event()
					{
						if (mode!=-1&&data!=nullptr)
							delete[] (char*)data;
					}
			};
			
		protected:
			IPaddress IP;
			TCPsocket so=NULL;
			std::atomic_bool On;
			SDL_Thread *Th_Recv=NULL;
			const unsigned int XORCode=0;
			
			static int RecvLengthData(TCPsocket so,char *buffer,unsigned int len)
			{
				if (len==0||buffer==nullptr) return 0;
				unsigned int tot=0;
				int size;
				while (tot<len)
				{
					size=SDLNet_TCP_Recv(so,buffer+tot,len-tot);
					if (size<=0)
						return ERR_RecvError;
					else tot+=size;
				}
				return 0;
			}
			
			static int ThreadFunc_Recv(void *userdata)
			{
				EasySDLNetWithPUI *This=(EasySDLNetWithPUI*)userdata;
				int ERRtype=0; 
				while (This->On&&!ERRtype)
				{
					unsigned int modelen[2];
					char *data=NULL;
					if (RecvLengthData(This->so,(char*)modelen,8))
						ERRtype=ERR_RecvError;
					else
					{
						data=new char[modelen[1]+1];
						if (data==nullptr)
							ERRtype=ERR_NewError;
						else if (modelen[1]&&RecvLengthData(This->so,data,modelen[1]))
							ERRtype=ERR_RecvError;
						else
						{
							data[modelen[1]]=0;
							Event *event=new Event();
							event->type=EventType;
							event->timeStamp=SDL_GetTicks();
							event->mode=modelen[0];
							event->len=modelen[1];
							event->data=data;
							unsigned int XORCode=This->XORCode;
							if (XORCode!=0)
							{
								unsigned int p=modelen[1]/4*4;
								for (unsigned int i=0;i<p;i+=4)
									*(int*)(data+i)^=XORCode;
								for (unsigned int i=p;i<modelen[1];++i)
									data[i]^=XORCode>>(i-p)*8&0xFF;
							}
							data=nullptr;
							PAL_GUI::PUI_SendEvent(event);
						}
					}
					if (data!=nullptr)
						delete[] data;
				}
				if (This->On)
				{
					Event *event=new Event();
					event->type=EventType;
					event->timeStamp=SDL_GetTicks();
					event->mode=-1;
					event->len=ERRtype;
					event->data=This;
					PAL_GUI::PUI_SendEvent(event);
					return ERRtype;
				}
				else return 0;
			}
			
		public:
			inline int Send(unsigned int mode,unsigned int len,const char *data)
			{
				if (data==nullptr&&len!=0)
					return ERR_DataIsNULL;
				unsigned int header[2]{mode,len};
				if (SDLNet_TCP_Send(so,(char*)header,8)<8)
					return ERR_SendError;
				else if (len)
				{
					char *data2=nullptr;
					if (XORCode==0)
						data2=(char*)(void*)data;
					else data2=new char[len];
					if (data==nullptr)
						return ERR_NewError;
					if (XORCode!=0)
					{
						unsigned int p=len/4*4;
						for (unsigned int i=0;i<p;i+=4)
							*(int*)(data2+i)=*(int*)(data+i)^XORCode;
						for (unsigned int i=p;i<len;++i)
							data2[i]=data[i]^XORCode>>(i-p)*8&0xFF;//??
					}
					int re=SDLNet_TCP_Send(so,data2,len);
					if (XORCode!=0)
						delete[] data2;
					if (re<len)
						return ERR_SendError;
				}
				return 0;
			}
			
			inline int Send(unsigned int mode,const std::string &str)
			{return Send(mode,str.length(),str.c_str());}
			
			inline int Send(unsigned int mode,const SizedBuffer &buffer)
			{return Send(mode,buffer.size,buffer.data);}
			
			int DisConnect()
			{
				if (!On) return 0;
				On=0;
				SDLNet_TCP_Close(so);
				so=NULL;
				int re;
				SDL_WaitThread(Th_Recv,&re);
				Th_Recv=NULL;
				return re;
			}
			
			int Connect(const std::string &host,unsigned short port)
			{
				DisConnect();
				if (SDLNet_ResolveHost(&IP,host.c_str(),port)!=0)
					return ERR_ResolveError;
				else if ((so=SDLNet_TCP_Open(&IP))==NULL)
					return ERR_TCPOpenError;
				else
				{
					On=1;
					Th_Recv=SDL_CreateThread(ThreadFunc_Recv,"",this);
				}
				return 0;
			}
			
			int Connect(const std::string &hostPort)
			{
				auto vec=DivideStringByChar(hostPort,':');
				if (vec.size()!=2)
					return ERR_TargetError;
				else
				{
					int port=GetAndCheckStringNaturalNumber(vec[1]);
					if (!InRange(port,0,65535))
						return ERR_TargetError;
					return Connect(vec[0],port);
				};
			}
			
			inline bool Connected() const
			{return On;}
			
			inline IPaddress GetIPAddr() const
			{return IP;}
			
			EasySDLNetWithPUI(unsigned int xorcode=0):XORCode(xorcode)
			{
				SDLNet_Init();//??
				On=0;
			}
	};
	const unsigned int EasySDLNetWithPUI::EventType=PAL_GUI::PUI_Event::RegisterEvent();
}

#endif
