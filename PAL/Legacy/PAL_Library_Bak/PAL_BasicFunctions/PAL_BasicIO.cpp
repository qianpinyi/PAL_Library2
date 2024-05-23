#ifndef PAL_BASICIO_CPP
#define PAL_BASICIO_CPP 1

#include "../PAL_BuildConfig.h"

#include <cstring>
#include <string>
#include <vector>
#include "PAL_BasicFunctions_0.cpp"

class BasicIO//22.1.23
{
	public:
		enum IOType
		{
			IOType_None=0,
			IOType_CFILE,
			IOType_CppFstream,
			IOType_Win32FileHandle,
			IOType_Memory,
			IOType_Pipe,
			IOType_Socket,
			
			IOType_UserAssigned=10000
		};
		
		enum ErrorType
		{
			ERR_None=0,
			ERR_FailedToOpen,
			ERR_FailedToClose,
			ERR_ParameterWrong,
			ERR_UnsupportedFeature,
			ERR_InvalidIO,
			ERR_UncompleteIO,
			ERR_IOFailed,
			ERR_FailedToFlush,
			ERR_FailedToSeek,
			ERR_FailedToAllocate,
			ERR_SeekOutofRange,
			ERR_ReadOutofRange,
			ERR_WriteOutofRange,
			
			ERR_DeriveError=10000//>10000:Error of derived class
		};
		
		enum FeatureBit
		{
			Feature_Read,//Can read
			Feature_Write,//Can write
			Feature_UnknownSize,//Size is unknown
			Feature_ConstSize,//Size is const
			Feature_Seek,//Can seek(if not,Pos won't work)
			Feature_Stream,//Work as stream(Pointer auto plus)
			Feature_Flush,//Have inner buffer, can flush
			Feature_SharePosRW,//always have PosR==PosW
//			Feature_MultibyteUnit,
//			Feature_BigEndian,
//			Feature_LowEndian,
		};
		
		enum SeekFlag
		{
			Seek_Beg=0,
			Seek_Cur,//if not posR!=posW,it will base on the posR 
			Seek_End,
			SeekR_Beg,
			SeekR_Cur,
			SeekR_End,
			SeekW_Beg,
			SeekW_Cur,
			SeekW_End
		};
		
	protected:
		const IOType type;
		const std::string typeName;
		Uint32 rwUnit=1;//(floor)
		Uint64 Features;
		Uint64 IOSize;
		Uint64 posR,
			   posW;
		ErrorType LastErrorCode=ERR_None;
		bool IsValid=0;//if not valid, operation won't take effect
		
		void SetFeature(FeatureBit feature,bool onoff)
		{
			if (onoff)
				Features|=1<<feature;
			else Features&=~(1<<feature);
		}
		
		BasicIO(IOType _type,const std::string &_typename)
		:type(_type),typeName(_typename) {}
		
	public:
		static IOType AssignIOType()
		{
			static int AssignedCnt=0;
			return IOType((int)IOType_UserAssigned+(++AssignedCnt));
		}
		
		virtual ErrorType Write(const void *data,Uint64 size)=0;
		
		virtual ErrorType Read(void *data,Uint64 size)=0;
		
		virtual ErrorType Seek(Sint64 offset,SeekFlag mode=Seek_Beg)
		{return LastErrorCode=ERR_UnsupportedFeature;}
		
		virtual ErrorType Flush()
		{return LastErrorCode=ERR_UnsupportedFeature;}
		
		template <typename T> ErrorType WriteT(const T &x)
		{return Write(&x,sizeof(T));}
		
		template <typename T> ErrorType ReadT(T &x)
		{return Read(&x,sizeof(T));}
		
		inline ErrorType Read(SizedBuffer &tar)
		{return Read(tar.data,tar.size);}
		
		inline ErrorType Write(SizedBuffer &tar)
		{return Write(tar.data,tar.size);}
		
		inline ErrorType Read(const DataWithSize &tar)
		{return Read(tar.data,tar.size);}
		
		inline ErrorType Write(const DataWithSize &tar)
		{return Write(tar.data,tar.size);}
		
		Uint64 Size() const
		{return IOSize;}
		
		Uint32 RWUnit() const
		{return rwUnit;}
		
		Uint64 PosR() const
		{return posR;}
		
		Uint64 PosW() const
		{return posW;}
		
		bool Valid() const
		{return IsValid;}
		
		ErrorType LastError() const
		{return LastErrorCode;}
		
		bool HaveFeature(FeatureBit feature) const
		{return Features&1<<feature;}
		
		Uint64 GetAllFeature() const
		{return Features;}
		
		const std::string& TypeName() const
		{return typeName;}
		
		IOType Type() const
		{return type;}
		
		virtual ~BasicIO() {}
};

class BasicBinIO//22.1.23
{
	public:
		enum ErrorType
		{
			ERR_None,
			ERR_NoBasicIO,
			ERR_BasicIO,
		};
		
	protected:
		using TagSizeType=Uint32;//??
		
		BasicIO *io=nullptr;
		bool AutoDelete=1;
		ErrorType LastErrorCode=ERR_None;
		
	public:
		BasicIO* IO()
		{return io;}
		
		ErrorType LastError() const
		{return LastErrorCode;}
		
		BasicIO::ErrorType IOError() const
		{return io->LastError();}
		
		BasicBinIO& WriteN(const void *data,Uint64 size)
		{
			if (io==nullptr)
				LastErrorCode=ERR_NoBasicIO;
			else if (io->Write(data,size))
				LastErrorCode=ERR_BasicIO;
			else LastErrorCode=ERR_None;
			return *this;
		}
		
		BasicBinIO& ReadN(void *data,Uint64 size)
		{
			if (io==nullptr)
				LastErrorCode=ERR_NoBasicIO;
			else if (io->Read(data,size))
				LastErrorCode=ERR_BasicIO;
			else LastErrorCode=ERR_None;
			return *this;
		}
		
		template <typename T> BasicBinIO& operator << (const T &x)
		{
			if (io==nullptr)
				LastErrorCode=ERR_NoBasicIO;
			else if (io->WriteT(x))
				LastErrorCode=ERR_BasicIO;
			else LastErrorCode=ERR_None;
			return *this; 
		}
		
		template <typename T> BasicBinIO& operator >> (T &x)
		{
			if (io==nullptr)
				LastErrorCode=ERR_NoBasicIO;
			else if (io->ReadT(x))
				LastErrorCode=ERR_BasicIO;
			else LastErrorCode=ERR_None;
			return *this;
		}
		
		BasicBinIO& operator << (const DataWithSize &x)
		{
			if (io==nullptr)
				LastErrorCode=ERR_NoBasicIO;
			else if (io->Write(x.data,x.size))
				LastErrorCode=ERR_BasicIO;
			else LastErrorCode=ERR_None;
			return *this; 
		}
		
		BasicBinIO& operator >> (DataWithSize &x)
		{
			if (io==nullptr)
				LastErrorCode=ERR_NoBasicIO;
			else if (io->Read(x.data,x.size))
				LastErrorCode=ERR_BasicIO;
			else LastErrorCode=ERR_None;
			return *this;
		}
		
		BasicBinIO& operator << (const std::string &str)
		{
			if (io==nullptr)
				LastErrorCode=ERR_NoBasicIO;
			else
			{
				TagSizeType len=str.length();
				if (io->WriteT(len))
					LastErrorCode=ERR_BasicIO;
				else if (io->Write(str.c_str(),len))
					LastErrorCode=ERR_BasicIO;
				else LastErrorCode=ERR_None;
			}
			return *this;
		}
		
		BasicBinIO& operator >> (std::string &str)
		{
			if (io==nullptr)
				LastErrorCode=ERR_NoBasicIO;
			else
			{
				TagSizeType len=0;
				if (io->ReadT(len))
					LastErrorCode=ERR_BasicIO;
				else if (len!=0)
				{
					str.clear();
					str.resize(len);
					if (io->Read((char*)str.data(),len))//??
						LastErrorCode=ERR_BasicIO;
					else LastErrorCode=ERR_None;
				}
			}
			return *this;
		}
		
		template <typename T> BasicBinIO& operator << (const std::vector <T> &vec)
		{
			if (io==nullptr)
				LastErrorCode=ERR_NoBasicIO;
			else
			{
				TagSizeType cnt=vec.size();
				if (io->WriteT(cnt))
					LastErrorCode=ERR_BasicIO;
				else for (const auto &vp:vec)
					if ((*this<<(vp)).LastError())//!!Do not use "operator << (vp)", which is inner function rather than global.
						break;
			}
			return *this;
		}
		
		template <typename T> BasicBinIO& operator >> (std::vector <T> &vec)
		{
			if (io==nullptr)
				LastErrorCode=ERR_NoBasicIO;
			else
			{
				vec.clear();
				TagSizeType cnt;
				if (io->ReadT(cnt))
					LastErrorCode=ERR_BasicIO;
				else
				{
					vec.resize(cnt,T());
					for (auto &vp:vec)
						if ((*this>>(vp)).LastError())
							break;
				}
			}
			return *this;
		}
		
//		template <typename T> BasicBinIO& operator | (const T &x)
//		{return operator << (x);}
//		
//		template <typename T> BasicBinIO& operator | (T &x)
//		{return operator >> (x);}
		
		~BasicBinIO()
		{
			if (AutoDelete&&io!=nullptr)
				delete io;
		}
		
		BasicBinIO(BasicIO &_io):io(&_io),AutoDelete(0) {}
		
		BasicBinIO(BasicIO *_io,bool autoDelete):io(_io),AutoDelete(autoDelete)
		{
			if (io==nullptr)
				LastErrorCode=ERR_NoBasicIO;
		}
};

class MemoryIO:public BasicIO//22.1.25
{
	public:
		enum WorkingMode
		{
			WorkingMode_None,
			WorkingMode_ConstRW,
			WorkingMode_ConstR,
			WorkingMode_ConstW,
			WorkingMode_RingRW,
			WorkingMode_RingR,
			WorkingMode_RingW,
			WorkingMode_DynamicRW,//similar to vector; Cannot be used in outside target
			WorkingMode_DynamicR,
			WorkingMode_DynamicW
		};
		
	protected:
		WorkingMode Mode;
		char *Mem=nullptr;
		Uint64 MemSize=0;
		bool SelfAllowcated=0;
		
		ErrorType DynamicResize(Uint64 targetSize)//can only be called in WorkingMode_Dynamic(not check)
		{
			Uint64 newMemSize=MemSize;
			while (newMemSize<targetSize)
				newMemSize<<=1;
			if (MemSize==newMemSize)
				return LastErrorCode=ERR_None;
			char *newMem=new char[newMemSize];
			memcpy(newMem,Mem,MemSize);
			memset(newMem+MemSize,0,newMemSize-MemSize);
			delete[] Mem;
			Mem=newMem;
			MemSize=newMemSize;
			return LastErrorCode=ERR_None;
		}
		
		void SetOpenedFeature(WorkingMode mode,bool sharePosRW)
		{
			switch (mode)
			{
				case WorkingMode_None:																										break;
				case WorkingMode_ConstRW:	Features=1<<Feature_Read	|1<<Feature_Write		|1<<Feature_ConstSize	|1<<Feature_Seek;	break;
				case WorkingMode_ConstR:	Features=1<<Feature_Read	|1<<Feature_ConstSize	|1<<Feature_Seek;							break;
				case WorkingMode_ConstW:	Features=1<<Feature_Write	|1<<Feature_ConstSize	|1<<Feature_Seek;							break;
				case WorkingMode_RingRW:	Features=1<<Feature_Read	|1<<Feature_Write		|1<<Feature_UnknownSize;					break;
				case WorkingMode_RingR:		Features=1<<Feature_Read	|1<<Feature_UnknownSize;											break;
				case WorkingMode_RingW:		Features=1<<Feature_Write	|1<<Feature_UnknownSize;											break;
				case WorkingMode_DynamicRW:	Features=1<<Feature_Read	|1<<Feature_Write		|1<<Feature_Seek;							break;
				case WorkingMode_DynamicR:	Features=1<<Feature_Read	|1<<Feature_Seek;													break;
				case WorkingMode_DynamicW:	Features=1<<Feature_Write	|1<<Feature_Seek;													break;
			}
			SetFeature(Feature_Stream,1);
			SetFeature(Feature_SharePosRW,sharePosRW);
		}
		
		ErrorType Open(WorkingMode mode,void *mem,Uint64 memsize,bool selfAllocated)
		{
			if (Close())
				return LastErrorCode;
			if (!InRange(mode,WorkingMode_ConstRW,WorkingMode_DynamicW)||mem==nullptr)
				return LastErrorCode=ERR_ParameterWrong;
			else if (InThisSet(mode,WorkingMode_DynamicRW,WorkingMode_DynamicR,WorkingMode_DynamicW)&&!selfAllocated)
				return LastErrorCode=ERR_ParameterWrong;
			Mode=mode;
			Mem=(char*)mem;
			MemSize=memsize;
			SelfAllowcated=selfAllocated;
			IsValid=1;
			posR=posW=0;
			if (InThisSet(mode,WorkingMode_DynamicRW,WorkingMode_DynamicR,WorkingMode_DynamicW))
				IOSize=0;
			else IOSize=MemSize;
			SetOpenedFeature(mode,1);
			return LastErrorCode=ERR_None;
		}
		
	public:
		virtual ErrorType Write(const void *data,Uint64 size)
		{
			if (data==nullptr)
				return LastErrorCode=ERR_ParameterWrong;
			if (!IsValid)
				return LastErrorCode=ERR_InvalidIO;
			if (!HaveFeature(Feature_Write))
				return LastErrorCode=ERR_UnsupportedFeature;
			if (!InRange(posW,0,IOSize))
				return LastErrorCode=ERR_WriteOutofRange;
			for (Uint64 i=0;i<size;++i)
			{
				if (posW==IOSize)
					if (InThisSet(Mode,WorkingMode_RingRW,WorkingMode_RingW))
						posW=0;
					else if (InThisSet(Mode,WorkingMode_ConstRW,WorkingMode_ConstW))
						return LastErrorCode=ERR_WriteOutofRange;
					else
					{
						if (IOSize==MemSize&&DynamicResize(IOSize+1))
							return LastErrorCode;
						++IOSize;
					}
				Mem[posW]=*((char*)data+i);
				++posW;
			}
			if (HaveFeature(Feature_SharePosRW))
				posR=posW;
			return LastErrorCode=ERR_None;
		}
		
		virtual ErrorType Read(void *data,Uint64 size)
		{
			if (data==nullptr)
				return LastErrorCode=ERR_ParameterWrong;
			if (!IsValid)
				return LastErrorCode=ERR_InvalidIO;
			if (!HaveFeature(Feature_Read))
				return LastErrorCode=ERR_UnsupportedFeature;
			if (!InRange(posR,0,IOSize))
				return LastErrorCode=ERR_ReadOutofRange;
			for (Uint64 i=0;i<size;++i)
			{
				if (posR==IOSize)
					if (InThisSet(Mode,WorkingMode_RingRW,WorkingMode_RingR))
						posR=0;
					else return LastErrorCode=ERR_ReadOutofRange;
				*((char*)data+i)=Mem[posR];
				++posR;
			}
			if (HaveFeature(Feature_SharePosRW))
				posW=posR;
			return LastErrorCode=ERR_None;
		}
		
		virtual ErrorType Seek(Sint64 offset,SeekFlag mode=Seek_Beg)
		{
			if (!InRange(mode,Seek_Beg,SeekW_End))
				return LastErrorCode=ERR_ParameterWrong;
			if (!IsValid)
				return LastErrorCode=ERR_InvalidIO;
			if (!HaveFeature(Feature_Seek))
				return LastErrorCode=ERR_UnsupportedFeature;
			Uint64 LastPosR=posR,
				   LastPowW=posW;
			switch (HaveFeature(Feature_SharePosRW)?mode%3:mode)
			{
				case Seek_Beg:	posR=posW=offset;		break;
				case Seek_Cur:	posR=posW=posR+offset;	break;
				case Seek_End:	posR=posW=IOSize-offset;break;
				case SeekR_Beg:	posR=offset;			break;
				case SeekR_Cur:	posR+=offset;			break;
				case SeekR_End:	posR=IOSize-offset;		break;
				case SeekW_Beg:	posW=offset;			break;
				case SeekW_Cur:	posW+=offset;			break;
				case SeekW_End:	posW=IOSize-offset;		break;
			}
			if (!InRange(posR,0,IOSize)&&!InRange(posW,0,IOSize))
				return posR=LastPosR,posW=LastPowW,LastErrorCode=ERR_SeekOutofRange;
			else if (!InRange(posR,0,IOSize))
				return posR=LastPosR,LastErrorCode=ERR_SeekOutofRange;
			else if (!InRange(posW,0,IOSize))
				return posW=LastPowW,LastErrorCode=ERR_SeekOutofRange;
			else return LastErrorCode=ERR_None;
		}
		
		ErrorType Close()
		{
			if (Mem==nullptr)
				return LastErrorCode=ERR_None;
			if (SelfAllowcated)
				delete[] Mem;
			Mem=nullptr;
			SelfAllowcated=0;
			Mode=WorkingMode_None;
			IsValid=0;
			return LastErrorCode=ERR_None;
		}
		
		inline ErrorType Open(WorkingMode mode,void *mem,Uint64 memsize)
		{return Open(mode,mem,memsize,0);}
		
		ErrorType Open(WorkingMode mode,Uint64 memsize)
		{
			if (memsize==0)
				return LastErrorCode=ERR_ParameterWrong;
			char *p=new char[memsize];
			if (p==nullptr)
				return LastErrorCode=ERR_FailedToAllocate;
			else
			{
				memset(p,0,memsize);
				if (Open(mode,p,memsize,1))
				{
					delete[] p;
					return LastErrorCode;
				}
				else return LastErrorCode=ERR_None;
			}
		}
		
		inline ErrorType Open(Uint64 memsize)
		{return Open(WorkingMode_ConstRW,memsize);}
		
		inline ErrorType Open()
		{return Open(WorkingMode_DynamicRW,256);}
		
		inline void* Memory()
		{return Mem;}
		
		inline Uint64 MemorySize() const
		{return MemSize;}
		
		virtual ~MemoryIO()
		{
			Close();
		}
		
		MemoryIO& operator = (const MemoryIO &tar)=delete;
		MemoryIO& operator = (const MemoryIO &&tar)=delete;
		MemoryIO(const MemoryIO &tar)=delete;
		MemoryIO(const MemoryIO &&tar)=delete;
		
		template <typename...Ts> MemoryIO(const Ts &...args):BasicIO(IOType_Memory,"MemoryIO")
		{
			Open(args...);
		}
};

#endif 
