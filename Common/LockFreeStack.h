
#ifndef  __LOCK_FREE_STACK__
#define  __LOCK_FREE_STACK__
#include "MemTLS.h"

using namespace std;

namespace server_baby
{

	template<typename DATA>
	struct NodeForLFS
	{
		DATA data = 800000;
		NodeForLFS* next = nullptr;

		NodeForLFS()
		{
			data = 800000;
			next = nullptr;
		}

		NodeForLFS(DATA data)
		{
			data = data;
			next = nullptr;
		}
	};

	template<typename DATA>
	class LockFreeStack
	{
		enum MASK : unsigned long long
		{
			STAMP_MASK = 0xffff800000000000,
			ADDRESS_MASK = ~(0xffff800000000000),
			INCREMENTING_STAMP = ((unsigned long long)1 << 48)
		};

	public:
		explicit LockFreeStack()
		{
			//==========================================================			
			SYSTEM_INFO system_info;
			GetSystemInfo(&system_info);

			unsigned long long max_memory_address = (unsigned long long)system_info.lpMaximumApplicationAddress;

			//현재 메모리풀 구조로 사용한 주소 상위 비트 개수는 17개.
			//여기서 CRASH가 났다는 건, GetSystemInfo로 얻어진 상위 비트 개수가 변동되었다는 의미이다.
			if (ADDRESS_MASK != GetMsb64(max_memory_address))
				Error(L"Address Mask Need To Be Changed");
			//==========================================================
			nodePool_ = new MemTLS<NodeForLFS<DATA>>(200, 1, NULL);
			size_ = 0;
		}

		~LockFreeStack(){}

		void Push(DATA data);
		bool Pop(DATA* data);
		long long Size();
		static long long GetUsedCount();
		static long long GetCapacityCount();
		static void DeleteTLS();
		static void DeletePool();

	private:
		void Error(const WCHAR* message);
		bool isEmpty(NodeForLFS<DATA>* masked_top);
		NodeForLFS<DATA>* MaskAddress(NodeForLFS<DATA>* address);
		unsigned long long MaskAndIncrementStamp(NodeForLFS<DATA>* address);
		unsigned long long GetMsb64(unsigned long long n);

	private:

		NodeForLFS<DATA>* top_ = nullptr;
		char padding[56] = { 0 };
		long size_ = 0;

		static MemTLS<NodeForLFS<DATA>>* nodePool_;
	};

	template<typename DATA>
	MemTLS<NodeForLFS<DATA>>* LockFreeStack<DATA>::nodePool_ = nullptr;

	template<typename DATA>
	void LockFreeStack<DATA>::Push(DATA data)
	{
		NodeForLFS<DATA>* new_node = nodePool_->Alloc();
		new_node->data = data;

		NodeForLFS<DATA>* local_top = nullptr;
		NodeForLFS<DATA>* stamp_added_new_node = nullptr;
		unsigned long long masked_stamp = 0;

		do {

			local_top = top_;
			new_node->next = local_top;
			masked_stamp = MaskAndIncrementStamp(local_top);

			stamp_added_new_node = (NodeForLFS<DATA>*)((unsigned long long)new_node +
				(unsigned long long)masked_stamp);

		} while (local_top !=
			InterlockedCompareExchangePointer
			((PVOID*)&top_, stamp_added_new_node, local_top));

		InterlockedIncrement(&size_);


	};

	template<typename DATA>
	bool LockFreeStack<DATA>::Pop(DATA* data)
	{
		NodeForLFS<DATA>* local_top = nullptr;
		NodeForLFS<DATA>* masked_local_top = nullptr;
		NodeForLFS<DATA>* new_top = nullptr;
		unsigned long long masked_stamp = 0;

		InterlockedDecrement(&size_);

		do {

			local_top = top_;
			masked_stamp = MaskAndIncrementStamp(local_top);
			masked_local_top = MaskAddress(local_top);

			if (isEmpty(masked_local_top))
			{
				InterlockedIncrement(&size_);
				return false;

			}
			NodeForLFS<DATA>* local_next = masked_local_top->next;
			NodeForLFS<DATA>* masked_local_next = MaskAddress(local_next);

			new_top = (NodeForLFS<DATA>*)((unsigned long long)masked_local_next
				+ (unsigned long long)masked_stamp);

		} while (local_top !=
			InterlockedCompareExchangePointer
			((PVOID*)&top_, new_top, local_top));

		*data = masked_local_top->data;
		nodePool_->Free(masked_local_top);

		return true;

	};

	template<typename DATA>
	long long LockFreeStack<DATA>::Size()
	{
		return size_;
	}

	template<typename DATA>
	long long LockFreeStack<DATA>::GetUsedCount()
	{
		return nodePool_->GetTotalUseCount();
	}

	template<typename DATA>
	long long LockFreeStack<DATA>::GetCapacityCount()
	{
		return nodePool_->GetTotalCapacity();
	}

	template<typename DATA>
	inline void LockFreeStack<DATA>::DeleteTLS()
	{
		nodePool_->RemoveRemainders();
	}

	template<typename DATA>
	inline void LockFreeStack<DATA>::DeletePool()
	{
		delete nodePool_;
	}

	template<typename DATA>
	void LockFreeStack<DATA>::Error(const WCHAR* message)
	{
		SystemLogger::GetInstance()->LogText(L"LockFreeStack", LEVEL_SYSTEM, message);
		CrashDump::Crash();
	}

	template<typename DATA>
	bool LockFreeStack<DATA>::isEmpty(NodeForLFS<DATA>* masked_top)
	{
		if (masked_top == nullptr)
			return true;
		else
			return false;
	}

	template<typename DATA>
	NodeForLFS<DATA>* LockFreeStack<DATA>::MaskAddress(NodeForLFS<DATA>* address)
	{
		NodeForLFS<DATA>* local_addr = (NodeForLFS<DATA>*)((unsigned long long)address & ADDRESS_MASK);
		return local_addr;
	}

	template<typename DATA>
	unsigned long long LockFreeStack<DATA>::MaskAndIncrementStamp(NodeForLFS<DATA>* address)
	{
		unsigned long long local_stamp = ((unsigned long long)address & STAMP_MASK) + INCREMENTING_STAMP;
		return local_stamp;
	}

	template<typename DATA>
	unsigned long long LockFreeStack<DATA>::GetMsb64(unsigned long long n)
	{
		n |= n >> 1;
		n |= n >> 2;
		n |= n >> 4;
		n |= n >> 8;
		n |= n >> 16;
		n |= n >> 32;

		return n;
	}

}
#endif