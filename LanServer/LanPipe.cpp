

#define _WINSOCKAPI_
#include <Windows.h>
#include <process.h>
#include "LanPipe.h"
#include "../Common/SystemLogger.h"
#include "../Common/Crash.h"
#include "LanSessionID.h"
#include "LanServer.h"
#include "LanPacket.h"
#include "LanLargePacket.h"
#include "LanPacketSet.h"
#include "LanUser.h"

using namespace server_baby;

server_baby::LanPipe::LanPipe(LanRoot* server, unsigned int framePerSecond)
    : framePerSecond_(framePerSecond), threadHandle_(INVALID_HANDLE_VALUE), runningFlag_(nullptr), server_(server)
{}

void server_baby::LanPipe::Initialize(LanPipeID ID)
{
    if (pipeID_.total_)
    {
        SystemLogger::GetInstance()->LogText(L"LanPipe", LEVEL_ERROR, L"Double Initialized");
        CrashDump::Crash();
    }

    pipeID_.total_ = ID.total_;
}

void server_baby::LanPipe::Start(const bool* const flag)
{
    SetRunningFlag(flag);
    
    unsigned int threadIDforWorker = NULL;
    threadHandle_ = (HANDLE)_beginthreadex(
        NULL,
        0,
        (_beginthreadex_proc_type)&WorkerThread,
        (LPVOID)this,
        0,
        &threadIDforWorker);

    if (threadHandle_ == NULL)
    {
        SystemLogger::GetInstance()->LogText(L"LanNode", LEVEL_SYSTEM, L"BeginThreadEX Failed!!");
        CrashDump::Crash();
    }

}

void server_baby::LanPipe::SetRunningFlag(const bool* const flag)
{
    runningFlag_ = flag;
}

unsigned int __stdcall server_baby::LanPipe::WorkerThread(LPVOID arg)
{
    LanPipe* pipe = static_cast<LanPipe*>(arg);
    return pipe->MyWorkerProc();
}

unsigned int server_baby::LanPipe::MyWorkerProc()
{
    timeBeginPeriod(1); 

    unsigned int millisecPerFrame = 1000 / framePerSecond_;

    unsigned int currentTick = NULL;
    unsigned int updateTick = NULL;
    unsigned int frameTick = NULL;

    
    LanLoopInfo info;
    info.startTimer_ = time(NULL);
    updateTick = timeGetTime();

    OnStart();

    while ((*runningFlag_)) 
    {
        currentTick = timeGetTime();
        if (currentTick >= frameTick + 1000)
        {
            OnMonitor(&info);
            frameTick = currentTick;
            info.frameCountPerSecond_ = 0;
        }

        int sleepTime = (updateTick + millisecPerFrame) - currentTick;

        if (sleepTime > 0)
        {
            Sleep(sleepTime);
            continue;
        }

        updateTick += millisecPerFrame;

        Join();
        Move();
        Leave();

        MessageProc();
        UpdateProc();

        info.frameCountPerSecond_++;
    }

    OnStop();

    Move();
    Leave();

    server_->TLSClear();

    timeEndPeriod(1);  

    return 0;
}

void server_baby::LanPipe::Join()
{
    LanSessionID ID;
    while (joinQ_.Dequeue(&ID))
    {
        LanUser* user = OnUserJoin(ID);
        if (!userMap_.Insert(ID.total_, user))
        {
            SystemLogger::GetInstance()->LogText(L"LanPipe", LEVEL_ERROR, L"Join - Insert Failed");
            CrashDump::Crash();
        }

        server_->AfterPipeEnter(ID, this);
    }
}

void server_baby::LanPipe::Move()
{
    LanUser* user = nullptr;
    while (moveInQ_.Dequeue(&user))
    {
        user->curPipe_ = pipeID_;
        OnUserMoveIn(user);
        if (!userMap_.Insert(user->GetSessionID().total_, user))
        {

            SystemLogger::GetInstance()->LogText(L"LanPipe", LEVEL_ERROR, L"MoveIn - Insert Failed");
            CrashDump::Crash();
        }

        server_->AfterPipeEnter(user->GetSessionID(), this);
        user->PipeMoveEnd();
    }

    user = nullptr;
    while (moveOutQ_.Dequeue(&user))
    {
        OnUserMoveOut(user);
        if (!userMap_.Delete(user->GetSessionID().total_))
        {

            SystemLogger::GetInstance()->LogText(L"LanPipe", LEVEL_ERROR, L"MoveOut - Delete Failed");
            CrashDump::Crash();
        }

        server_->AfterPipeMoveOut(user);
    }
}

void server_baby::LanPipe::Leave()
{
    LanSessionID ID;
    while (leaveQ_.Dequeue(&ID))
    {

        LanUser* user = FindUser(ID);

        if (!user)
        {
            SystemLogger::GetInstance()->LogText(L"LanPipe", LEVEL_ERROR, L"Leave - Find Failed");
            CrashDump::Crash();
        }

        if(user->GetSessionID().total_ != ID.total_)
        {
            SystemLogger::GetInstance()->LogText(L"LanPipe", LEVEL_ERROR, L"Leave - DeqID, UserID Not Equal");
            CrashDump::Crash();
        }

        if (!userMap_.Delete(ID.total_)) 
        {
            SystemLogger::GetInstance()->LogText(L"LanPipe", LEVEL_ERROR, L"Leave - Delete Failed");
            CrashDump::Crash();
        }

        OnUserLeave(user);

        server_->DeletePipeUser(ID);   
    }
}

void server_baby::LanPipe::MessageProc()
{
    userMap_.Foreach([this](ULONG id, LanUser* user) {
        DequeueJob(user);
        });
}

void server_baby::LanPipe::UpdateProc()
{
    OnUpdate();

    userMap_.Foreach([this](ULONG id, LanUser* user) {
        OnUserUpdate(user);
        });

}

void server_baby::LanPipe::DequeueJob(LanUser* const user)
{
    for (;;)
    {
        LanPacketSet* packetQ = user->DequeueJob();
        if (!packetQ)
            break;

        if (packetQ->GetSessionID().total_ != user->GetSessionID().total_)
        {
            SystemLogger::GetInstance()->LogText(L"LanPipe", LEVEL_ERROR, L"DeqJob - packet ID, user ID Not Equal");
            CrashDump::Crash();
        }
        bool ret = UnpackMsg(packetQ);
        LanPacketSet::Free(packetQ);

        if (!ret)
        {
            SystemLogger::GetInstance()->LogText(L"LanPipe", LEVEL_DEBUG, L"Disconnect, %d, %d", user->GetSessionID().element_.index_, user->GetSessionID().element_.unique_);
            server_->Disconnect(user->GetSessionID());
            break;
        }
    }
}

bool server_baby::LanPipe::UnpackMsg(LanPacketSet* const msgPack)
{
    switch (msgPack->GetType())
    {
    case eLAN_RECVED_PACKET_SET:
    {
        while (msgPack->GetSize() > 0)
        {
            LanDummyPacket* packet = nullptr;
            msgPack->Dequeue(&packet);
      
            if (!OnMessage(msgPack->GetSessionID(), packet))
                return false;
        }
        break;
    }
    default: 
        SystemLogger::GetInstance()->LogText(L"LanPipe", LEVEL_ERROR, L"UnpackMsg - Undefined PacketSet Type");
        CrashDump::Crash();
        break;
    }

    return true;
}
