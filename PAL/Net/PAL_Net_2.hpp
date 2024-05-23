#ifndef PAL_NET_PAL_NET_2_HPP
#define PAL_NET_PAL_NET_2_HPP

namespace PAL::Net
{
	
	struct IPv4
	{
		Uint32 ip;
		Uint16 port;
	};
	
	struct IPv6
	{
		Uint128 ip;
		Uint16 port;
	};
	
	class PAL_TCP
	{
		protected:
			
			
		public:
			
			/*
				Open
				Accept
				Send
				Recv
				Close
			*/
	};
	
	class PAL_UDP
	{
		
	};
};

#endif
