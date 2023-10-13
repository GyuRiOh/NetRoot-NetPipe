

#ifndef  __LAN__PACKET__SET__
#define  __LAN__PACKET__SET__
#include "LanSession.h"
#include "LanDummyPacket.h"
#include "LanLargePacket.h"
#include "../Common/Queue.h"
#include "../Common/MemTLS.h"
#include "LanEnums.h"


namespace server_baby
{
    class LanPacketSet final
    {
    public:
        explicit LanPacketSet() = delete;
        ~LanPacketSet() = default;

        static LanPacketSet* Alloc(const LanSessionID ID, const BYTE type);
        static LanPacketSet* Alloc(const LanSessionID ID, LanLargePacket* const original);
        static bool Free(LanPacketSet* const packetQ);

        static void DeleteTLS();
        static void DeletePool();
        static int GetUsedCount();
        static int GetCapacity();


        int RegisterPackets(LanLargePacket* const packet);
        bool Dequeue(LanDummyPacket** const packet);

        LanSessionID GetSessionID() const;
        BYTE GetType() const;
        int GetSize();

        

    private:
        bool Enqueue(char* const packet, const int size);

    private:
        LanDummyPacket packet_[eLAN_PACKET_SET_MAX_COUNT];
        LanLargePacket* originalPacket_;
        LanSessionID sessionID_;
        unsigned int writePos_;
        unsigned int readPos_;
        BYTE type_;

        static server_baby::MemTLS<LanPacketSet>* packetQPool_;
        friend class RootServer;
    };


    inline bool LanPacketSet::Enqueue(char* const packet, const int size)
    {

        if (writePos_ >= eLAN_PACKET_SET_MAX_COUNT)
            return false;

        packet_[writePos_].Initialize(packet, (packet + size));
        writePos_++;

        return true;
    }

    inline bool LanPacketSet::Dequeue(LanDummyPacket** const packet)
    {
        if (readPos_ == writePos_)
            return false;

        *packet = &packet_[readPos_];
        readPos_++;

        return true;
    }

    inline LanSessionID LanPacketSet::GetSessionID() const
    {
        return sessionID_;
    }

    inline BYTE LanPacketSet::GetType() const
    {
        return type_;
    }

    inline int LanPacketSet::GetSize()
    {
        return (writePos_ - readPos_);
    }

    inline void LanPacketSet::DeleteTLS()
    {
        packetQPool_->RemoveRemainders();
        
    }

    inline void LanPacketSet::DeletePool()
    {
        delete packetQPool_;
    }

    inline int LanPacketSet::GetUsedCount()
    {
        return packetQPool_->GetTotalUseCount();
    }

    inline int LanPacketSet::GetCapacity()
    {
        return packetQPool_->GetTotalCapacity();
    }

    inline LanPacketSet* server_baby::LanPacketSet::Alloc(const LanSessionID ID, const BYTE type)
    {
        LanPacketSet* packetQ = packetQPool_->Alloc();
        packetQ->writePos_ = NULL;
        packetQ->readPos_ = NULL;
        packetQ->sessionID_ = ID;
        packetQ->originalPacket_ = nullptr;
        packetQ->type_ = type;

        return packetQ;
    }

    inline LanPacketSet* server_baby::LanPacketSet::Alloc(const LanSessionID ID, LanLargePacket* const original)
    {
        LanPacketSet* packetQ = packetQPool_->Alloc();
        packetQ->writePos_ = NULL;
        packetQ->readPos_ = NULL;
        packetQ->sessionID_ = ID;
        packetQ->originalPacket_ = original;
        packetQ->type_ = NULL;

        return packetQ;
    }

    inline bool server_baby::LanPacketSet::Free(LanPacketSet* const packetQ)
    {
        if (packetQ->originalPacket_)
            LanLargePacket::Free(packetQ->originalPacket_);
        packetQPool_->Free(packetQ);
        return true;
    }

}
#endif
