
#include "LanSession.h"
#include "LanPacketSet.h"
#include "LanPacket.h"
using namespace std;

#define LAST_RECV_TIME_DEAD 0xffffffffffffffff

namespace server_baby
{
    LanSession::LanSession() : sock_(INVALID_SOCKET), sentCount_(0),
        recvPacket_(nullptr), lastRecvTime_(LAST_RECV_TIME_DEAD), isSending_(false), IOcount_(LAN_DELETE_FLAG_BIT),
        isIOCanceled_(false), curPipe_(nullptr), destPipe_(nullptr), isMoving_(false)
    {
        ZeroMemory(&recvOverlapped_, sizeof(recvOverlapped_));
        ZeroMemory(&sendOverlapped_, sizeof(sendOverlapped_));
        ZeroMemory(&clientAddr_, sizeof(clientAddr_));
        ZeroMemory(&sentQ_, sizeof(sentQ_));

        jobQ_ = new QueueWithoutCount<LanPacketSet*>;
    }

    LanSession::~LanSession()
    {
        Destroy();
        delete jobQ_;
    }

    void LanSession::ErrorQuit(const WCHAR* const  msg)
    {
        SystemLogger::GetInstance()->Console(L"LanSession", LEVEL_SYSTEM, msg);
        SystemLogger::GetInstance()->LogText(L"LanSession", LEVEL_SYSTEM, msg);

        CrashDump::Crash();
    }

    void LanSession::ErrorDisplay(const WCHAR* const  msg)
    {
        SystemLogger::GetInstance()->Console(L"LanSession", LEVEL_SYSTEM, msg);
        SystemLogger::GetInstance()->LogText(L"LanSession", LEVEL_SYSTEM, msg);
    }

    bool LanSession::Initialize(const SOCKET sock, SOCKADDR_IN* const  addr, const LanSessionID ID)
    {
        IncrementIOCount();

        ID_ = ID;
        sock_ = sock;
        recvPacket_ = LanLargePacket::Alloc();
        lastRecvTime_ = GetTickCount64();
        isIOCanceled_ = false;
        clientAddr_ = *addr;

        InterlockedAnd16((short*)&IOcount_, LAN_DELETE_FLAG_MASK);

        return true;
    }


    bool LanSession::Destroy()
    {
        FreeSentPacket();
        SetSendFlagFalse();

        if(recvPacket_)
            LanLargePacket::Free(recvPacket_);
        
        while (sendQ_.Size() > 0)
        {
            LanPacket* packet = nullptr;
            if (!sendQ_.Dequeue(&packet))
                ErrorQuit(L"Destoy - sendQ size is more than 0 - dequeue failed");

            LanPacket::Free(packet);
        }

        if (sendQ_.Size() != 0)
            ErrorQuit(L"Destoy - SendQ Size Not Zero");
        
        lastRecvTime_ = LAST_RECV_TIME_DEAD;
        closesocket(sock_);
        if (InterlockedExchange64((LONG64*)&sock_, (LONG64)INVALID_SOCKET) == INVALID_SOCKET)
            ErrorQuit(L"Destroy - Socket Closed Twice");

        curPipe_ = nullptr;
        destPipe_ = nullptr;
        ClearJobQ();

        isMoving_ = false;
        return true;

    }

    char LanSession::RecvPost()
    {
        IncrementIOCount();

        DWORD flags = 0;
        WSABUF recvBuf[1];

        recvBuf->buf = recvPacket_->GetWritePos();
        recvBuf->len = (ULONG)recvPacket_->GetEmptySize() - 1;
        

        int retval = WSARecv(sock_,
            recvBuf,
            1,
            NULL,
            &flags,
            GetRecvOverlapped(),
            NULL);

        if (retval == SOCKET_ERROR)
        {
            errno_t err = WSAGetLastError();
            if (err != WSA_IO_PENDING)
            {
                ErrorCheck(err, L"WSARecv()");
                return LAN_FAILURE_IO_REQUEST;
            }

            if (isIOCanceled_)
            {
                CancelIO();
                return LAN_FAILURE_IO_CANCEL_FLAG_ON;
            }

            return LAN_SUCCESS_IO_PENDING;
        }

        return LAN_SUCCESS;

    }


    char LanSession::SendPost(long* const  oldSendCount)
    {
        if (!isSendFlagFalse())
            return LAN_SEND_FAILURE_NOT_OKAY_TO_SEND;

        *oldSendCount = sentCount_;
        FreeSentPacket();

        short sendCount = 0;
        while (sendQ_.Size() > 0)
        {
            LanPacket* packet = nullptr;
            if (!DequeueSendQ(&packet))
                break;

            sentQ_[sendCount].buf = packet->GetPacketStart();
            sentQ_[sendCount].len = static_cast<ULONG>(packet->GetPacketUsedSize());

            sendCount++;

        }

        if (sendCount > eLAN_PACKET_SEND_Q_MAX_SIZE)
            CrashDump::Crash();

        if (sendCount == 0)
        {
            SetSendFlagFalse();
            return LAN_SEND_FAILURE_SENDQ_EMPTY;
        }

        sentCount_ = sendCount;

        int retval = WSASend(sock_,
            sentQ_,
            sendCount,
            NULL,
            0,
            GetSendOverlapped(),
            NULL);


        if (retval == SOCKET_ERROR)
        {
            errno_t err = WSAGetLastError();
            if (err != WSA_IO_PENDING)
            {
                ErrorCheck(err, L"WSASend()");
                return LAN_FAILURE_IO_REQUEST;
            }

            if (isIOCanceled_)
            {
                CancelIO();
                return LAN_FAILURE_IO_CANCEL_FLAG_ON;
            }

            return LAN_SUCCESS_IO_PENDING;
        }

        return LAN_SUCCESS;


    }

    char LanSession::SendAndDisconnectPost(long* const oldSendCount)
    {
        //보내고 끊기 다시만들기
        Disconnect();

        while (!isSendFlagFalse()) { };

        isIOCanceled_ = true;

        *oldSendCount = sentCount_;
        FreeSentPacket();

        short sendCount = 0;
        while (sendQ_.Size() > 0)
        {
            LanPacket* packet = nullptr;
            if (!DequeueSendQ(&packet))
                break;

            sentQ_[sendCount].buf = packet->GetPacketStart();
            sentQ_[sendCount].len = static_cast<ULONG>(packet->GetPacketUsedSize());

            sendCount++;

        }

        if (sendCount == 0)
        {
            SetSendFlagFalse();
            return LAN_SEND_FAILURE_SENDQ_EMPTY;
        }

        sentCount_ = sendCount;
        int retval = WSASend(sock_,
                sentQ_,
                sendCount,
                NULL,
                0,
                GetSendOverlapped(),
                NULL);


        if (retval == SOCKET_ERROR)
        {
            errno_t err = WSAGetLastError();
            if (err != WSA_IO_PENDING)
            {
                ErrorCheck(err, L"WSASend()");
                return LAN_FAILURE_IO_REQUEST;
            }

            return LAN_SUCCESS_IO_PENDING;
        }

        return LAN_SUCCESS;
    }


    char server_baby::LanSession::CompleteRecvCheck_PacketQ(LanPacketSet** const packetQBuf, int* const packetCount, const  DWORD transferred)
    {
        lastRecvTime_ = GetTickCount64();

        LanLargePacket* packet = recvPacket_;
        if (!packet->MoveWritePos(transferred))
        {
            ErrorDisplay(L"RECV_PACKET_FULL_DISCONNECT");
            return LAN_FAILURE_RECV_PACKET_FULL;
        }
     

        LanPacketSet* packetQ = LanPacketSet::Alloc(GetTotalID(), packet);
        int packetCnt = packetQ->RegisterPackets(packet);
        if(packetCnt == LAN_PACKET_ERROR)
        {
            LanPacketSet::Free(packetQ);
            return LAN_FAILURE_PACKET_ERROR;
        }

        LanLargePacket* newPacket = packet->CopyRemainderToNewPacket(packetCnt);
        recvPacket_ = newPacket;

        *packetCount = packetCnt;
        if (packetCnt > 0)
        {
            *packetQBuf = packetQ;
            return LAN_SUCCESS;
        }
        else
        {
            LanPacketSet::Free(packetQ);
            return LAN_SUCCESS_ZERO_PACKET_COUNT;
        }

    }

    void server_baby::LanSession::ErrorCheck(const errno_t err, const WCHAR* const msg)
    {
        switch (err)
        {
        case 10053:
        case 10054:
            break;
        default:
        {
            SystemLogger::GetInstance()->LogText(L"Session_Disconnect",
                LEVEL_ERROR, L"%ws : error code - %d", msg, err);           
        }
        break;
        } 
        
        Disconnect();

    }

    void LanSession::FreeSentPacket()
    {
        //직렬화버퍼들 릴리즈하자.
        for (int i = 0; i < sentCount_; i++)
        {
            LanPacket::Free((LanPacket*)sentQ_[i].buf);
        }

        sentCount_ = 0;
    }

    void LanSession::ClearJobQ()
    {
        LanPacketSet* set = nullptr;
        while (jobQ_->Dequeue(&set))
        {
            LanPacketSet::Free(set);
        }
        jobQ_->Clear();
    }
}
