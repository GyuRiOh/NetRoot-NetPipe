
#ifndef __LAN__PIPE__USER__
#define __LAN__PIPE__USER__

#include "LanPipe.h"
#include "../Common/Queue.h"
#include "LanServer.h"

namespace server_baby
{
	class LanPacket;

	class LanUser
	{
	public:
		explicit LanUser(const LanSessionID ID, LanRoot* const  server, const LanPipeID pipeID) : isMoving_(false), jobQ_(nullptr), server_(nullptr)
		{
			Initialize(ID, server, pipeID);
		}
		virtual ~LanUser() 
		{ 
			sessionID_.total_ = NULL;
			curPipe_.total_ = NULL;
			jobQ_ = nullptr;
			server_ = nullptr;
		}

		void Initialize(const LanSessionID ID, LanRoot* const server, const LanPipeID pipeID);
		void SendPacket(LanPacket* const packet);
		void SendPacket(LanSessionIDSet* const idSet, LanPacket* const packet);
		bool Disconnect();
		void DisconnectAfterLastMessage(LanPacket* const  packet);
		void DisconnectAfterLastMessage(LanSessionIDSet* const idSet, LanPacket* const packet);

		LanSessionID GetSessionID() { return sessionID_; }
		LanPipeID GetCurrentPipeID() { return curPipe_; }

		bool IsJobQEmpty() { return jobQ_->isEmpty(); }
		LanPacketSet* DequeueJob();
		void PipeMoveStart() { isMoving_ = true; }
		void PipeMoveEnd() { isMoving_ = false; }
		bool isMovingPipe() { return isMoving_; }

	private:
		void ErrorQuit(const wchar_t* const  msg);

	private:
		LanSessionID sessionID_;
		QueueWithoutCount<LanPacketSet*>* jobQ_;
		LanPipeID curPipe_;
		LanRoot* server_;
		bool isMoving_;

		friend class LanPipe;
	};	


	inline void LanUser::Initialize(const LanSessionID ID, LanRoot* const  server, const LanPipeID pipeID)
	{
		sessionID_ = ID;
		server_ = server;
		curPipe_ = pipeID;
		jobQ_ = server_->GetSessionJobQ(ID);
	}

	inline void LanUser::SendPacket(LanPacket* const  packet)
	{
		server_->AsyncSendPacket(sessionID_, packet);
	}

	inline void LanUser::SendPacket(LanSessionIDSet* const  idSet, LanPacket* const  packet)
	{
		server_->AsyncSendPacket(idSet, packet);
	}

	inline bool LanUser::Disconnect()
	{
		server_->Disconnect(sessionID_);
	}

	inline void LanUser::DisconnectAfterLastMessage(LanPacket* const packet)
	{
		server_->DisconnectAfterLastMessage(sessionID_, packet);
	}

	inline void LanUser::DisconnectAfterLastMessage(LanSessionIDSet* const idSet, LanPacket* const packet)
	{
		server_->DisconnectAfterLastMessage(idSet, packet);
	}

	inline LanPacketSet* LanUser::DequeueJob()
	{
		if (isMoving_)
			return nullptr;

		LanPacketSet* set = nullptr;
		jobQ_->Dequeue(&set);

		return set;
	}

	inline void LanUser::ErrorQuit(const wchar_t* const msg)
	{

		SystemLogger::GetInstance()->Console(L"LanUser", LEVEL_SYSTEM, msg);
		SystemLogger::GetInstance()->LogText(L"LanUser", LEVEL_SYSTEM, msg);

		CrashDump::Crash();
	}
}

#endif