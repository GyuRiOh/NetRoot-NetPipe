#include "NetPacketSet.h"
#include "NetEnums.h"
using namespace server_baby;

MemTLS<NetPacketSet>* NetPacketSet::packetQPool_ = new MemTLS<NetPacketSet>(500, 1, eQUEUEITEM_POOL_CODE);

int server_baby::NetPacketSet::RegisterPackets(NetLargePacket* const packet)
{
    int packetCnt = 0;
    packet->PreserveReadPos();
    while (!packet->isSmallerThanHeaderSize())
    {
        char* newPacket = packet->GetReadPos();
        if (*newPacket != eNET_PACKET_CODE)
        {
            originalPacket_ = nullptr;
            return NET_PACKET_ERROR;
        }

        unsigned short packetSize = *reinterpret_cast<unsigned short*>(newPacket + 1);
        if (packetSize > static_cast<unsigned short>(eNET_PROTOCOL_MSG_MAX_LEN))
        {
            originalPacket_ = nullptr;
            return NET_PACKET_ERROR;
        }

        if (packet->GetUsedSizeWithoutHeader() < packetSize)
            break;

        packet->MoveReadPosWithHeaderSize(packetSize);
        
        if (!Decode(newPacket))
        {
            originalPacket_ = nullptr;
            return NET_PACKET_ERROR;
        }

        newPacket = newPacket + eNET_HEADER_SIZE;
        if (!Enqueue(newPacket, packetSize))
        {
            SystemLogger::GetInstance()->LogText(L"PacketSet", LEVEL_ERROR, L"PacketSet Enq Failed : Already Max Size");
            CrashDump::Crash();
        }

        packetCnt++;
    }

    if (packetCnt == 0)
        originalPacket_ = nullptr;

    return packetCnt;

}


bool server_baby::NetPacketSet::Decode(const char* const data)
{
    //1번째 바이트 
    unsigned char* cursor = (unsigned char*)data;

    //1번째 바이트를 받았는데 이게 패킷코드가 아니면 나감
    if (*cursor != eNET_PACKET_CODE)
        return false;

    //커서 1개 옮기고
    //short로 캐스팅해서 길이를 뽑음.
    unsigned short len = *(reinterpret_cast<unsigned short*>(++cursor)) + 1;
    cursor += 2;

    //랜덤키 받고
    //커서 1옮김
    unsigned char localRandKey = *cursor++;

    //체크섬부터 디코딩
    unsigned char* decodedPayload = cursor; //배열로 사용
    unsigned char decodeTemp = 0;
    unsigned char payloadTemp = 0;
    unsigned char decodedPrevTemp = 0;
    unsigned char* decodePos = cursor;

    unsigned int checkSum = 0;

    for (int i = 0; i < len; i++)
    {
        char encoded = *decodePos++;
        decodeTemp = encoded ^ (payloadTemp + eNET_CERTAIN_KEY + (i + 1));
        decodedPayload[i] = decodeTemp ^ (decodedPrevTemp + localRandKey + (i + 1));

        payloadTemp = encoded;
        decodedPrevTemp = decodeTemp;
        checkSum += decodedPayload[i];
    }

    checkSum -= decodedPayload[0];

    return ((unsigned char)checkSum == *cursor);
}
