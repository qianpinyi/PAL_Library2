#ifndef PAL_COMMON_PAL_POINTERS_0_HPP
#define PAL_COMMON_PAL_POINTERS_0_HPP

namespace PAL
{
	template <class T> class SharedPtr
	{
		protected:
			T *p=nullptr;
			unsigned int *count=nullptr;
			
			inline void CopyFrom(const SharedPtr &tar)
			{
				if (!tar)
					return;
				p=tar.p;
				count=tar.count;
				++*count;
			}
			
			inline void Deconstruct()
			{
				--*count;
				if (*count==0)
				{
					delete p;
					delete count;
				}
				p=nullptr;
				count=nullptr;
			}
		
		public:
			inline operator T* ()
			{return p;}
			
			inline T* operator -> ()
			{return p;}
			
			inline T& operator * ()
			{return *p;}
			
			inline bool operator ! () const
			{return p==nullptr||count==nullptr;}
			
			inline T* Target() const
			{return p;}
			
			inline unsigned Count() const
			{return count==nullptr?0:*count;}
			
			SharedPtr& operator = (const SharedPtr &tar)
			{
				if (&tar==this)
					return *this;
				if (p!=nullptr)
					Deconstruct();
				CopyFrom(tar);
				return *this;
			}
			
			~SharedPtr()
			{
				if (p!=nullptr)
					Deconstruct();
			}
			
			SharedPtr(const SharedPtr &tar)
			{CopyFrom(tar);}
			
			explicit SharedPtr(T *tar)
			{
				if (tar!=nullptr)
				{
					p=tar;
					count=new unsigned int(1);
				}
			}
			
			explicit SharedPtr() {}
			
	//		template <typename...Ts> SharedPtr(const Ts &...args):SharedPtr(new T(args...)) {}//??
	};
	
	template <class T> class GroupPtr
	{
		protected:
			T **p=nullptr;
			unsigned int *count=nullptr;
			
			inline void CopyFrom(const GroupPtr &tar)
			{
				if (!tar)
					return;
				p=tar.p;
				count=tar.count;
				++*count;
			}
			
			inline void Deconstruct()
			{
				--*count;
				if (*count==0)
				{
					delete p;
					delete count;
				}
				p=nullptr;
				count=nullptr;
			}
			
		public:
			inline void SetTarget(T *tar)//if NULL,the group will break down; if not NULL, the group changes their target(Use this carefully!).
			{
				if (tar!=nullptr)
				{
					if (p!=nullptr)
					{
						if (*p!=nullptr)
						{
							*p=tar;
							return;
						}
						else Deconstruct();
					}
					p=new T*(tar);
					count=new unsigned(1);
				}
				else if (p!=nullptr)
				{
					*p=nullptr;
					Deconstruct();
				}
			}
			
			inline operator T* ()
			{return p==nullptr?nullptr:*p;}
			
			inline T* operator -> ()
			{return *p;}
			
			inline bool operator ! () const
			{return p==nullptr||*p==nullptr;}
			
			inline bool Valid() const
			{return p!=nullptr&&*p!=nullptr;}
			
			inline T* Target() const
			{return p==nullptr?nullptr:*p;}
			
			inline unsigned Count() const
			{return p==nullptr?0:*count;}
			
			GroupPtr& operator = (const GroupPtr &tar)
			{
				if (&tar==this)
					return *this;
				if (p!=nullptr)
					Deconstruct();
				CopyFrom(tar);
				return *this;
			}
			
			~GroupPtr()
			{
				if (p!=nullptr)
					Deconstruct();
			}
			
			GroupPtr(const GroupPtr &tar)
			{CopyFrom(tar);}
			
			GroupPtr() {}
	};
};

#endif
