
#ifndef  __LAN__LARGE__PACKET__
#define  __LAN__LARGE__PACKET__
#include "../Common/Crash.h"
#include "../Common/SystemLogger.h"
#include "LanEnums.h"
#include "../Common/MemTLS.h"
#include <vector>
namespace server_baby
{

	class LanLargePacket final
	{
		enum en_PACKET
		{
			eINIT_FLAG = 0x12345678,
			eBUFFER_SIZE = 16384,
			eBUFFER_MASK = eBUFFER_SIZE - 1,
			eBUFFER_CENTER = eBUFFER_SIZE / 2
		};

	public:
		explicit LanLargePacket() = delete;
		~LanLargePacket() {};

		static LanLargePacket* Alloc();
		static bool Free(LanLargePacket* const packet);
		static int GetBufferCount();
		static int GetUsedCount();
		static int GetCapacity();
		static void DeletePool();
		static void DeleteTLS();

		bool MoveWritePos(const int size); //음수 이동 불가
		LanLargePacket* CopyRemainderToNewPacket(const int packetCnt);

		char* GetWritePos(void) const;
		long long GetEmptySize(void);

	private:
		void Initialize();
		void Clear(void);

		long long GetUsedSize(void) const;
		long long GetUsedSizeWithoutHeader(void);
		char* GetReadPos(void) const;
		char* SetReadPosToRemainStart(const long long headerSize);
		void MoveReadPosWithHeaderSize(const int size);

		void PreserveReadPos();
		void ZeroData();

		bool isOkayToMoveWritePos(const int size);
		bool isOkayToMoveReadPos(const int size);
		bool isFull() const;
		bool isFullWithWritePos(const int size) const;
		bool isEmptyWithReadPos(const int size) const;
		bool isSmallerThanHeaderSize();

		void ErrorQuit(const WCHAR* const message) const;
		void ErrorDisplay(const WCHAR* const message) const;

	private:
		char* data_;
		char* start_;
		char* end_;
		char* readPos_;
		char* prevReadPos_;
		char* writePos_;
		long initFlag_;

		static vector<char*> bufferList_;
		static server_baby::MemTLS<LanLargePacket>* packetPool_;
		static RTL_SRWLOCK bufferListLock_;
		friend struct server_baby::DataBlock<LanLargePacket>;
		friend class LanPacketSet;

	};

	inline bool server_baby::LanLargePacket::MoveWritePos(const int size)
	{
		if (isOkayToMoveWritePos(size))
		{
			writePos_ += size;
			return true;
		}
		else
			return false;
	}

	inline void server_baby::LanLargePacket::ErrorQuit(const WCHAR* const message) const
	{
		SystemLogger::GetInstance()->LogText(L"LargePacket", LEVEL_SYSTEM, message);
		CrashDump::Crash();
	}

	inline void server_baby::LanLargePacket::ErrorDisplay(const WCHAR* const message) const
	{
		SystemLogger::GetInstance()->LogText(L"LargePacket", LEVEL_SYSTEM, message);
	}


	inline bool LanLargePacket::isSmallerThanHeaderSize()
	{
		return (GetUsedSize() < eLAN_HEADER_SIZE);
	}


	inline long long LanLargePacket::GetUsedSize(void) const
	{
		long long usedSize = writePos_ - readPos_;
		return usedSize;
	}

	inline long long LanLargePacket::GetUsedSizeWithoutHeader(void)
	{
		long long usedSize = writePos_ - readPos_ - eLAN_HEADER_SIZE;
		return usedSize;
	}

	inline long long LanLargePacket::GetEmptySize(void)
	{
		long long emptySize = end_ - writePos_;
		return emptySize;
	}

	inline char* LanLargePacket::GetReadPos(void) const
	{
		return readPos_;
	}

	inline char* LanLargePacket::GetWritePos(void) const
	{
		return writePos_;
	}

	inline void LanLargePacket::Clear(void)
	{
		readPos_ = writePos_ = prevReadPos_ = start_;
	}

	inline void LanLargePacket::DeletePool()
	{
		for (int i = 0; i < bufferList_.size(); i++)
		{
			if (!bufferList_[i])
				break;

			_aligned_free(bufferList_[i]);
		}

		delete packetPool_;
	}

	inline void LanLargePacket::DeleteTLS()
	{
		packetPool_->RemoveRemainders();
	}

	inline int LanLargePacket::GetUsedCount()
	{
		return packetPool_->GetTotalUseCount();
	}

	inline int LanLargePacket::GetCapacity()
	{
		return packetPool_->GetTotalCapacity();
	}

	inline char* LanLargePacket::SetReadPosToRemainStart(const long long headerSize)
	{
		readPos_ = start_ - headerSize;
		return readPos_;
	}

	inline void LanLargePacket::MoveReadPosWithHeaderSize(const int size)
	{
		int moveSize = size + eLAN_HEADER_SIZE;
		isOkayToMoveReadPos(moveSize);

		readPos_ += (moveSize);
	}

	inline void LanLargePacket::PreserveReadPos()
	{
		prevReadPos_ = readPos_;
	}

	inline void LanLargePacket::ZeroData()
	{
		long long usedSize = GetUsedSize();
		ZeroMemory(readPos_, usedSize);

	}

	inline bool LanLargePacket::isOkayToMoveWritePos(const int size)
	{
		if (isFullWithWritePos(size))
		{
			ErrorDisplay(L"MoveDataWritePos : Packet is full");
			return false;
		}

		return true;
	}

	inline bool LanLargePacket::isOkayToMoveReadPos(const int size)
	{
		if (isEmptyWithReadPos(size))
		{
			ErrorDisplay(L"MoveDataReadPos : Packet is Empty");
			return false;
		}

		return true;
	}

	inline bool LanLargePacket::isFull() const
	{
		long long diff = GetUsedSize() - eBUFFER_SIZE;
		return (diff >= 0);
	}

	inline bool LanLargePacket::isFullWithWritePos(const int size) const
	{
		void* sum = writePos_ + size;
		return (sum >= end_);
	}

	inline bool LanLargePacket::isEmptyWithReadPos(const int size) const
	{
		void* sum = readPos_ + size;
		return (sum > writePos_);
	}

	inline bool server_baby::LanLargePacket::Free(LanLargePacket* const packet)
	{
		return packetPool_->Free(packet);
	}

	inline int LanLargePacket::GetBufferCount()
	{
		return static_cast<int>(bufferList_.size());
	}

	inline LanLargePacket* server_baby::LanLargePacket::Alloc()
	{
		LanLargePacket* packet = packetPool_->Alloc();
		packet->Initialize();
		return packet;
	}
}
#endif