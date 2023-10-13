
#ifndef __NET__SESSION__ID__
#define __NET__SESSION__ID__

#include "../Common/MemTLS.h"
#include "../Common/Queue.h"

namespace server_baby
{
    enum NetPostKey
    {
        eNET_SENDPACKET_KEY = 0xabababab,
        eNET_SENDPACKET_QUEUE_KEY = 0xaeaeaeae,
        eNET_SENDSIGN_KEY = 0xafafafaf,
        eNET_SEND_DISCONNECT_KEY = 0xbabababa,
        eNET_SEND_DISCONNECT_QUEUE_KEY = 0xbdbdbdbd,
        eNET_CLIENT_LEAVE_KEY = 0xfafafafa,
        eNET_DISCONNECT_KEY = 0xfbfbfbfb

    };

    class NetSession;
    //=======================
    //세션 ID 공용체
    //=======================
    union NetSessionID
    {
        struct Element_
        {
            unsigned long unique_;
            unsigned int index_;
        } element_;
        unsigned long long total_;

        NetSessionID() : total_(0) {}
        NetSessionID(NetSession* session)
        {
            total_ = (unsigned long long)session;
        }

        NetSessionID& operator=(const NetSessionID& rhs) {

            this->total_ = rhs.total_;
            return *this;
        }
    };

    class NetSessionIDSet final
    {
        NetSessionIDSet() = default;
        ~NetSessionIDSet() = default;
    public:

        static NetSessionIDSet* Alloc()
        {
            NetSessionIDSet* idSet = idSetPool_->Alloc();
            idSet->queue_.Clear();
            return idSet;
        }

        static bool Free(NetSessionIDSet* const idSet)
        {
            return idSetPool_->Free(idSet);
        }

        static void DeletePool()
        {
            delete idSetPool_;
        }

        static void DeleteTLS()
        {
            idSetPool_->RemoveRemainders();
        }

        static int GetUsedCount()
        {
            return idSetPool_->GetTotalUseCount();
        }

        static int GetCapacity()
        {
            return idSetPool_->GetTotalCapacity();
        }

        unsigned short GetSize()
        {
            return queue_.Size();
        }

        bool Enqueue(const NetSessionID ID)
        {
            return queue_.Enqueue(ID);
        }

        bool Dequeue(NetSessionID* const ID)
        {
            return queue_.Dequeue(ID);
        }

    private:
        Queue<NetSessionID> queue_;
        static server_baby::MemTLS<NetSessionIDSet>* idSetPool_;
    }; 


}

#endif