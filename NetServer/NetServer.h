
#ifndef __ROOT__NET__SERVER__
#define __ROOT__NET__SERVER__

#include "NetSession.h"
#include "../Common/Crash.h"
#include "../Common/SystemLogger.h"
#include "NetDummyPacket.h"
#include "NetSessionArray.h"
#include "NetPacketSet.h"
#include "../Common/MyMultimap.h"
#include "../Common/SizedMemoryPool.h"
#include "../Common/MyList.h"
#include <process.h>
#include <vector>

using namespace std;

namespace server_baby
{
    union NetPipeID;
    class NetUser;
    class NetPipe;
    class NetStub;

    class NetRoot
    {
        enum Settings
        {
            eTCPIP_HEADER_SIZE = 40,
            eSEGMENT_SIZE = 1460
        };

        struct TPSInfo 
        {
            long sendTPS_ = 0;
            long recvTPS_ = 0;
            long recvIOPending_ = 0;
            long sendIOPending_ = 0;
            long recvTraffic_ = 0;
            long sendTraffic_ = 0;

        };
    
    protected:
        struct MonitoringInfo
        {
            __int64 sessionCount_ = 0;
            int packetCount_ = 0;
            int packetReadyCount_ = 0;

            int largePacketCount_ = 0;
            int largePacketReadyCount_ = 0;

            int packetQCount_ = 0;
            int packetQReadyCount_ = 0;

            __int64 stackCount_ = 0;
            __int64 stackReadyCount_ = 0;
            __int64 stackSize_ = 0;

            __int64 LFQNodeUsed_ = 0;
            __int64 LFQNodeCap_ = 0;

            int sessionSetUsed_ = 0;
            int sessionSetCap_ = 0;

            int recvTPS_ = 0;
            int sendTPS_ = 0;
            int acceptTPS_ = 0;
            long long acceptTotal_ = 0;
            int recvTraffic_ = 0;
            int sendTraffic_ = 0;

            int recvIOPending_ = 0;
            int sendIOPending_ = 0;
        };

    public:
        explicit NetRoot();
        virtual ~NetRoot();

        void Start(
            const unsigned short port,
            const int waitingThreadNum,
            const int runningThreadNum,
            const bool nagle,
            const int timeOutInterval,
            const bool logicMultithreaded,
            const bool useSendIOPending,
            const bool autoManage = false,
            const int monitorInterval = 1000); //오픈IP, 포트, 워커 스레드 수, 네이글 옵션, 최대 접속자 수
        //모니터링 인터벌 ms단위

        void Stop(void);
 
        //===========================
        // 컨텐츠에서 요청하는 IO관련 함수들
        //===========================
        bool SendPacket(const NetSessionID NetSessionID, NetPacket* const packet);
        void AsyncSendPacket(const NetSessionID sessionID, NetPacket* const packet);
        void AsyncSendPacket(NetSessionIDSet* const idSet, NetPacket* const packet);

        bool Disconnect(const NetSessionID sessionID);
        void DisconnectAfterLastMessage(const NetSessionID sessionID, NetPacket* const packet);
        void DisconnectAfterLastMessage(NetSessionIDSet* const idSet, NetPacket* const packet);


        //===========================
        // Pipe 
        //=========================== 
        bool RegisterPipe(const unsigned short code, NetPipe* const pipe);
        void DeletePipe(const unsigned short code, NetPipe* const pipe);
        void DeleteAllPipe(const unsigned short code);

        bool MovePipe(const NetSessionID ID, const unsigned int pipeCode);
        bool MovePipe(NetUser* const user, const unsigned int pipeCode);
        void TLSClear();

        void AfterPipeMoveOut(NetUser* const user);
        void AfterPipeEnter(const NetSessionID ID, NetPipe* const thisPipe);
        void DeletePipeUser(const NetSessionID sessionID);
        QueueWithoutCount<NetPacketSet*>* GetPipeUserJobQ(const NetSessionID NetSessionID); //Pipe 이외에서 사용 금지
        SOCKADDR_IN GetPipeUserAddress(const NetSessionID sessionID); //Pipe이외에서 사용 금지

        void ForeachForSamePipe(std::function<void(ULONG, NetPipe*)> func, const unsigned short code);
        int DestroyZeroUserPipe(const unsigned short code);
        void RevivePipe(const unsigned short code);

        bool isServerCreated();
        bool isServerRunning();
        void StartPipe(NetPipe* const pipe);

        //=============================
        // RPC
        //=============================
        bool RegisterStub(NetStub* const stub);
        bool PacketProc(NetPacketSet* const packetSet);

    protected:
        //==========================
        //순수 가상함수들
        //오버라이딩해서 사용하는 용도이다.
        //==========================
        virtual bool OnConnectionRequest(const SOCKADDR_IN* const addr) = 0; //Accept 직후. return false시 클라이언트 거부, true시 접속 허용
        virtual void OnClientJoin(const NetSessionID NetSessionID) = 0; //Accept 후 접속 처리 완료 후 호출.
        virtual void OnClientLeave(const NetSessionID NetSessionID) = 0; //Release 후 호출
        virtual void OnRecv(NetPacketSet* const packetList) = 0;  //패킷 수신 완료 후
        virtual void OnSend(const NetSessionID NetSessionID, const int sendSize) = 0; //패킷 송신 완료 후
        virtual void OnWorkerThreadBegin() = 0; //워커스레드 GQCS 하단에서 호출
        virtual void OnWorkerThreadEnd() = 0; //워커스레드 1루프 종료 후
        virtual void OnError(const int errCode, WCHAR* msg) = 0;
        virtual void OnMonitor(const MonitoringInfo* const info) = 0;
        virtual void OnStart() = 0;
        virtual void OnStop() = 0;
        virtual void OnTimeout(const NetSessionID sessionID) { Disconnect(sessionID); }

        //==========================
        //에러 정의
        //==========================
        void ErrorQuit(const WCHAR* const msg);
        void ErrorQuitWithErrorCode(const WCHAR* const function);
        void ErrorDisplay(const WCHAR* const msg);
        void ErrorDisplayWithErrorCode(const WCHAR* const function);

        //==========================
        //컨텐츠 편의 함수
        //==========================
        void SetTLSIndex(DWORD* const index);
        void DefaultOnRecv(NetPacketSet* const packetList) { EnqueueJob(packetList->GetSessionID(), packetList); }

        //==========================
        //세션 큐에 직접 job 넣기
        //==========================
        bool EnqueueJob(const NetSessionID NetSessionID, NetPacketSet* const job);


    private:
        //=============================
        // 스레드 함수
        //=============================
        static DWORD WINAPI WorkerThread(LPVOID arg); //작업자 스레드 함수
        static DWORD WINAPI AcceptThread(LPVOID arg); //Accept 스레드 함수
        static DWORD WINAPI ObserverThread(LPVOID arg); //TPS 측정용 타이머 함수
        static DWORD WINAPI TimeoutThread(LPVOID arg);
        
        DWORD MyWorkerProc(); //워커스레드 실제 함수
        DWORD MyAcceptProc(); //Accept 실제 스레드 함수
        DWORD MyObserverProc(); //TPS 실제 스레드 함수
        DWORD MyTimeoutProc();
  
        //=============================
        // Recv, Send Post
        //=============================
        void RecvPost(NetSession* const session);
        void SendPost(NetSession* const session);
        void SendAndDisconnectPost(NetSession* const session);
        void OnRecvComplete(NetSession* const session, const DWORD transferred);
        void OnSendComplete(NetSession* const session);
        void PostOtherProcedures(const NetSessionID session, ULONG_PTR const transferred, NetPacket* const packet);

        //=============================
        // 서버 가동시 초기화
        //=============================
        void ServerInitiate();
        void ZeroLogs();
        bool PrepareWinSock();
        bool CreateIOCP();
        bool PrepareSocket();
        bool Bind();
        bool Listen();
        bool RunAcceptThread();
        bool RunObserverThread();
        bool RunWorkerThread();
        bool RunPipes();

        //===============================================
        //TPS 계산, 초기화, 겟터
        //===============================================
        void AddRecvTPS(const DWORD recvCount);
        void AddSendTPS(const DWORD sendCount);
        void AddRecvIOPending(const DWORD recvIOPendingCount);
        void AddSendIOPending(const DWORD sendIOPendingCount);
        void AddRecvTraffic(const DWORD transferred);
        void AddSendTraffic(const DWORD transferred);
        DWORD GetRecvTPS();
        DWORD GetSendTPS();
        DWORD GetRecvIOPending();
        DWORD GetSendIOPending();
        DWORD GetRecvTraffic();
        DWORD GetSendTraffic();
        void ZeroTPS();
        TPSInfo* GetTPS();
        TPSInfo* MakeTPS();

        //=============================================
        //세션 IOCount 증감 및 감소
        //=============================================
        void IncrementIOCount(NetSession* const session);
        void DecrementIOCount(NetSession* const session);
        void AddIOCount(NetSession* const session, const int num);
        void SubIOCount(NetSession* const session, const int num);
        void ReleaseSession(NetSession* const session);
        void DeleteSession(NetSession* const session);

        //=============================================
        //세션 탐색
        //=============================================
        NetSession* FindSession(const NetSessionID NetSessionID);
        NetSession* FindSessionWithoutInterlock(const NetSessionID NetSessionID);
        NetSession* FindSessionForSendPacket(const NetSessionID NetSessionID);

        //=============================================
        //세션 정리
        //=============================================
        void DisconnectSession(NetSession* const session);
        void DisconnectSessionForSendPacket(NetSession* const session);

        //=============================================
        //Pipe
        //=============================================
        NetPipe* FindPipe_LoadBalanced(const unsigned int pipeCode); 

    private:
        NetSessionArray* onlineArray_;
        long long acceptTotal_;
        long acceptTPS_;

        std::vector<HANDLE> threadHandle_;
        HANDLE timeoutHandle_;
        SOCKET listenSocket_;
        NetStub* stub_;
        HANDLE hcp_;
        int waitingThreadNum_;
        int runningThreadNum_;
        int timeoutInterval_;
        int monitorInterval_;
        unsigned short port_;
        bool isRunning_;
        bool isAcceptThreadRunning_;
        bool isObserverThreadRunning_;
        bool isSendIOPending_;
        bool isUsingNagle_;
        bool isLogicMultithreaded_;
        bool isCreated_;
        bool isAutoManaged_;

        MyMultimap<ULONG, NetPipe*> pipeMap_;
        RTL_SRWLOCK pipeLock_;
        ULONG pipeIndex_;

        std::vector<TPSInfo*> TPSArray_;
        DWORD TPS_TLS_;
        SHORT TPSIndex_;

        struct tm startT_ = { 0 };
        time_t startTimer_;

    };


    inline void server_baby::NetRoot::AddRecvTPS(const DWORD recvCount)
    {
        TPSInfo* info = GetTPS();
        info->recvTPS_ += recvCount;
    }

    inline void server_baby::NetRoot::AddSendTPS(const DWORD sendCount)
    {
        TPSInfo* info = GetTPS();
        info->sendTPS_ += sendCount;
    }

    inline void server_baby::NetRoot::AddRecvIOPending(const DWORD recvIOPendingCount)
    {
        TPSInfo* info = GetTPS();
        info->recvIOPending_ += recvIOPendingCount;
    }

    inline void server_baby::NetRoot::AddSendIOPending(const DWORD sendIOPendingCount)
    {
        TPSInfo* info = GetTPS();
        info->sendIOPending_ += sendIOPendingCount;
    }

    inline void server_baby::NetRoot::AddRecvTraffic(const DWORD transferred)
    {
        int headerCount = (int)(transferred / eSEGMENT_SIZE);
        int traffic = transferred + ((headerCount + 1) * eTCPIP_HEADER_SIZE);

        TPSInfo* info = GetTPS();
        info->recvTraffic_ += traffic;
    }

    inline void server_baby::NetRoot::AddSendTraffic(const DWORD transferred)
    {
        int headerCount = (int)(transferred / eSEGMENT_SIZE);
        int traffic = transferred + ((headerCount + 1) * eTCPIP_HEADER_SIZE);

        TPSInfo* info = GetTPS();
        info->sendTraffic_ += traffic;
    }

    inline DWORD server_baby::NetRoot::GetRecvTPS()
    {
        DWORD recvTPS = 0;
        for (int i = 0; i < TPSIndex_; i++)
        {
            recvTPS += TPSArray_[i]->recvTPS_;
        }

        return recvTPS;
    }

    inline DWORD server_baby::NetRoot::GetSendTPS()
    {
        DWORD sendTPS = 0;
        for (int i = 0; i < TPSIndex_; i++)
        {
            sendTPS += TPSArray_[i]->sendTPS_;
        }

        return sendTPS;
    }

    inline DWORD server_baby::NetRoot::GetRecvIOPending()
    {
        DWORD recvIOPending = 0;
        for (int i = 0; i < TPSIndex_; i++)
        {
            recvIOPending += TPSArray_[i]->recvIOPending_;
        }

        return recvIOPending;
    }

    inline DWORD server_baby::NetRoot::GetSendIOPending()
    {

        DWORD sendIOPending = 0;
        for (int i = 0; i < TPSIndex_; i++)
        {
            sendIOPending += TPSArray_[i]->sendIOPending_;
        }

        return sendIOPending;
    }

    inline DWORD server_baby::NetRoot::GetRecvTraffic()
    {
        DWORD recvTraffic = 0;
        for (int i = 0; i < TPSIndex_; i++)
        {
            recvTraffic += TPSArray_[i]->recvTraffic_;
        }

        return recvTraffic;
    }

    inline DWORD server_baby::NetRoot::GetSendTraffic()
    {
        DWORD sendTraffic = 0;
        for (int i = 0; i < TPSIndex_; i++)
        {
            sendTraffic += TPSArray_[i]->sendTraffic_;
        }

        return sendTraffic;
    }

    inline void server_baby::NetRoot::ZeroTPS()
    {
        for (int i = 0; i < TPSIndex_; i++)
        {
            TPSArray_[i]->recvIOPending_ = 0;
            TPSArray_[i]->sendIOPending_ = 0;
            TPSArray_[i]->recvTPS_ = 0;
            TPSArray_[i]->sendTPS_ = 0;
            TPSArray_[i]->recvTraffic_ = 0;
            TPSArray_[i]->sendTraffic_ = 0;
        }
    }

    inline bool server_baby::NetRoot::isServerRunning()
    {
        return isRunning_;
    }

    inline bool NetRoot::isServerCreated()
    {
        return isCreated_;
    }

    inline bool server_baby::NetRoot::EnqueueJob(const NetSessionID sessionID, NetPacketSet* const job)
    {
        NetSession* session = FindSessionWithoutInterlock(sessionID);
        return session->jobQ_->Enqueue(job);
    }

    inline server_baby::NetRoot::TPSInfo* server_baby::NetRoot::GetTPS()
    {
        TPSInfo* tps = (TPSInfo*)TlsGetValue(TPS_TLS_);
        if (tps)
            return tps;

        return MakeTPS();
    }

    inline void NetRoot::IncrementIOCount(NetSession* const session)
    {
        session->IncrementIOCount();
    }

    inline void NetRoot::DecrementIOCount(NetSession* const session)
    {
        short ioCount = session->DecrementIOCount();
        if (ioCount == 0)
            ReleaseSession(session);
        else if (ioCount == 0x7FFF)
            ErrorDisplay(L"IOCount Under Zero");
        
    }

    inline void NetRoot::AddIOCount(NetSession* const session, const int num)
    {
        session->AddIOCount(num);
    }

    inline void NetRoot::SubIOCount(NetSession* const session, const int num)
    {
        short ioCount = session->SubIOCount(num);
        if (ioCount == 0)
            ReleaseSession(session);
        else if (ioCount == 0x7FFE)
            ErrorDisplay(L"IOCount Under Zero");
    }

    inline NetSession* NetRoot::FindSession(const NetSessionID sessionID)
    {
        
        NetSession* session = onlineArray_->GetSession(sessionID);
        session->IncrementIOCount();

        if (!session->AcquireSession(sessionID))
        {
            DecrementIOCount(session);
            return nullptr;
        }

        return session;
    }

    inline NetSession* NetRoot::FindSessionWithoutInterlock(const NetSessionID sessionID)
    {
        NetSession* session = onlineArray_->GetSession(sessionID);
        return session;
    }

    inline NetSession* NetRoot::FindSessionForSendPacket(const NetSessionID sessionID)
    {
        NetSession* session = onlineArray_->GetSession(sessionID);
        session->AddIOCount(2);

        if (!session->AcquireSession(sessionID))
        {
            SubIOCount(session, -2);
            return nullptr;
        }

        return session;
    }

    inline void NetRoot::DisconnectSession(NetSession* const session)
    {
        session->Disconnect();
        DecrementIOCount(session);
    }

    inline void NetRoot::DisconnectSessionForSendPacket(NetSession* const session)
    {
        SystemLogger::GetInstance()->LogText(L"SendPacket",
            LEVEL_ERROR,
            L"Enq Fail - Disconecct. Cur ID : %d, IO Count : %d",
            session->GetTotalID().element_.unique_, session->GetIOCount());

        session->Disconnect();
        SubIOCount(session, -2);
    }

    
    inline server_baby::NetRoot::TPSInfo* server_baby::NetRoot::MakeTPS()
    {
        TPSInfo* newTps = new TPSInfo();
        short tempIndex = 0;
        short newIndex = 0;
        do {
            tempIndex = TPSIndex_;
            newIndex = tempIndex + 1;

        } while (InterlockedCompareExchange16(
            (SHORT*)&TPSIndex_,
            newIndex,
            tempIndex) != tempIndex);

        TPSArray_[tempIndex] = newTps;

        if (TlsSetValue(TPS_TLS_, newTps) == false)
            ErrorQuit(L"TlsSetValue");

        return newTps;
    }


    inline void NetRoot::ZeroLogs()
    {
        acceptTPS_ = 0;
        ZeroTPS();
    }

    inline void server_baby::NetRoot::DeleteSession(NetSession* const session)
    {
        session->Destroy();
        onlineArray_->PushSessionIndex(session->GetTotalID());
    }
}




#endif