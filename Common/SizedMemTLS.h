#ifndef  __SIZED_MEM_TLS__
#define  __SIZED_MEM_TLS__
#include <Windows.h>
#include <vector>
#include "Crash.h"
#include "ChunkLockFree.h"
#include "Singleton.h"

#define DEFAULT_CHUNK_SIZE 200

//정적TLS로 바꾸기
using namespace std;

namespace server_baby
{
	template<unsigned int SIZE>
	class SizedMemTLS
	{

		struct Top
		{
			SizedDataBlock<SIZE>* data = nullptr;
			size_t index = NULL;
			short capacity = 0;
		};

	public:

		explicit SizedMemTLS(unsigned short chunkSize = 200, unsigned short chunkNum = 1)
		{

			chunkSize_ = chunkSize;

			if (!chunkPool_)
				chunkPool_ = new ChunkLockFree<SizedDataBlock<SIZE>>(chunkSize, chunkNum, SIZE);

			InitializeSRWLock(&topArrayLock_);
		}


		~SizedMemTLS()
		{
			Destroy();
		}

		static char* Alloc();
		static bool Free(char* pData);
		int GetTotalCapacity();
		int	GetTotalUseCount();
		void RemoveRemainders();
		void Destroy();

	private:

		static DWORD SetTLSIndex();
		static SizedDataBlock<SIZE>* AllocateNewChunk();
		static void SetTLSTop(Top* top);
		static void ReturnDataToChunkPool();

		static void Error(const WCHAR* message);
		static void ErrorWithCode(const WCHAR* message);

		static Top* GetTLSTop_Alloc()
		{
			Top* top = (Top*)TlsGetValue(top_TLS_);
			if (top != nullptr)
			{
				if (top->capacity == 0)
				{
					top->data = AllocateNewChunk();
					top->capacity = chunkSize_;
				}
				return top;
			}

			Top* newTop = new Top();
			newTop->data = AllocateNewChunk();
			newTop->capacity = chunkSize_;

			AcquireSRWLockExclusive(&topArrayLock_);
			topArray_.push_back(move(newTop));
			newTop->index = topArray_.size() - 1;
			ReleaseSRWLockExclusive(&topArrayLock_);

			SetTLSTop(newTop);
			return newTop;
		}

		static Top* GetTLSTop_Free()
		{
			Top* top = reinterpret_cast<Top*>(TlsGetValue(top_TLS_));
			if (top != nullptr)
				return top;

			Top* newTop = new Top();

			AcquireSRWLockExclusive(&topArrayLock_);
			topArray_.push_back(move(newTop));
			newTop->index = topArray_.size() - 1;
			ReleaseSRWLockExclusive(&topArrayLock_);

			SetTLSTop(newTop);

			return newTop;
		}

	private:
		static vector<LPVOID> topArray_;
		static RTL_SRWLOCK topArrayLock_;
		static DWORD top_TLS_;
		static unsigned short chunkSize_;
		static ChunkLockFree<SizedDataBlock<SIZE>>* chunkPool_;
	};

	template<unsigned int SIZE>
	ChunkLockFree<SizedDataBlock<SIZE>>* SizedMemTLS<SIZE>::chunkPool_ = nullptr;

	template<unsigned int SIZE>
	vector<LPVOID> SizedMemTLS<SIZE>::topArray_;

	template<unsigned int SIZE>
	DWORD SizedMemTLS<SIZE>::top_TLS_ = SizedMemTLS::SetTLSIndex();

	template<unsigned int SIZE>
	unsigned short SizedMemTLS<SIZE>::chunkSize_ = NULL;

	template<unsigned int SIZE>
	RTL_SRWLOCK SizedMemTLS<SIZE>::topArrayLock_;

	template<unsigned int SIZE>
	inline char* SizedMemTLS<SIZE>::Alloc(void)
	{
		Top* top = GetTLSTop_Alloc();

		SizedDataBlock<SIZE>* retData = top->data;
		top->data = retData->next;
		top->capacity--;

		retData->underguard = UNDERGUARD;

		return retData->data;
	}

	template<unsigned int SIZE>
	bool SizedMemTLS<SIZE>::Free(char* pData)
	{
		SizedDataBlock<SIZE>* block = nullptr;
		block = (SizedDataBlock<SIZE>*)((char*)pData - 16);

		//오버, 언더플로우 여부 체크	
		if (block->overguard != OVERGUARD || block->underguard != UNDERGUARD)
			Error(L"Free : Overflow or Underflow");

		Top* top = GetTLSTop_Free();
		block->next = top->data;
		top->data = block;
		top->capacity++;

		block->underguard = UNDERGUARD_AFTER_FREE;

		if (top->capacity >= (chunkSize_ * 2))
			ReturnDataToChunkPool();

		return true;

	}

	template<unsigned int SIZE>
	inline int SizedMemTLS<SIZE>::GetTotalCapacity(void)
	{
		if (!chunkPool_)
			return 0;
		else
			return chunkPool_->GetBlockCapacity();
	}

	template<unsigned int SIZE>
	inline int SizedMemTLS<SIZE>::GetTotalUseCount(void)
	{
		if (!chunkPool_)
			return 0;
		else
			return chunkPool_->GetBlockUsed();
	}

	template<unsigned int SIZE>
	inline void SizedMemTLS<SIZE>::RemoveRemainders()
	{
		SizedMemTLS<SIZE>::Top* top = reinterpret_cast<Top*>(TlsGetValue(top_TLS_));
		if (!top)
			return;

		if (top->capacity)
		{
			SizedDataBlock<SIZE>* head = top->data;
			SizedDataBlock<SIZE>* tail = head;

			for (int i = 0; i < (top->capacity - 1); i++)
			{
				tail = tail->next;
			}

			chunkPool_->FreeFromTLSDestruction(head, tail, top->capacity);
		}

		AcquireSRWLockExclusive(&topArrayLock_);
		topArray_[top->index] = nullptr;
		ReleaseSRWLockExclusive(&topArrayLock_);

		delete top;
		SetTLSTop(nullptr);
	}

	template<unsigned int SIZE>
	inline void SizedMemTLS<SIZE>::Destroy()
	{
		AcquireSRWLockExclusive(&topArrayLock_);
		for (int i = 0; i < topArray_.size(); i++)
		{
			if (topArray_[i])
			{
				SizedMemTLS<SIZE>::Top* top = reinterpret_cast<SizedMemTLS<SIZE>::Top*>(topArray_[i]);
				SizedDataBlock<SIZE>* head = top->data;
				SizedDataBlock<SIZE>* tail = head;

				for (int i = 0; i < (top->capacity - 1); i++)
				{
					tail = tail->next;
				}

				chunkPool_->FreeFromTLSDestruction(head, tail, top->capacity);

				delete topArray_[i];
				topArray_[i] = nullptr;
			}
		}
		ReleaseSRWLockExclusive(&topArrayLock_);

		delete chunkPool_;
		TlsFree(top_TLS_);
	}

	template<unsigned int SIZE>
	inline void SizedMemTLS<SIZE>::Error(const WCHAR* message)
	{
		SystemLogger::GetInstance()->LogText(L"SizedMemTLS", LEVEL_SYSTEM, message);
		CrashDump::Crash();
	}

	template<unsigned int SIZE>
	inline void SizedMemTLS<SIZE>::ErrorWithCode(const WCHAR* message)
	{
		SystemLogger::GetInstance()->LogText(L"SizedMemTLS",
			LEVEL_SYSTEM, L"%ws : error code - %d", message, GetLastError());

		CrashDump::Crash();
	}

	template<unsigned int SIZE>
	inline SizedDataBlock<SIZE>* SizedMemTLS<SIZE>::AllocateNewChunk()
	{
		return chunkPool_->Alloc();
	}

	template<unsigned int SIZE>
	inline bool isEmpty(SizedDataBlock<SIZE>* top)
	{
		return (top == nullptr);
	}

	template<unsigned int SIZE>
	inline DWORD SizedMemTLS<SIZE>::SetTLSIndex()
	{

		DWORD tempIndex = TlsAlloc();
		if (tempIndex == TLS_OUT_OF_INDEXES)
		{
			//TLS Alloc 함수가 비트 플래그 배열로부터 프리 상태인 플래그를 찾지 못했다.
			Error(L"TLSAlloc Failed - OUT OF INDEX");
		}

		return tempIndex;
	}

	template<unsigned int SIZE>
	inline void SizedMemTLS<SIZE>::SetTLSTop(SizedMemTLS<SIZE>::Top* top)
	{
		BOOL retval = TlsSetValue(top_TLS_, top);

		if (retval == false)
			ErrorWithCode(L"TlsSetValue");
	}

	template<unsigned int SIZE>
	void SizedMemTLS<SIZE>::ReturnDataToChunkPool()
	{
		SizedMemTLS<SIZE>::Top* top = GetTLSTop_Alloc();
		SizedDataBlock<SIZE>* head = top->data;
		SizedDataBlock<SIZE>* tail = head;

		for (int i = 0; i < (chunkSize_ - 1); i++)
		{
			tail = tail->next;
		}

		top->data = tail->next;
		tail->next = nullptr;

		chunkPool_->Free(head, tail);
		top->capacity -= chunkSize_;
	}


}
#endif