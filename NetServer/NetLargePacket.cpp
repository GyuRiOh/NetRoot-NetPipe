#include "NetLargePacket.h"
#include "../Common/MemTLS.h"

using namespace server_baby;
using namespace std;

MemTLS<NetLargePacket>* NetLargePacket::packetPool_ = new MemTLS<NetLargePacket>(200, 1, eLARGE_PACKET_POOL_CODE);


NetLargePacket* server_baby::NetLargePacket::CopyRemainderToNewPacket(const int packetCnt)
{
	if (packetCnt < 0)
		ErrorQuit(L"CopyRemainderToNewPacket - PacketCnt Minus");

	if (packetCnt == 0)
	{
		readPos_ = prevReadPos_;
		return this;
	}

	NetLargePacket* newPacket = Alloc();
	long long useSize = GetUsedSize();

	if (useSize > 0)
	{
		if (useSize > eBUFFER_CENTER)
			ErrorQuit(L"CopyRemainterToNew - Not Enough Buffer Space");

		memmove(newPacket->SetReadPosToRemainStart(useSize), readPos_, useSize);
	}

	return newPacket;

}

void server_baby::NetLargePacket::Initialize()
{

	start_ = &data_[eBUFFER_CENTER];
	end_ = &data_[eBUFFER_MASK];
	readPos_ = start_;
	writePos_ = start_;
	prevReadPos_ = start_;
}


