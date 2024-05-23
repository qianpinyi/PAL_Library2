#ifndef PAL_LEASTRECENTLYUSED
#define PAL_LEASTRECENTLYUSED 1

#include <map>
#include <vector>
#include "PAL_Tuple.cpp"
namespace PAL::Legacy::PAL_DS
{
	template <class T_Key,class T_Data> class LRU_LinkHashTable
	{
		friend struct LinkTable;
		protected:
			struct LinkTable
			{
				T_Key Key;
				T_Data Data;
				unsigned long long size=1;
				LinkTable *pre=NULL,
						  *nxt=NULL;
				LinkTable(const T_Key &key,const T_Data &data):Key(key),Data(data) {}
			};
			LinkTable *Head=NULL,
					  *Tail=NULL;
			std::map <T_Key,LinkTable*> KeyToNode;
			unsigned long long SizeLimit=0,
							   TotalSize=0;
			unsigned int ErrorState=0;
			void (*KickoutCallback)(const T_Key&,const T_Data&)=NULL;
			
			LinkTable* LT_Update(LinkTable *LT)
			{
				if (Head!=LT)
				{
					LT->pre->nxt=LT->nxt;
					if (LT->nxt!=NULL)
						LT->nxt->pre=LT->pre;
					else Tail=LT->pre;
					LT->pre=NULL;
					LT->nxt=Head;
					Head->pre=LT;
					Head=LT;
				}
				return LT;
			}
			
			void LT_Delete(LinkTable *LT)
			{
				TotalSize-=LT->size;
				if (LT==Head&&LT==Tail)
					Head=Tail=NULL;
				else if (LT==Head)
				{
					LT->nxt->pre=NULL;
					Head=LT->nxt;
				}
				else if (LT==Tail)
				{
					LT->pre->nxt=NULL;
					Tail=LT->pre;
				}
				else
				{
					LT->pre->nxt=LT->nxt;
					LT->nxt->pre=LT->pre;
				}
				delete LT;
			}
			
			LinkTable* LT_New(const T_Key &_key,const T_Data &_data,unsigned long long _size=1)
			{
				LinkTable *LT=new LinkTable(_key,_data);
				LT->size=_size;
				TotalSize+=LT->size;
				if (Head!=NULL)
				{
					Head->pre=LT;
					LT->nxt=Head;
					Head=LT;
				}
				else Head=Tail=LT;
				return LT;
			}
			
			LinkTable* Find(const T_Key &key)
			{
				if (Head==NULL)
					return NULL;
				auto mp=KeyToNode.find(key);
				if (mp==KeyToNode.end())
					return NULL;
				return mp->second;
			}
			
			void UpdateLimit()
			{
				while (!KeyToNode.empty()&&TotalSize>SizeLimit)
				{
					if (KickoutCallback!=NULL)
						KickoutCallback(Tail->Key,Tail->Data);
					if (!Erase(Tail->Key))
					{
						ErrorState|=1;
						break;
					}
				}
			}
			
		public:
			std::vector <Doublet<T_Key,T_Data> > GetLinkedData() const
			{
				std::vector <Doublet <T_Key,T_Data> > re;
				LinkTable *p=Head;
				while (p)
				{
					re.push_back({p->Key,p->Data});
					p=p->nxt;
				}
				return re;
			}
			
			T_Data* Get(const T_Key &key)
			{
				auto *p=Find(key);
				if (p==NULL)
					return NULL;
				return &LT_Update(p)->Data;
			}
			
			bool PopHead(T_Data &dst)//if exist
			{
				if (Head==nullptr)
					return 0;
				dst=Head->Data;
				KeyToNode.erase(Head->Key);
				LT_Delete(Head);
				return 1;
			}
			
			bool PopTail(T_Data &dst)
			{
				if (Tail==nullptr)
					return 0;
				dst=Tail->Data;
				KeyToNode.erase(Tail->Key);
				LT_Delete(Tail);
				return 1;
			}
			
			bool Check(const T_Key &key)
			{return Find()!=NULL;}

			bool Erase(const T_Key &key)
			{
				auto *p=Find(key);
				if (p!=NULL)
				{
					KeyToNode.erase(key);
					LT_Delete(p);
					return 1;
				}
				else return 0;
			}
			
			bool Insert(const T_Key &key,const T_Data &data,unsigned long long size=1)
			{
				if (KeyToNode.find(key)==KeyToNode.end()&&size<=SizeLimit)
				{
					auto *q=LT_New(key,data,size);
					KeyToNode.insert({key,q});
					UpdateLimit();
					return 1;
				}
				else return 0;
			}
			
			void Clear()//??
			{
				while (Head!=NULL)
					LT_Delete(Head);
				KeyToNode.clear();
			}
			
			inline unsigned long long Count() const
			{return KeyToNode.size();}
			
			inline unsigned long long Size() const
			{return TotalSize;}
			
			inline void SetSizeLimit(unsigned long long sizelimit)
			{
				SizeLimit=sizelimit;
				UpdateLimit();
			}
			
			inline void SetKickoutCallback(void (*func)(const T_Key&,const T_Data&))
			{KickoutCallback=func;}
		
			LRU_LinkHashTable& operator = (const LRU_LinkHashTable &tar)=delete;
			LRU_LinkHashTable& operator = (const LRU_LinkHashTable &&tar)=delete;
			LRU_LinkHashTable(const LRU_LinkHashTable &tar)=delete;
			LRU_LinkHashTable(const LRU_LinkHashTable &&tar)=delete;
			
			~LRU_LinkHashTable()
			{Clear();}
			
			LRU_LinkHashTable(unsigned long long sizelimit)
			:SizeLimit(sizelimit) {}
	};
};
#endif
