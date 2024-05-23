#ifndef PAL_IO_PAL_XOUT_1_HPP
#define PAL_IO_PAL_XOUT_1_HPP

#include "../PAL_Config.h"

#include "PAL_Xout_0.hpp"

#include <iostream>
#include <cstdio>
#include <string>
#include <vector>

namespace PAL::IO
{
	class XoutFILEChannel:public XoutChannel
	{
		protected:
			bool allowConseq=false,
				 managedfile=false;
			FILE *f=NULL;
		
			XoutFILEChannel() {}
		public:
			virtual XoutChannel* Dump() const
			{
				XoutFILEChannel *re=new XoutFILEChannel(f);
				re->managedfile=false;
				re->allowConseq=allowConseq;
				return re;
			}
			
			virtual void Putchar(char c,int t=Normal)
			{
				if (!Enabled||f==NULL||t==Conseq&&!allowConseq)
					return;
				fputc(c,f);
			}
			
			virtual void Puts(const char *s,int t=Normal)
			{
				if (s==nullptr||f==NULL||!Enabled||t==Conseq&&!allowConseq)
					return;
				fputs(s,f);
			}
			
			virtual void Flush()
			{
				if (f==NULL||!Enabled)
					return;
				fflush(f);
			}
			
			virtual void Endl()
			{
				if (f==NULL||!Enabled)
					return;
				fflush(f);
			}
			
			void AllowConseq(bool allow)
			{
				allowConseq=allow;
			}
			
			virtual ~XoutFILEChannel()
			{
				if (managedfile&&f!=NULL)
					fclose(f);
			}
			
			XoutFILEChannel(FILE *_f):f(_f) {}
	};
	
	class XoutFileChannel:public XoutFILEChannel
	{
		protected:
			const char *filepath=nullptr,
					   *openmode=nullptr;
			const wchar_t *wfilepath=nullptr,
						  *wopenmode=nullptr;
		
		public:
			virtual XoutChannel* Dump() const
			{
				XoutFileChannel *re=nullptr;
				if (filepath!=nullptr)
					re=new XoutFileChannel(filepath,openmode);
		#if PAL_CurrentPlatform==PAL_Platform_Windows
				else re=new XoutFileChannel(wfilepath,wopenmode);
		#endif
				re->AllowConseq(allowConseq);
				return re;
			}
			
			XoutFileChannel(const char *path,const char *open_mode="w+"):filepath(path),openmode(open_mode)
			{
				f=fopen(path,open_mode);
				if (f!=NULL)
					managedfile=true;
			}
			
		#if PAL_CurrentPlatform==PAL_Platform_Windows
			XoutFileChannel(const wchar_t *path,const wchar_t *open_mode=L"w+"):wfilepath(path),wopenmode(open_mode)
			{
				f=_wfopen(path,open_mode);
				if (f!=NULL)
					managedfile=true;
			}
		#endif
	};
	
	class XoutStdoutChannel:public XoutFILEChannel
	{
		public:
			XoutStdoutChannel():XoutFILEChannel(stdout)
			{
				allowConseq=true;
			}
	};
	
	class XoutStderrChannel:public XoutFILEChannel
	{
		public:
			XoutStderrChannel():XoutFILEChannel(stderr)
			{
				allowConseq=true;
			}
	};
	
	class XoutCoutChannel:public XoutChannel
	{
		protected:
			bool allowConseq=false;
		
		public:
			virtual XoutChannel* Dump() const
			{
				XoutCoutChannel *re=new XoutCoutChannel();
				re->allowConseq=allowConseq;
				return re;
			}
			
			virtual void Putchar(char c,int t=Normal)
			{
				if (!Enabled||t==Conseq&&!allowConseq)
					return;
				std::cout<<c;
			}
			
			virtual void Puts(const char *s,int t=Normal)
			{
				if (s==nullptr||!Enabled||t==Conseq&&!allowConseq)
					return;
				std::cout<<s;
			}
			
			virtual void Flush()
			{
				std::cout<<std::flush;
			}
			
			virtual void Endl()
			{
				std::cout<<std::flush;
			}
			
			void AllowConseq(bool allow)
			{
				allowConseq=allow;
			}
	};
	
	class XoutStringChannel:public XoutChannel
	{
		protected:
			bool allowConseq=false;
			bool managedstr=false;
			std::string *S=nullptr;
		
		public:
			virtual XoutChannel* Dump() const
			{
				XoutStringChannel *re=new XoutStringChannel(*S);
				re->allowConseq=allowConseq;
				return re;
			}
			
			virtual void Putchar(char c,int t=Normal)
			{
				if (!Enabled||t==Conseq&&!allowConseq)
					return;
				Str()+=c;
			}
			
			virtual void Puts(const char *s,int t=Normal)
			{
				if (s==nullptr||!Enabled||t==Conseq&&!allowConseq)
					return;
				Str()+=s;
			}
			
			void AllowConseq(bool allow)
			{
				allowConseq=allow;
			}
			
			std::string& Str()
			{
				return *S;
			}
			
			const std::string& Str() const
			{
				return *S;
			}
			
			virtual ~XoutStringChannel()
			{
				if (managedstr&&S!=nullptr)
					delete S;
			}
			
			XoutStringChannel(const std::string &str=""):managedstr(true),S(new std::string(str)) {}
			XoutStringChannel(std::string *str,bool manage):managedstr(manage),S(str) {}
	};
	
	struct Tab
	{
		int n;
		
		Tab operator * (int x) const
		{
			return Tab(n*x);
		}
		
		Tab(int _n):n(_n) {}
	};
	Tab Tab2=Tab(2),
		Tab4=Tab(4),
		Tab6=Tab(6),
		Tab8=Tab(8),
		Tab16=Tab(16);
	
	inline Xout& operator << (Xout &o,Tab tab)
	{
		if (o.Precheck())
			for (int i=0;i<tab.n;++i)
				o<<" ";
		return o;
	}
	
	inline Xout& operator << (Xout &o,const std::string &str)
	{
		if (o.Precheck())
			o<<str.c_str();
		return o;
	}
	
	template <typename Ta,typename Tb> inline Xout& operator << (Xout &o,const std::pair <Ta,Tb> &pa)
	{
		if (o.Precheck())
			o<<"<"<<pa.first<<","<<pa.second<<">";
		return o;
	}
	
	template <typename T> inline Xout& operator << (Xout &o,const std::vector <T> &vec)//??
	{
		if (o.Precheck())
		{
			o<<"vector "<<&vec<<" with size "<<vec.size()<<":"<<endline;
			for (const auto &vp:vec)
				o<<vp<<endline;
		}
		return o;
	}
};

#endif
