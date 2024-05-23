
	
	class BaseTypeFuncAndData
	{
		public:
			virtual int CallFunc(int usercode)=0;
			virtual ~BaseTypeFuncAndData() {};
	};
	
	template <class T> class TypeFuncAndData:public BaseTypeFuncAndData
	{
		protected:
			int (*func)(T&,int)=nullptr;
			T funcdata;
		public:
			virtual int CallFunc(int usercode)
			{
				if (func!=nullptr)
					return func(funcdata,usercode);
				else return 0;
			}
			
			virtual ~TypeFuncAndData() {}
			
			TypeFuncAndData(int (*_func)(T&,int),const T &_funcdata):func(_func),funcdata(_funcdata) {}
	};
	
	template <class T> class TypeFuncAndDataV:public BaseTypeFuncAndData
	{
		protected:
			void (*func)(T&)=nullptr;
			T funcdata;
		public:
			virtual int CallFunc(int usercode)
			{
				if (func!=nullptr)
					func(funcdata);
				return 0;
			}
			
			virtual ~TypeFuncAndDataV() {}
			
			TypeFuncAndDataV(void (*_func)(T&),const T &_funcdata):func(_func),funcdata(_funcdata) {}
	};
	
	class VoidFuncAndData:public BaseTypeFuncAndData
	{
		protected:
			void (*func)(void*)=nullptr;
			void *funcdata=nullptr;
		public:
			virtual int CallFunc(int)
			{
				if (func!=nullptr)
					func(funcdata);
				return 0;
			}
			
			VoidFuncAndData(void (*_func)(void*),void *_funcdata=nullptr):func(_func),funcdata(_funcdata) {}
	};
