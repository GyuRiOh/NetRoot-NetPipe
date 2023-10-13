
#ifndef  __NET__PIPE__HANDLER__
#define  __NET__PIPE__HANDLER__

#include "NetServer.h"
#include "NetPacket.h"
#include <vector>

namespace server_baby
{
	template <typename Pipe>
	class PipeManager final
	{
	public:
		explicit PipeManager(
			const unsigned int maxPipeCount,
			const unsigned int maxPipeUser,
			NetRoot* const server, 
			const unsigned short code, 
			const unsigned int framePerSecond, 
			const bool autoManage = false) 
			: pipeCount_(NULL), server_(server), framePerSec_(framePerSecond), code_(code), maxPipeUser_(maxPipeUser*0.7),
			autoManage_(autoManage), threadHandle_(INVALID_HANDLE_VALUE), maxPipeCount_(maxPipeCount)
		{}

		~PipeManager(){}

		void Start()
		{
			if (!autoManage_)
			{
				for (unsigned int i = 0; i < maxPipeCount_; i++)
				{
					NewPipe();
				}

				return;
			}
			else
				NewPipe();

			unsigned int threadIDforWorker = NULL;
			threadHandle_ = (HANDLE)_beginthreadex(
				NULL,
				0,
				(_beginthreadex_proc_type)&WorkerThread,
				(LPVOID)this,
				0,
				&threadIDforWorker);

			if (threadHandle_ == NULL)
			{
				SystemLogger::GetInstance()->Console(L"PipeManager", LEVEL_SYSTEM, L"BeginThread failed!");
				SystemLogger::GetInstance()->LogText(L"PipeManager", LEVEL_SYSTEM, L"BeginThread failed!");

				CrashDump::Crash();
			}
		}

		void NewPipe()
		{
			Pipe* pipe = new Pipe(server_, framePerSec_);
			if(server_->isServerRunning())
				server_->StartPipe(pipe);

			server_->RegisterPipe(code_, pipe);
			InterlockedIncrement(&pipeCount_);
		}

		bool DeleteAllPipe()
		{
			server_->DeleteAllPipe(code_);
		}

		unsigned short GetCode()
		{
			return code_;
		}

		unsigned int GetCurrentPipeCount()
		{
			return pipeCount_;
		}

	private:
		void AutoCreate()
		{
			if (pipeCount_ >= maxPipeCount_)
				return;

			bool isOverloaded = false;
			auto function = [this, &isOverloaded](ULONG id, NetPipe* pipe)
			{
				if (static_cast<unsigned int>(pipe->GetUserSize()) >= maxPipeUser_)
					isOverloaded = true;
			};

			server_->ForeachForSamePipe(function, code_);

			if (isOverloaded)
				NewPipe();
		}

		bool GarbageCollect()
		{
			if (pipeCount_ <= 1)
			{
				server_->RevivePipe(code_);
				return false;
			}

			int destroyedPipeCount = server_->DestroyZeroUserPipe(code_);

			if(destroyedPipeCount)
				InterlockedAdd((volatile LONG*) & pipeCount_, -(destroyedPipeCount));
			
			//메모리 정리
			NetPacketSet::GarbageCollect();
			NetLargePacket::GarbageCollect();
			NetPacket::GarbageCollect();
			MyRedBlackTree<INT64, NetUser*>::GarbageCollectNodePool();

			return true;
		}

		static unsigned int __stdcall WorkerThread(LPVOID arg)
		{
			PipeManager* handler = reinterpret_cast<PipeManager*>(arg);
			return handler->WorkerProc();
		}

		unsigned int WorkerProc()
		{
			while (server_->isServerCreated())
			{				
				AutoCreate();
				Sleep(5000);
				GarbageCollect();
			}

			return 0;
		}


	private:
		NetRoot* server_;
		HANDLE threadHandle_;
		unsigned int pipeCount_;
		unsigned int maxPipeUser_;	
		unsigned int maxPipeCount_;
		unsigned int framePerSec_;
		unsigned short code_;
		bool autoManage_;
	};

}

#endif