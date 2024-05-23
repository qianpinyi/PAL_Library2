#ifndef PAL_BASICFUNCTIONS_FILE_CPP
#define PAL_BASICFUNCTIONS_FILE_CPP 1

#include "../PAL_BuildConfig.h"

#include <string>
#include <vector>
#include <fstream>
#include <algorithm>

#include "PAL_Charset.cpp"
#include "PAL_StringEX.cpp"
#include "PAL_BasicIO.cpp"
#include "../PAL_DataStructure/PAL_Tuple.cpp"

#if PAL_CurrentPlatform == PAL_Platform_Windows
	#include <io.h>
#elif PAL_CurrentPlatform == PAL_Platform_Linux
	#include <dirent.h>
	#include <sys/stat.h> 
#endif

namespace PAL::Legacy
{
	class CFileIO:public BasicIO//Recommended compiler flag: -D_FILE_OFFSET_BITS=64
	{
		public:
			enum OpenMode
			{
				OpenMode_None=0,
				OpenMode_ReadOnly,
				OpenMode_WriteOnly,
				OpenMode_AppendOnly,
				OpenMode_RWExist,
				OpenMode_RWEmpty,
				OpenMode_RWAppend
			};
		
		protected:
			FILE *fp;
			bool isOpen=0;
			bool NeedClose=0;
			
			ErrorType Open(FILE *_fp,bool needClose)
			{
				if (isOpen)
					Close();
				if (_fp==nullptr)
					return LastErrorCode=ERR_FailedToOpen;
				fp=_fp;
				NeedClose=needClose;
				isOpen=1;
				IsValid=1;
				posR=posW=ftell(fp);
				fseek(fp,0,SEEK_END);
	//			_fseeki64(fp,0,SEEK_END);
				IOSize=ftell(fp);
				fseek(fp,posR,SEEK_SET);
	//			_fseeki64(fp,posR,SEEK_END);
				return LastErrorCode=ERR_None;
			}
			
			void Open_SetFeature(OpenMode mode)
			{
				switch (mode)
				{
					case OpenMode_ReadOnly:
						SetFeature(Feature_Read,1);
						SetFeature(Feature_Write,0);
						return;
					case OpenMode_WriteOnly:
					case OpenMode_AppendOnly:
						SetFeature(Feature_Read,0);
						SetFeature(Feature_Write,1);
						return;
					case OpenMode_RWExist:
					case OpenMode_RWEmpty:	
					case OpenMode_RWAppend:		
						SetFeature(Feature_Read,1);
						SetFeature(Feature_Write,1);
						return;	
					default:	
						SetFeature(Feature_Read,0);
						SetFeature(Feature_Write,0);
						return;	
				}
			}
			
		public:
			using BasicIO::Read;//??
			using BasicIO::Write;
			
			virtual ErrorType Write(const void *data,Uint64 size)
			{
				if (data==nullptr)
					return LastErrorCode=ERR_ParameterWrong;
				if (!IsValid)
					return LastErrorCode=ERR_InvalidIO;
				if (!HaveFeature(Feature_Write))
					return LastErrorCode=ERR_UnsupportedFeature;
				Sint64 re=fwrite(data,1,size,fp);
				if (re==0)
					return LastErrorCode=ERR_IOFailed;
				posR=posW=ftell(fp);//?
				if (posR>IOSize)//???
					IOSize=posR;
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
				Sint64 re=fread(data,1,size,fp);
				if (re==0)
					return LastErrorCode=ERR_IOFailed;
				posR=posW=ftell(fp);
				return LastErrorCode=ERR_None;
			}
			
			virtual ErrorType Seek(Sint64 offset,SeekFlag mode=Seek_Beg)
			{
				if (!InRange(mode,Seek_Beg,SeekW_End))
					return LastErrorCode=ERR_ParameterWrong;
				if (!IsValid)
					return LastErrorCode=ERR_InvalidIO;
				static int seekmode[3]{SEEK_SET,SEEK_CUR,SEEK_END};
				if (fseek(fp,offset,seekmode[mode%3]))
	//			if (_fseeki64(fp,offset,seekmode[mode%3]))
					return LastErrorCode=ERR_FailedToSeek;
				posR=posW=ftell(fp);
				return LastErrorCode=ERR_None;
			}
			
			virtual ErrorType Flush()//??
			{
				if (!IsValid)
					return LastErrorCode=ERR_InvalidIO;
				if (fflush(fp))
					return LastErrorCode=ERR_FailedToFlush;
				else return LastErrorCode=ERR_None;
			}
			
			ErrorType Close()
			{
				if (!isOpen)
					return LastErrorCode=ERR_None;
				if (NeedClose)
					fclose(fp);//How about return value?
				NeedClose=0;
				isOpen=0;
				IsValid=0;
				fp=nullptr;
				return LastErrorCode=ERR_None;
			}
			
	//		ErrorType Open(FILE *_fp)
	//		{return Open(_fp,0);}
		
		#if PAL_CurrentPlatform==PAL_Platform_Windows
			ErrorType Open(const wchar_t *path,OpenMode mode)
			{
				if (path==nullptr)
					return ERR_ParameterWrong;
				Open_SetFeature(mode);
				switch (mode)
				{
					case OpenMode_ReadOnly:		return Open(_wfopen(path,L"rb"),1);
					case OpenMode_WriteOnly:	return Open(_wfopen(path,L"wb"),1);
					case OpenMode_AppendOnly:	return Open(_wfopen(path,L"ab"),1);
					case OpenMode_RWExist:		return Open(_wfopen(path,L"rb+"),1);
					case OpenMode_RWEmpty:		return Open(_wfopen(path,L"wb+"),1);
					case OpenMode_RWAppend:		return Open(_wfopen(path,L"ab+"),1);
					default:					return LastErrorCode=ERR_ParameterWrong;
				}
			}
			
			ErrorType Open(const char *path,OpenMode mode)
			{return Open(Charset::Utf8ToUnicode(path),mode);}
		
			ErrorType Open(const std::wstring &path,OpenMode mode)
			{return Open(path.c_str(),mode);}

			ErrorType Open(const std::string &path,OpenMode mode)
			{return Open(Charset::Utf8ToUnicode(path),mode);}
		#elif PAL_CurrentPlatform==PAL_Platform_Linux
			ErrorType Open(const char *path,OpenMode mode)
			{
				if (path==nullptr)
					return ERR_ParameterWrong;
				Open_SetFeature(mode);
				switch (mode)
				{
					case OpenMode_ReadOnly:		return Open(fopen(path,"rb"),1);
					case OpenMode_WriteOnly:	return Open(fopen(path,"wb"),1);
					case OpenMode_AppendOnly:	return Open(fopen(path,"ab"),1);
					case OpenMode_RWExist:		return Open(fopen(path,"rb+"),1);
					case OpenMode_RWEmpty:		return Open(fopen(path,"wb+"),1);
					case OpenMode_RWAppend:		return Open(fopen(path,"ab+"),1);
					default:					return LastErrorCode=ERR_ParameterWrong;
				}
			}
			
	//		ErrorType Open(const wchar_t *path,OpenMode mode)
	//		{return Open(path,mode);}
		
	//		ErrorType Open(const std::wstring &path,OpenMode mode)
	//		{return Open(path.c_str(),mode);}

			ErrorType Open(const std::string &path,OpenMode mode)
			{return Open(path,mode);}
		#else
			#error TODO
		#endif
		
			inline bool IsOpen() const
			{return isOpen;}
			
			virtual ~CFileIO()
			{
				Close();
			}
			
			CFileIO& operator = (const CFileIO &tar)=delete;
			CFileIO& operator = (const CFileIO &&tar)=delete;
			CFileIO(const CFileIO &tar)=delete;
			CFileIO(const CFileIO &&tar)=delete;
			
			CFileIO():BasicIO(IOType_CFILE,"CFileIO")
			{
				Features=1<<Feature_Seek|1<<Feature_Stream|1<<Feature_Flush|1<<Feature_SharePosRW;
			}
			
			template <typename...Ts> CFileIO(const Ts &...args):CFileIO()
			{
				Open(args...);
			}
	};

	class FstreamIO:public BasicIO
	{
		protected:
			
			
		public:
			
			
	};

	template <typename T> inline void ReadDataBin(std::ifstream &fin,T &x)
	{fin.read((char*)&x,sizeof(x));}

	template <typename T> inline void WriteDataBin(std::ofstream &fout,const T &x)
	{fout.write((char*)&x,sizeof(x));}

	std::string ReadTextFromBasicIO(BasicIO &io)
	{
		if (!io.Valid())
			return "";
		std::string re;
		char ch=-1;
		while (io.ReadT(ch)==0)
			if (ch==-1)
				break;
			else if (NotInSet(ch,0,'\r'))//??
				re+=ch;
		return re;
	}

	int WriteTextToBasicIO(BasicIO &io,const std::string &str,bool haveeof=true)
	{
		if (!io.Valid())
			return 1;
		io.Write(str.c_str(),str.length());
		char eof=-1;
		if (haveeof)
			io.WriteT(eof);
		return 0;
	}

	std::string ReadTextFromFile(const std::string &path)
	{
		std::ifstream fin(Charset::Utf8ToAnsi(path));//??
		if (!fin.is_open()) return "";
		std::string re,str;
		while (getline(fin,str))
			re+=str,re+="\n";
		return re;
	}

	int WriteTextToFile(const std::string &str,const std::string &path)
	{
		std::ofstream fout(Charset::Utf8ToAnsi(path));//??
		if (!fout.is_open()) return 1;
		fout<<str;
		return 0;
	}

	struct CSVFileData
	{
		std::vector <std::string> Title;
		std::vector <std::vector <std::string> > Data;
		
		inline std::vector <std::string>& operator [] (int p)
		{return Data[p];}
		
		inline std::vector <std::string>& Back()
		{return Data.back();}
		
		inline void Insert(int p,const std::vector <std::string> &tar=std::vector <std::string>())
		{Data.insert(Data.begin()+p,tar);}
		
		inline void AddLine(const std::vector <std::string> &tar=std::vector <std::string>())
		{Data.push_back(tar);}
		
		inline void DeleteLine(int p)
		{Data.erase(Data.begin()+p);}
		
		inline void Clear()
		{
			Title.clear();
			Data.clear();
		}
		
		inline size_t Size() const
		{return Data.size();}

		inline size_t Columns() const
		{return Title.size();}

		void ConvertFromCSVLine(const std::string &str,std::vector <std::string> &vec)
		{
			vec.clear();
			std::string s;
			int flag=0;
			for (size_t i=0;i<str.length();++i)
				if (str[i]=='\"')
					if (flag)
						if (i+1<str.length()&&str[i+1]=='\"')
							s+='\"',++i;
						else flag=0;
					else flag=1;
				else if (str[i]==',')
					if (flag==0) vec.push_back(s),s="";
					else s+=str[i];
				else s+=str[i];
			vec.push_back(s);
		}
		
		void ConvertToCSVLine(const std::vector <std::string> &vec,std::string &str)
		{
			str="";
			bool first=1;
			for (auto vp:vec)
			{
				if (first)
					first=0;
				else str+=",";
				std::string s;
				bool flag=0;
				for (auto sp:vp)
					switch (sp)//here not need break;
					{
						case '\"':	flag=1;	s+='\"';
						case ',':	flag=1;
						default:	s+=sp;
					}
				if (flag)
					str+="\""+s+"\"";
				else str+=s;
			}
		}

		inline int Read(std::ifstream &fin,bool HaveTitle=1)
		{
			if (!fin.is_open())
				return 1;
			Clear();
			std::string str;
			if (HaveTitle)
			{
				getline(fin,str);
				ConvertFromCSVLine(str,Title);
			}
			while (getline(fin,str))
			{
				Data.push_back(std::vector<std::string>());
				ConvertFromCSVLine(str,Data.back());
			}
			return 0;
		}
		
		inline int Read(const std::string &path,bool HaveTitle=1)
		{
			std::ifstream fin(path);
			return Read(fin,HaveTitle);
		}
		
		inline int Write(std::ofstream &fout,bool HaveTitle=1)
		{
			if (!fout.is_open())
				return 1;
			std::string str;
			if (HaveTitle)
			{
				ConvertToCSVLine(Title,str);
				fout<<str<<std::endl;
			}
			for (auto vp:Data)
			{
				ConvertToCSVLine(vp,str);
				fout<<str<<std::endl;
			}
			return 0;
		}
		
		inline int Write(const std::string &path,bool HaveTitle=1)
		{
			std::ofstream fout(path);
			return Write(fout,HaveTitle);
		}
	};

	//class FileInfo
	//{
	//	public:
	//		unsigned int type=0,//string hash of aftername
	//					 size=0;
	//		std::string path,showName;
	//		int userCode=0;//user define
	//		void *userData=NULL;
	//		
	//		
	//		
	//};

	std::vector <std::string>* GetAllFile_UTF8(std::string str,bool fileFlag)
	{
		using namespace std;
		vector <string> *ret=new vector <string>;
		
	#if PAL_CurrentPlatform == PAL_Platform_Windows
		wstring wstr=Charset::Utf8ToUnicode(str+"\\*");
		intptr_t hFiles=0;
		_wfinddata_t da;
		if ((hFiles=_wfindfirst(wstr.c_str(),&da))!=-1)
		{
			do
			{
				str=DeleteEndBlank(Charset::UnicodeToUtf8(da.name));
				//str.erase(str.length()-1);
				if (!(da.attrib&_A_HIDDEN))
					if (da.attrib&_A_SUBDIR)
					{
						if (!fileFlag)
							if (str!="."&&str!="..")
								ret->push_back(str);
					}
					else
					{
						if (fileFlag)
							ret->push_back(str);
					}
			}
			while (_wfindnext(hFiles,&da)==0);
		}
		_findclose(hFiles);
	#elif PAL_CurrentPlatform == PAL_Platform_Linux
		str+="/";
	    DIR *dir;
	    struct dirent *entry;
	    struct stat statbuf;
		if ((dir=opendir(str.c_str()))==NULL)
			return DeleteToNULL(ret);
		while ((entry=readdir(dir))!=NULL)
		{
			string name=entry->d_name;
			if (name=="."||name=="..")
				continue;
			if (stat((str+name).c_str(),&statbuf)==-1)
				continue;
			if (S_ISDIR(statbuf.st_mode))
				if (!fileFlag)
					ret->push_back(name);
				else DoNothing;
			else
				if (fileFlag)
					ret->push_back(name);
		}
		closedir(dir);
	#endif

		return ret;
	}

	std::vector <std::string>* GetAllFile_UTF8_SortWithNum(const std::string &str,bool fileFlag)
	{
		using namespace std;
		vector <string> *ret=GetAllFile_UTF8(str,fileFlag);
		if (ret!=nullptr)
			sort(ret->begin(),ret->end(),[](const string &a,const string &b)->bool
				{
					return SortComp_WithNum(a,b);
				});
		return ret;
	}

	std::vector <std::pair<std::string,int> >* GetAllFile_UTF8(std::string str)
	{
		using namespace std;
		vector <pair <string,int> > *ret=new vector <pair <string,int> >;

	#if PAL_CurrentPlatform == PAL_Platform_Windows
		wstring wstr=Charset::Utf8ToUnicode(str+"\\*");
		intptr_t hFiles=0;
		_wfinddata_t da;
		if ((hFiles=_wfindfirst(wstr.c_str(),&da))!=-1)
		{
			do
			{
				str=DeleteEndBlank(Charset::UnicodeToUtf8(da.name));
				if (!(da.attrib&_A_HIDDEN))
					if (da.attrib&_A_SUBDIR)
					{
						if (str!="."&&str!="..")
							ret->push_back({str,0});
					}
					else ret->push_back({str,1});
			}
			while (_wfindnext(hFiles,&da)==0);
		}
		_findclose(hFiles);
	#elif PAL_CurrentPlatform == PAL_Platform_Linux
		str+="/";
	    DIR *dir;
	    struct dirent *entry;
	    struct stat statbuf;
		if ((dir=opendir(str.c_str()))==NULL)
			return DeleteToNULL(ret);
		while ((entry=readdir(dir))!=NULL)
		{
			string name=entry->d_name;
			if (str=="."||str=="..")
				continue;
			if (stat((str+name).c_str(),&statbuf)==-1)
				continue;
			ret->push_back({name,((S_ISDIR(statbuf.st_mode))?0:1)});
		}
		closedir(dir);
	#endif
		return ret;
	}

	std::vector <PAL_DS::Triplet<std::string,bool,unsigned long long> >* GetAllFile_UTF8_WithSize(std::string str)
	{
		using namespace std;
		auto *ret=new vector <PAL_DS::Triplet<std::string,bool,unsigned long long> >;
	
	#if PAL_CurrentPlatform == PAL_Platform_Windows
		wstring wstr=Charset::Utf8ToUnicode(str+"\\*");
		intptr_t hFiles=0;
		_wfinddata_t da;
		if ((hFiles=_wfindfirst(wstr.c_str(),&da))!=-1)
		{
			do
			{
				str=DeleteEndBlank(Charset::UnicodeToUtf8(da.name));
				if (!(da.attrib&_A_HIDDEN))
					if (da.attrib&_A_SUBDIR)
					{
						if (str!="."&&str!="..")
							ret->push_back({str,false,da.size});
					}
					else ret->push_back({str,true,da.size});
			}
			while (_wfindnext(hFiles,&da)==0);
		}
		_findclose(hFiles);
	#elif PAL_CurrentPlatform == PAL_Platform_Linux
		str+="/";
	    DIR *dir;
	    struct dirent *entry;
	    struct stat statbuf;
		if ((dir=opendir(str.c_str()))==NULL)
			return DeleteToNULL(ret);
		while ((entry=readdir(dir))!=NULL)
		{
			string name=entry->d_name;
			if (str=="."||str=="..")
				continue;
			if (stat((str+name).c_str(),&statbuf)==-1)
				continue;
			ret->push_back({name,((S_ISDIR(statbuf.st_mode))?false:true),(unsigned long long)statbuf.st_size});
		}
		closedir(dir);
	#endif
		return ret;
	}
}

#endif
