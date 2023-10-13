
#ifndef __NET__PIPE__USER__
#define __NET__PIPE__USER__

#include <Windows.h>
#include <WS2tcpip.h>
#pragma comment(lib, "ws2_32")
#include "NetPipe.h"
#include "../Common/Queue.h"
#include "NetServer.h"

namespace server_baby
{
	class NetPacket;

	class NetUser
	{
	public:
		explicit NetUser(const NetSessionID ID, NetRoot* const server, const NetPipeID pipeID) 
			: isMoving_(false), jobQ_(nullptr), server_(nullptr), port_(0)
		{
			Initialize(ID, server, pipeID);
		}
		virtual ~NetUser() 
		{ 
			sessionID_.total_ = NULL;
			curPipe_.total_ = NULL;
			jobQ_ = nullptr;
			server_ = nullptr;
		}

		void SendPacket(NetPacket* const packet);
		void SendPacket(NetSessionIDSet* const idSet, NetPacket* const packet);
		bool Disconnect();
		void DisconnectAfterLastMessage(NetPacket* const packet);
		void DisconnectAfterLastMessage(NetSessionIDSet* const idSet, NetPacket* const packet);

		NetSessionID GetSessionID() { return sessionID_; }
		NetPipeID GetCurrentPipeID() { return curPipe_; }
		void GetIP(char* buf) const { memmove(buf, IP_, INET_ADDRSTRLEN); }
		USHORT GetPort() const { return port_; }
		

		bool IsJobQEmpty() { return jobQ_->isEmpty(); }
		NetPacketSet* DequeueJob();
		void PipeMoveStart() { isMoving_ = true; }
		void PipeMoveEnd() { isMoving_ = false; }
		bool isMovingPipe() { return isMoving_; }

	private:
		void Initialize(const NetSessionID ID, NetRoot* const server, const NetPipeID pipeID);
		void ErrorQuit(const wchar_t* const msg);

	private:
		NetSessionID sessionID_;
		QueueWithoutCount<NetPacketSet*>* jobQ_;
		NetPipeID curPipe_;
		NetRoot* server_;
		bool isMoving_;
		char IP_[INET_ADDRSTRLEN] = { 0 };
		USHORT port_;

		friend class NetPipe;
	};	


	inline void NetUser::Initialize(const NetSessionID ID, NetRoot* const server, const NetPipeID pipeID)
	{
		sessionID_ = ID;
		server_ = server;
		curPipe_ = pipeID;
		jobQ_ = server_->GetPipeUserJobQ(ID);
		SOCKADDR_IN clientAddr = server_->GetPipeUserAddress(ID);

		PCSTR ret = inet_ntop(AF_INET, &clientAddr, IP_, INET_ADDRSTRLEN);

		if (!ret)
		{
			SystemLogger::GetInstance()->LogText(L"NetUser", LEVEL_ERROR, L"inet_ntop Failed");
			CrashDump::Crash();
		}

		port_ = ntohs(clientAddr.sin_port);
	}

	inline void NetUser::SendPacket(NetPacket* const packet)
	{
		server_->AsyncSendPacket(sessionID_, packet);
	}

	inline void NetUser::SendPacket(NetSessionIDSet* const idSet, NetPacket* const packet)
	{
		server_->AsyncSendPacket(idSet, packet);
	}

	inline bool NetUser::Disconnect()
	{
		server_->Disconnect(sessionID_);
	}

	inline void NetUser::DisconnectAfterLastMessage(NetPacket* const packet)
	{
		server_->DisconnectAfterLastMessage(sessionID_, packet);
	}

	inline void NetUser::DisconnectAfterLastMessage(NetSessionIDSet* const idSet, NetPacket* const packet)
	{
		server_->DisconnectAfterLastMessage(idSet, packet);
	}

	inline NetPacketSet* NetUser::DequeueJob()
	{
		if (isMoving_)
			return nullptr;

		NetPacketSet* set = nullptr;
		jobQ_->Dequeue(&set);

		return set;
	}

	inline void NetUser::ErrorQuit(const wchar_t* const msg)
	{

		SystemLogger::GetInstance()->Console(L"NetUser", LEVEL_SYSTEM, msg);
		SystemLogger::GetInstance()->LogText(L"NetUser", LEVEL_SYSTEM, msg);

		CrashDump::Crash();
	}
}

#endif