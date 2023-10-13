
#include "LanPacket.h"

using namespace server_baby;

MemTLS<LanPacket>* LanPacket::packetPool_ = new MemTLS<LanPacket>(500, 1, eLAN_PACKET_POOL_CODE);

LanPacket::~LanPacket(){}
