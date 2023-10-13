#include "LanLargePacket.h"
#include "../Common/MemTLS.h"

using namespace server_baby;
using namespace std;

MemTLS<LanLargePacket>* LanLargePacket::packetPool_ = new MemTLS<LanLargePacket>(500, 1, eLAN_LARGE_PACKET_POOL_CODE);
vector<char*> LanLargePacket::bufferList_;
RTL_SRWLOCK LanLargePacket::bufferListLock_;


LanLargePacket* server_baby::LanLargePacket::CopyRemainderToNewPacket(int packetCnt)
{
	if (packetCnt < 0)
		ErrorQuit(L"CopyRemainderToNewPacket - PacketCnt Minus");

	if (packetCnt == 0)
	{
		readPos_ = prevReadPos_;
		return this;
	}

	LanLargePacket* newPacket = Alloc();
	long long useSize = GetUsedSize();
	if (useSize > 0)
	{
		if (useSize > eBUFFER_CENTER)
			ErrorQuit(L"CopyRemainterToNew - Not Enough Buffer Space");

		memmove(newPacket->SetReadPosToRemainStart(useSize), readPos_, useSize);
	}

	return newPacket;

}

void server_baby::LanLargePacket::Initialize()
{
	if (initFlag_ != eINIT_FLAG)
	{
		char* data = (char*)_aligned_malloc(eBUFFER_SIZE, eBUFFER_SIZE);
		if (data == nullptr)
			ErrorQuit(L"Aligned Malloc Failed");

		ZeroMemory(data, eBUFFER_SIZE);

		AcquireSRWLockExclusive(&bufferListLock_);
		bufferList_.push_back(move(data));
		ReleaseSRWLockExclusive(&bufferListLock_);

		*(LanLargePacket**)data = this;

		start_ = &data[eBUFFER_CENTER];
		end_ = &data[eBUFFER_MASK];
		data_ = data;
		initFlag_ = eINIT_FLAG;

	}

	readPos_ = start_;
	writePos_ = start_;
	prevReadPos_ = start_;
}


