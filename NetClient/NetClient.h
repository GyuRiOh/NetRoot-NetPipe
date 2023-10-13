
#ifndef  __NET__CLIENT__
#define  __NET__CLIENT__

#include "../NetServer/NetPacketSet.h"
#include "../NetServer/NetSession.h"
#include "../NetServer/NetPacket.h"

namespace server_baby
{
	class NetClient
	{
	public:
		explicit NetClient();
		virtual ~NetClient();

		void Start(
			char* serverIP,
			unsigned short serverPort);

		void Stop();
		bool SendPacket(NetPacket* packet);

	protected:
		virtual bool OnRecv(NetPacketSet* packetList) = 0; //패킷 수신 완료 후	
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

		NetLargePacket* recvBuffer_;
		NetSessionID clientID_;
	};

}

#endif