
#include "NetSessionArray.h"

namespace server_baby
{


    NetSessionArray::NetSessionArray(long interval)
        : stamp_(1), sessionIndexStack_(new LockFreeStack<int>()), lastTimeoutTime_(GetTickCount64()), timeoutInterval_(interval)
    {
        Validate();
    }

    NetSessionArray::~NetSessionArray()
    {
        Invalidate();
    }

    void NetSessionArray::Timeout(std::function<void(NetSessionID)> func)
    {
        if (!timeoutInterval_)
            return;

        ULONGLONG nowTime = GetTickCount64();
        if (nowTime < (lastTimeoutTime_ + timeoutInterval_))
            return;

        for (int i = eINDEX_START; i < eNET_SESSION_MAX_COUNT; i++)
        {
            NetSession* session = &sessionArray_[i];
            if ((session->lastRecvTime_ < lastTimeoutTime_))
                func(session->GetTotalID());

        }

        lastTimeoutTime_ = nowTime;
        return;
    }

    void NetSessionArray::Validate()
    {
        for (int i = (eNET_SESSION_MAX_COUNT - 1); i >= eINDEX_START; i--)
        {
            sessionIndexStack_->Push(i);
        }
    }

    void NetSessionArray::Invalidate()
    {
        for (int i = eINDEX_START; i < eNET_SESSION_MAX_COUNT; i++)
        {
            NetSession* session = &sessionArray_[i];
            if(!session->isDead())
                session->Disconnect();
        }
    }

   

    NetSession* NetSessionArray::AllocSession(const SOCKET sock, SOCKADDR_IN* const addr)
    {
        // 세션에 인덱스 및 ID 할당
        int index = -1;
        if (!sessionIndexStack_->Pop(&index))
        {
            ErrorDisplay(L"CreateSession : Session MAX");
            return nullptr;
        }

        NetSessionID newID = MakeNetSessionID(index);
        sessionArray_[index].Initialize(sock, addr, newID);

        return &sessionArray_[index];
    }

}
