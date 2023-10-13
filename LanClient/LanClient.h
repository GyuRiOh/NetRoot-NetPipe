
#ifndef  __LAN__CLIENT__
#define  __LAN__CLIENT__

#include "../LanServer/LanPacketSet.h"
#include "../LanServer/LanSession.h"
#include "../LanServer/LanPacket.h"

namespace server_baby
{
	class LanClient
	{
	public:
		explicit LanClient();
		virtual ~LanClient();

		void Start(
			char* serverIP,
			unsigned short serverPort);

		void Stop();
		bool SendPacket(LanPacket* packet);

	protected:
		virtual bool OnRecv(LanPacketSet* packetList) = 0; //패킷 수신 완료 후	
		virtual void OnConnect() = 0;

		void ErrorQuit(const WCHAR* msg);
		void ErrorQuitWithErrorCode(const WCHAR* function);
		void LogDisplay(const WCHAR* msg);
		void LogDisplayWithErrorCode(const WCHAR* function);

	private:
		void Network();
		void Reconnect(SOCKET oldSock);
		static DWORD WINAPI WorkerThread(LPVOID arg);

	private:
		CHAR serverIP_[16] = { 0 };
		USHORT serverPort_;

		SOCKADDR_IN serverAddr_;

		HANDLE workerThread_;
		SOCKET sock_;
		unsigned int workerThreadID_;
		bool isRunning_;

		LanLargePacket* recvBuffer_;
		LanSessionID clientID_;

	};



}

#endif