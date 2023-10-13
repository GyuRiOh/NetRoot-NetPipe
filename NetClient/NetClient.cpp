
#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include "NetClient.h"
#include <process.h>

server_baby::NetClient::NetClient() : serverPort_(NULL), isRunning_(false),
	workerThreadID_(NULL), workerThread_(INVALID_HANDLE_VALUE), sock_(INVALID_SOCKET),
	recvBuffer_(nullptr)
{
    CrashDump::GetInstance();
    setlocale(LC_ALL, "");
}

server_baby::NetClient::~NetClient(){}

void server_baby::NetClient::Start(char* serverIP, unsigned short serverPort)
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

    recvBuffer_ = NetLargePacket::Alloc();

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

void server_baby::NetClient::Stop()
{
    closesocket(sock_);
    isRunning_ = false;
    WSACleanup();

    LogDisplay(L"Client Stopped...");
}

DWORD __stdcall server_baby::NetClient::WorkerThread(LPVOID arg)
{
    NetClient* client = (NetClient*)arg;
    client->Network();

    return 0;
}


bool server_baby::NetClient::SendPacket(NetPacket* packet)
{
    
    packet->AddRef();
    packet->Encode();

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
                SystemLogger::GetInstance()->LogText(L"NetClient", LEVEL_ERROR, L"Send Socket ERROR!! : %d", errorCode);
                SystemLogger::GetInstance()->Console(L"NetClient", LEVEL_ERROR, L"Send Socket ERROR!! : %d", errorCode);
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

    NetPacket::Free(packet);
    return true;
}

void server_baby::NetClient::ErrorQuit(const WCHAR* msg)
{
    SystemLogger::GetInstance()->Console(L"NetClient", LEVEL_SYSTEM, msg);
    SystemLogger::GetInstance()->LogText(L"NetClient", LEVEL_SYSTEM, msg);

    CrashDump::Crash();
}

void server_baby::NetClient::ErrorQuitWithErrorCode(const WCHAR* function)
{
    int errorCode = WSAGetLastError();

    SystemLogger::GetInstance()->Console(L"NetClient",
        LEVEL_SYSTEM, L"%ws : error code - %d", function, errorCode);

    SystemLogger::GetInstance()->LogText(L"NetClient",
        LEVEL_SYSTEM, L"%ws : error code - %d", function, errorCode);

    CrashDump::Crash();
}

void server_baby::NetClient::LogDisplay(const WCHAR* msg)
{
    SystemLogger::GetInstance()->Console(L"NetClient", LEVEL_SYSTEM, msg);
    SystemLogger::GetInstance()->LogText(L"NetClient", LEVEL_SYSTEM, msg);

}

void server_baby::NetClient::LogDisplayWithErrorCode(const WCHAR* function)
{
    int errorCode = WSAGetLastError();

    SystemLogger::GetInstance()->Console(L"NetClient",
        LEVEL_SYSTEM, L"%ws : error code - %d", function, errorCode);

    SystemLogger::GetInstance()->LogText(L"NetClient",
        LEVEL_SYSTEM, L"%ws : error code - %d", function, errorCode);

}

void server_baby::NetClient::Network()
{
    LogDisplay(L"Client Start...");

    while (isRunning_)
    {
        Sleep(0);

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
                    SystemLogger::GetInstance()->LogText(L"NetClient", LEVEL_ERROR, L"Recv Socket ERROR!! : %d", errorCode);
                    SystemLogger::GetInstance()->Console(L"NetClient", LEVEL_ERROR, L"Recv Socket ERROR!! : %d", errorCode);
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

        NetLargePacket* packet = recvBuffer_;
        if (!packet->MoveWritePos(retval))
            ErrorQuit(L"RECV_PACKET_FULL");

        NetPacketSet* packetQ = NetPacketSet::Alloc(clientID_, packet);
        int packetCnt = packetQ->RegisterPackets(packet);
        if (packetCnt == NET_PACKET_ERROR)
        {
            NetPacketSet::Free(packetQ);
            LogDisplay(L"Packet Error");
            break;
        }

        NetLargePacket* newPacket = packet->CopyRemainderToNewPacket(packetCnt);
        recvBuffer_ = newPacket;

        if (packetCnt == 0)
        {
            NetPacketSet::Free(packetQ);
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

void server_baby::NetClient::Reconnect(SOCKET oldSock)
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
        SystemLogger::GetInstance()->Console(L"NetClient", LEVEL_DEBUG, L"Try to Connect");
        retval = connect(sock, (SOCKADDR*)&serverAddr_, sizeof(serverAddr_));
    }

    if (InterlockedCompareExchangePointer((volatile PVOID*)&sock_, (PVOID)sock, (PVOID)oldSock) == (PVOID)oldSock)
        OnConnect();
    else
        closesocket(sock);
}
