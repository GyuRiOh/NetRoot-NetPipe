
#ifndef  __LAN__PIPE__
#define  __LAN__PIPE__
#include "../Common/JobQueue.h"
#include "../Common/MyMultimap.h"
#include "LanServer.h"
#include "../Common/MyRedBlackTree.h"

namespace server_baby
{
	union LanSessionID;
	
	class LanRoot;
	class LanUser;

	union LanPipeID
	{
		struct Element_
		{
			unsigned int code_;
			unsigned int index_;
		} element_;
		unsigned long long total_;

		LanPipeID() : total_(0) {}
	};

	struct LanLoopInfo
	{
		unsigned int  syncCountPerSecond_ = NULL;
		unsigned int  frameCountPerSecond_ = NULL;

		struct tm startT_ = { 0 };
		time_t startTimer_ = NULL;

	};

	struct LanMoveRequest
	{
		LanSessionID ID;
		LanPipeID dest;
	};

	class LanPipe
	{
	public:
		explicit LanPipe(LanRoot* const server, const unsigned int framePerSecond);
		virtual ~LanPipe(){}

		void Initialize(const LanPipeID ID);
		LanPipeID GetPipeID() { return pipeID_; }
		LanRoot* GetRoot() { return server_; }

		void Start(const bool* const flag);

		void RequestEnter(const LanSessionID sessionID);
		void RequestEnter(LanUser* const user);
		void RequestLeave(const LanSessionID sessionID);
		void RequestMoveOut(LanUser* const user);
	
		void SendPacket(const LanSessionID sessionID, LanPacket* const packet);
		void SendPacket(LanSessionIDSet* const idSet, LanPacket* const packet);
		bool Disconnect(const LanSessionID sessionID);
		void DisconnectAfterLastMessage(const LanSessionID sessionID, LanPacket* const packet);
		void DisconnectAfterLastMessage(LanSessionIDSet* const idSet, LanPacket* const packet);

		void MovePipe(const LanSessionID sessionID, const unsigned int pipeCode);
		void MovePipe(LanUser* const user, const unsigned int pipeCode);

		LanUser* FindUser(const LanSessionID sessionID);
		int GetUserSize();

	protected:
		virtual void OnStart() = 0;
		virtual void OnStop() = 0;
		virtual bool OnMessage(const LanSessionID sessionId, LanDummyPacket* const packet) = 0;
		virtual void OnUpdate() = 0;
		virtual void OnUserUpdate(LanUser* const user) = 0;
		virtual LanUser* OnUserJoin(const LanSessionID sessionID) = 0;
		virtual void OnUserMoveIn(LanUser* const user) = 0;
		virtual void OnUserMoveOut(const LanSessionID sessionID) = 0;
		virtual void OnUserMoveOut(LanUser* const user) = 0;
		virtual void OnUserLeave(LanUser* const user) = 0;
		virtual void OnMonitor(const LanLoopInfo* const info) = 0;
	
	private:
		void SetRunningFlag(const bool* const flag);
		static unsigned int __stdcall WorkerThread(LPVOID arg);
		unsigned int  MyWorkerProc();

		void Join();
		void Move();
		void Leave();
		void MessageProc();
		void UpdateProc();
		void DequeueJob(LanUser* user);
		bool UnpackMsg(LanPacketSet* msg);

	protected: 
		unsigned int framePerSecond_;

	private:
		LockFreeEnqJobQ<LanSessionID, 10000> joinQ_;
		LockFreeEnqJobQ<LanSessionID, 10000> leaveQ_;
		LockFreeEnqJobQ<LanUser*, 10001> moveInQ_;
		LockFreeEnqJobQ<LanUser*, 10001> moveOutQ_;

		//Key : Session Unique ID
		//value : User
		MyRedBlackTree<INT64, LanUser*> userMap_;

		HANDLE threadHandle_;
		const bool* runningFlag_;
		LanPipeID pipeID_;
		LanRoot* server_;
	};

	inline void server_baby::LanPipe::RequestEnter(const LanSessionID sessionID)
	{
		joinQ_.Enqueue(sessionID);
	}

	inline void server_baby::LanPipe::RequestEnter(LanUser* const user)
	{
		moveInQ_.Enqueue(user);
	}

	inline void server_baby::LanPipe::RequestLeave(const LanSessionID sessionID)
	{
		leaveQ_.Enqueue(sessionID);
	}

	inline void LanPipe::RequestMoveOut(LanUser* const user)
	{
		moveOutQ_.Enqueue(user);
	}

	inline void LanPipe::SendPacket(const LanSessionID sessionID, LanPacket* const packet)
	{
		server_->AsyncSendPacket(sessionID, packet);
	}

	inline void LanPipe::SendPacket(LanSessionIDSet* const idSet, LanPacket* const packet)
	{
		server_->AsyncSendPacket(idSet, packet);
	}

	inline bool LanPipe::Disconnect(const LanSessionID sessionID)
	{
		return server_->Disconnect(sessionID);
	}

	inline void LanPipe::DisconnectAfterLastMessage(const LanSessionID sessionID, LanPacket* const packet)
	{
		server_->DisconnectAfterLastMessage(sessionID, packet);
	}

	inline void LanPipe::DisconnectAfterLastMessage(LanSessionIDSet* const idSet, LanPacket* const packet)
	{
		server_->DisconnectAfterLastMessage(idSet, packet);
	}

	inline void LanPipe::MovePipe(const LanSessionID sessionID, const unsigned int pipeCode)
	{
		server_->MovePipe(sessionID, pipeCode);
	}

	inline void LanPipe::MovePipe(LanUser* const user, const unsigned int pipeCode)
	{
		server_->MovePipe(user, pipeCode);
	}

	inline LanUser* LanPipe::FindUser(const LanSessionID sessionID)
	{
		return userMap_.Find(sessionID.total_);
	}

	inline int LanPipe::GetUserSize()
	{
		return userMap_.Size();
	}

}

#endif
