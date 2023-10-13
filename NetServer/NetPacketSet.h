

#ifndef  __NET__PACKET__SET__
#define  __NET__PACKET__SET__
#include "NetSession.h"
#include "NetDummyPacket.h"
#include "NetLargePacket.h"
#include "../Common/Queue.h"
#include "../Common/MemTLS.h"
#include "NetEnums.h"


namespace server_baby
{
    class NetPacketSet final
    {
    public:
        explicit NetPacketSet() = delete;
        ~NetPacketSet() = default;

        static NetPacketSet* Alloc(const NetSessionID ID, const BYTE type);
        static NetPacketSet* Alloc(const NetSessionID ID, NetLargePacket* const original);
        static void Free(NetPacketSet* const packetQ);

        static void DeleteTLS();
        static void DeletePool();
        static int GetUsedCount();
        static int GetCapacity();
        static void GarbageCollect();


        int RegisterPackets(NetLargePacket* const packet);
        bool Dequeue(NetDummyPacket** const packet);

        bool Decode(const char* const data);

        NetSessionID GetSessionID() const;
        BYTE GetType() const;
        int GetSize();

        

    private:
        bool Enqueue(char* const packet, const int size);

    private:
        NetDummyPacket packet_[eNET_PACKET_SET_MAX_COUNT];
        NetLargePacket* originalPacket_;
        NetSessionID NetSessionID_;
        unsigned int writePos_;
        unsigned int readPos_;
        BYTE type_;

        static server_baby::MemTLS<NetPacketSet>* packetQPool_;
        friend class RootServer;
    };


    inline bool NetPacketSet::Enqueue(char* const packet, const int size)
    {

        if (writePos_ >= eNET_PACKET_SET_MAX_COUNT)
            return false;

        packet_[writePos_].Initialize(packet, (packet + size));
        writePos_++;

        return true;
    }

    inline bool NetPacketSet::Dequeue(NetDummyPacket** const packet)
    {
        if (readPos_ == writePos_)
            return false;

        *packet = &packet_[readPos_];
        readPos_++;

        return true;
    }

    inline NetSessionID NetPacketSet::GetSessionID() const
    {
        return NetSessionID_;
    }

    inline BYTE NetPacketSet::GetType() const
    {
        return type_;
    }

    inline int NetPacketSet::GetSize()
    {
        return (writePos_ - readPos_);
    }

    inline void NetPacketSet::DeleteTLS()
    {
        packetQPool_->RemoveRemainders();
        
    }

    inline void NetPacketSet::DeletePool()
    {
        delete packetQPool_;
    }

    inline int NetPacketSet::GetUsedCount()
    {
        return packetQPool_->GetTotalUseCount();
    }

    inline int NetPacketSet::GetCapacity()
    {
        return packetQPool_->GetTotalCapacity();
    }

    inline void NetPacketSet::GarbageCollect()
    {
        packetQPool_->GarbageCollect();
    }

    inline NetPacketSet* server_baby::NetPacketSet::Alloc(const NetSessionID ID, const BYTE type)
    {
        NetPacketSet* packetQ = packetQPool_->Alloc();
        packetQ->writePos_ = NULL;
        packetQ->readPos_ = NULL;
        packetQ->NetSessionID_ = ID;
        packetQ->originalPacket_ = nullptr;
        packetQ->type_ = type;

        return packetQ;
    }

    inline NetPacketSet* server_baby::NetPacketSet::Alloc(const NetSessionID ID, NetLargePacket* const original)
    {
        NetPacketSet* packetQ = packetQPool_->Alloc();
        packetQ->writePos_ = NULL;
        packetQ->readPos_ = NULL;
        packetQ->NetSessionID_ = ID;
        packetQ->originalPacket_ = original;
        packetQ->type_ = NULL;

        return packetQ;
    }

    inline void server_baby::NetPacketSet::Free(NetPacketSet* packetQ)
    {
        if (packetQ->originalPacket_)
            NetLargePacket::Free(packetQ->originalPacket_);

        packetQPool_->Free(packetQ);
    }

}
#endif
