
#ifndef  __LAN__ENUMS__
#define  __LAN__ENUMS__

namespace server_baby
{

#pragma pack(push, 1)
	struct LanHeader
	{
		unsigned short length;
	};
#pragma pack(pop)

	enum LAN_PACKET_CODE 
	{
		eLAN_RECVED_PACKET_SET = 0,
		eLAN_CLIENT_JOIN = 11,
		eLAN_CLIENT_LEAVE = 12,
	};

	enum LAN_PACKET_COMMON
	{
		eLAN_HEADER_SIZE = sizeof(LanHeader),
		eLAN_PROTOCOL_MSG_MAX_LEN = 512,
		eLAN_PACKET_SET_MAX_COUNT = 512,
	};

	enum LAN_POOL_CODE
	{
		eLAN_SESSION_POOL_CODE = 0x1001,
		eLAN_PACKET_POOL_CODE = 0x1002,
		eLAN_INDEX_CODE = 0x1003,
		eLAN_LARGE_PACKET_POOL_CODE = 0x1005,
		eLAN_QUEUEITEM_POOL_CODE = 0x1006,
		eLAN_CLIENT_POOL_CODE = 0x1007,
		eLAN_SESSION_ID_SET_POOL_CODE = 0x1008,
		eLAN_SESSION_SET_POOL_CODE = 0x1009,
	};

	enum LAN_SESSION
	{
		eLAN_SESSION_MAX_COUNT = 1001,
		eLAN_PACKET_SEND_Q_MAX_SIZE = 296,
	};

}

#endif