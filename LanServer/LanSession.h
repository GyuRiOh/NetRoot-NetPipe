
#ifndef __LAN__SESSION__
#define __LAN__SESSION__

#define _CRT_SECURE_NO_WARNINGS
#pragma comment(lib, "ws2_32")
#pragma comment(lib, "Winmm.lib")
#include <WinSock2.h>
#include <BaseTsd.h>
#include <timeapi.h>
#include "../Common/LockFreeEnqQueue.h"
#include "LanLargePacket.h"
#include "../Common/SystemLogger.h"
#include "LanSessionID.h"
#include "../Common/Queue.h"
#include "LanEnums.h"
#include "../Common/QueueWithLock.h"

using namespace std;

namespace server_baby
{
    class LanPacketSet;
    class LanPacket;
    class LanPipe;
    constexpr unsigned short LAN_DELETE_FLAG_MASK = 0x7FFF;
    constexpr unsigned short LAN_DELETE_FLAG_BIT = 0x8000;

    enum LanErrorCode
    {
        LAN_PACKET_ERROR = -1,
        LAN_SUCCESS = 1,
        LAN_SUCCESS_ZERO_PACKET_COUNT = 2,
        LAN_SUCCESS_IO_PENDING = 3,
        LAN_FAILURE_RECV_PACKET_FULL = 4,
        LAN_FAILURE_PACKET_ERROR = 5,
        LAN_SEND_FAILURE_NOT_OKAY_TO_SEND = 6,
        LAN_SEND_FAILURE_SENDQ_EMPTY = 7, //에러코드 표 다시 만들기
        LAN_FAILURE_IO_REQUEST = 8,
        LAN_FAILURE_IO_CANCEL_FLAG_ON = 9,
    };

    //===============================
    //소켓 정보 저장을 위한 클래스
    //===============================
    class LanSession final
    {
        enum Setting
        {
            PACKET_ATTACK = -1,
            SEND_Q_CODE = 0x1004,
        };

    public:
        explicit LanSession();
        ~LanSession();

        char RecvPost();       
        char CompleteRecvCheck_PacketQ(LanPacketSet** const packetQBuf, int* const packetCount, const DWORD transferred);
        void SaveOverlappedError(const int transferred, const int errorCode, const LPOVERLAPPED overlapped);

        char SendPost(long* const  oldSendCount);
        char SendAndDisconnectPost(long* const oldSendCount);
        bool EnqueueSendQ(LanPacket* const  packet);
        void SetSendFlagFalse();

        bool isIOCanceled() const;
        bool isDead() const;
        bool isRecvOverlapped(const LPOVERLAPPED overlapped) const;
        bool isSendOverlapped(const LPOVERLAPPED overlapped) const;

        void GetOverlappedResult(const int transferred, const  LPOVERLAPPED overlapped);

        short GetIOCount() const;
        LanSessionID GetTotalID() const;
        short GetSendCount() const;
        LPOVERLAPPED GetRecvOverlapped();
        LPOVERLAPPED GetSendOverlapped();

        unsigned short AddIOCount(const unsigned short value);
        unsigned short SubIOCount(const unsigned short value);
        unsigned short IncrementIOCount();
        unsigned short DecrementIOCount();

        void Disconnect();
        bool SetDeleteFlag();
        bool isSending();
        bool AcquireSession(const LanSessionID ID);

        void ClearJobQ();

        static long GetSendQPoolCount();
        static long GetSendQPoolCapacity();
        static void DeleteSendQPoolTLS();

        bool Destroy();
        bool Initialize(const SOCKET sock, SOCKADDR_IN* const  addr, const  LanSessionID ID);

        void PipeMoveStart() { isMoving_ = true; }
        void PipeMoveEnd() { isMoving_ = false; }
        bool isMovingPipe() { return isMoving_; }

    private:

        void FreeSentPacket();
        bool isSendFlagFalse();
        bool DequeueSendQ(LanPacket** const  packet);

        void CancelIO() const;

        void ErrorCheck(const errno_t err, const WCHAR* const  msg);
        void ErrorQuit(const WCHAR* const msg);
        void ErrorDisplay(const WCHAR* const msg);

    public:
        QueueWithoutCount<LanPacketSet*>* jobQ_;
        LanPipe* curPipe_;
        LanPipe* destPipe_;

    private:
        //====================================
        // 센드 완료 큐 (WSASend에 넣고, 이후 직렬화버퍼에서 Packet 해제)
        //====================================
        bool isSending_;
        WSABUF sentQ_[eLAN_PACKET_SEND_Q_MAX_SIZE] = { 0 };
        unsigned short IOcount_;

        //====================================
        // 오버랩 구조체
        // Each size : 32
        // Total Size : 32
        WSAOVERLAPPED recvOverlapped_;
        WSAOVERLAPPED sendOverlapped_;
        //====================================

        LockFreeEnqQueue<LanPacket*, SEND_Q_CODE, eLAN_PACKET_SEND_Q_MAX_SIZE> sendQ_;
        LanLargePacket* recvPacket_;
        ULONGLONG lastRecvTime_;
        short sentCount_;
        bool isIOCanceled_;
        bool isMoving_;
        
        alignas(64) SOCKADDR_IN clientAddr_;
        LanSessionID ID_;
        SOCKET sock_;

        friend class LanSessionArray;
    };

    inline short LanSession::GetIOCount() const
    {
        return IOcount_;
    }

    inline LanSessionID LanSession::GetTotalID() const
    {
        return ID_;
    }

    inline short LanSession::GetSendCount() const
    {
        return sentCount_;
    }

    inline LPOVERLAPPED LanSession::GetRecvOverlapped()
    {
        ZeroMemory(&recvOverlapped_, sizeof(recvOverlapped_));
        return &recvOverlapped_;
    }

    inline LPOVERLAPPED LanSession::GetSendOverlapped()
    {
        ZeroMemory(&sendOverlapped_, sizeof(sendOverlapped_));
        return &sendOverlapped_;
    }

    inline bool LanSession::isIOCanceled() const
    {
        return isIOCanceled_;
    }

    inline bool LanSession::isDead() const
    {
        return ((short)IOcount_ < 0);
    }

    inline bool LanSession::isRecvOverlapped(const LPOVERLAPPED overlapped) const
    {
        return (&recvOverlapped_ == overlapped);
    }

    inline bool LanSession::isSendOverlapped(const LPOVERLAPPED overlapped) const
    {
        return (&sendOverlapped_ == overlapped);
    }

    inline void LanSession::GetOverlappedResult(const int transferred, const LPOVERLAPPED overlapped)
    {
        DWORD transferredBuf = transferred;
        DWORD flags = 0;
        bool retval = WSAGetOverlappedResult(sock_, overlapped, &transferredBuf, false, &flags);
        if (!retval)
            ErrorCheck(WSAGetLastError(), L"GetOverlappedResult");
    }

    inline void LanSession::SetSendFlagFalse()
    {
        isSending_ = false;
    }

    inline long server_baby::LanSession::GetSendQPoolCount()
    {
        return LockFreeEnqQueue<LanPacket*, SEND_Q_CODE, eLAN_PACKET_SEND_Q_MAX_SIZE>::GetPoolCount();
    }

    inline long server_baby::LanSession::GetSendQPoolCapacity()
    {
        return LockFreeEnqQueue<LanPacket*, SEND_Q_CODE, eLAN_PACKET_SEND_Q_MAX_SIZE>::GetPoolCapacity();
    }

    inline void server_baby::LanSession::DeleteSendQPoolTLS()
    {
        LockFreeEnqQueue<LanPacket*, SEND_Q_CODE, eLAN_PACKET_SEND_Q_MAX_SIZE>::DeleteTLS();
    }

    inline bool LanSession::EnqueueSendQ(LanPacket* const packet)
    {
        if (isIOCanceled())
            return false;

        bool ret = sendQ_.Enqueue(packet);
        if (!ret)
            SystemLogger::GetInstance()->LogText(L"SendQ", LEVEL_DEBUG, L"SendQ Q Enq Failed - FULL");
        return ret;
    }

    inline void LanSession::Disconnect()
    {
        isIOCanceled_ = true;
        CancelIO();
    }

    inline void server_baby::LanSession::CancelIO() const
    {
        CancelIoEx((HANDLE)sock_, NULL);
    }


    inline bool LanSession::AcquireSession(const LanSessionID oldID)
    {
        //DeleteFlag가 켜져있는 상태인지 확인
        if (isDead())
            return false;

        //재사용 되었는지 확인
        if (ID_.element_.unique_ != oldID.element_.unique_)
            return false;

        return true;
    }

    inline unsigned short LanSession::AddIOCount(const unsigned short value)
    {
        return static_cast<unsigned short>(InterlockedAdd((LONG*)&IOcount_, value));
    }

    inline unsigned short LanSession::SubIOCount(const unsigned short value)
    {
        return static_cast<unsigned short>(InterlockedAdd((LONG*)&IOcount_, value));
    }

    inline bool LanSession::DequeueSendQ(LanPacket** const  packet)
    {
        if (!sendQ_.Dequeue(packet))
        {
            ErrorDisplay(L"SendQ Deq Failed");
            return false;
        }

        return true;
    }

    inline unsigned short LanSession::IncrementIOCount()
    {
        return InterlockedIncrement16((SHORT*)&IOcount_);
    }

    inline unsigned short LanSession::DecrementIOCount()
    {
        return InterlockedDecrement16((SHORT*)&IOcount_);
    }

    inline bool LanSession::isSendFlagFalse()
    {
        return (InterlockedExchange8((CHAR*)&isSending_, true) == false);
    }

    inline bool LanSession::SetDeleteFlag()
    {
        return static_cast<bool>(InterlockedCompareExchange16
        ((SHORT*)&IOcount_,
            LAN_DELETE_FLAG_BIT,
            NULL));
    }

    inline bool LanSession::isSending()
    {
        return isSending_;
    }

    inline void LanSession::SaveOverlappedError(const int transferred, const int errorCode, const LPOVERLAPPED overlapped)
    {
        if (isRecvOverlapped(overlapped))
            SystemLogger::GetInstance()->LogText(L"Recv GQCS false", LEVEL_SYSTEM, L"Error Code : %d, Transferred : %d", errorCode, transferred);
        else if (isSendOverlapped(overlapped))
            SystemLogger::GetInstance()->LogText(L"Send GQCS false", LEVEL_SYSTEM, L"Error Code : %d, Transferred : %d", errorCode, transferred);

        if (overlapped)
            GetOverlappedResult(transferred, overlapped);
    }

}

#endif