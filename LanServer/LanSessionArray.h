

#ifndef __LAN__SESSION__ARRAY__
#define __LAN__SESSION__ARRAY__

#include "LanSession.h"
#include "LanSessionID.h"
#include "../Common/LockFreeStack.h"
#include <functional>

namespace server_baby
{
    class LanSessionArray final
    {
        enum Setting
        {
            eINDEX_START = 1
        };
    public:
        explicit LanSessionArray(const long interval);
        ~LanSessionArray();

        void Timeout(std::function<void(LanSessionID)> func);
        void Validate();
        void Invalidate();

        LanSession* AllocSession(const SOCKET sock, SOCKADDR_IN* const addr);

        void ErrorQuit(const WCHAR* const  msg);
        void ErrorDisplay(const WCHAR* const  msg);

        long long GetSessionCount();
        long long GetStackUsedCount();
        long long GetStackCapacity();
        long long GetStackSize();
        static void DeleteStackTLS();

        LanSession* GetSession(const LanSessionID sessionID);
        void PushSessionIndex(const LanSessionID sessionID);

    private:
        LanSessionID MakeLanSessionID(const int index);

    private:
        LockFreeStack<int>* sessionIndexStack_;
        ULONGLONG lastTimeoutTime_;
        long stamp_;
        LanSession sessionArray_[eLAN_SESSION_MAX_COUNT];
        long timeoutInterval_;
    };

    inline void LanSessionArray::ErrorQuit(const WCHAR* const msg)
    {
        SystemLogger::GetInstance()->Console(L"LanSessionArray", LEVEL_SYSTEM, msg);
        SystemLogger::GetInstance()->LogText(L"LanSessionArray", LEVEL_SYSTEM, msg);

        CrashDump::Crash();
    }

    inline void LanSessionArray::ErrorDisplay(const WCHAR* const msg)
    {
        SystemLogger::GetInstance()->Console(L"LanSessionArray", LEVEL_SYSTEM, msg);
        SystemLogger::GetInstance()->LogText(L"LanSessionArray", LEVEL_SYSTEM, msg);
    }

    inline long long LanSessionArray::GetSessionCount()
    {
        return (eLAN_SESSION_MAX_COUNT - 1 - sessionIndexStack_->Size());
    }

    inline long long LanSessionArray::GetStackUsedCount()
    {
        return sessionIndexStack_->GetUsedCount();
    }

    inline long long LanSessionArray::GetStackCapacity()
    {
        return sessionIndexStack_->GetCapacityCount();
    }

    inline long long LanSessionArray::GetStackSize()
    {
        return sessionIndexStack_->Size();
    }

    inline void LanSessionArray::DeleteStackTLS()
    {
        LockFreeStack<int>::DeleteTLS();
    }

    inline void LanSessionArray::PushSessionIndex(const LanSessionID sessionID)
    {
        sessionIndexStack_->Push(sessionID.element_.index_);
    }

    inline LanSession* LanSessionArray::GetSession(const LanSessionID sessionID)
    {
        return &sessionArray_[sessionID.element_.index_];
    }

    inline LanSessionID LanSessionArray::MakeLanSessionID(const int index)
    {
        LanSessionID newID;
        newID.element_.index_ = index;
        newID.element_.unique_ = stamp_++;

        return newID;
    }
}

#endif