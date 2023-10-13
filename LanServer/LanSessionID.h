
#ifndef __LAN__SESSION__ID__
#define __LAN__SESSION__ID__

#include "../Common/MemTLS.h"
#include "../Common/Queue.h"

namespace server_baby
{
    enum LanPostKey
    {
        eLAN_SENDPACKET_KEY = 0xabababab,
        eLAN_SENDPACKET_QUEUE_KEY = 0xaeaeaeae,
        eLAN_SENDSIGN_KEY = 0xafafafaf,
        eLAN_SEND_DISCONNECT_KEY = 0xbabababa,
        eLAN_SEND_DISCONNECT_QUEUE_KEY = 0xbdbdbdbd,
        eLAN_CLIENT_LEAVE_KEY = 0xfafafafa,
        eLAN_DISCONNECT_KEY = 0xfbfbfbfb

    };

    class LanSession;
    //=======================
    //세션 ID 공용체
    //=======================
    union LanSessionID
    {
        struct Element_
        {
            unsigned long unique_;
            unsigned int index_;
        } element_;
        unsigned long long total_;

        LanSessionID() : total_(0) {}
        LanSessionID(LanSession* session)
        {
            total_ = (unsigned long long)session;
        }

        LanSessionID& operator=(const LanSessionID& rhs) {

            this->total_ = rhs.total_;
            return *this;
        }
    };

    class LanSessionIDSet final
    {
        LanSessionIDSet() = default;
        ~LanSessionIDSet() = default;
    public:

        static LanSessionIDSet* Alloc()
        {
            LanSessionIDSet* idSet = idSetPool_->Alloc();
            idSet->queue_.Clear();
            return idSet;
        }

        static bool Free(LanSessionIDSet* const idSet)
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

        bool Enqueue(LanSessionID ID)
        {
            return queue_.Enqueue(ID);
        }

        bool Dequeue(LanSessionID* ID)
        {
            return queue_.Dequeue(ID);
        }

    private:
        Queue<LanSessionID> queue_;
        static server_baby::MemTLS<LanSessionIDSet>* idSetPool_;
    }; 


}

#endif