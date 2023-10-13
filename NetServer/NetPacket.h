
#ifndef  __NET__PACKET__
#define  __NET__PACKET__
#include "../Common/Crash.h"
#include "../Common/SystemLogger.h"
#include "NetEnums.h"
#include "../Common/MemTLS.h"

namespace server_baby
{
	class NetPacket final
	{	
	private:
		enum en_PACKET
		{
			eINIT_FLAG = 0x15, 
			ePACKET_DEFAULT = 4096,	// 패킷의 기본 버퍼 사이즈.
			eDATA_DEFAULT = ePACKET_DEFAULT - eNET_HEADER_SIZE,
		};

	public:
		explicit NetPacket() = delete;
		~NetPacket();

		static NetPacket* Alloc();
		static bool Free(NetPacket* const packet);
		static void DeletePool();
		static void DeleteTLS();
		static int GetUsedCount();
		static int GetCapacity();
		static void GarbageCollect();

		int	DeqData(char* const dest, const int size); 
		int	EnqData(const char* const src, const int size);

		//음수 이동 불가
		int MoveBodyWritePos(const int size);
		bool Encode();
		void Clear();
		void AddRef();
		void AddRef(const int num);

		char* GetPacketStart();
		long long GetPacketUsedSize();

		NetPacket& operator << (const unsigned char byValue);
		NetPacket& operator << (const char chValue);

		NetPacket& operator << (const short shValue);
		NetPacket& operator << (const unsigned short wValue);

		NetPacket& operator << (const int iValue);
		NetPacket& operator << (const long lValue);
		NetPacket& operator << (const DWORD dwValue);

		NetPacket& operator << (const float fValue);
		NetPacket& operator << (const __int64 iValue);
		NetPacket& operator << (const double dValue);

		NetPacket& operator >> (BYTE& byValue);
		NetPacket& operator >> (char& chValue);

		NetPacket& operator >> (short& shValue);
		NetPacket& operator >> (WORD& wValue);

		NetPacket& operator >> (int& iValue);
		NetPacket& operator >> (DWORD& dwValue);
		NetPacket& operator >> (float& fValue);

		NetPacket& operator >> (__int64& iValue);
		NetPacket& operator >> (double& dValue);


	private:
		void Error(const WCHAR* const message) const;
		void Initialize();

		void isOkayToMoveWritePos(const int size);
		void isOkayToMoveReadPos(const int size);

		//음수 이동 불가
		int MoveBodyReadPos(const int size);
		bool isFullWithWritePos(const int size) const;
		bool isEmptyWithReadPos(const int size) const;

	private:
		char data_[ePACKET_DEFAULT] = { 0 };
		char* start_ = nullptr;
		char* end_ = nullptr;
		char* header_ = nullptr;		
		char* readPos_ = nullptr;
		char* writePos_ = nullptr;
		bool encodeFlag_ = false;
		long refCnt_ = 0;

		static server_baby::MemTLS<NetPacket>* packetPool_;
		friend class NetSession;
	};


	inline void NetPacket::AddRef()
	{
		InterlockedIncrement(&refCnt_);
	}

	inline void NetPacket::AddRef(const int num)
	{
		InterlockedAdd(&refCnt_, num);
	}


	inline char* NetPacket::GetPacketStart()
	{
		return header_;
	}

	inline long long NetPacket::GetPacketUsedSize(void)
	{
		long long usedSize = writePos_ - readPos_ + eNET_HEADER_SIZE;
		return usedSize;
	}

	inline void NetPacket::Clear(void)
	{
		readPos_ = writePos_ = start_;
	};

	inline void NetPacket::DeletePool()
	{
		delete packetPool_;
	}

	inline void NetPacket::DeleteTLS()
	{
		packetPool_->RemoveRemainders();
	}

	inline int NetPacket::GetUsedCount()
	{
		return packetPool_->GetTotalUseCount();
	}

	inline int NetPacket::GetCapacity()
	{
		return packetPool_->GetTotalCapacity();
	}

	inline void NetPacket::GarbageCollect()
	{
		packetPool_->GarbageCollect();
	}

	inline int NetPacket::DeqData(char* const dest, int size)
	{
		char* localReadPos = readPos_;
		MoveBodyReadPos(size);
		memmove(dest, localReadPos, size);
		return size;
	}

	inline int NetPacket::EnqData(const char* const src, int size)
	{
		char* localWritePos = writePos_;
		MoveBodyWritePos(size);
		memmove(localWritePos, src, size);

		return size;
	}

	inline int server_baby::NetPacket::MoveBodyWritePos(int size)
	{
		isOkayToMoveWritePos(size);

		writePos_ += size;
		return size;
	}


	inline NetPacket* NetPacket::Alloc()
	{
		NetPacket* packet = packetPool_->Alloc();
		packet->Initialize();
		return packet;
	}

	inline bool NetPacket::Free(NetPacket* const packet)
	{
		bool isFree = false;
		long refCnt = InterlockedDecrement(&packet->refCnt_);
		if (refCnt == 0)
			isFree = packetPool_->Free(packet);

		return isFree;
	}

	inline void NetPacket::Error(const WCHAR* const message) const
	{
		SystemLogger::GetInstance()->LogText(L"NetPacket", LEVEL_SYSTEM, message);
		CrashDump::Crash();
	}

	inline void server_baby::NetPacket::Initialize()
	{
		header_ = &data_[0];
		start_ = &data_[eNET_HEADER_SIZE];
		end_ = &data_[ePACKET_DEFAULT - 1];

		readPos_ = start_;
		writePos_ = start_;
		refCnt_ = 1;
		encodeFlag_ = false;
	}

	inline void server_baby::NetPacket::isOkayToMoveWritePos(int size)
	{
		if (isFullWithWritePos(size))
			Error(L"MoveDataWritePos : Packet is full");
	}

	inline void server_baby::NetPacket::isOkayToMoveReadPos(int size)
	{
		if (isEmptyWithReadPos(size))
			Error(L"MoveDataReadPos : Packet is Empty");
	}

	inline int server_baby::NetPacket::MoveBodyReadPos(int size)
	{
		isOkayToMoveReadPos(size);

		readPos_ += size;
		return size;
	}

	inline bool server_baby::NetPacket::isFullWithWritePos(int size) const
	{
		void* sum = writePos_ + size;
		return (sum >= end_);
	}

	inline bool server_baby::NetPacket::isEmptyWithReadPos(int size) const
	{
		void* sum = readPos_ + size;
		return (sum > writePos_);
	}

	inline NetPacket& NetPacket::operator<<(unsigned char byValue)
	{
		
		char* tempWritePos = writePos_;
		MoveBodyWritePos(sizeof(byValue));
		memmove(tempWritePos, &byValue, sizeof(byValue));
		return *this;
	}

	inline NetPacket& NetPacket::operator<<(char chValue)
	{
		char* tempWritePos = writePos_;
		MoveBodyWritePos(sizeof(chValue));
		memmove(tempWritePos, &chValue, sizeof(chValue));
		return *this;
	}

	inline NetPacket& NetPacket::operator<<(short shValue)
	{
		char* tempWritePos = writePos_;
		MoveBodyWritePos(sizeof(shValue));
		memmove(tempWritePos, &shValue, sizeof(shValue));
		return *this;
	}

	inline NetPacket& NetPacket::operator<<(unsigned short wValue)
	{
		char* tempWritePos = writePos_;
		MoveBodyWritePos(sizeof(wValue));
		memmove(tempWritePos, &wValue, sizeof(wValue));
		return *this;
	}

	inline NetPacket& NetPacket::operator<<(int iValue)
	{
		char* tempWritePos = writePos_;
		MoveBodyWritePos(sizeof(iValue));
		memmove(tempWritePos, &iValue, sizeof(iValue));
		return *this;
	}

	inline NetPacket& NetPacket::operator<<(long lValue)
	{
		char* tempWritePos = writePos_;
		MoveBodyWritePos(sizeof(lValue));
		memmove(tempWritePos, &lValue, sizeof(lValue));
		return *this;
	}

	inline NetPacket& NetPacket::operator<<(DWORD dwValue)
	{
		char* tempWritePos = writePos_;
		MoveBodyWritePos(sizeof(dwValue));
		memmove(tempWritePos, &dwValue, sizeof(dwValue));
		return *this;
	}

	inline NetPacket& NetPacket::operator<<(float fValue)
	{
		char* tempWritePos = writePos_;
		MoveBodyWritePos(sizeof(fValue));
		memmove(tempWritePos, &fValue, sizeof(fValue));
		return *this;
	}

	inline NetPacket& NetPacket::operator<<(__int64 i64Value)
	{
		char* tempWritePos = writePos_;
		MoveBodyWritePos(sizeof(i64Value));
		memmove(tempWritePos, &i64Value, sizeof(i64Value));
		return *this;
	}

	inline NetPacket& NetPacket::operator<<(double dValue)
	{
		char* tempWritePos = writePos_;
		MoveBodyWritePos(sizeof(dValue));
		memmove(tempWritePos, &dValue, sizeof(dValue));
		return *this;
	}

	inline NetPacket& NetPacket::operator>>(BYTE& byValue)
	{
		char* tempReadPos = readPos_;
		MoveBodyReadPos(sizeof(byValue));
		memmove(&byValue, tempReadPos, sizeof(byValue));
		return *this;
	}

	inline NetPacket& NetPacket::operator>>(char& chValue)
	{
		char* tempReadPos = readPos_;
		MoveBodyReadPos(sizeof(chValue));
		memmove(&chValue, tempReadPos, sizeof(chValue));
		return *this;
	}

	inline NetPacket& NetPacket::operator>>(short& shValue)
	{
		char* tempReadPos = readPos_;
		MoveBodyReadPos(sizeof(shValue));
		memmove(&shValue, tempReadPos, sizeof(shValue));
		return *this;
	}

	inline NetPacket& NetPacket::operator>>(WORD& wValue)
	{
		char* tempReadPos = readPos_;
		MoveBodyReadPos(sizeof(wValue));
		memmove(&wValue, tempReadPos, sizeof(wValue));
		return *this;
	}

	inline NetPacket& NetPacket::operator>>(int& iValue)
	{
		char* tempReadPos = readPos_;
		MoveBodyReadPos(sizeof(iValue));
		memmove(&iValue, tempReadPos, sizeof(iValue));
		return *this;
	}

	inline NetPacket& NetPacket::operator>>(DWORD& dwValue)
	{
		char* tempReadPos = readPos_;
		MoveBodyReadPos(sizeof(dwValue));
		memmove(&dwValue, tempReadPos, sizeof(dwValue));
		return *this;
	}

	inline NetPacket& NetPacket::operator>>(float& fValue)
	{
		char* tempReadPos = readPos_;
		MoveBodyReadPos(sizeof(fValue));
		memmove(&fValue, tempReadPos, sizeof(fValue));
		return *this;
	}

	inline NetPacket& NetPacket::operator>>(__int64& i64Value)
	{
		char* tempReadPos = readPos_;
		MoveBodyReadPos(sizeof(i64Value));
		memmove(&i64Value, tempReadPos, sizeof(i64Value));
		return *this;
	}

	inline NetPacket& NetPacket::operator>>(double& dValue)
	{
		char* tempReadPos = readPos_;
		MoveBodyReadPos(sizeof(dValue));
		memmove(&dValue, tempReadPos, sizeof(dValue));
		return *this;
	}

}
#endif



