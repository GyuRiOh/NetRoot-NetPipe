
#ifndef  __NET__LARGE__PACKET__
#define  __NET__LARGE__PACKET__
#include "../Common/Crash.h"
#include "../Common/SystemLogger.h"
#include "NetEnums.h"
#include "../Common/MemTLS.h"
#include <vector>

namespace server_baby
{

	class NetLargePacket final
	{
		enum en_PACKET
		{
			eINIT_FLAG = 0x12345678,
			eBUFFER_SIZE = 8192,
			eBUFFER_MASK = eBUFFER_SIZE - 1,
			eBUFFER_CENTER = eBUFFER_SIZE / 2
		};

	public:
		explicit NetLargePacket() = delete;
		~NetLargePacket() {};

		static NetLargePacket* Alloc();
		static bool Free(NetLargePacket* const packet);
		static int GetUsedCount();
		static int GetCapacity();
		static void DeletePool();
		static void DeleteTLS();
		static void GarbageCollect();

		bool MoveWritePos(const int size); //음수 이동 불가
		NetLargePacket* CopyRemainderToNewPacket(const int packetCnt);

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
		char data_[eBUFFER_SIZE];
		char* start_;
		char* end_;
		char* readPos_;
		char* prevReadPos_;
		char* writePos_;

		static server_baby::MemTLS<NetLargePacket>* packetPool_;
		friend struct server_baby::DataBlock<NetLargePacket>;
		friend class NetPacketSet;

	};

	inline bool server_baby::NetLargePacket::MoveWritePos(const int size)
	{
		if (isOkayToMoveWritePos(size))
		{
			writePos_ += size;
			return true;
		}
		else
			return false;
	}

	inline void server_baby::NetLargePacket::ErrorQuit(const WCHAR* const message) const
	{
		SystemLogger::GetInstance()->LogText(L"LargePacket", LEVEL_SYSTEM, message);
		CrashDump::Crash();
	}

	inline void server_baby::NetLargePacket::ErrorDisplay(const WCHAR* const message) const
	{
		SystemLogger::GetInstance()->LogText(L"LargePacket", LEVEL_SYSTEM, message);
	}


	inline bool NetLargePacket::isSmallerThanHeaderSize()
	{
		return (GetUsedSize() < eNET_HEADER_SIZE);
	}


	inline long long NetLargePacket::GetUsedSize(void) const
	{
		long long usedSize = writePos_ - readPos_;
		return usedSize;
	}

	inline long long NetLargePacket::GetUsedSizeWithoutHeader(void)
	{
		long long usedSize = writePos_ - readPos_ - eNET_HEADER_SIZE;
		return usedSize;
	}

	inline long long NetLargePacket::GetEmptySize(void)
	{
		long long emptySize = end_ - writePos_;
		return emptySize;
	}

	inline char* NetLargePacket::GetReadPos(void) const
	{
		return readPos_;
	}

	inline char* NetLargePacket::GetWritePos(void) const
	{
		return writePos_;
	}

	inline void NetLargePacket::Clear(void)
	{
		readPos_ = writePos_ = prevReadPos_ = start_;
	}

	inline void NetLargePacket::DeletePool()
	{
		delete packetPool_;
	}

	inline void NetLargePacket::DeleteTLS()
	{
		packetPool_->RemoveRemainders();
	}

	inline void NetLargePacket::GarbageCollect()
	{
		packetPool_->GarbageCollect();
	}

	inline int NetLargePacket::GetUsedCount()
	{
		return packetPool_->GetTotalUseCount();
	}

	inline int NetLargePacket::GetCapacity()
	{
		return packetPool_->GetTotalCapacity();
	}

	inline char* NetLargePacket::SetReadPosToRemainStart(const long long headerSize)
	{
		readPos_ = start_ - headerSize;
		return readPos_;
	}

	inline void NetLargePacket::MoveReadPosWithHeaderSize(const int size)
	{
		int moveSize = size + eNET_HEADER_SIZE;
		isOkayToMoveReadPos(moveSize);

		readPos_ += (moveSize);
	}

	inline void NetLargePacket::PreserveReadPos()
	{
		prevReadPos_ = readPos_;
	}

	inline void NetLargePacket::ZeroData()
	{
		long long usedSize = GetUsedSize();
		ZeroMemory(readPos_, usedSize);

	}

	inline bool NetLargePacket::isOkayToMoveWritePos(const int size)
	{
		if (isFullWithWritePos(size))
		{
			ErrorDisplay(L"MoveDataWritePos : Packet is full");
			return false;
		}

		return true;
	}

	inline bool NetLargePacket::isOkayToMoveReadPos(const int size)
	{
		if (isEmptyWithReadPos(size))
		{
			ErrorDisplay(L"MoveDataReadPos : Packet is Empty");
			return false;
		}

		return true;
	}

	inline bool NetLargePacket::isFull() const
	{
		long long diff = GetUsedSize() - eBUFFER_SIZE;
		return (diff >= 0);
	}

	inline bool NetLargePacket::isFullWithWritePos(const int size) const
	{
		void* sum = writePos_ + size;
		return (sum >= end_);
	}

	inline bool NetLargePacket::isEmptyWithReadPos(const int size) const
	{
		void* sum = readPos_ + size;
		return (sum > writePos_);
	}

	inline bool server_baby::NetLargePacket::Free(NetLargePacket* const packet)
	{
		return packetPool_->Free(packet);
	}

	inline NetLargePacket* server_baby::NetLargePacket::Alloc()
	{
		NetLargePacket* packet = packetPool_->Alloc();
		packet->Initialize();
		return packet;
	}
}
#endif