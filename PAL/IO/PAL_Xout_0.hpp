#ifndef PAL_IO_PAL_XOUT_0_HPP
#define PAL_IO_PAL_XOUT_0_HPP

#include "../Enums/SGR_Name.hpp"
#include "../Common/PAL_TemplateTools_0.hpp"

namespace PAL::IO
{
	using namespace PAL::Enums::SGR_Namespace;
	
	class Xout;
	class XoutType
	{
		friend class Xout;
		friend XoutType PredefinedXoutType(int id);
		friend XoutType DebugXoutType(const char *file,const char *func,int line);
		friend Xout& endline(Xout &o);
		friend Xout& endout(Xout &o);
		friend Xout& endl(Xout &o);
		protected:
			enum
			{
				Ope_None,
				Ope_Start,
				Ope_endl,
				Ope_endline,
				Ope_endout
			};
			
			enum{ReservedXoutIDNum=8};
			
			static int XoutTypeIDAssign;
			static void (*FaultSolver)(void);
			
			bool Enabled=true;//checked in Xout
			int id=-1;
			SGR_Name effect0=UnknownSGR,
					 effect1=UnknownSGR;
			const char *name=nullptr;
			
			const char *debug_file=nullptr,
					   *debug_func=nullptr;
			int debug_line=-1;
			
			XoutType(bool enabled,int _id,SGR_Name e0,SGR_Name e1,const char *na,const char *d_file,const char *d_func,int d_line)
			:Enabled(enabled),id(_id),effect0(e0),effect1(e1),name(na),debug_file(d_file),debug_func(d_func),debug_line(d_line) {}
			
			XoutType(int _id,SGR_Name e0,SGR_Name e1,const char *na):XoutType(true,_id,e0,e1,na,nullptr,nullptr,-1) {}
			
			void Operator(Xout &o,int ope);

		public:
			static XoutType RegisterNewType(const char *na,SGR_Name e0=UnknownSGR,SGR_Name e1=UnknownSGR)
			{
				int id=XoutTypeIDAssign++;
				return XoutType(id,e0,e1,na);
			}
			
			static void SetFaultSolver(void (*func)(void))
			{FaultSolver=func;}
			
			void SetEffect(int which,SGR_Name e)
			{
				if (which==0)
					effect0=e;
				else if (which==1)
					effect1=e;
			}
			
			void Enable(bool enabled=true)
			{
				Enabled=enabled;
			}
			
			void Disable()
			{
				Enabled=false;
			}
	};
	
	XoutType PredefinedXoutType(int id)//only used to init basic XoutTypes.
	{
		switch (id)
		{
			case 0:	return XoutType(0,Cyan,UnknownSGR,"Info");
			case 1:	return XoutType(1,Yellow,UnknownSGR,"Warning");
			case 2:	return XoutType(2,Red,UnknownSGR,"Error");
			case 3:	return XoutType(3,LightMagenta,UnknownSGR,"Debug");
			case 4:	return XoutType(4,LightRed,UnknownSGR,"Fault");
			case 5:	return XoutType(5,LightCyan,UnknownSGR,"Test");
			case -1:return XoutType(-1,UnknownSGR,UnknownSGR,"");
			default:return XoutType(-1,UnknownSGR,UnknownSGR,"");
		}
	}
	
	extern XoutType Info,
					Warning,
					Error,
					Debug,
					Fault,
					Test,
					NoneXoutType;
	
	XoutType DebugXoutType(const char *file,const char *func,int line)
	{
		return XoutType(Debug.Enabled,3,LightMagenta,UnknownSGR,"Debug",file,func,line);
	}

	#define DEBug DebugXoutType(__FILE__,__PRETTY_FUNCTION__,__LINE__)
	#define DeBug DebugXoutType(nullptr,__PRETTY_FUNCTION__,__LINE__)
	
	static void SwitchBasicTypeOnoff(int id,bool onoff)
	{
		switch (id)
		{
			case -1:NoneXoutType.Enable(onoff);	break;
			case 0:	Info.Enable(onoff);			break;
			case 1:	Warning.Enable(onoff);		break;
			case 2:	Error.Enable(onoff);		break;
			case 3:	Debug.Enable(onoff);		break;
			case 4:	Fault.Enable(onoff);		break;
			case 5:	Test.Enable(onoff);			break;
		};
	}
	
	class XoutChannel
	{
		public:
			enum
			{
				Normal=0,
				Conseq
			};
		
		protected:
			bool Enabled=true;//used by channel themself while XoutType's Enabled is used by Xout
			
		public:
			virtual XoutChannel* Dump() const=0;
			
			virtual void Putchar(char c,int t=Normal)=0;
			
			virtual void Puts(const char *s,int t=Normal)
			{
				if (s==nullptr||!Enabled)
					return;
				while (*s)
					Putchar(*s++,t);
			}
			
			virtual void Flush() {}
			
			virtual void Endl() {}
			
			void Enable(bool enabled=true)
			{
				Enabled=enabled;
			}
			
			void Disable()
			{
				Enabled=false;
			}
			
			XoutChannel(const XoutChannel &)=delete;
			XoutChannel(XoutChannel &&)=delete;
			XoutChannel& operator = (const XoutChannel &)=delete;
			XoutChannel& operator = (XoutChannel &&)=delete;
			
			virtual ~XoutChannel() {}
			XoutChannel() {}
	};
	
	class XoutMultiChannel:public XoutChannel
	{
		protected:
			struct ChannelList
			{
				XoutChannel *chan=nullptr;
				ChannelList *nxt=nullptr;
				bool managed=false;
				int id=0;
			}head;
			int idassign=0;
			
			#define ForEachChanList(x) for (ChannelList *x=head.nxt;x;x=x->nxt)
			#define ForEachChan ForEachChanList(p) (p->chan)
			
			void Deconstruct(ChannelList *p)
			{
				if (p==nullptr)
					return;
				Deconstruct(p->nxt);
				if (p->managed)
					delete p->chan;
				delete p;
			}
			
		public:
			virtual XoutChannel* Dump() const
			{
				XoutMultiChannel *re=new XoutMultiChannel();
				re->idassign=idassign;
				const ChannelList *p=&head;
				ChannelList *q=&re->head;
				for (;p->nxt;p=p->nxt,q=q->nxt)
				{
					q->nxt=new ChannelList();
					q->nxt->chan=p->nxt->chan->Dump();
					q->nxt->managed=true;
					q->nxt->id=p->nxt->id;
				}
				return re;
			}
			
			virtual void Putchar(char c,int t=Normal)
			{
				if (!Enabled)
					return;
				ForEachChan->Putchar(c,t);
			}
			
			virtual void Puts(const char *s,int t=Normal)
			{
				if (!Enabled)
					return;
				ForEachChan->Puts(s,t);
			}
			
			virtual void Flush()
			{
				if (!Enabled)
					return;
				ForEachChan->Flush();
			}
			
			virtual void Endl()
			{
				if (!Enabled)
					return;
				ForEachChan->Endl();
			}
			
			int Add(XoutChannel *chan,bool managed)
			{
				ChannelList *p=new ChannelList();
				p->chan=chan;
				p->managed=managed;
				p->nxt=head.nxt;
				p->id=++idassign;
				head.nxt=p;
				return p->id;
			}
			
			int Add(const XoutChannel &chan)
			{
				return Add(chan.Dump(),true);
			}
			
			XoutChannel* GetChannel(int id)
			{
				ForEachChanList(p)
					if (p->id==id)
						return p->chan;
				return nullptr; 
			}
			
			bool RemoveChannel(int id)
			{
				for (ChannelList *p=&head;p->nxt;p=p->nxt)
					if (p->nxt->id==id)
					{
						ChannelList *q=p->nxt;
						p->nxt=p->nxt->nxt;
						if (q->managed)
							delete q->chan;
						delete q;
						return true;
					}
				return false;
			}
			
			virtual ~XoutMultiChannel()
			{
				Deconstruct(head.nxt);
				head.nxt=nullptr;
			}
			
			XoutMultiChannel() {}
			
			XoutMultiChannel(XoutChannel *chan0,XoutChannel *chan1,bool managed)
			{
				Add(chan0,managed);
				Add(chan1,managed);
			}
			
			XoutMultiChannel(const XoutChannel &chan0,const XoutChannel &chan1)
			{
				Add(chan0);
				Add(chan1);
			}
			
			XoutMultiChannel(XoutChannel *chan0,XoutChannel *chan1,XoutChannel *chan2,bool managed)
			{
				Add(chan0,managed);
				Add(chan1,managed);
				Add(chan2,managed);
			}
			
			XoutMultiChannel(const XoutChannel &chan0,const XoutChannel &chan1,const XoutChannel &chan2)
			{
				Add(chan0);
				Add(chan1);
				Add(chan2);
			}
			
			#undef ForEachChan
			#undef ForEachChanList
	};

	class Xout
	{
		friend Xout& endline(Xout &o);
		friend Xout& endout(Xout &o);
		friend Xout& endl(Xout &o);
		protected:
			XoutChannel *Chan=nullptr;
			bool ManagedChannel=false;
			bool GlobalEnabled=true;
			XoutType CurrentType=NoneXoutType;
			bool EnableConseq=true,
				 EnabledImprovedFormat=true,
				 EnableEndoutFlush=true;
				
			static int ullTOpadic(char *dst,unsigned size,unsigned long long x,bool isA=1,unsigned char p=16,unsigned w=1)//return str length without last '\0' added.
			{
				if (dst==nullptr||size==0||p>=36||w>size)
					return 0;
				unsigned i=0;
				while (x&&i<size)
				{
					int t=x%p;
					if (t<=9) t+='0';
					else t+=(isA?'A':'a')-10;
					dst[i++]=t;
					x/=p;
				}
				if (i==size&&x)
					return -1;
				if (w!=0)
					while (i<w)
						dst[i++]='0';
				for (int j=(i>>1)-1;j>=0;--j)
					Swap(dst[j],dst[i-j-1]);
				return i;//\0 is not added automaticly!
			}
			
			void SwitchCurrentType(const XoutType &type)
			{
				CurrentType=type;
				CurrentType.Operator(*this,XoutType::Ope_Start);
			}
			
			void PrintHex(unsigned char x,bool isA=1)
			{
				if ((x>>4)<=9)
					Putchar((x>>4)+'0');
				else Putchar((x>>4)-10+(isA?'A':'a'));
				if ((x&0xF)<=9)
					Putchar((x&0xF)+'0');
				else Putchar((x&0xF)-10+(isA?'A':'a'));
			}
			
		public:
			void SetGlobalEnabled(bool enabled)
			{
				GlobalEnabled=enabled;
			}
			
			void SetCurrentTypeEnabled(bool enabled)
			{
				CurrentType.Enable(enabled);
			}
			
			void SetConseqEnabled(bool enabled)
			{
				EnableConseq=enabled;
			}
			
			void SetImprovedFormatEnabled(bool enabled)
			{
				EnabledImprovedFormat=enabled;
			}

			void SetChannel(XoutChannel *chan,bool managed)
			{
				if (Chan!=nullptr&&managed)
					DeleteToNULL(Chan);
				Chan=chan;
				ManagedChannel=managed;
			}
			
			void SetChannel(const XoutChannel &chan)
			{
				SetChannel(chan.Dump(),true);
			}
			
			bool ImprovedFormatEnabled()
			{
				return EnabledImprovedFormat;
			}
			
			bool IsEndoutFlushEnabled()
			{
				return EnableEndoutFlush;
			}

			bool Precheck()//only performed in operator <<, so Putchar and Puts are not controlled.
			{
				return GlobalEnabled&&CurrentType.Enabled;
			}
			
			Xout& operator [] (const XoutType &type)
			{
				SwitchCurrentType(type);
				return *this;
			}
			
			Xout& operator [] (int id)//yes, id is supported to be compatible with DD, but only on basic types.
			{
				switch (id)
				{
					case 0:	SwitchCurrentType(Info);		break;
					case 1:	SwitchCurrentType(Warning);		break;
					case 2:	SwitchCurrentType(Error);		break;
					case 3:	SwitchCurrentType(Debug);		break;
					case 4:	SwitchCurrentType(Fault);		break;
					case 5:	SwitchCurrentType(Test);		break;
					default:SwitchCurrentType(NoneXoutType);break;
				}
				return *this;
			}
			
			Xout& operator [] (const char *type)
			{
				SwitchCurrentType(NoneXoutType);
				return *this<<Reset<<"["<<type<<"] ";
			}
			
			void Putchar(char c,int t=XoutChannel::Normal)
			{
				if (Chan!=nullptr)
					Chan->Putchar(c,t);
			}
			
			void Puts(const char *s,int t=XoutChannel::Normal)
			{
				if (Chan!=nullptr)
					Chan->Puts(s,t);
			}
			
			void PutUint64(unsigned long long x,int t=XoutChannel::Normal)
			{
				constexpr int size=21;//unsigned long long max is of 20 char width, so 21 sized buffer is used.
				char buffer[size];
				int len=ullTOpadic(buffer,size,x,1,10);
				if (len!=-1)
				{
					buffer[len]=0;
					Puts(buffer,t);
				}
			}
			
			void Flush()
			{
				Chan->Flush();
			}
			
			Xout& operator << (SGR_Name effect)
			{
				if (Precheck()&&EnableConseq&&effect!=UnknownSGR)
				{
					Puts("\e[",XoutChannel::Conseq);
					PutUint64(effect,XoutChannel::Conseq);
					Putchar('m',XoutChannel::Conseq);
				}
				return *this;
			}
			
			Xout& operator << (Xout& (*func)(Xout&))
			{
				return func(*this);
			}
			
			Xout& operator << (const char *s)
			{
				if (Precheck())
				{
					if (EnabledImprovedFormat&&s==nullptr)
						Puts("{Empty string}");
					else Puts(s);
				}
				return *this;
			}
			
			Xout& operator << (char *s)
			{
				return *this<<(const char*)s;
			}
			
			Xout& operator << (bool b)
			{
				if (Precheck())
					if (EnabledImprovedFormat)
						Puts(b?"true":"false");
					else Putchar(b?'1':'0'); 
				return *this;
			}
			
			Xout& operator << (char ch)
			{
				if (Precheck())
					Putchar(ch);
				return *this;
			}
			
			Xout& operator << (unsigned char ch)
			{
				if (Precheck())
					Putchar(ch);
				return *this;
			}
			
			Xout& operator << (unsigned long long x)
			{
				if (Precheck())
					PutUint64(x);
				return *this;
			}
			
			Xout& operator << (unsigned int x)
			{
				if (Precheck())
					operator << ((unsigned long long)x);
				return *this;
			}
			
			Xout& operator << (unsigned short x)
			{
				if (Precheck())
					operator << ((unsigned long long)x);
				return *this;
			}
			
			Xout& operator << (unsigned long x)
			{
				if (Precheck())
					operator << ((unsigned long long)x);
				return *this;
			}
			
			Xout& operator << (long long x)
			{
				if (Precheck())
				{
					if (x<0)
					{
						Putchar('-');
						if ((unsigned long long)x!=0x8000000000000000ull)
							x=-x;
					}
					operator << ((unsigned long long)x);
				}
				return *this;
			}
			
			Xout& operator << (int x)
			{
				if (Precheck())
					operator << ((long long)x);
				return *this;
			}
			
			Xout& operator << (short x)
			{
				if (Precheck())
					operator << ((long long)x);
				return *this;
			}
			
			Xout& operator << (long x)
			{
				if (Precheck())
					operator << ((long long)x);
				return *this;
			}
			
			Xout& operator << (double x)
			{
				if (Precheck())
				{
					unsigned long long y=x;
					unsigned long long z=x*1000000;
					while (z%10==0&&z!=0)
						z/=10;
					operator << (y);
					if (z!=0)
						*this<<"."<<z;
				}
				return *this;
			}
			
			Xout& operator << (long double &x)
			{
				if (Precheck())
					operator << ((double)x);
				return *this;
			}
			
			Xout& operator << (float x)
			{
				if (Precheck())
					operator << ((double)x);
				return *this;
			}
			
			template <typename T> Xout& operator << (T *p)
			{
				if (Precheck())
				{
					static_assert(sizeof(p)<=8);
					constexpr int size=17;//unsigned long long max is of 16 char width in hex, so 17 sized buffer is used.
					char buffer[size];
					int len=ullTOpadic(buffer,size,(unsigned long long)p);
					if (len!=-1)
					{
						buffer[len]=0;
						Putchar('0');
						Putchar('x');
						Puts(buffer);
					}
				}
				return *this;
			}
			
			template <typename T,typename ...Ts> Xout& operator << (T(*p)(Ts...))
			{
				if (Precheck())
				{
					operator << ((void*)p);
					if (EnabledImprovedFormat)
						Puts("[F]");
				}
				return *this;
			}
			
//			KOUT& operator << (const DataWithSizeUnited &x)
//			KOUT& operator << (const DataWithSize &x)
//			template <typename T> KOUT& operator << (const T &x)

//			operator >> 
		
		protected:
			template <typename T> void PrintFirst(const char *s,const T &x)
			{
				operator << (x);
				Puts(s);
			}
			
			template <typename T,typename ...Ts> void PrintFirst(const char *s,const T &x,const Ts &...args)
			{
				operator << (x);
				operator () (s,args...);
			}
			
		public:
			template <typename ...Ts> Xout& operator () (const char *s,const Ts &...args)
			{
				if (Precheck())
					while (*s)
						if (*s!='%')
							Putchar(*s),++s;
						else if (*(s+1)=='%')
							Putchar('%'),s+=2;
						else if (InRange(*(s+1),'a','z')||InRange(*(s+1),'A','Z'))//bug need fix here...
						{
							PrintFirst(s+2,args...);
							return *this;
						}
						else Putchar(*s),++s;
				return *this;
			}
			
			~Xout()
			{
				if (ManagedChannel&&Chan!=nullptr)
					DeleteToNULL(Chan);
			}
			
			template <typename ...Ts> Xout(const Ts &...args)
			{
				SetChannel(args...);
			}
			
			Xout() {}
	};
	
	inline Xout& endline(Xout &o)
	{
		o.CurrentType.Operator(o,XoutType::Ope_endline);
		return o;
	}
	
	inline Xout& endout(Xout &o)
	{
		o.CurrentType.Operator(o,XoutType::Ope_endout);
		return o;
	}
	
	inline Xout& endl(Xout &o)
	{
		o.CurrentType.Operator(o,XoutType::Ope_endl);
		return o;
	}
	
	void XoutType::Operator(Xout &o,int ope)
	{
		switch (ope)
		{
			case Ope_Start:
				if (id==-1)
					return;
				if (name==nullptr) o<<Reset<<effect0<<effect1;
				else o<<Reset<<effect0<<effect1<<"["<<name<<"] ";
				break;
			case Ope_endline:
				o<<"\n";
				break;
			case Ope_endl:
				o<<"\n";
			case Ope_endout:
				if (id==3)
				{
					if (debug_file!=nullptr||debug_func!=nullptr||debug_line!=-1)
					{
						o<<"In ";
						if (debug_func!=nullptr)
						{
							o<<"function: "<<debug_func;
							if (debug_line!=-1||debug_file!=nullptr)
								o<<", ";
							else o<<".\n";
						}
						if (debug_line!=-1)
						{
							o<<"line: "<<debug_line;
							if (debug_file!=nullptr)
								o<<", ";
							else o<<".\n";
						}
						if (debug_file!=nullptr)
							o<<"file: "<<debug_file<<".\n";
					}
					o<<Reset;
				}
				else if (id==4)//Fault
				{
					o<<Reset;
					if (o.IsEndoutFlushEnabled())
						o.Flush();
					if (FaultSolver!=nullptr)
						FaultSolver();
				}
				else o<<Reset;
				if (o.IsEndoutFlushEnabled())
					o.Flush();
				break;
		}
	}
	
	extern Xout xout;
};

#endif
