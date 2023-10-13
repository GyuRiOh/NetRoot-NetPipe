
#ifndef  __JOB__Q__
#define  __JOB__Q__
#include "LockFreeEnqQueue.h"
#include "LockFreeQueue.h"
#include "QueueWithLock.h"

namespace server_baby
{
	template <typename DATA, unsigned short CODE>
	class LockFreeJobQ
	{
		enum Settings
		{
			JOBQ_MAX_SIZE = 20000
		};
	public:
		explicit LockFreeJobQ() {}
		~LockFreeJobQ() {}

		bool Enqueue(DATA data)
		{
			bool ret = jobQ_.Enqueue(data);
			if (!ret)
				SystemLogger::GetInstance()->LogText(L"JobQ", LEVEL_DEBUG, L"Job Q Enq Failed - FULL");
			return ret;
		}

		static void DeleteTLS()
		{
			LockFreeQueue<DATA, CODE, JOBQ_MAX_SIZE>::DeleteTLS();
		}

		bool Dequeue(DATA* data)
		{
			return jobQ_.Dequeue(data);
		}

		bool isEmpty()
		{
			return (jobQ_.Size() == 0);
		}

		bool isFull()
		{
			return jobQ_.isFull();
		}

		long GetSize()
		{
			return jobQ_.Size();
		}

	private:
		LockFreeQueue<DATA, CODE, JOBQ_MAX_SIZE> jobQ_;
	};

	template <typename DATA, unsigned short CODE>
	class LockFreeEnqJobQ
	{
		enum Settings
		{
			JOBQ_MAX_SIZE = 20000
		};
	public:
		explicit LockFreeEnqJobQ() {}
		~LockFreeEnqJobQ() {}

		bool Enqueue(DATA data)
		{
			bool ret = jobQ_.Enqueue(data);
			if (!ret)
				SystemLogger::GetInstance()->LogText(L"JobQ", LEVEL_DEBUG, L"Job Q Enq Failed - FULL");
			return ret;
		}

		static void DeleteTLS()
		{
			LockFreeEnqQueue<DATA, CODE, JOBQ_MAX_SIZE>::DeleteTLS();
		}

		bool Dequeue(DATA* data)
		{
			return jobQ_.Dequeue(data);
		}

		bool isEmpty()
		{
			return (jobQ_.Size() == 0);
		}

		bool isFull()
		{
			return jobQ_.isFull();
		}

		long GetSize()
		{
			return jobQ_.Size();
		}

	private:
		LockFreeEnqQueue<DATA, CODE, JOBQ_MAX_SIZE> jobQ_;
	};
	

}
#endif