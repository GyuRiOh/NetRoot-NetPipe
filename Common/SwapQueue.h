
#ifndef __SWAP__QUEUE__
#define __SWAP__QUEUE__

#define _WINSOCKAPI_
#include <Windows.h>
using namespace std;

namespace server_baby
{
    class CrashDump;
    class SystemLogger;

    template <class DATA>
    class SwapQueue
    {
       
    public:
        struct Queue
        {
            enum QueueSetting
            {
                QUEUE_MAX_SIZE_MASK = 511,
                QUEUE_MAX_SIZE = 512
            };

            DATA arr_[QUEUE_MAX_SIZE] = { 0 };
            int front_;
            int rear_;
            RTL_SRWLOCK lock_;

            Queue() : front_(0), rear_(0)
            {
                InitializeSRWLock(&lock_);
            }

            void Clear()
            {
                WriteLock();
                front_ = 0;
                rear_ = 0;
                WriteUnlock();
            }

            bool Dequeue(DATA* buf)
            {
                if (isEmpty())
                    return false;

                *buf = arr_[front_];
                arr_[front_] = NULL;
                front_ = (front_ + 1) & QUEUE_MAX_SIZE_MASK;
                return true;
            }


            bool isEmpty()
            {
                bool ret = (rear_ == front_);
                return ret;
            }

        private:
            bool Enqueue(DATA item)
            {
                WriteLock();
                if (isFull())
                {
                    WriteUnlock();
                    ErrorDisplay(L"Array Queue Full");
                    return false;
                }

                arr_[rear_] = item;
                rear_ = (rear_ + 1) & QUEUE_MAX_SIZE_MASK;
                WriteUnlock();
                return true;
            }


            void WriteLock()
            {
                AcquireSRWLockExclusive(&lock_);
            }

            void WriteUnlock()
            {
                ReleaseSRWLockExclusive(&lock_);
            }

            void ReadLock()
            {
                AcquireSRWLockShared(&lock_);
            }

            void ReadUnlock()
            {
                ReleaseSRWLockShared(&lock_);
            }

            bool isFull()
            {
                bool ret = (((rear_ + 1) & QUEUE_MAX_SIZE_MASK) == front_);
                return ret;
            }

            void ErrorQuit(const WCHAR* message)
            {
                SystemLogger::GetInstance()->LogText(L"QueueWithLock", 2, message);
                CrashDump::Crash();
            }

            void ErrorDisplay(const WCHAR* message)
            {
                SystemLogger::GetInstance()->LogText(L"QueueWithLock", 2, message);
            }


        };

        SwapQueue::Queue* curQueue_;
        SwapQueue::Queue* subQueue_;

        explicit SwapQueue() : curQueue_(&queue1_), subQueue_(&queue2_) { InitializeSRWLock(&lock_); }
        virtual ~SwapQueue() {}

        void Clear()
        {
            queue1_.Clear();
            queue2_.Clear();

            WriteLock();
            curQueue_ = queue1_;
            subQueue_ = queue2_;
            WriteUnlock();
        }

        SwapQueue::Queue* Swap()
        {

            WriteLock();
            SwapQueue::Queue* tempQueue = curQueue_;
            curQueue_ = subQueue_;
            subQueue_ = tempQueue;
            WriteUnlock();

            return tempQueue;
        }

        bool Enqueue(DATA item)
        {
            ReadLock();
            bool ret = curQueue_->Enqueue(item);
            ReadUnlock();
            return ret;
        }

    private:
        void WriteLock()
        {
            AcquireSRWLockExclusive(&lock_);
        }

        void WriteUnlock()
        {
            ReleaseSRWLockExclusive(&lock_);
        }

        void ReadLock()
        {
            AcquireSRWLockShared(&lock_);
        }

        void ReadUnlock()
        {
            ReleaseSRWLockShared(&lock_);
        }

        SwapQueue::Queue queue1_;
        SwapQueue::Queue queue2_;
        RTL_SRWLOCK lock_;


    };
}

#endif