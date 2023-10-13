
#ifndef  __NET__ENUMS__
#define  __NET__ENUMS__

namespace server_baby
{

#pragma pack(push, 1)
	struct NetHeader
	{
		unsigned char code;
		unsigned short length;
		unsigned char randKey;
		unsigned char checkSum;
	};
#pragma pack(pop)

	enum NET_PACKET_CODE 
	{
		eNET_RECVED_PACKET_SET = 0,
		eNET_CONTENTS_USER_JOIN = 11,
		eNET_CONTENTS_USER_LEAVE = 12,
	};

	enum NET_PACKET_COMMON
	{
		eNET_HEADER_SIZE = sizeof(NetHeader),
		eNET_PACKET_CODE = 0x77,
		eNET_CERTAIN_KEY = 0x32,
		eNET_CHECKSUM_CODE = 256,
		eNET_PROTOCOL_MSG_MAX_LEN = 512,
		eNET_PACKET_SET_MAX_COUNT = 300,
	};

	enum NET_POOL_CODE
	{
		eSESSION_POOL_CODE = 0x1001,
		ePACKET_POOL_CODE = 0x1002,
		eINDEX_CODE = 0x1003,
		eLARGE_PACKET_POOL_CODE = 0x1005,
		eQUEUEITEM_POOL_CODE = 0x1006,
		ePLAYER_POOL_CODE = 0x1007,
		eSESSION_ID_SET_POOL_CODE = 0x1008,
		eSESSION_SET_POOL_CODE = 0x1009,
		eLIST_NODE_CODE = 0x2001,
		eRED_BLACK_TREE_NODE_CODE = 0x2002,
		eHASH_MAP_CODE = 0x2003
	};

	enum NET_SESSION
	{
		eNET_SESSION_MAX_COUNT = 10501,
		eNET_PACKET_SEND_Q_MAX_SIZE = 1024, 
		eKILO_BYTE = 1000, //¿Å±â±â
		eMEGA_BYTE = 1000000,
	};

}

#endif