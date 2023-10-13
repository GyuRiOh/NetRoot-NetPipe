

#ifndef __NET__SESSION__ARRAY__
#define __NET__SESSION__ARRAY__

#include "NetSession.h"
#include "NetSessionID.h"
#include "../Common/LockFreeStack.h"
#include <functional>

namespace server_baby
{
    class NetSessionArray final
    {
        enum Setting
        {
            eINDEX_START = 1
        };
    public:
        explicit NetSessionArray(const long interval);
        ~NetSessionArray();

        void Timeout(std::function<void(NetSessionID)> func);
        void Validate();
        void Invalidate();

        NetSession* AllocSession(const SOCKET sock, SOCKADDR_IN* const addr);

        void ErrorQuit(const WCHAR* const msg);
        void ErrorDisplay(const WCHAR* const msg);

        long long GetSessionCount();
        long long GetStackUsedCount();
        long long GetStackCapacity();
        long long GetStackSize();
        static void DeleteStackTLS();

        NetSession* GetSession(const NetSessionID NetSessionID);
        void PushSessionIndex(const NetSessionID NetSessionID);

    private:
        NetSessionID MakeNetSessionID(const int index);

    private:
        LockFreeStack<int>* sessionIndexStack_;
        ULONGLONG lastTimeoutTime_;
        long stamp_;
        NetSession sessionArray_[eNET_SESSION_MAX_COUNT];
        long timeoutInterval_;
    };

    inline void NetSessionArray::ErrorQuit(const WCHAR* const msg)
    {
        SystemLogger::GetInstance()->Console(L"SessionArray", LEVEL_SYSTEM, msg);
        SystemLogger::GetInstance()->LogText(L"SessionArray", LEVEL_SYSTEM, msg);

        CrashDump::Crash();
    }

    inline void NetSessionArray::ErrorDisplay(const WCHAR* const msg)
    {
        SystemLogger::GetInstance()->Console(L"SessionArray", LEVEL_SYSTEM, msg);
        SystemLogger::GetInstance()->LogText(L"SessionArray", LEVEL_SYSTEM, msg);
    }

    inline long long NetSessionArray::GetSessionCount()
    {
        return (eNET_SESSION_MAX_COUNT - 1 - sessionIndexStack_->Size());
    }

    inline long long NetSessionArray::GetStackUsedCount()
    {
        return sessionIndexStack_->GetUsedCount();
    }

    inline long long NetSessionArray::GetStackCapacity()
    {
        return sessionIndexStack_->GetCapacityCount();
    }

    inline long long NetSessionArray::GetStackSize()
    {
        return sessionIndexStack_->Size();
    }

    inline void NetSessionArray::DeleteStackTLS()
    {
        LockFreeStack<int>::DeleteTLS();
    }

    inline void NetSessionArray::PushSessionIndex(const NetSessionID NetSessionID)
    {
        sessionIndexStack_->Push(NetSessionID.element_.index_);
    }

    inline NetSession* NetSessionArray::GetSession(const NetSessionID NetSessionID)
    {
        return &sessionArray_[NetSessionID.element_.index_];
    }

    inline NetSessionID NetSessionArray::MakeNetSessionID(const int index)
    {
        NetSessionID newID;
        newID.element_.index_ = index;
        newID.element_.unique_ = stamp_++;

        return newID;
    }
}

#endif