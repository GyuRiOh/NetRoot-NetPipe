
#ifndef  __LAN__PACKET__
#define  __LAN__PACKET__
#include "../Common/Crash.h"
#include "../Common/SystemLogger.h"
#include "LanEnums.h"
#include "../Common/MemTLS.h"

namespace server_baby
{
	class LanPacket final
	{	
	private:
		enum en_PACKET
		{
			eINIT_FLAG = 0x15, 
			ePACKET_DEFAULT = 1024,	// 패킷의 기본 버퍼 사이즈.
			eDATA_DEFAULT = ePACKET_DEFAULT - eLAN_HEADER_SIZE,
		};

	public:
		explicit LanPacket() = delete;
		~LanPacket();

		static LanPacket* Alloc();
		static bool Free(LanPacket* const packet);
		static void DeletePool();
		static void DeleteTLS();
		static int GetUsedCount();
		static int GetCapacity();

		int	DeqData(char* const dest, const int size); 
		int	EnqData(char* const src, const int size);

		//음수 이동 불가
		int MoveBodyWritePos(const int size);
		void Clear();
		void AddRef();
		void AddRef(const int num);

		char* GetPacketStart();
		long long GetPacketUsedSize();

		void SetHeader();

		LanPacket& operator << (const unsigned char byValue);
		LanPacket& operator << (const char chValue);

		LanPacket& operator << (const short shValue);
		LanPacket& operator << (const unsigned short wValue);

		LanPacket& operator << (const int iValue);
		LanPacket& operator << (const long lValue);
		LanPacket& operator << (const DWORD dwValue);

		LanPacket& operator << (const float fValue);
		LanPacket& operator << (const __int64 iValue);
		LanPacket& operator << (const double dValue);

		LanPacket& operator >> (BYTE& byValue);
		LanPacket& operator >> (char& chValue);

		LanPacket& operator >> (short& shValue);
		LanPacket& operator >> (WORD& wValue);

		LanPacket& operator >> (int& iValue);
		LanPacket& operator >> (DWORD& dwValue);
		LanPacket& operator >> (float& fValue);

		LanPacket& operator >> (__int64& iValue);
		LanPacket& operator >> (double& dValue);


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
		char initFlag_ = NULL;
		long refCnt_ = 0;

		static server_baby::MemTLS<LanPacket>* packetPool_;
		friend class LanSession;
	};


	inline void LanPacket::AddRef()
	{
		InterlockedIncrement(&refCnt_);
	}

	inline void LanPacket::AddRef(const int num)
	{
		InterlockedAdd(&refCnt_, num);
	}


	inline char* LanPacket::GetPacketStart()
	{
		return header_;
	}

	inline long long LanPacket::GetPacketUsedSize(void)
	{
		long long usedSize = writePos_ - readPos_ + eLAN_HEADER_SIZE;
		return usedSize;
	}

	inline void LanPacket::SetHeader()
	{
		WORD size = static_cast<WORD>(GetPacketUsedSize() - eLAN_HEADER_SIZE);
		*((WORD*)GetPacketStart()) = size;
	}

	inline void LanPacket::Clear(void)
	{
		readPos_ = writePos_ = start_;
	};

	inline void LanPacket::DeletePool()
	{
		delete packetPool_;
	}

	inline void LanPacket::DeleteTLS()
	{
		packetPool_->RemoveRemainders();
	}

	inline int LanPacket::GetUsedCount()
	{
		return packetPool_->GetTotalUseCount();
	}

	inline int LanPacket::GetCapacity()
	{
		return packetPool_->GetTotalCapacity();
	}

	inline int LanPacket::DeqData(char* const dest, const int size)
	{
		char* localReadPos = readPos_;
		MoveBodyReadPos(size);
		memmove(dest, localReadPos, size);
		return size;
	}

	inline int LanPacket::EnqData(char* const src, const int size)
	{
		char* localWritePos = writePos_;
		MoveBodyWritePos(size);
		memmove(localWritePos, src, size);

		return size;
	}

	inline int server_baby::LanPacket::MoveBodyWritePos(const int size)
	{
		isOkayToMoveWritePos(size);

		writePos_ += size;
		return size;
	}


	inline LanPacket* LanPacket::Alloc()
	{
		LanPacket* packet = packetPool_->Alloc();
		packet->Initialize();
		return packet;
	}

	inline bool LanPacket::Free(LanPacket* const packet)
	{
		bool isFree = false;
		long refCnt = InterlockedDecrement(&packet->refCnt_);
		if (refCnt == 0)
			isFree = packetPool_->Free(packet);

		return isFree;
	}

	inline void LanPacket::Error(const WCHAR* const message) const
	{
		SystemLogger::GetInstance()->LogText(L"LanPacket", LEVEL_SYSTEM, message);
		CrashDump::Crash();
	}

	inline void server_baby::LanPacket::Initialize()
	{
		if (initFlag_ != eINIT_FLAG)
		{
			header_ = &data_[0];
			start_ = &data_[eLAN_HEADER_SIZE];
			end_ = &data_[ePACKET_DEFAULT - 1];
			refCnt_ = 0;
		}

		readPos_ = start_;
		writePos_ = start_;
		initFlag_ = eINIT_FLAG;
		refCnt_ = 1;
	}

	inline void server_baby::LanPacket::isOkayToMoveWritePos(const int size)
	{
		if (isFullWithWritePos(size))
			Error(L"MoveDataWritePos : Packet is full");
	}

	inline void server_baby::LanPacket::isOkayToMoveReadPos(const int size)
	{
		if (isEmptyWithReadPos(size))
			Error(L"MoveDataReadPos : Packet is Empty");
	}

	inline int server_baby::LanPacket::MoveBodyReadPos(const int size)
	{
		isOkayToMoveReadPos(size);

		readPos_ += size;
		return size;
	}

	inline bool server_baby::LanPacket::isFullWithWritePos(const int size) const
	{
		void* sum = writePos_ + size;
		return (sum >= end_);
	}

	inline bool server_baby::LanPacket::isEmptyWithReadPos(const int size) const
	{
		void* sum = readPos_ + size;
		return (sum > writePos_);
	}

	inline LanPacket& LanPacket::operator<<(unsigned char byValue)
	{
		
		char* tempWritePos = writePos_;
		MoveBodyWritePos(sizeof(byValue));
		memmove(tempWritePos, &byValue, sizeof(byValue));
		return *this;
	}

	inline LanPacket& LanPacket::operator<<(char chValue)
	{
		char* tempWritePos = writePos_;
		MoveBodyWritePos(sizeof(chValue));
		memmove(tempWritePos, &chValue, sizeof(chValue));
		return *this;
	}

	inline LanPacket& LanPacket::operator<<(short shValue)
	{
		char* tempWritePos = writePos_;
		MoveBodyWritePos(sizeof(shValue));
		memmove(tempWritePos, &shValue, sizeof(shValue));
		return *this;
	}

	inline LanPacket& LanPacket::operator<<(unsigned short wValue)
	{
		char* tempWritePos = writePos_;
		MoveBodyWritePos(sizeof(wValue));
		memmove(tempWritePos, &wValue, sizeof(wValue));
		return *this;
	}

	inline LanPacket& LanPacket::operator<<(int iValue)
	{
		char* tempWritePos = writePos_;
		MoveBodyWritePos(sizeof(iValue));
		memmove(tempWritePos, &iValue, sizeof(iValue));
		return *this;
	}

	inline LanPacket& LanPacket::operator<<(long lValue)
	{
		char* tempWritePos = writePos_;
		MoveBodyWritePos(sizeof(lValue));
		memmove(tempWritePos, &lValue, sizeof(lValue));
		return *this;
	}

	inline LanPacket& LanPacket::operator<<(DWORD dwValue)
	{
		char* tempWritePos = writePos_;
		MoveBodyWritePos(sizeof(dwValue));
		memmove(tempWritePos, &dwValue, sizeof(dwValue));
		return *this;
	}

	inline LanPacket& LanPacket::operator<<(float fValue)
	{
		char* tempWritePos = writePos_;
		MoveBodyWritePos(sizeof(fValue));
		memmove(tempWritePos, &fValue, sizeof(fValue));
		return *this;
	}

	inline LanPacket& LanPacket::operator<<(__int64 i64Value)
	{
		char* tempWritePos = writePos_;
		MoveBodyWritePos(sizeof(i64Value));
		memmove(tempWritePos, &i64Value, sizeof(i64Value));
		return *this;
	}

	inline LanPacket& LanPacket::operator<<(double dValue)
	{
		char* tempWritePos = writePos_;
		MoveBodyWritePos(sizeof(dValue));
		memmove(tempWritePos, &dValue, sizeof(dValue));
		return *this;
	}

	inline LanPacket& LanPacket::operator>>(BYTE& byValue)
	{
		char* tempReadPos = readPos_;
		MoveBodyReadPos(sizeof(byValue));
		memmove(&byValue, tempReadPos, sizeof(byValue));
		return *this;
	}

	inline LanPacket& LanPacket::operator>>(char& chValue)
	{
		char* tempReadPos = readPos_;
		MoveBodyReadPos(sizeof(chValue));
		memmove(&chValue, tempReadPos, sizeof(chValue));
		return *this;
	}

	inline LanPacket& LanPacket::operator>>(short& shValue)
	{
		char* tempReadPos = readPos_;
		MoveBodyReadPos(sizeof(shValue));
		memmove(&shValue, tempReadPos, sizeof(shValue));
		return *this;
	}

	inline LanPacket& LanPacket::operator>>(WORD& wValue)
	{
		char* tempReadPos = readPos_;
		MoveBodyReadPos(sizeof(wValue));
		memmove(&wValue, tempReadPos, sizeof(wValue));
		return *this;
	}

	inline LanPacket& LanPacket::operator>>(int& iValue)
	{
		char* tempReadPos = readPos_;
		MoveBodyReadPos(sizeof(iValue));
		memmove(&iValue, tempReadPos, sizeof(iValue));
		return *this;
	}

	inline LanPacket& LanPacket::operator>>(DWORD& dwValue)
	{
		char* tempReadPos = readPos_;
		MoveBodyReadPos(sizeof(dwValue));
		memmove(&dwValue, tempReadPos, sizeof(dwValue));
		return *this;
	}

	inline LanPacket& LanPacket::operator>>(float& fValue)
	{
		char* tempReadPos = readPos_;
		MoveBodyReadPos(sizeof(fValue));
		memmove(&fValue, tempReadPos, sizeof(fValue));
		return *this;
	}

	inline LanPacket& LanPacket::operator>>(__int64& i64Value)
	{
		char* tempReadPos = readPos_;
		MoveBodyReadPos(sizeof(i64Value));
		memmove(&i64Value, tempReadPos, sizeof(i64Value));
		return *this;
	}

	inline LanPacket& LanPacket::operator>>(double& dValue)
	{
		char* tempReadPos = readPos_;
		MoveBodyReadPos(sizeof(dValue));
		memmove(&dValue, tempReadPos, sizeof(dValue));
		return *this;
	}

}
#endif



