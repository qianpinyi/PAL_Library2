#ifndef PAL_SDLNET_EX_CPP
#define PAL_SDLNET_EX_CPP 1

#include "../PAL_BuildConfig.h"

#include "../PAL_BasicFunctions/PAL_BasicFunctions_0.cpp"
#include "../PAL_BasicFunctions/PAL_StringEX.cpp"

#include <atomic>
#include <set>
#include <queue>
#include <iostream>

#include PAL_SDL_netHeaderPath

template <class T> class EasySDLNetServer//It is user's work to avoid thread conflict in this class
{
	public:
		enum
		{
			ERR_None=0,
			ERR_CannotOpen,
			ERR_RecvError,
			ERR_DataIsNULL,
			ERR_SendError,
			ERR_NewError,
			ERR_ClientInvalid
		};
		
		enum
		{
			Event_None=0,
			Event_Accept,
			Event_Lost,
			Event_Closed,
			Event_Gap
		};
		
		class ClientData;
		struct SafeSharedPtr//??
		{
			protected:
				int *count=NULL;
				ClientData *data=NULL;
				SDL_mutex *mu=NULL;
				
				void Deconstruct();
				
				void CopyFrom(const SafeSharedPtr &tar)
				{
					SDL_LockMutex(mu);
					data=tar.data;
					count=tar.count;
					mu=tar.mu;
					if (data!=NULL)
						++*count;
					SDL_UnlockMutex(mu);
				}
				
			public:
				inline void Lock()
				{
					if (mu!=NULL)
						SDL_LockMutex(mu);
				}
				
				inline void Unlock()
				{
					if (mu!=NULL)
						SDL_UnlockMutex(mu);
				}
				
				inline bool operator ! ()
				{return data==NULL;}
				
				inline ClientData* operator -> ()
				{return data;}
				
				inline bool operator == (const SafeSharedPtr &tar) const
				{return data==tar.data;}
				
				inline bool operator != (const SafeSharedPtr &tar) const
				{return data!=tar.data;}
				
				SafeSharedPtr& operator = (const SafeSharedPtr &tar)
				{
					if (&tar==this)
						return *this;
					Deconstruct();
					CopyFrom(tar);
					return *this;
				}
				
				~SafeSharedPtr()
				{Deconstruct();}
				
				SafeSharedPtr(const SafeSharedPtr &tar)
				{CopyFrom(tar);}
				
				explicit SafeSharedPtr(ClientData *tar)
				{
					data=tar;
					if (data!=NULL)
					{
						count=new int(1);
						mu=SDL_CreateMutex();
					}
				}
				
				explicit SafeSharedPtr() {}
		};
	
	protected:
		std::atomic_uint AcceptDelayTime=100;
		std::atomic_bool on;
		unsigned short ListenPort;
		const unsigned int XORCode=0;
		std::set <ClientData*> AllClients;
		std::queue <ClientData*> ClientToDelete;
		SDL_mutex *mu=NULL;//global access needs this mutex,userdata needs user's mutex
		void (*EventFunc)(SafeSharedPtr,int)=NULL;//int:eventtype
		int (*RecvFunc)(SafeSharedPtr,unsigned int,unsigned int,void*)=NULL;//normal return 0; return 1 for handover this data
		
	public:
		T CommonData;

		class ClientData
		{
			friend class EasySDLNetServer;
			friend class SafeSharedPtr;
			protected:
				EasySDLNetServer * const server=NULL;
				IPaddress * const IP=NULL;
				const TCPsocket so;
				std::atomic_bool on;
				
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
					SafeSharedPtr *This=(SafeSharedPtr*)userdata;
					while ((*This)->on)
					{
						unsigned int modelen[2]{0,0};
						char *data=NULL;
						if (RecvLengthData((*This)->so,(char*)modelen,8))
							break;
						else
						{
							data=new char[modelen[1]+1];
							if (data==nullptr)
								break;
							else if (RecvLengthData((*This)->so,data,modelen[1]))
							{
								DELETEtoNULL(data);
								break;
							}
							else
							{
								data[modelen[1]]=0;
								const unsigned int XORCode=(*This)->server->XORCode;
								if (XORCode!=0)
								{
									unsigned int p=modelen[1]/4*4;
									for (unsigned int i=0;i<p;i+=4)
										*(int*)(data+i)^=XORCode;
									for (unsigned int i=p;i<modelen[1];++i)
										data[i]^=XORCode>>(i-p)*8&0xFF;
								}
								if ((*This)->server->RecvFunc!=NULL)
									if ((*This)->server->RecvFunc(*This,modelen[0],modelen[1],data)!=1)
										DELETEtoNULL(data);
							}
						}
					}

					if ((*This)->server->EventFunc!=NULL)
						(*This)->server->EventFunc(*This,(*This)->on?Event_Lost:Event_Closed);
					delete This;
					return 0;
				}
				
			public:
				T UserData;
				
				inline bool Valid() const
				{return on;}
				
				void Close()
				{
					on=0;
					SDLNet_TCP_Close(so);
				}
				
				inline const IPaddress* GetIP() const
				{return IP;}
				
				inline const EasySDLNetServer* Server() const
				{return Server;}
				
				int Send(unsigned int mode,unsigned int len,const char *data)
				{
					if (!on)
						return ERR_ClientInvalid;
					if (data==nullptr&&len!=0)
						return ERR_DataIsNULL;
					unsigned int header[2]{mode,len};
					if (SDLNet_TCP_Send(so,(char*)header,8)<8)
						return ERR_SendError;
					else if (len)
					{
						char *data2=nullptr;
						unsigned XORCode=server->XORCode;
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
								data2[i]=data[i]^XORCode>>(i-p)*8&0xFF;
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
				
				~ClientData()
				{
					Close();
					server->AllClients.erase(this);
				}
				
				ClientData(EasySDLNetServer *_server,IPaddress *_IP,TCPsocket _so):server(_server),IP(_IP),so(_so),on(1){}
		};
		
		int Stop()
		{
			if (on)
			{
				on=0;
				SDL_LockMutex(mu);
				for (auto sp:AllClients)
					sp->Close();
				SDL_UnlockMutex(mu);
				SDL_LockMutex(mu);
				while (!ClientToDelete.empty())
				{
					ClientData *p=ClientToDelete.front();
					ClientToDelete.pop();
					if (AllClients.find(p)!=AllClients.end())
					{
						AllClients.erase(p);
						delete p;
					}
				}
				SDL_UnlockMutex(mu);
			}
			return 0;
		}
		
		int StartListen(unsigned int port)
		{
			Stop();
			ListenPort=port;
			
			IPaddress IP;
			SDLNet_ResolveHost(&IP,NULL,ListenPort);
			TCPsocket soi=SDLNet_TCP_Open(&IP),so;
			if (soi==NULL)
				return ERR_CannotOpen;
			on=1;
			
			while (on)
			{
				SDL_Delay(AcceptDelayTime);
				if ((so=SDLNet_TCP_Accept(soi))!=NULL)
				{
					SafeSharedPtr *data=new SafeSharedPtr(new ClientData(this,SDLNet_TCP_GetPeerAddress(so),so));
					SafeSharedPtr data2=*data;
					SDL_Thread *th=SDL_CreateThread(ClientData::ThreadFunc_Recv,"",(void*)data);
					SDL_DetachThread(th);
					so=NULL;
					if (EventFunc!=NULL)
						EventFunc(data2,Event_Accept);
				}
				SDL_LockMutex(mu);
				while (!ClientToDelete.empty())
				{
					ClientData *p=ClientToDelete.front();
					ClientToDelete.pop();
					if (AllClients.find(p)!=AllClients.end())
					{
						AllClients.erase(p);
						delete p;
					}
				}
				SDL_UnlockMutex(mu);
				if (EventFunc!=NULL)
					EventFunc(SafeSharedPtr(),Event_Gap);
			}
			SDLNet_TCP_Close(soi);
			return 0;
		}
		
		inline void SetRecvFunc(int (*recvfunc)(SafeSharedPtr,unsigned int,unsigned int,void*))
		{RecvFunc=recvfunc;}
		
		inline void SetEventFunc(void (*eventfunc)(SafeSharedPtr,int))
		{EventFunc=eventfunc;}
		
		inline void SetCommonFuncData(const T &commondata)
		{CommonData=commondata;}
		
		inline bool IsRunning() const
		{return on;}
		
		~EasySDLNetServer()
		{
			Stop();
			SDL_DestroyMutex(mu);
		}
		
		EasySDLNetServer(unsigned int xorcode=0,unsigned int acceptdelaytime=100):XORCode(xorcode),AcceptDelayTime(acceptdelaytime)
		{
			SDLNet_Init();
			on=0;
			mu=SDL_CreateMutex();
		}
};
		
template <class T> void EasySDLNetServer<T>::SafeSharedPtr::Deconstruct()
{
	if (data!=NULL)
	{
		SDL_LockMutex(mu);
		--*count;
		if (*count==0)
		{
			SDL_LockMutex(data->server->mu);
			data->server->ClientToDelete.push(data);
			SDL_UnlockMutex(data->server->mu);
			data=NULL;
			DeleteToNULL(count);
			SDL_UnlockMutex(mu);
			SDL_DestroyMutex(mu);
			mu=NULL;
		}
		else SDL_UnlockMutex(mu);
	}
}

#endif
