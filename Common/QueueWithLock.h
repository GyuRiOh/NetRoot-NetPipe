
#ifndef __QUEUE__WITH__LOCK__
#define __QUEUE__WITH__LOCK__

#define _WINSOCKAPI_
#include <Windows.h>
using namespace std;

namespace server_baby
{
    class CrashDump;
    class SystemLogger;

    template <class DATA>
    class QueueWithLock
    {
        enum QueueSetting
        {
            QUEUE_MAX_SIZE_MASK = 2047,
            QUEUE_MAX_SIZE = 2048
        };

        DATA arr_[QUEUE_MAX_SIZE] = { 0 };    
        int front_; 
        int rear_;
        RTL_SRWLOCK lock_;

    public:
        explicit QueueWithLock() : front_(0), rear_(0) { InitializeSRWLock(&lock_);}
        virtual ~QueueWithLock() {}

        void Initialize()
        {
            InitializeSRWLock(&lock_);
            front_ = 0;
            rear_ = 0;
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
            WriteLock();
            if (rear_ == front_)
            {
                WriteUnlock();
                return false;
            }

            *buf = arr_[front_];
            arr_[front_] = NULL;
            front_ = (front_ + 1) & QUEUE_MAX_SIZE_MASK;
            WriteUnlock();
            return true;
        }

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

        bool isEmpty()
        {
            ReadLock();
            bool ret = (rear_ == front_);
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
}

#endif