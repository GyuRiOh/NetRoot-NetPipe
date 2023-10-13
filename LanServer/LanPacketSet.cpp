#include "LanPacketSet.h"
#include "LanEnums.h"
using namespace server_baby;

MemTLS<LanPacketSet>* LanPacketSet::packetQPool_ = new MemTLS<LanPacketSet>(500, 1, eLAN_QUEUEITEM_POOL_CODE);

int server_baby::LanPacketSet::RegisterPackets(LanLargePacket* const packet)
{
    int packetCnt = 0;
    packet->PreserveReadPos();
    while (!packet->isSmallerThanHeaderSize())
    {
        char* newPacket = packet->GetReadPos();

        unsigned short packetSize = *reinterpret_cast<unsigned short*>(newPacket);
        if (packetSize > static_cast<unsigned short>(eLAN_PROTOCOL_MSG_MAX_LEN))
        {
            originalPacket_ = nullptr;
            return LAN_PACKET_ERROR;
        }

        if (packet->GetUsedSizeWithoutHeader() < packetSize)
            break;

        packet->MoveReadPosWithHeaderSize(packetSize); 

        newPacket = newPacket + eLAN_HEADER_SIZE;
        if (!Enqueue(newPacket, packetSize))
        {
            SystemLogger::GetInstance()->LogText(L"LanPacketSet", LEVEL_ERROR, L"PacketSet Enq Failed : Already Max Size");
            CrashDump::Crash();
        }

        packetCnt++;
    }

    if (packetCnt == 0)
        originalPacket_ = nullptr;

    return packetCnt;

}