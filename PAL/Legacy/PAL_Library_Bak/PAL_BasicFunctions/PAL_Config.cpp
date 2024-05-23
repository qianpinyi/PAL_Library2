#ifndef PAL_BASICFUNCTIONS_CONFIG_CPP
#define PAL_BASICFUNCTIONS_CONFIG_CPP 1

#include "../PAL_BuildConfig.h"

#include <fstream>
#include "PAL_BasicFunctions_0.cpp"
#include "PAL_Time.cpp"
#include "PAL_StringEX.cpp"
#include "../PAL_DataStructure/PAL_TrieTree.cpp"
#include "PAL_File.cpp"

/*
	XXX V1.0...
*/
class PAL_Config_V1
{
	protected:
		#define PAL_Config_Version "PAL_Config_Version_1.0"
		PAL_DS::TrieTree <std::vector <std::string> > configs;
		std::string cfgFile,cfgName; 
		int lastStat=InitStat;
		bool Locked,NextLocked=0,OpenedSuccessfully=0;
		
		static void SolveAllStrFunc(void *funcdata,const std::string &str,std::vector <std::string> &vec)
		{
			std::ofstream &fout=*((std::ofstream*)funcdata);
			fout<<str;
			for (auto &vp:vec)
				if (vp=="")
					fout<<" {}";
				else if (vp.find(' ')==vp.npos)
					fout<<" "<<vp;
				else fout<<" {"<<vp<<"}";
			fout<<std::endl;
		}
		
		static void SolveAllStrFunc_NoEmpty(void *funcdata,const std::string &str,std::vector <std::string> &vec)
		{
			std::ofstream &fout=*((std::ofstream*)funcdata);
			bool flag=0;
			for (auto &vp:vec)
				if (vp!="")
					flag=1;
			if (flag)
			{
				fout<<str;
				for (auto &vp:vec)
					if (vp=="")
						fout<<" {}";
					else if (vp.find(' ')==vp.npos)
						fout<<" "<<vp;
					else fout<<" {"<<vp<<"}";
				fout<<std::endl;
			}
		}
	
	public:
		enum AutoRWFlag{AutoRWoff,AutoRWon}AutoRW=AutoRWoff;//for easier use;
		
		enum
		{
			OperatedSuccessfully=0,
			InitStat,
			CfgFileNotOpen,
			WrongVersion,
			WrongEndFlag,
			UnableSovle,
			
			NotOpenSucessfully,
			CfgLocked
		};
		
		void SetConfigValue(const std::string &key,const std::string &value)
		{
			PAL_DS::TrieTree<std::vector <std::string> >::TrieTreeNode *p=configs.Find(key);
			if (p==NULL)
				p=configs.Insert(key,std::vector<std::string>(1,""));
			(*p)().clear();
			(*p)().push_back(value);
		}
		
		void PushbackConfigValue(const std::string &key,const std::string &value)
		{
			PAL_DS::TrieTree<std::vector <std::string> >::TrieTreeNode *p=configs.Find(key);
			if (p==NULL)
				p=configs.Insert(key,std::vector<std::string>());
			(*p)().push_back(value);
		}
		
		std::vector <std::string>& operator [] (const std::string &key)
		{
			PAL_DS::TrieTree<std::vector <std::string> >::TrieTreeNode *p=configs.Find(key);
			if (p==NULL)
				p=configs.Insert(key,std::vector<std::string>(1,""));
			return (*p)();
		}
		
		std::string& operator () (const std::string &key,int pos=0)
		{
			PAL_DS::TrieTree<std::vector <std::string> >::TrieTreeNode *p=configs.Find(key);
			if (p==NULL)
				p=configs.Insert(key,std::vector<std::string>(1,""));
			return (*p)()[EnsureInRange(pos,0,(int)(*p)().size()-1)];
		}
		
		inline void Erase(const std::string &key)
		{configs.Erase(key);}
		
		inline void Clear()
		{configs.Clear();}
		
		inline void SetLockNextConfig(bool lock)
		{NextLocked=lock;}
		
		inline void SetCfgFile(const std::string &str)
		{cfgFile=str;}
		
		inline std::string GetCfgFile()
		{return cfgFile;}
		
		inline std::string GetCfgName()
		{return cfgName;}
		
		inline int GetLastStat()
		{return lastStat;}
		
		int Read()
		{
			OpenedSuccessfully=0;
			configs.Clear();
			std::ifstream fin(cfgFile);
			if (fin.is_open())
			{
				std::string Time_Check_Str,Cfg_Version,tmp;
				fin>>Time_Check_Str
				   >>tmp>>Cfg_Version
				   >>tmp>>cfgName
				   >>tmp>>Locked
				   ;
				while (Time_Check_Str[0]<0) Time_Check_Str.erase(0,1);
				if (Cfg_Version==PAL_Config_Version)
				{
					std::string stra,strb,strc;
					while (fin>>stra)
					{
						strb.clear();strc.clear();
						getline(fin,strb);
						int stat=0;//0:solve Blank/table char  1:solve common value  2:solve WithBlankValue
						for (size_t i=0;i<strb.length();++i)
							if (stat==0)
								if (strb[i]==' '||strb[i]=='\t') continue;
								else if (strb[i]=='{') stat=2,strc="";
								else stat=1,strc=strb[i];
							else if (stat==1)
								if (strb[i]==' '||strb[i]=='\t') stat=0,PushbackConfigValue(stra,strc);
								else strc+=strb[i];
							else if (stat==2)
								if (strb[i]=='}'&&(i==strb.length()-1||strb[i+1]==' '||strb[i+1]=='\t')) stat=0,PushbackConfigValue(stra,strc);
								else strc+=strb[i];
						if (stat!=0&&!strc.empty()) PushbackConfigValue(stra,strc);
					}
					fin.close();
					if (stra==Time_Check_Str)
					{
						configs.Erase(stra);
						OpenedSuccessfully=1;
						return lastStat=OperatedSuccessfully;
					}
					
					return lastStat=WrongEndFlag;
				}
				fin.close();
				return lastStat=WrongVersion;
			}
			else return lastStat=CfgFileNotOpen;
		}
		
		int Write(bool NotWriteEmptyConfig=0)
		{
			if (!OpenedSuccessfully) return lastStat=NotOpenSucessfully;
			if (Locked) return lastStat=CfgLocked;
			std::ofstream fout(cfgFile);
			if (fout.is_open())
			{
				std::string OutPut_Time_Flag=GetTime1();
				fout<<ReplaceCharInStr(OutPut_Time_Flag,' ','_')
					<<"Version "<<PAL_Config_Version<<std::endl
					<<"ConfigName "<<ReplaceCharInStr(cfgName,' ','_')<<std::endl
					<<"Locked "<<NextLocked<<std::endl
					<<std::endl;
				configs.GetAllNode(NotWriteEmptyConfig?SolveAllStrFunc_NoEmpty:SolveAllStrFunc,&fout);
				fout<<std::endl<<ReplaceCharInStr(OutPut_Time_Flag,' ','_');
				fout.close();
				return lastStat=OperatedSuccessfully;
			}
			else return lastStat=CfgFileNotOpen;
		}
		
		~PAL_Config_V1()
		{
			if (AutoRW==AutoRWon)
				Write();
		}
		
		PAL_Config_V1(const std::string &_cfgFile,AutoRWFlag autoRW=AutoRWoff)
		:cfgFile(_cfgFile),AutoRW(autoRW)
		{
			if (AutoRW==AutoRWon)
				Read();
		}
		
		PAL_Config_V1() {}
		#undef PAL_Config_Version
};

using PAL_Config=PAL_Config_V1;

#endif
