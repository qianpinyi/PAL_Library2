#ifndef PAL_LUA_CPP
#define PAL_LUA_CPP 1

namespace PAL_Lua
{
	#include <lua/lua.hpp>
	
	template <class T> class PAL_LuaStackOperation
	{
		public:
			static void Push(lua_State *L,const T &data);
			static void Pop(lua_State *L);
			static T Top(lua_State *L);
			
			static inline T Get(lua_State *L)
			{
				T re=Top();
				Pop();
				return re;
			}
	};

	template <> class StackOperation <int>
	{
		public:
			void Push(lua_State *L,const int &x)
			{lua_pushnumber(L,data);}
			
			void Pop(lua_State *L)
			{lua_pop(L,1);}
			
			int Top(lua_State *L)
			{return lua_tonumber(L,-1);}
	};
	
	template <> class StackOperation <unsigned int>
	{
		public:
			
	};
	
	template <> class StackOperation <short>
	{
		
	};
	
	template <> class StackOperation <unsigned short>
	{
		
	};
	
	template <> class StackOperation <long long>
	{
		
	};
	
	template <> class StackOperation <unsigned long long>
	{
		
	};
	
	template <> class StackOperation <bool>
	{
		
	};
	
	template <> class StackOperation <unsigned char>
	{
		
	};
	
	template <> class StackOperation <char>
	{
		
	};
	
	class PAL_EasyLuaState
	{
		public:
			enum ErrorType
			{
				ERR_None=0
			};

		protected:
			lua_State *L=nullptr;
			
		public:
			inline lua_State* L()
			{return L;}
			
			template <class T> void Push(const T &data)
			{
				StackOperation<T>().Push(L,data);
			}
			
			template <class T> void Pop()
			{
				StackOperation<T>().Pop(L);
			}
			
			template <class T> T Top()
			{
				return StackOperation<T>().Top(L);
			}
			
			template <class T> T Get()
			{
				return StackOperation<T>().Get(L);
			}
			
			PAL_LuaState() {}
	};
	
};

#endif 
