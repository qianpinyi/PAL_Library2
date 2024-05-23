#ifndef PAL_EX_ESP32_NETWORK_HPP
#define PAL_EX_ESP32_NETWORK_HPP

namespace PAL::EX::ESP32
{
	struct DataHeader
	{
		int mode,
			len;
	};
	
	template <int N> struct DataPackN
	{
		DataHeader header;
		Uint8 data[N-sizeof(header)];
		
		static_assert(N-sizeof(header)>=0);
	};
	using DataPackX=DataPackN<8>;
	using DataPack16=DataPackN<16>;
	using DataPack128=DataPackN<128>;
	using DataPack512=DataPackN<512>;
	using DataPack4096=DataPackN<4096>;
	
	class EasyNetwork
	{
		public:
			enum ERR
			{
				ERR_None=0
			};
			
			enum
			{
				Type_None=0,
				Type_Continue,
				
				Type_UserDefined=1000,
			};
		
		protected:
			
			
		public:
			
			ERR Send()
			{
				
			}
			
			ERR Recv()
			{
				
			}
			
			ERR Listen()
			{
				
			}
			
			ERR Connect()
			{
				
			}
	};
};

#endif
