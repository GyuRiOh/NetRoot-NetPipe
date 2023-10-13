
#ifndef  __NET__PIPE__
#define  __NET__PIPE__
#include "../Common/JobQueue.h"
#include "../Common/MyMultimap.h"
#include "NetServer.h"
#include "../Common/MyRedBlackTree.h"
#include "NetStub.h"

namespace server_baby
{
	union NetSessionID;
	
	class NetRoot;
	class NetUser;

	union NetPipeID
	{
		struct Element_
		{
			unsigned int code_;
			unsigned int index_;
		} element_;
		unsigned long long total_;

		NetPipeID() : total_(0) {}
	};

	struct LoopInfo
	{
		unsigned int  syncCountPerSecond_ = NULL;
		unsigned int  frameCountPerSecond_ = NULL;

		struct tm startT_ = { 0 };
		time_t startTimer_ = NULL;

	};

	struct MoveRequest
	{
		NetSessionID ID;
		NetPipeID dest;
	};

	class NetPipe
	{
	public:
		explicit NetPipe(NetRoot* const server, const unsigned int framePerSecond);
		virtual ~NetPipe() 
		{ 
			if (stub_)
				delete stub_; 
		}

		void Initialize(const NetPipeID ID);
		NetPipeID GetPipeID() { return pipeID_; }
		NetRoot* GetRoot() { return server_; }

		void Start(const bool* const flag);
		void Stop();

		void RequestEnter(const NetSessionID sessionID);
		void RequestEnter(NetUser* const user);
		void RequestLeave(const NetSessionID sessionID);
		void RequestMoveOut(NetUser* const user);
	
		void AsyncSendPacket(const NetSessionID sessionID, NetPacket* const packet);
		void AsyncSendPacket(NetSessionIDSet* const idSet, NetPacket* const packet);
		bool Disconnect(const NetSessionID sessionID);
		void DisconnectAfterLastMessage(const NetSessionID sessionID, NetPacket* const packet);
		void DisconnectAfterLastMessage(NetSessionIDSet* const idSet, NetPacket* const packet);

		void MovePipe(const NetSessionID sessionID, const unsigned int pipeCode);
		void MovePipe(NetUser* const user, const unsigned int pipeCode);

		NetUser* FindUser(const NetSessionID sessionID);
		int GetUserSize();
		void AddZeroUserCount() { zeroUserCount_++; }
		int GetZeroUserCount() { return zeroUserCount_; }
		void InitZeroUserCount() { zeroUserCount_ = NULL; }

		bool RegisterPipeStub(NetStub* const stub);

	protected:
		virtual void OnStart() = 0;
		virtual void OnStop() = 0;
		virtual bool OnMessage(const NetSessionID sessionId, NetDummyPacket* const packet) { return false; };
		virtual void OnUpdate() = 0;
		virtual void OnUserUpdate(NetUser* const user) = 0;
		virtual NetUser* OnUserJoin(const NetSessionID sessionID) = 0;
		virtual void OnUserMoveIn(NetUser* const user) = 0;
		virtual void OnUserMoveOut(NetUser* const user) = 0;
		virtual void OnUserLeave(NetUser* const user) = 0;
		virtual void OnMonitor(const LoopInfo* const info) = 0;
	
	private:
		static void __stdcall WorkerThread(PTP_CALLBACK_INSTANCE instance, PVOID arg);
		void SetRunningFlag(const bool* const flag);
		unsigned int  MyWorkerProc();

		void Join();
		void Move();
		void Leave();
		void MessageProc();
		void UpdateProc();
		void DequeueJob(NetUser* const user);
		bool UnpackMsg(NetPacketSet* const msg);

		void ErrorQuit(const wchar_t* const msg);

	protected: 
		unsigned int framePerSecond_;

	private:
		LockFreeEnqJobQ<NetSessionID, 10000> joinQ_;
		LockFreeEnqJobQ<NetSessionID, 10000> leaveQ_;
		LockFreeEnqJobQ<NetUser*, 10001> moveInQ_;
		LockFreeEnqJobQ<NetUser*, 10001> moveOutQ_;

		//Key : Session Unique ID
		//value : User
		MyRedBlackTree<INT64, server_baby::NetUser*> userMap_;

		const bool* runningFlag_;
		NetPipeID pipeID_;
		NetRoot* server_;
		unsigned short zeroUserCount_;
		bool stopFlag_;

		NetStub* stub_;
	};

	inline void server_baby::NetPipe::RequestEnter(const NetSessionID sessionID)
	{
		joinQ_.Enqueue(sessionID);
	}

	inline void server_baby::NetPipe::RequestEnter(NetUser* const user)
	{
		moveInQ_.Enqueue(user);
	}

	inline void server_baby::NetPipe::RequestLeave(const NetSessionID sessionID)
	{
		leaveQ_.Enqueue(sessionID);
	}

	inline void NetPipe::RequestMoveOut(NetUser* const user)
	{
		moveOutQ_.Enqueue(user);
	}

	inline void NetPipe::AsyncSendPacket(const NetSessionID sessionID, NetPacket* const packet)
	{
		server_->AsyncSendPacket(sessionID, packet);
	}

	inline void NetPipe::AsyncSendPacket(NetSessionIDSet* const idSet, NetPacket* const packet)
	{
		server_->AsyncSendPacket(idSet, packet);
	}

	inline bool NetPipe::Disconnect(const NetSessionID sessionID)
	{
		return server_->Disconnect(sessionID);
	}

	inline void NetPipe::DisconnectAfterLastMessage(const NetSessionID sessionID, NetPacket* const packet)
	{
		server_->DisconnectAfterLastMessage(sessionID, packet);
	}

	inline void NetPipe::DisconnectAfterLastMessage(NetSessionIDSet* const idSet, NetPacket* const packet)
	{
		server_->DisconnectAfterLastMessage(idSet, packet);
	}

	inline void NetPipe::MovePipe(const NetSessionID sessionID, const unsigned int pipeCode)
	{
		server_->MovePipe(sessionID, pipeCode);
	}

	inline void NetPipe::MovePipe(NetUser* const user, const unsigned int pipeCode)
	{
		server_->MovePipe(user, pipeCode);
	}

	inline NetUser* NetPipe::FindUser(const NetSessionID sessionID)
	{
		return userMap_.Find(sessionID.total_);
	}

	inline int NetPipe::GetUserSize()
	{
		return userMap_.Size();
	}

	inline bool NetPipe::RegisterPipeStub(NetStub* const stub)
	{
		if (!stub_)
		{
			stub_ = stub;
			return true;
		}
		else
		{
			ErrorQuit(L"Register Stub : Stub Already Exists");
			return false;
		}
	}

	inline void NetPipe::ErrorQuit(const wchar_t* const msg)
	{

		SystemLogger::GetInstance()->Console(L"NetPipe", LEVEL_SYSTEM, msg);
		SystemLogger::GetInstance()->LogText(L"NetPipe", LEVEL_SYSTEM, msg);

		CrashDump::Crash();
	}
}

#endif
