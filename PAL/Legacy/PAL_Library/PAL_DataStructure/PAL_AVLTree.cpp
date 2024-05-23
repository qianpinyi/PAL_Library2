#ifndef PAL_AVLTREE_CPP
#define PAL_AVLTREE_CPP 1

namespace PAL::Legacy
{
	template <class T> class AVLTreeNode
	{
		friend class AVLTree <T>;
		protected:
			static AVLTree <T> * const nullNode;
			int size=0;
			AVLTreeNode *l=NULL,*r=NULL,*fa=NULL;
			char BalanceFactor=0;

			void Zig()//Left rotate
			{
				if (r==nullNode) return;
				SplayTreeNode *te=r;
				te->size=size;
				size-=te->r->size+1;
				if (this==fa->l) fa->l=te;
				else fa->r=te;
				te->fa=fa;
				fa=te;
				te->l->fa=this;
				r=te->l;
				te->l=this;
			}
			
			void Zag()
			{
				if (l==nullNode) return;
				SplayTreeNode *te=l;
				te->size=size;
				size-=te->l->size+1;
				if (this==fa->l) fa->l=te;
				else fa->r=te;
				te->fa=fa;
				fa=te;
				te->r->fa=this;
				l=te->r;
				te->r=this;
			}
			
			
			
			
			
			
			SplayTreeNode<T>* Splay(SplayTreeNode *s)
			{
				if (this==nullNode) return nullNode;
				s=s->fa;
				while (fa!=s)
				{
					SplayTreeNode *ff=fa->fa;
					if (ff==s)
						if (this==fa->l) fa->Zag();
						else fa->Zig();
					else if (this==fa->l&&fa==ff->l)
						ff->Zag(),fa->Zag();
					else if (this==fa->r&&fa==ff->r)
						ff->Zig(),fa->Zig();
					else if (this==fa->l&&fa==ff->r)
						fa->Zag(),ff->Zig();
					else if (this==fa->r&&fa==ff->l)
						fa->Zig(),ff->Zag();
				}
				return this;
			}
			
			SplayTreeNode<T>* GetKth(int k)
			{
				if (this==nullNode) return this;
				if (l->size>=k) return l->GetKth(k);
				else if (l->size==k-1) return this;
				else return r->GetKth(k-l->size-1);
			}
			
			SplayTreeNode() {}
			
			SplayTreeNode(int _size,SplayTreeNode *_l,SplayTreeNode *_r,SplayTreeNode *_fa)
			:size(_size),l(_l),r(_r),fa(_fa) {}
		
			~SplayTreeNode()//It will delete the subTree
			{
				if (l!=nullNode)
					delete l;
				if (r!=nullNode)
					delete r;
			}
			
			SplayTreeNode(int L,int R,const T src[])
			{
				data=src[L+R>>1];
				size=1;
				fa=nullNode;
				if (L<=(L+R>>1)-1)
				{
					l=new SplayTreeNode<T>(L,(L+R>>1)-1,src);
					l->fa=this;
					size+=l->size;
				}
				else l=nullNode;
				if (R>=(L+R>>1)+1)
				{
					r=new SplayTreeNode<T>((L+R>>1)+1,R,src);
					r->fa=this;
					size+=r->size;
				}
				else r=nullNode;
			}
			
		public:
			T data;
			
			T& operator () ()
			{return data;}
			
			SplayTreeNode(const T &_data):data(_data)
			{
				size=1;
				l=r=fa=nullNode;
			}
			
			
	};

	template <class T> class AVLTree
	{
		protected:
			
			
		public:
			
			
			
	};
}

#endif
