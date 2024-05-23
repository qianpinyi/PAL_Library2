#ifndef PAL_SYNCHRONIZEUNIT_CPP
#define PAL_SYNCHRONIZEUNIT_CPP 1

#include "../PAL_BuildConfig.h"

#include "../PAL_BasicFunctions/PAL_BasicFunctions_0.cpp"

#include <map>

namespace PAL_SynchronizeUnit
{
	using namespace std;
	
	//Configs:
	using PSU_ID=long long;
	using PSU_Timecode=unsigned long long;
	using PSU_EventType=unsigned char;
	using PSU_ClientID=unsigned long long;
	//End of Configs
	
	enum
	{
		PSU_Event_None=0,
		PSU_Event_Create,
		PSU_Event_Delete,
		PSU_Event_Update,
		PSU_Event_AssignID,
		PSU_Event_Disconnect,
		PSU_Event_Connect
	};
	
	struct PSU_Event
	{
		PSU_EventType type;
		PSU_Timecode timecode;
		PSU_ID id;
	};
	
	struct PSU_EventPacket
	{
		
	};

	class PSU_UnitT
	{
		protected:
			
			
		public:
			
			
	};
	
	class PSU_Unit
	{
		protected:
			PSU_ID ID;
			PSU_Timecode Timecode;
			unsigned int Size;
			void *data=nullptr;
			
		public:
			
			
	};
	
	class PSU_Server
	{
		protected:
			set <PSU_ClientID> ClientList;
			
			PSU_ID AssignID()
			{
				
			}
			
		public:
			
			void EventLoop()
			{
				
			}
	};
	
	class PSU_Client
	{
		protected:
			map <PSU_ID,PSU_Unit> Datas;
			PSU_EventPacket UpdatePacket;
			int ConnectMode;
			PSU_ClientID ClientID;
			
			PSU_ID AssignID()
			{
				
			}
			
			void ServerEvent()
			{
				
			}
			
		public:
			
			PSU_UnitT* NewUnit()
			{
				
			}
			
			void DeleteUnit(PSU_ID id)
			{
				
			}
			
			PSU_UnitT* GetUnit(PSU_ID id)
			{
				
			}
			
			void Synchronize()
			{
				
			}
	};
};

#endif
