
#ifndef __NET__STUB__
#define __NET__STUB__

#include "NetServer.h"
using namespace std;

namespace server_baby
{
	class NetStub
	{
	public:
		bool PacketProc(NetPacketSet* msgPack)
		{
			switch (msgPack->GetType())
			{
			case eNET_RECVED_PACKET_SET:
			{
				while (msgPack->GetSize() > 0)
				{
					NetDummyPacket* packet = nullptr;
					if (msgPack->Dequeue(&packet) == false)
						CrashDump::Crash();

					if (!PacketProc(msgPack->GetSessionID(), packet))
					{
						NetPacketSet::Free(msgPack);
						return false;
					}
				}
				break;
			}  
			
			case eNET_CONTENTS_USER_JOIN:
				OnContentsUserJoin(msgPack->GetSessionID());
				break;
			case eNET_CONTENTS_USER_LEAVE:
				OnContentsUserLeave(msgPack->GetSessionID());
				break;
			default:
				SystemLogger::GetInstance()->LogText(L"NetStub", LEVEL_ERROR, L"Undefined Packet Code");
				CrashDump::Crash();
				break;
			}

			NetPacketSet::Free(msgPack);
			return true;
		}

		virtual void OnRecv(NetPacketSet* msgPack) {};
		virtual void OnContentsUserJoin(NetSessionID sessionID) {};
		virtual void OnContentsUserLeave(NetSessionID sessionID) {};
		virtual void OnWorkerClientJoin(NetSessionID sessionID) {};
		virtual void OnWorkerClientLeave(NetSessionID sessionID) {};
		virtual bool PacketProc(NetSessionID sessionID, NetDummyPacket* msg) = 0;
	};
}


#endif