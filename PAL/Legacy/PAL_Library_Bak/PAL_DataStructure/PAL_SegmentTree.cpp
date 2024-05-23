#ifndef PAL_SEGMENTTREE_CPP
#define PAL_SEGMENTTREE_CPP 1

namespace PAL_DS
{
	class PAL_SegmentTree//Pos count from 1~size
	{
		#define lson (p<<1)
		#define rson (p<<1|1)
		#define mid ((l+r)>>1)
		protected:
			struct SegmentTreeNode
			{
				long long sum=0, 
						  lazy=0;
			};
			SegmentTreeNode *s=nullptr;
			unsigned size;
			
			inline void PushUp(unsigned p)
			{s[p].sum=s[lson].sum+s[rson].sum;}
			
			inline void PushDown(unsigned p,unsigned l,unsigned r)
			{
				if (s[p].lazy!=0)
					s[lson].sum+=s[p].lazy*(mid-l+1),
					s[rson].sum+=s[p].lazy*(r-mid),
					s[lson].lazy+=s[p].lazy,
					s[rson].lazy+=s[p].lazy,
					s[p].lazy=0;
			}  
			
			void AddSegment(unsigned p,unsigned l,unsigned r,unsigned L,unsigned R,long long c)
			{
				if (L<=l&&r<=R)
				{
					s[p].sum+=(r-l+1)*c;
					s[p].lazy+=c;
					return;
				}
				PushDown(p,l,r);
				if (L<=mid) AddSegment(lson,l,mid,L,R,c);
				if (mid<R)  AddSegment(rson,mid+1,r,L,R,c);
				PushUp(p);
			}
			
			long long QuerySegmentTree(unsigned p,unsigned l,unsigned r,unsigned L,unsigned R)
			{
				if (L<=l&&r<=R) return s[p].sum;
				PushDown(p,l,r);
				long long re=0;
				if (L<=mid) re+=QuerySegmentTree(lson,l,mid,L,R);
				if (mid<R)  re+=QuerySegmentTree(rson,mid+1,r,L,R);
				return re;
			}
			
			void BuildSegmentTree(unsigned p,unsigned l,unsigned r,long long a[])
			{
				if (l==r)
					s[p].sum=a[l];
				else
					BuildSegmentTree(lson,l,mid,a),
					BuildSegmentTree(rson,mid+1,r,a),
					PushUp(p);
			}
			
			void BuildSegmentTree(unsigned p,unsigned l,unsigned r,long long x)
			{
				if (l==r)
					s[p].sum=x;
				else
					BuildSegmentTree(lson,l,mid,x),
					BuildSegmentTree(rson,mid+1,r,x),
					PushUp(p);
			}
			
		public:
			inline void AddSegment(unsigned L,unsigned R,long long c)
			{
				if (size==0||L>R) return;
				if (L<1) L=1;
				if (R>size) R=size;
				AddSegment(1,1,size,L,R,c);
			}
			
//			inline void SetSegment(unsigned L,unsigned R,long long c)
//			{
//
//			}
			
			inline long long QuerySegment(unsigned L,unsigned R)
			{
				if (size==0||L>R) return 0;
				if (L<1) L=1;
				if (R>size) R=size;
				return QuerySegmentTree(1,1,size,L,R);
			}

//			inline unsigned int Lowerbound(long long sum)//When exsit negative number, it may have some problem...
//			{
//				
//			}
//			
//			inline unsigned int Upperbound(long long sum)
//			{
//				
//			}

			inline unsigned int Size() const 
			{return size;}
			
			inline bool Valid() const
			{return s!=nullptr;}

			PAL_SegmentTree& operator = (const PAL_SegmentTree&)=delete;
			PAL_SegmentTree& operator = (PAL_SegmentTree&&)=delete;
			PAL_SegmentTree(const PAL_SegmentTree&)=delete;
			PAL_SegmentTree(PAL_SegmentTree&&)=delete;
			
			~PAL_SegmentTree()
			{
				if (s!=nullptr)
					delete[] s;
			}
			
			PAL_SegmentTree(unsigned _size,long long a[])
			{
				size=_size;
				s=new SegmentTreeNode[(size+1)<<2];
				if (s==nullptr)
					size=0;
				else BuildSegmentTree(1,1,size,a);
			}
			
			PAL_SegmentTree(unsigned _size,long long x)
			{
				size=_size;
				s=new SegmentTreeNode[(size+1)<<2];
				if (s==nullptr)
					size=0;
				else BuildSegmentTree(1,1,size,x);
			}
		#undef lson
		#undef rson
		#undef mid
	};
	
	class PAL_SegmentTreeT
	{
		protected:
			
			
		public:
			
			
	};
	
	class PAL_SparseSegmentTree
	{
		#define mid ((l+r)>>1)
		#define ULLPos unsigned long long
		protected:
			struct SegmentTreeNode
			{
				SegmentTreeNode *lson=nullptr,
								*rson=nullptr;
				long long sum=0,
						  lazy=0;
				
				~SegmentTreeNode()
				{
					if (lson)
						delete lson;
					if (rson)
						delete rson;
				}
				
				SegmentTreeNode(long long _sum=0):sum(_sum) {}
			};
			SegmentTreeNode *root=nullptr;
			unsigned long long size=0;
			long long initValue=0;
			
			inline void PushUp(SegmentTreeNode *p)
			{p->sum=p->lson->sum+p->rson->sum;}
				
			inline void PushDown(SegmentTreeNode *p,ULLPos l,ULLPos r)
			{
				if (!p->lson)
					p->lson=new SegmentTreeNode(initValue*(mid-l+1));
				if (!p->rson)
					p->rson=new SegmentTreeNode(initValue*(r-mid));
				if (p->lazy!=0)
				{
					p->lson->sum+=p->lazy*(mid-l+1);
					p->rson->sum+=p->lazy*(r-mid);
					p->lson->lazy+=p->lazy;
					p->rson->lazy+=p->lazy;
					p->lazy=0;
				}
			}
			
			void AddSegment(SegmentTreeNode *p,ULLPos l,ULLPos r,ULLPos L,ULLPos R,long long c)
			{
				if (L<=l&&r<=R)
				{
					p->sum+=(r-l+1)*c;
					p->lazy+=c;
					return;
				}
				PushDown(p,l,r);
				if (L<=mid) AddSegment(p->lson,l,mid,L,R,c);
				if (mid<R)  AddSegment(p->rson,mid+1,r,L,R,c);
				PushUp(p);
			}
			
			long long QuerySegmentTree(SegmentTreeNode *p,ULLPos l,ULLPos r,ULLPos L,ULLPos R)
			{
				if (L<=l&&r<=R) return p->sum;
				PushDown(p,l,r);
				long long re=0;
				if (L<=mid) re+=QuerySegmentTree(p->lson,l,mid,L,R);
				if (mid<R)  re+=QuerySegmentTree(p->rson,mid+1,r,L,R);
				return re;
			}
			
		public:
			inline void AddSegment(ULLPos L,ULLPos R,long long c)
			{
				if (size==0||L>R) return;
				if (L<1) L=1;
				if (R>size) R=size;
				AddSegment(root,1,size,L,R,c);
			}
			
			inline long long QuerySegment(ULLPos L,ULLPos R)
			{
				if (size==0||L>R) return 0;
				if (L<1) L=1;
				if (R>size) R=size;
				return QuerySegmentTree(root,1,size,L,R);
			}

			inline unsigned long long Size() const 
			{return size;}
			
			inline bool Valid() const
			{return size!=0;}

			PAL_SparseSegmentTree& operator = (const PAL_SparseSegmentTree&)=delete;
			PAL_SparseSegmentTree& operator = (PAL_SparseSegmentTree&&)=delete;
			PAL_SparseSegmentTree(const PAL_SparseSegmentTree&)=delete;
			PAL_SparseSegmentTree(PAL_SparseSegmentTree&&)=delete;
			
			~PAL_SparseSegmentTree()
			{
				if (root!=nullptr)
					delete root;
			}
			
			PAL_SparseSegmentTree(unsigned long long _size,long long initvalue=0)
			{
				size=_size;
				initValue=initvalue;
				root=new SegmentTreeNode(size*initvalue);
				if (root==nullptr)
					size=0;
			}
		#undef mid
		#undef ULLPos
	};
};

#endif
