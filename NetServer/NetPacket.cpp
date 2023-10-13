
#include "NetPacket.h"

using namespace server_baby;

MemTLS<NetPacket>* NetPacket::packetPool_ = new MemTLS<NetPacket>(1000, 1, ePACKET_POOL_CODE);

NetPacket::~NetPacket(){}

bool server_baby::NetPacket::Encode()
{
	if (encodeFlag_)
		return false;

	unsigned char randomKey = rand();
	long long usedSize = writePos_ - readPos_;
	unsigned char* checkSumPos = reinterpret_cast<unsigned char*>(readPos_);

	NetHeader header;
	header.code = eNET_PACKET_CODE;
	header.length = static_cast<unsigned short>(usedSize);
	header.randKey = randomKey;

	//체크섬 만들기
	int sum = 0;
	for (int i = 0; i < usedSize; i++)
	{
		sum += *checkSumPos++;
	}

	header.checkSum = sum % eNET_CHECKSUM_CODE;
	memmove(header_, &header, eNET_HEADER_SIZE);

	//인코딩
	unsigned char* payload = reinterpret_cast<unsigned char*>(readPos_ - 1);
	unsigned char encodeTemp = 0;
	unsigned char payloadTemp = 0;
	unsigned char* encodePos = payload;
	for (int i = 0; i < usedSize + 1; i++)
	{
		char originalData = *encodePos++;
		encodeTemp = originalData ^ (encodeTemp + randomKey + (i + 1));
		payload[i] = encodeTemp ^ (payloadTemp + eNET_CERTAIN_KEY + (i + 1));
		payloadTemp = payload[i];
	}

	encodeFlag_ = true;
	return true;

}
