#ifndef PAL_HTTP_CPP
#define PAL_HTTP_CPP

#include <string>

namespace PAL::Legacy::PAL_Net
{
	enum
	{
		Http_GET,
		Http_POST,
		Http_PUT,
		Http_HEAD,
		Http_DELETE,
		Http_OPTIONS,
		Http_TRACE,
		Http_CONNECT 
	};
	
	class HttpHeader
	{
		public:
			enum HeaderName
			{
				
			};

			const char* HeaderNameStr(HeaderName k) const
			{
				switch(k)
				{
					default:	return "";
				}
			}

		protected:
			string key,
				   val;
			
		public:

			bool operator < (const HttpHeader &tar) const
			{
				return key<tar.key;
			}
			
			HttpHeader(std::string k,std::string v=""):key(std::move(k)),val(std::move(v)) {}
			HttpHeader(HeaderName k,std::string v=""):HttpHeader(HeaderNameStr(k),v) {}
			HttpHeader() {}
	};
	
	class HttpHeaders
	{
		protected:
			
			
		public:
			
			
	};
	
	class HttpRequest
	{
		protected:
			int method;
			std::string url,
						version;
		
		public:
			
			
	};
	
	class HttpResponse
	{
		protected:
			
			
		public:
			
			
	};
	
	class HttpServer
	{
		protected:
			
			
		public:
			
			
	};
};

#endif
