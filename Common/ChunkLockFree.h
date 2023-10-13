#pragma once

#ifndef  __CHUNK_LOCK_FREE__
#define  __CHUNK_LOCK_FREE__
#include "Crash.h"
#include "SystemLogger.h"

constexpr unsigned long long OVERGUARD = 0xcdcdcdcdcdcdcdcd;
constexpr unsigned long long UNDERGUARD = 0xfdfdfdfdfdfdfdfd;
constexpr unsigned long long UNDERGUARD_AFTER_FREE = 0xfffffffffffffff;

namespace server_baby
{
	template<typename DATA>
	struct DataBlock
	{
		unsigned long long code; 
		unsigned long long overguard;
		DATA data; 
		unsigned long long underguard; 
		DataBlock<DATA>* next = nullptr; 
		unsigned long long* pCode;	

		DataBlock(unsigned long long codeParam)
			: code(NULL),
			overguard(OVERGUARD),
			underguard(UNDERGUARD_AFTER_FREE),
			next(nullptr),
			pCode(&code)
		{
			code = codeParam;
		}

		~DataBlock() {};

		void Initialize(unsigned long long codeParam)
		{
			code = codeParam;
			overguard = OVERGUARD;
			underguard = UNDERGUARD_AFTER_FREE;
			next = nullptr;
			pCode = &code;
		}
	};


	template<unsigned long long SIZE>
	struct SizedDataBlock
	{
		unsigned long long code;
		unsigned long long overguard; //8
		char data[SIZE] = { 0 };
		unsigned long long underguard; //8
		SizedDataBlock<SIZE>* next = nullptr; //8

		SizedDataBlock(unsigned long long sizeParam)
			: code(NULL),
			overguard(OVERGUARD),
			underguard(UNDERGUARD_AFTER_FREE),
			next(nullptr)
		{
			code = sizeParam;
		}

		~SizedDataBlock() = default;

		void Initialize(unsigned long long sizeParam)
		{
			code = sizeParam;
			overguard = OVERGUARD;
			underguard = UNDERGUARD_AFTER_FREE;
			next = nullptr;
		}

	};



	template<typename DATA>
	class ChunkLockFree
	{
		struct Chunk
		{
			DATA* head = nullptr;
			DATA* tail = nullptr;
			Chunk* nextChunk = nullptr;

			//init함수 빼보기??

			void Clear()
			{
				head = nullptr;
				tail = nullptr;
				nextChunk = nullptr;
			}

		};

		struct TempChunk
		{
			Chunk chunk;
			DWORD count = NULL;
		};

		enum MASK : unsigned long long
		{
			STAMP_MASK = 0xffff800000000000,
			ADDRESS_MASK = ~(0xffff800000000000),
			INCREMENTING_STAMP = ((unsigned long long)1 << 48)
		};

	public:
		explicit ChunkLockFree(unsigned short chunkSize, unsigned short chunkCount, unsigned short code)
			: chunkUsed_(NULL), chunkHeap_(HeapCreate(NULL, NULL, NULL)), chunkSize_(NULL), code_(NULL)
		{
			//==========================================================			
			SYSTEM_INFO systeminfo;
			GetSystemInfo(&systeminfo);

			unsigned long long maxAddr = (unsigned long long)systeminfo.lpMaximumApplicationAddress;

			//현재 메모리풀 구조로 사용한 주소 상위 비트 개수는 17개.
			//여기서 CRASH가 났다는 건, GetSystemInfo로 얻어진 상위 비트 개수가 변동되었다는 의미이다.
			if (ADDRESS_MASK != GetMsb64(maxAddr))
				Error(L"Address Mask Need To Be Changed");

			//==========================================================

			code_ = code;
			chunkSize_ = chunkSize;
			chunkCapacity_ = chunkCount* 3;

			InitializeSRWLock(&freeChunkLock_);

			Initialize();

		}

		~ChunkLockFree()
		{
			HeapDestroy(chunkHeap_);
		}

		DATA* Alloc(void);
		bool Free(DATA* head, DATA* tail);
		void FreeFromTLSDestruction(DATA* head, DATA* tail, DWORD count);
		long GetChunkCapacity(void) const;
		long GetChunkUsed(void) const;
		long GetBlockUsed(void) const;
		long GetBlockCapacity(void) const;
		HANDLE GetHeapHandle(void) const;

		void GarbageCollect();

	private:
		void Error(const WCHAR* message);
		bool isEmpty(Chunk* maskedTop) const;
		void Initialize();
		unsigned long long MaskAndIncrementStamp(Chunk* address);
		unsigned long long GetMsb64(unsigned long long n);
		DATA* CreateNewData();

		Chunk* CreateDummyChunk()
		{
			Chunk* chunk = (Chunk*)HeapAlloc(chunkHeap_, 0, sizeof(Chunk));
			if (chunk == nullptr)
				Error(L"HeapAlloc Failed");

			chunk->Clear();

			return chunk;
		}

		Chunk* CreateDummyChunkForFree(DATA* head, DATA* tail) {

			Chunk* chunk = (Chunk*)HeapAlloc(chunkHeap_, 0, sizeof(Chunk));
			if (chunk == nullptr)
				Error(L"HeapAlloc Failed");

			chunk->head = head;
			chunk->tail = tail;
			chunk->nextChunk = nullptr;

			return chunk;
		}

		Chunk* CreateNewChunk()
		{
			Chunk* chunk = (Chunk*)HeapAlloc(chunkHeap_, 0, sizeof(Chunk));
			if (chunk == nullptr)
				Error(L"HeapAlloc Failed");

			DATA* nextData = CreateNewData();
			chunk->nextChunk = nullptr;
			chunk->head = nextData;

			for (int i = 0; i < chunkSize_ - 1; i++)
			{
				DATA* newBlock = CreateNewData();
				nextData->next = newBlock;
				nextData = newBlock;

			}

			chunk->tail = nextData;

			return chunk;

		}

		Chunk* MaskAddress(Chunk* address)
		{
			Chunk* localAddr = reinterpret_cast<Chunk*>(
				reinterpret_cast<unsigned long long>(address) & ADDRESS_MASK);
			return localAddr;
		}

	private:
		TempChunk freeChunk_;
		Chunk* dummyTopChunk_;
		Chunk* topChunk_;
		RTL_SRWLOCK freeChunkLock_;
		HANDLE chunkHeap_;
		long chunkCapacity_;
		long chunkUsed_;
		unsigned short chunkSize_;
		unsigned short code_;


	};


	template<typename DATA>
	DATA* ChunkLockFree<DATA>::Alloc(void)
	{
		ChunkLockFree<DATA>::Chunk* localTop = nullptr;
		ChunkLockFree<DATA>::Chunk* newTop = nullptr;
		ChunkLockFree<DATA>::Chunk* maskedLocalTop = nullptr;
		ChunkLockFree<DATA>::Chunk* next = nullptr;
		ChunkLockFree<DATA>::Chunk* maskedNext = nullptr;
		ChunkLockFree<DATA>::Chunk* retChunk = nullptr;
		unsigned long long maskedStamp = 0;

		DATA* temp = nullptr;

		do {

			localTop = topChunk_;
			maskedStamp = MaskAndIncrementStamp(localTop);
			maskedLocalTop = MaskAddress(localTop);

			if (isEmpty(maskedLocalTop))
			{
				InterlockedIncrement(&chunkCapacity_);
				retChunk = CreateNewChunk();


				break;

			}

			next = maskedLocalTop->nextChunk;
			maskedNext = MaskAddress(next);
			retChunk = maskedLocalTop;


			newTop = reinterpret_cast<ChunkLockFree<DATA>::Chunk*>(
				(unsigned long long)maskedNext + maskedStamp);

		} while (localTop !=
			InterlockedCompareExchangePointer(
				(PVOID*)&topChunk_,
				newTop,
				localTop));

		InterlockedIncrement(&chunkUsed_);

		DATA* retData = retChunk->head;
		temp = retChunk->head;
		retChunk->Clear();

		int ret = HeapFree(chunkHeap_, NULL, (LPVOID)retChunk);
		if (!ret)
			Error(L"HeapFree Failed");

		return retData;
	}

	template<typename DATA>
	bool ChunkLockFree<DATA>::Free(DATA* head, DATA* tail)
	{
		ChunkLockFree<DATA>::Chunk* chunk = CreateDummyChunkForFree(head, tail);
		ChunkLockFree<DATA>::Chunk* localTop = nullptr;
		ChunkLockFree<DATA>::Chunk* newTop = nullptr;
		unsigned long long maskedStamp = NULL;


		InterlockedDecrement(&chunkUsed_);


		do {

			localTop = topChunk_;
			chunk->nextChunk = localTop;
			maskedStamp = MaskAndIncrementStamp(localTop);

			newTop = reinterpret_cast<ChunkLockFree<DATA>::Chunk*>(
				reinterpret_cast<unsigned long long>(chunk) + maskedStamp);

		} while (localTop !=
			InterlockedCompareExchangePointer((PVOID*)&topChunk_,
				newTop,
				localTop));



		return true;

	}

	template<typename DATA>
	void ChunkLockFree<DATA>::FreeFromTLSDestruction(DATA* head, DATA* tail, DWORD count)
	{
		if (count == chunkSize_)
		{
			Free(head, tail);
			return;
		}

		AcquireSRWLockExclusive(&freeChunkLock_);

		if (freeChunk_.chunk.head)
			freeChunk_.chunk.tail->next = head;
		else
		{
			freeChunk_.chunk.head = head;
			freeChunk_.chunk.tail = head;
		}

		freeChunk_.chunk.tail = tail;
		freeChunk_.count += count;

		if (freeChunk_.count >= chunkSize_)
		{
			DATA* tempHead = freeChunk_.chunk.head;
			DATA* tempTail = tempHead;

			for (int i = 0; i < (chunkSize_ - 1); i++)
			{
				tempTail = tempTail->next;
			}

			freeChunk_.chunk.head = tempTail->next;
			tempTail->next = nullptr;

			Free(tempHead, tempTail);
			freeChunk_.count -= chunkSize_;
		}

		ReleaseSRWLockExclusive(&freeChunkLock_);

		GarbageCollect();

	}


	template<typename DATA>
	inline long	ChunkLockFree<DATA>::GetChunkCapacity(void) const
	{
		return chunkCapacity_;
	}

	template<typename DATA>
	inline long	ChunkLockFree<DATA>::GetChunkUsed(void) const
	{
		return chunkUsed_;
	}

	template<typename DATA>
	inline long	ChunkLockFree<DATA>::GetBlockUsed(void) const
	{
		return (chunkUsed_ * chunkSize_);
	}

	template<typename DATA>
	inline long ChunkLockFree<DATA>::GetBlockCapacity(void) const
	{
		return (chunkCapacity_ * chunkSize_);
	}

	template<typename DATA>
	inline HANDLE ChunkLockFree<DATA>::GetHeapHandle(void) const
	{
		return chunkHeap_;
	}

	template<typename DATA>
	inline void ChunkLockFree<DATA>::GarbageCollect()
	{
		int diff = chunkCapacity_ - (chunkUsed_ * 3);

		if (diff > 0)
		{
			for (int i = 0; i < diff; i++)
			{
				DATA* data = Alloc();

				while (data)
				{
					DATA* nextData = data->next;
					int ret = HeapFree(chunkHeap_, NULL, (LPVOID)data);
					if (!ret)
						Error(L"HeapFree Failed");

					data = nextData;
				}

			}

			InterlockedAdd((volatile LONG*)&chunkUsed_, -(diff));
			InterlockedAdd((volatile LONG*)&chunkCapacity_, -(diff));
		}

	}

	template<typename DATA>
	inline void ChunkLockFree<DATA>::Error(const WCHAR* message)
	{
		SystemLogger::GetInstance()->LogText(L"ChunkLockFree", LEVEL_SYSTEM, message);
		SystemLogger::GetInstance()->LogText(L"ChunkLockFree", LEVEL_SYSTEM, L"ErrorCode : %d", GetLastError());
		CrashDump::Crash();
	}

	template<typename DATA>
	inline void ChunkLockFree<DATA>::Initialize()
	{
		dummyTopChunk_ = CreateDummyChunk();

		ChunkLockFree<DATA>::Chunk* nextChunk = dummyTopChunk_;
		for (int i = 0; i < chunkCapacity_; i++)
		{
			ChunkLockFree<DATA>::Chunk* newChunk = CreateNewChunk();

			nextChunk->nextChunk = newChunk;
			nextChunk = newChunk;

		}

		topChunk_ = dummyTopChunk_->nextChunk;
	}

	template<typename DATA>
	inline DATA* ChunkLockFree<DATA>::CreateNewData()
	{

		DATA* data = (DATA*)HeapAlloc(chunkHeap_, 0, sizeof(DATA));

		if (data == nullptr)
			Error(L"HeapAlloc Failed");

		data->Initialize(code_);
		return data;
	}

	template<typename DATA>
	inline bool ChunkLockFree<DATA>::isEmpty(Chunk* maskedTop) const
	{
		if (maskedTop == nullptr)
			return true;
		else
			return false;
	}

	template<typename DATA>
	inline unsigned long long ChunkLockFree<DATA>::MaskAndIncrementStamp(Chunk* address)
	{
		unsigned long long localStamp = (reinterpret_cast<unsigned long long>(address) & STAMP_MASK);
		localStamp += INCREMENTING_STAMP;
		return localStamp;
	}

	template<typename DATA>
	inline unsigned long long ChunkLockFree<DATA>::GetMsb64(unsigned long long n)
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