
#ifndef  __LOCK_FREE_QUEUE__
#define  __LOCK_FREE_QUEUE__
#include "MemTLS.h"

using namespace std;
namespace server_baby
{
    template<class DATA>
    struct NodeForLFQ
    {
        DATA data;
        NodeForLFQ<DATA>* next;

        NodeForLFQ() : data(0), next(nullptr) {}

        NodeForLFQ(DATA data) : data(0), next(nullptr)
        {
            data = data;
        }
    };

    template<class DATA, unsigned short CODE, int QUEUE_MAX_SIZE>
    class LockFreeQueue
    {
        enum MASK : unsigned long long
        {
            STAMP_MASK = 0xffff800000000000,
            ADDRESS_MASK = ~(0xffff800000000000),
            INCREMENTING_STAMP = ((unsigned long long)1 << 48)
        };

    public:
        LockFreeQueue() : tail_(nullptr), head_(nullptr), size_(0)
        {
            Initialize();
        }

        ~LockFreeQueue() { Destroy(); }

        static long GetPoolCount();
        static long GetPoolCapacity();
        static void DeleteTLS();

        bool Enqueue(DATA data);
        bool Dequeue(DATA* data);

        bool isFull() const;
        long Size() const;

        void Initialize();
        void Destroy();
    private:
        void ErrorQuit(const WCHAR* message);
        void ErrorDisplay(const WCHAR* message);

        void MaskCheck();
        NodeForLFQ<DATA>* MaskAddress(NodeForLFQ<DATA>* address);
        unsigned long long MaskAndIncrementStamp(NodeForLFQ<DATA>* address);
        unsigned long long GetMsb64(unsigned long long digit);

    private:
        NodeForLFQ<DATA>* tail_;
        alignas(64) NodeForLFQ<DATA>* head_;
        alignas(64) LONG size_;
        static MemTLS<NodeForLFQ<DATA>>* nodePool_;
    };

    template<typename DATA, unsigned short code, int QUEUE_MAX_SIZE>
    inline void LockFreeQueue<DATA, code, QUEUE_MAX_SIZE>::DeleteTLS()
    {
        nodePool_->RemoveRemainders();
    }

    template<class DATA, unsigned short CODE, int QUEUE_MAX_SIZE>
    MemTLS<NodeForLFQ<DATA>>* LockFreeQueue<DATA, CODE, QUEUE_MAX_SIZE>::nodePool_ =
        new MemTLS<NodeForLFQ<DATA>>(300, 1, CODE);


    template<class DATA, unsigned short CODE, int QUEUE_MAX_SIZE>
    bool LockFreeQueue<DATA, CODE, QUEUE_MAX_SIZE>::Enqueue(DATA data)
    {

        if (isFull())
        {
            ErrorDisplay(L"LockFree Q was full of packets");
            return false;
        }


        NodeForLFQ<DATA>* newNode = nodePool_->Alloc();
        newNode->data = data;
        newNode->next = (NodeForLFQ<DATA>*)this;

        for(;;)
        {
            NodeForLFQ<DATA>* localTail = tail_;
            NodeForLFQ<DATA>* maskedLocalTail = MaskAddress(localTail);
            NodeForLFQ<DATA>* localNext = maskedLocalTail->next;

            //마스킹
            unsigned long long maskedStamp = MaskAndIncrementStamp(localTail);
            NodeForLFQ<DATA>* stampAddedNewNode = reinterpret_cast<NodeForLFQ<DATA>*>
                ((unsigned long long)newNode + (unsigned long long)maskedStamp);


            if (localNext == (NodeForLFQ<DATA>*)this)
            {
                if (InterlockedCompareExchangePointer(
                    (PVOID*)&maskedLocalTail->next,
                    stampAddedNewNode,
                    localNext)
                    == localNext)
                {
                    InterlockedCompareExchangePointer(
                        (PVOID*)&tail_,
                        stampAddedNewNode,
                        localTail);
                    break;
                }
            }
            else
                InterlockedCompareExchangePointer(
                    (PVOID*)&tail_,
                    localNext,
                    localTail);
        }

        InterlockedIncrement(&size_);

        return true;
    }

    template<class DATA, unsigned short CODE, int QUEUE_MAX_SIZE>
    bool LockFreeQueue<DATA, CODE, QUEUE_MAX_SIZE>::Dequeue(DATA* data)
    {
        if (InterlockedDecrement(&size_) < 0)
        {
            InterlockedIncrement(&size_);
            return false;
        }

        for(;;)
        {
            NodeForLFQ<DATA>* localHead = head_;
            NodeForLFQ<DATA>* maskedLocalHead = MaskAddress(localHead);
            NodeForLFQ<DATA>* localNext = maskedLocalHead->next;
            NodeForLFQ<DATA>* maskedLocalNext = MaskAddress(localNext);

            if (localHead != head_)
                continue;

            if (localNext == (NodeForLFQ<DATA>*)this)
                continue;

            if (localHead == tail_ && localHead == head_)
                InterlockedCompareExchangePointer(
                    (PVOID*)&tail_,
                    localNext,
                    localHead);

            *data = maskedLocalNext->data;
            if (InterlockedCompareExchangePointer(
                (PVOID*)&head_,
                localNext, localHead)
                == localHead)
            {

                nodePool_->Free(maskedLocalHead);
                break;

            }

        }

        return true;
    }

    template<class DATA, unsigned short CODE, int QUEUE_MAX_SIZE>
    long LockFreeQueue<DATA, CODE, QUEUE_MAX_SIZE>::GetPoolCount()
    {
        return nodePool_->GetTotalUseCount();
    }

    template<class DATA, unsigned short CODE, int QUEUE_MAX_SIZE>
    long LockFreeQueue<DATA, CODE, QUEUE_MAX_SIZE>::GetPoolCapacity()
    {
        return nodePool_->GetTotalCapacity();
    }

    template<class DATA, unsigned short CODE, int QUEUE_MAX_SIZE>
    long LockFreeQueue<DATA, CODE, QUEUE_MAX_SIZE>::Size() const
    {
        return size_;
    }

    template<class DATA, unsigned short CODE, int QUEUE_MAX_SIZE>
    void LockFreeQueue<DATA, CODE, QUEUE_MAX_SIZE>::Initialize()
    {
        MaskCheck();

        head_ = nodePool_->Alloc();
        head_->next = (NodeForLFQ<DATA>*)this;
        tail_ = head_;
        size_ = 0;

    }

    template<class DATA, unsigned short CODE, int QUEUE_MAX_SIZE>
    void LockFreeQueue<DATA, CODE, QUEUE_MAX_SIZE>::Destroy()
    {

        while (Size() > 0)
        {
            DATA data;
            if (!Dequeue(&data))
                ErrorQuit(L"Destoy - sendQ size is more than 0 - dequeue failed");
        }

        NodeForLFQ<DATA>* maskedLocalHead = MaskAddress(head_);
        nodePool_->Free(head_);
    }

    template<class DATA, unsigned short CODE, int QUEUE_MAX_SIZE>
    bool LockFreeQueue<DATA, CODE, QUEUE_MAX_SIZE>::isFull() const
    {
        return (size_ >= QUEUE_MAX_SIZE);
    }

    template<class DATA, unsigned short CODE, int QUEUE_MAX_SIZE>
    void LockFreeQueue<DATA, CODE, QUEUE_MAX_SIZE>::ErrorQuit(const WCHAR* message)
    {
        SystemLogger::GetInstance()->LogText(L"LockFreeQueue", LEVEL_SYSTEM, message);
        CrashDump::Crash();
    }

    template<class DATA, unsigned short CODE, int QUEUE_MAX_SIZE>
    void LockFreeQueue<DATA, CODE, QUEUE_MAX_SIZE>::ErrorDisplay(const WCHAR* message)
    {
        SystemLogger::GetInstance()->LogText(L"LockFreeQueue", LEVEL_DEBUG, message);
    }

    template<class DATA, unsigned short CODE, int QUEUE_MAX_SIZE>
    void LockFreeQueue<DATA, CODE, QUEUE_MAX_SIZE>::MaskCheck()
    {
        //==========================================================			
        SYSTEM_INFO system_info;
        GetSystemInfo(&system_info);

        unsigned long long max_memory_address = (unsigned long long)system_info.lpMaximumApplicationAddress;

        //현재 메모리풀 구조로 사용한 주소 상위 비트 개수는 17개.
        //여기서 CRASH가 났다는 건, GetSystemInfo로 얻어진 상위 비트 개수가 변동되었다는 의미이다.
        if (ADDRESS_MASK != GetMsb64(max_memory_address))
            ErrorQuit(L"Address Mask Need To Be Changed");
        //==========================================================
    }

    template<class DATA, unsigned short CODE, int QUEUE_MAX_SIZE>
    NodeForLFQ<DATA>* LockFreeQueue<DATA, CODE, QUEUE_MAX_SIZE>::MaskAddress(NodeForLFQ<DATA>* address)
    {
        NodeForLFQ<DATA>* localAddr = (NodeForLFQ<DATA>*)((unsigned long long)address & ADDRESS_MASK);
        return localAddr;
    }

    template<class DATA, unsigned short CODE, int QUEUE_MAX_SIZE>
    unsigned long long LockFreeQueue<DATA, CODE, QUEUE_MAX_SIZE>::MaskAndIncrementStamp(NodeForLFQ<DATA>* address)
    {
        unsigned long long localStamp = ((unsigned long long)address & STAMP_MASK) + INCREMENTING_STAMP;
        return localStamp;
    }

    template<class DATA, unsigned short CODE, int QUEUE_MAX_SIZE>
    unsigned long long LockFreeQueue<DATA, CODE, QUEUE_MAX_SIZE>::GetMsb64(unsigned long long digit)
    {
        digit |= digit >> 1;
        digit |= digit >> 2;
        digit |= digit >> 4;
        digit |= digit >> 8;
        digit |= digit >> 16;
        digit |= digit >> 32;

        return digit;
    }

}
#endif