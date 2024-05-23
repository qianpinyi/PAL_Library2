#ifndef PAL_BASICFUNCTIONS_CHARSET_CPP
#define PAL_BASICFUNCTIONS_CHARSET_CPP

#include "../PAL_BuildConfig.h"

#include <iostream>
#include <string>

#if PAL_CurrentPlatform==PAL_Platform_Windows
	#include <Windows.h>
#endif

#include "PAL_StringEX.cpp"

namespace PAL::Legacy::Charset
{
	using namespace std;

#if PAL_CurrentPlatform!=PAL_Platform_Windows
	string UnicodeToAnsi(const wstring& unicode)
	{
		//<<...
		return "";
	}
	
	wstring AnsiToUnicode(const string& ansi)
	{
		//<<...
		return L"";	
	}
	
	string UnicodeToUtf8(const wstring& unicode)
	{
		//<<...
		return "";
	}
	
	wstring Utf8ToUnicode(const string& utf8)
	{
		//<<...
		return L"";
	}
	
	string AnsiToUtf8(const string& ansi)
	{
		//<<...
		return ansi;//??
	}
	
	string Utf8ToAnsi(const string& utf8)
	{
		//<<...
		return utf8;//??
	}
#endif

#if PAL_CurrentPlatform==PAL_Platform_Windows
	string UnicodeToAnsi(const wstring& unicode)
	{
		LPCWCH ptr=unicode.c_str();
		int size=WideCharToMultiByte(CP_THREAD_ACP,0,ptr,-1,NULL,0,NULL,NULL);
		string strRet(size,0);
		WideCharToMultiByte(CP_THREAD_ACP,0,ptr,-1,(LPSTR)strRet.c_str(),size,NULL,NULL);
		return DeleteEndBlank(strRet);
	}
	
	wstring AnsiToUnicode(const string& ansi)
	{
	    LPCCH ptr=ansi.c_str();
	    int size=MultiByteToWideChar(CP_ACP,0,ptr,-1,NULL,0);
		wstring wstrRet(size,0);
	    MultiByteToWideChar(CP_ACP,0,ptr,-1,(LPWSTR)wstrRet.c_str(),size);
		return DeleteEndBlank(wstrRet);
	}
	
	string UnicodeToUtf8(const wstring& unicode)
	{
	    LPCWCH ptr=unicode.c_str();
	    int size=WideCharToMultiByte(CP_UTF8,0,ptr,-1,NULL,0,NULL,NULL);
		string strRet(size,0);
	    WideCharToMultiByte(CP_UTF8,0,ptr,-1,(char*)strRet.c_str(),size,NULL,NULL);
		return DeleteEndBlank(strRet);
	}
	
	wstring Utf8ToUnicode(const string& utf8)
	{
	    LPCCH ptr=utf8.c_str();
	    int size=MultiByteToWideChar(CP_UTF8,0,ptr,-1,NULL,0);
		wstring wstrRet(size,0);
	    MultiByteToWideChar(CP_UTF8,0,ptr,-1,(LPWSTR)wstrRet.c_str(),size);
		return DeleteEndBlank(wstrRet);
	}
	
	string AnsiToUtf8(const string& ansi)
	{
	    LPCCH ptr=ansi.c_str();
	    int size=MultiByteToWideChar(CP_ACP,0,ptr,-1,NULL,0);
		wstring wstrTemp(size,0);
	    MultiByteToWideChar(CP_ACP,0,ptr,-1,(LPWSTR)wstrTemp.c_str(),size);
		return UnicodeToUtf8(wstrTemp);
	}
	
	string Utf8ToAnsi(const string& utf8)
	{
		wstring wstrTemp=Utf8ToUnicode(utf8);
		LPCWCH ptr=wstrTemp.c_str();
	    int size=WideCharToMultiByte(CP_ACP,0,ptr,-1,NULL,0,NULL,NULL);
		string strRet(size,0);
	    WideCharToMultiByte(CP_ACP,0,ptr,-1,(LPSTR)strRet.c_str(),size,NULL,NULL);
		return DeleteEndBlank(strRet);
	}
#endif

	bool IsUTF8(const void* pBuffer, long size)
	{
	    bool isUTF8 = true;
	    unsigned char* start = (unsigned char*)pBuffer;
	    unsigned char* end = (unsigned char*)pBuffer + size;
	    while (start < end)
	    {
	        if (*start < 0x80) 
	            start++;
	        else if (*start < (0xC0)) 
			{
	            isUTF8 = false;
	            break;
	        }
	        else if (*start < (0xE0)) 
			{
	            if (start >= end - 1)
	                break;
	            if ((start[1] & (0xC0)) != 0x80)
				{
	                isUTF8 = false;
	                break;
	            }
	            start += 2;
	        }
	        else if (*start < (0xF0)) 
			{ 
	            if (start >= end - 2)
	                break;
	            if ((start[1] & (0xC0)) != 0x80 || (start[2] & (0xC0)) != 0x80) 
				{
	                isUTF8 = false;
	                break;
	            }
	            start += 3;
	        }
	        else 
			{
	            isUTF8 = false;
	            break;
	        }
	    }
		return isUTF8;
	}
}

#endif
