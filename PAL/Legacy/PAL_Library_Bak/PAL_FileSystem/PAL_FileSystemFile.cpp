//#ifndef PAL_FILESYSTEMFILE_CPP
//#define PAL_FILESYSTEMFILE_CPP 1
//
//#include "../PAL_BuildConfig.h"
//
//#include <string>
//#include <stack>
//#include <vector>
//#include <map>
//#include <fstream>
//
//#include "../PAL_BasicFunctions/PAL_BasicFunctions_0.cpp"
//#include "../PAL_BasicFunctions/PAL_StringEX.cpp"
//
//#define PAL_FSF_Thumb_VersionCode 1
//
//class PAL_FSF_Thumb
//{
//	public:
//		enum
//		{
//			Error_None=0,
//			Error_Unknown=-1,
//			Error_BadOpenMode=-2,
//			Error_NowNotOpen=-3,
//			Error_NowIsOpen=-4,
//			Error_FileNotExsit=-5,
//			Error_FileCannotOpen=-6,
//			Error_BadDataFormat=-7,
//			Error_MainKeyNotExsit=-8,
//			Error_IndexOutOfRange=-9,
//		};
//		
//		enum
//		{
//			OpenMode_Normal=0,
//			OpenMode_NotCreate=1,
//			
//		};
//		
//	protected:
//		struct EachThumbIndex
//		{
//			unsigned int MessageCount=0,//In number
//						 ClusterPos=0,//In cluster,count from 1, 0 means empty
//						 ThumbDataSize=0,//In bytes
//						 PaddingSize=0;//In bytes
//			std::string MainKey;
//			std::vector <PAL_DS::Doublet<std::string,std::string> > MetaMessages;
//			int code=0;
//			bool Deleted=0;
//		};
//		unsigned int ClusterSize=4096;//In bytes
//		unsigned int StartTaggingFlag=0,
//					 ThumbDataStartCluster=0,//In cluster
//					 MetaMessageStartPos=0,//In bytes
//					 EmptyClusterStartPos=0,//In bytes
//					 ThumbDataStartPos=0,//In bytes
//					 ThumbDataClusterCount=0,//In number
//					 EffectiveDataSpace=0,//In bytes 
//					 MetaMessageCount=0,//In number 
//					 EmptyClusterCount=0,//In number 
//					 ThumbDataCount=0,//In number
//					 PaddingSpaceSize=0;//In bytes
//		std::vector <PAL_DS::Doublet<std::string,std::string> > MetaMessage;
//		std::stack <unsigned int> EmptyClusterPositions;
//		std::map <std::string,int> ThumbDataKeyToPos;
//		std::vector <EachThumbIndex> ThumbDataIndexs;//Remember plus 1 when using index
//		std::string Path;
//		int ThumbIsOpen=0;//0:Ready 1:OK
//		unsigned int OpenMode=0;
//		std::fstream FileStm;//Is c style fread faster? 
//		//index count from 1,0 means empty...
//		
//		void Reinit()
//		{
//			ThumbDataStartCluster=0;
//			MetaMessageStartPos=0;
//			EmptyClusterStartPos=0;
//			ThumbDataStartPos=0;
//			ThumbDataClusterCount=0;
//			EffectiveDataSpace=0;
//			MetaMessageCount=0;
//			EmptyClusterCount=0;
//			ThumbDataCount=0;
//			PaddingSpaceSize=0;
//			MetaMessage.clear();
//			EmptyClusterPositions.clear();
//			ThumbDataKeyToPos.clear();
//			ThumbDataIndexs.clear();
//			path="";
//			ThumbIsOpen=0;
//			OpenMode=0;
//			if (FileStm.is_open())
//				FileStm.close();
//		}
//		
//		void ApplyMetaModify()
//		{
//			//...
//		}
//		
//		void ApplyThumbModify()
//		{
//			//...
//		}
//		
//		bool IsFileExsit(const std::string &path)
//		{
//			//...
//		}
//		
//		int CreateThumbFile(const std::string &path)
//		{
//			//...
//			
//		}
//		
//		int ReadMetaData()
//		{
//			//...
//			
//			
//			ThumbIsOpen=1;
//		}
//		
//		int WriteMetaData()
//		{
//			//...
//		}
//		
//	public:
//		int Find(const std::string &key) const
//		{
//			if (!ThumbIsOpen)
//				return Error_NowNotOpen;
//			auto mp=ThumbDataKeyToPos.find(key);
//			if (mp==ThumbDataKeyToPos.end())
//				return Error_MainKeyNotExsit;
//			return mp->second;
//		}
//		
//		inline int operator [] (const std::string &key) const
//		{return Find(key);}
//		
//		int Read(int index,unsigned char *data,unsigned int maxsize)
//		{
//			if (!ThumbIsOpen)
//				return Error_NowNotOpen;
//			//...
//				
//				
//		}
//		
//		int Read(const std::string &key,unsigned char *data,unsigned int maxsize)
//		{
//			if (!ThumbIsOpen)
//				return Error_NowNotOpen;
//			//...
//				
//				
//		}
//		
//		int Write(int index,const unsigned char *data,unsigned int size)
//		{
//			if (!ThumbIsOpen)
//				return Error_NowNotOpen;
//			//...
//				
//				
//		}
//		
//		int Write(const std::string &key,const unsigned char *data,unsigned int size)
//		{
//			if (!ThumbIsOpen)
//				return Error_NowNotOpen;
//			//...
//				
//				
//		}
//		
//		int DeleteThumb(int index)
//		{
//			if (!ThumbIsOpen)
//				return Error_NowNotOpen;
//			//...
//				
//				
//		}
//		
//		inline int DeleteThumb(const std::string &key)
//		{
//			int p=Find(key);
//			if (p>=0)
//				return DeleteThumb(p);
//			else return p;
//		}
//
//		int InsertThumb(const std::string &key,const unsigned char *data,unsigned int size)
//		{
//			if (!ThumbIsOpen)
//				return Error_NowNotOpen;
//			//...
//				
//				
//		}
//
//		int InsertThumb(const std::string &key)
//		{
//			if (!ThumbIsOpen)
//				return Error_NowNotOpen;
//			//...
//				
//				
//		}
//		
//		int SetMessage(const std::string &msgkey,const std::string &msgvalue)
//		{
//			if (!ThumbIsOpen)
//				return Error_NowNotOpen;
//			//...
//				
//				
//		}
//		
//		int SetMessage(int index,const std::string &msgkey,const std::string &msgvalue)
//		{
//			if (!ThumbIsOpen)
//				return Error_NowNotOpen;
//			//...
//				
//				
//		}
//		
//		int SetMessage(const std::string &key,const std::string &msgkey,const std::string &msgvalue)
//		{
//			int p=Find(key);
//			if (p>=0)
//				return SetMessage(p,msgkey,msgvalue);
//			else return p;
//		}
//		
//		PAL_DS::Doublet <std::string,unsigned int> GetMessage(const std::string &msgkey)
//		{
//			if (!ThumbIsOpen)
//				return {"",Error_NowNotOpen};
//			//...
//				
//				
//		}
//		
//		PAL_DS::Doublet <std::string,unsigned int> GetMessage(int index,const std::string &msgkey)
//		{
//			if (!ThumbIsOpen)
//				return {"",Error_NowNotOpen};
//			//...
//				
//				
//		}
//		
//		PAL_DS::Doublet <std::string,unsigned int> GetMessage(const std::string &key,const std::string &msgkey)
//		{
//			int p=Find(key);
//			if (p>=0)
//				return GetMessage(p,msgkey);
//			else return {"",p};
//		}
//		
//		inline PAL_DS::Doublet <std::string,unsigned int> GetMainKey(int index)
//		{
//			if (!ThumbIsOpen)
//				return {"",Error_NowNotOpen};
//			if (InRange(index,1,ThumbDataIndexs.size()))	
//				return {ThumbDataIndexs[index-1].MainKey,0};
//			else return {"",Error_IndexOutOfRange};
//		}
//		
//		int DeleteMessage(const std::string &msgkey)
//		{
//			if (!ThumbIsOpen)
//				return Error_NowNotOpen;
//			//...
//				
//				
//		}
//		
//		int DeleteMessage(int index,const std::string &msgkey)
//		{
//			if (!ThumbIsOpen)
//				return Error_NowNotOpen;
//			//...
//				
//				
//		}
//		
//		inline int DeleteMessage(const std::string &key,const std::string &msgkey)
//		{
//			int p=Find(key);
//			if (p>=0)
//				return DeleteMessage(p,msgkey);
//			else return p;
//		}
//		
//		PAL_DS::Doublet <std::vector <std::string>,unsigned int> GetAllMessage()
//		{
//			if (!ThumbIsOpen)
//				return {std::vector<std::string>(),Error_NowNotOpen};
//			//...
//				
//				
//				
//			return {};
//		}
//		
//		PAL_DS::Doublet <std::vector <std::string>,unsigned int> GetAllMessage(int index)
//		{
//			if (!ThumbIsOpen)
//				return {std::vector<std::string>(),Error_NowNotOpen};
//			//...
//
//
//		}
//		
//		inline PAL_DS::Doublet <std::vector <std::string>,unsigned int> GetAllMessage(const std::string &key)
//		{
//			int p=Find(key);
//			if (p>=0)
//				return GetAllMessage(p);
//			else return {std::vector<std::string>(),p};
//		}
//		
//		int DeleteAllMessage()
//		{
//			if (!ThumbIsOpen)
//				return Error_NowNotOpen;
//			MetaMessage.clear();
//			ApplyMetaModify();
//			return 0;
//		}
//		
//		int DeleteAllMessage(int index)
//		{
//			if (!ThumbIsOpen)
//				return Error_NowNotOpen;
//			if (!InRange(index,1,ThumbDataIndexs.size()))
//				return Error_IndexOutOfRange;
//			ThumbDataIndexs[index-1].MetaMessages.clear();
//			ApplyMetaModify();
//			return 0;
//		}
//		
//		inline int DeleteAllMessage(const std::string &key)
//		{
//			int p=Find(key);
//			if (p>=0)
//				return DeleteAllMessage(p);
//			else return p;
//		}
//		
//		inline bool NeedMaintain() const
//		{return EmptyClusterCount>ThumbDataClusterCount*0.3;}
//		
//		int MainTain()
//		{
//			if (!ThumbIsOpen)
//				return Error_NowNotOpen;
//			//...
//			
//			
//			return 0;
//		}
//		
//		inline unsigned int GetThumbDataCount() const
//		{return ThumbDataCount;}
//		
//		inline unsigned int GetMetaMessageCount() const
//		{return MetaMessageCount;}
//		
//		inline unsigned int GetEffectiveDataSpace() const
//		{return EffectiveDataSpace;}
//
////		int SetFilePath()
////		{
////			
////		}
//		
//		inline std::string GetFilePath() const
//		{return path;}
//		
//		inline bool IsOpen() const
//		{return ThumbIsOpen;}
//		
//		int Open()
//		{
//			if (ThumbIsOpen)
//				return Error_NowIsOpen;
//			if (!IsFileExsit(Path))
//				if (OpenMode&OpenMode_NotCreate)
//					return Error_FileNotExsit;
//				else
//				{
//					int retCode=CreateThumbFile(Path);
//					if (retCode!=0)
//						return retCode;
//				}
//			FileStm.open(Path,ios::in|ios::out|ios::binary);
//			if (!FileStm.is_open())
//				return Error_FileCannotOpen;
//			return ReadMetaData();
//		}
//		
//		inline int Open(const std::string &_path)
//		{
//			Path=_path;
//			return Open();
//		}
//		
//		inline int Open(unsigned int _mode)
//		{
//			OpenMode=_mode;
//			return Open();
//		}
//		
//		inline int Open(const std::string &_path,unsigned int _mode)
//		{
//			Path=_path;
//			OpenMode=_mode;
//			return Open();
//		}
//		
//		int Close()
//		{
//			if (!ThumbIsOpen)
//				return Error_NowNotOpen;
//			int retCode=0;
//			if (NeedMaintain()) retCode=MainTain();
//			else retCode=WriteMetaData();
//			if (retCode!=0)
//				return retCode;
//			Reinit();
//			return 0;
//		}
//		
//		~PAL_FSF_Thumb()
//		{
//			Close();
//		}
//		
//		PAL_FSF_Thumb(const std::string &_path="",unsigned int _mode=0)
//		:Path(_path),OpenMode(_mode)
//		{
//			Open();
//		}
//		
//		PAL_FSF_Thumb() {}
//};
//
//#endif
