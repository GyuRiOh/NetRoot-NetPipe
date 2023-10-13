#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include "LanClient.h"
#include <process.h>

server_baby::LanClient::LanClient() : serverPort_(NULL), isRunning_(false),
    workerThreadID_(NULL), workerThread_(INVALID_HANDLE_VALUE), sock_(INVALID_SOCKET),
    recvBuffer_(nullptr)
{
    CrashDump::GetInstance();
    setlocale(LC_ALL, "");   
}

server_baby::LanClient::~LanClient(){}

void server_baby::LanClient::Start(char* serverIP, unsigned short serverPort)
{
    strcpy_s(serverIP_, serverIP);
    serverPort_ = serverPort;

    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
        ErrorQuit(L"WSAStartup Failed");

    SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == INVALID_SOCKET)
        ErrorQuit(L"socket() - INVALID SOCKET");

    ZeroMemory(&serverAddr_, sizeof(serverAddr_));
    serverAddr_.sin_addr.S_un.S_addr = inet_addr(serverIP_);
    serverAddr_.sin_family = AF_INET;
    serverAddr_.sin_port = htons(serverPort_);

    int retval = SOCKET_ERROR;
    while (retval == SOCKET_ERROR)
    {
        SystemLogger::GetInstance()->Console(L"LanClient", LEVEL_DEBUG, L"Start - Try to Connect");
        retval = connect(sock, (SOCKADDR*)&serverAddr_, sizeof(serverAddr_));
    }

    isRunning_ = true;
    sock_ = sock;
    
    recvBuffer_ = LanLargePacket::Alloc();

    workerThread_ = (HANDLE)_beginthreadex(
        NULL,
        0,
        (_beginthreadex_proc_type)&WorkerThread,
        (LPVOID)this,
        0,
        (unsigned int*)&workerThreadID_);

    if (!workerThread_)
        ErrorQuit(L"MonitorThread Start Failed");

    OnConnect();
}

void server_baby::LanClient::Stop()
{
    closesocket(sock_);
    isRunning_ = false;
    WSACleanup();

    LogDisplay(L"Client Stopped...");
}

DWORD __stdcall server_baby::LanClient::WorkerThread(LPVOID arg)
{
    LanClient* client = (LanClient*)arg;
    client->Network();

    return 0;
}

void server_baby::LanClient::Network()
{
    LogDisplay(L"Client Start...");

    while (isRunning_)
    {
        int retval = SOCKET_ERROR;
        while (retval == SOCKET_ERROR)
        {
            SOCKET sock = sock_;
            retval = recv(sock, recvBuffer_->GetWritePos(), recvBuffer_->GetEmptySize(), 0);
            if (retval == SOCKET_ERROR)
            {
                int errorCode = WSAGetLastError();
                if (errorCode != 10054 && errorCode != 10053)
                {
                    SystemLogger::GetInstance()->LogText(L"LanClient", LEVEL_ERROR, L"Recv Socket ERROR!! : %d", errorCode);
                    SystemLogger::GetInstance()->Console(L"LanClient", LEVEL_ERROR, L"Recv Socket ERROR!! : %d", errorCode);
                }
                LogDisplay(L"Disconnected From Server");
                LogDisplay(L"Recv - Try to Reconnect");
                Reconnect(sock);
                LogDisplay(L"Recv - Reconnect Success");
            }

        }
       
        if (retval == 0)
        {
            LogDisplay(L"Disconnected From Server");
            LogDisplay(L"Client Stop");
            break;
        }           

        LanLargePacket* packet = recvBuffer_;
        if (!packet->MoveWritePos(retval))
            ErrorQuit(L"RECV_PACKET_FULL");

        LanPacketSet* packetQ = LanPacketSet::Alloc(clientID_, packet);
        int packetCnt = packetQ->RegisterPackets(packet);
        if (packetCnt == LAN_PACKET_ERROR)
        {
            LanPacketSet::Free(packetQ);
            LogDisplay(L"Packet Error");
            break;
        }

        LanLargePacket* newPacket = packet->CopyRemainderToNewPacket(packetCnt);
        recvBuffer_ = newPacket;

        if (packetCnt == 0)
        {
            LanPacketSet::Free(packetQ);
            continue;
        }
        
        if (!OnRecv(packetQ))
        {
            LogDisplay(L"PacketProc Failed");
            break;
        }
    }

    Stop();
}

void server_baby::LanClient::Reconnect(SOCKET oldSock)
{
    SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == INVALID_SOCKET)
        ErrorQuit(L"socket() - INVALID SOCKET");

    ZeroMemory(&serverAddr_, sizeof(serverAddr_));
    serverAddr_.sin_addr.S_un.S_addr = inet_addr(serverIP_);
    serverAddr_.sin_family = AF_INET;
    serverAddr_.sin_port = htons(serverPort_);

    int retval = SOCKET_ERROR;
    while (retval == SOCKET_ERROR && sock_ == oldSock)
    {
        SystemLogger::GetInstance()->Console(L"LanClient", LEVEL_DEBUG, L"Try to Connect");
        retval = connect(sock, (SOCKADDR*)&serverAddr_, sizeof(serverAddr_));
    }

    if (InterlockedCompareExchangePointer((volatile PVOID*)&sock_, (PVOID)sock, (PVOID)oldSock) == (PVOID)oldSock)
        OnConnect();
    else
        closesocket(sock);
}

void server_baby::LanClient::ErrorQuit(const WCHAR* msg)
{

    SystemLogger::GetInstance()->Console(L"LanClient", LEVEL_SYSTEM, msg);
    SystemLogger::GetInstance()->LogText(L"LanClient", LEVEL_SYSTEM, msg);

    CrashDump::Crash();
}

void server_baby::LanClient::ErrorQuitWithErrorCode(const WCHAR* function)
{

    int errorCode = WSAGetLastError();

    SystemLogger::GetInstance()->Console(L"LanClient",
        LEVEL_SYSTEM, L"%ws : error code - %d", function, errorCode);

    SystemLogger::GetInstance()->LogText(L"LanClient",
        LEVEL_SYSTEM, L"%ws : error code - %d", function, errorCode);

    CrashDump::Crash();
}

void server_baby::LanClient::LogDisplay(const WCHAR* msg)
{

    SystemLogger::GetInstance()->Console(L"LanClient", LEVEL_SYSTEM, msg);
    SystemLogger::GetInstance()->LogText(L"LanClient", LEVEL_SYSTEM, msg);
}

void server_baby::LanClient::LogDisplayWithErrorCode(const WCHAR* function)
{

    int errorCode = WSAGetLastError();

    SystemLogger::GetInstance()->Console(L"LanClient",
        LEVEL_SYSTEM, L"%ws : error code - %d", function, errorCode);

    SystemLogger::GetInstance()->LogText(L"LanClient",
        LEVEL_SYSTEM, L"%ws : error code - %d", function, errorCode);
}

bool server_baby::LanClient::SendPacket(LanPacket* packet)
{

    packet->AddRef();
    packet->SetHeader();

    int retval = SOCKET_ERROR;
    while (retval == SOCKET_ERROR)
    {
        SOCKET sock = sock_;
        retval = send(sock, packet->GetPacketStart(), packet->GetPacketUsedSize(), 0);
        if (retval == SOCKET_ERROR)
        {
            int errorCode = WSAGetLastError();
            if (errorCode != 10054 && errorCode != 10053)
            {
                SystemLogger::GetInstance()->LogText(L"LanClient", LEVEL_ERROR, L"Send Socket ERROR!! : %d", errorCode);
                SystemLogger::GetInstance()->Console(L"LanClient", LEVEL_ERROR, L"Send Socket ERROR!! : %d", errorCode);
            }
            LogDisplay(L"Disconnected From Server");
            LogDisplay(L"Send - Try to Reconnect");
            Reconnect(sock);
            LogDisplay(L"Send - Reconnect Success");
        }

    }

    if (retval == 0)
    {
        LogDisplay(L"0 Send");
        LogDisplay(L"Client Stop");
        CrashDump::Crash();
    }

    LanPacket::Free(packet);
    return true;
}