
#ifndef  __NET__DUMMY__PACKET__
#define  __NET__DUMMY__PACKET__
#include "../Common/Crash.h"
#include "../Common/SystemLogger.h"
#include "NetEnums.h"
#include "NetLargePacket.h"

namespace server_baby
{
	class NetDummyPacket final
	{
	public:
		explicit NetDummyPacket() : data_(nullptr), readPos_(nullptr), end_(nullptr){};
		~NetDummyPacket(){};

		char* GetStart() const;
		char* GetReadPos() const;

		int DeqData(char* const dest, const int size);
		int GetSize();

		NetDummyPacket& operator >> (BYTE& byValue);
		NetDummyPacket& operator >> (char& chValue);

		NetDummyPacket& operator >> (short& shValue);
		NetDummyPacket& operator >> (WORD& wValue);

		NetDummyPacket& operator >> (int& iValue);
		NetDummyPacket& operator >> (DWORD& dwValue);
		NetDummyPacket& operator >> (float& fValue);

		NetDummyPacket& operator >> (__int64& iValue);
		NetDummyPacket& operator >> (double& dValue);

	private:
		void Initialize(char* const start, char* const end);
		void Clear();

		void isOkayToMoveReadPos(const int size);	
		bool isEmptyWithReadPos(const int size) const;
		int MoveBodyReadPos(const int size);

		void Error(const WCHAR* const message) const;

	private:
		char* data_;
		char* end_;
		char* readPos_;

		friend class NetPacketSet;

	};

	inline NetDummyPacket& NetDummyPacket::operator>>(BYTE& byValue)
	{
		char* tempReadPos = readPos_;
		MoveBodyReadPos(sizeof(byValue));
		memmove(&byValue, tempReadPos, sizeof(byValue));
		return *this;
	}

	inline NetDummyPacket& NetDummyPacket::operator>>(char& chValue)
	{
		char* tempReadPos = readPos_;
		MoveBodyReadPos(sizeof(chValue));
		memmove(&chValue, tempReadPos, sizeof(chValue));
		return *this;
	}

	inline NetDummyPacket& NetDummyPacket::operator>>(short& shValue)
	{
		char* tempReadPos = readPos_;
		MoveBodyReadPos(sizeof(shValue));
		memmove(&shValue, tempReadPos, sizeof(shValue));
		return *this;
	}

	inline NetDummyPacket& NetDummyPacket::operator>>(WORD& wValue)
	{
		char* tempReadPos = readPos_;
		MoveBodyReadPos(sizeof(wValue));
		memmove(&wValue, tempReadPos, sizeof(wValue));
		return *this;
	}

	inline NetDummyPacket& NetDummyPacket::operator>>(int& iValue)
	{
		char* tempReadPos = readPos_;
		MoveBodyReadPos(sizeof(iValue));
		memmove(&iValue, tempReadPos, sizeof(iValue));
		return *this;
	}

	inline NetDummyPacket& NetDummyPacket::operator>>(DWORD& dwValue)
	{
		char* tempReadPos = readPos_;
		MoveBodyReadPos(sizeof(dwValue));
		memmove(&dwValue, tempReadPos, sizeof(dwValue));
		return *this;
	}

	inline NetDummyPacket& NetDummyPacket::operator>>(float& fValue)
	{
		char* tempReadPos = readPos_;
		MoveBodyReadPos(sizeof(fValue));
		memmove(&fValue, tempReadPos, sizeof(fValue));
		return *this;
	}

	inline NetDummyPacket& NetDummyPacket::operator>>(__int64& i64Value)
	{
		char* tempReadPos = readPos_;
		MoveBodyReadPos(sizeof(i64Value));
		memmove(&i64Value, tempReadPos, sizeof(i64Value));
		return *this;
	}

	inline NetDummyPacket& NetDummyPacket::operator>>(double& dValue)
	{
		char* tempReadPos = readPos_;
		MoveBodyReadPos(sizeof(dValue));
		memmove(&dValue, tempReadPos, sizeof(dValue));
		return *this;
	}

	inline void NetDummyPacket::Error(const WCHAR* const message) const
	{
		SystemLogger::GetInstance()->LogText(L"DummyPacket", LEVEL_SYSTEM, message);
		CrashDump::Crash();
	}

	inline void NetDummyPacket::Initialize(char* const start, char* const end)
	{
		data_ = readPos_ = start;
		end_ = end;
	}

	inline void NetDummyPacket::Clear()
	{
		data_ = readPos_ = end_ = nullptr;
	}

	inline void NetDummyPacket::isOkayToMoveReadPos(const int size)
	{
		if (isEmptyWithReadPos(size))
			Error(L"MoveDataReadPos : Packet is Empty");
	}

	inline bool NetDummyPacket::isEmptyWithReadPos(const int size) const
	{
		return ((readPos_ + size) > end_);
	}

	inline int NetDummyPacket::MoveBodyReadPos(const int size)
	{
		isOkayToMoveReadPos(size);

		readPos_ += size;
		return size;
	}

	inline char* NetDummyPacket::GetStart() const
	{
		return data_;
	}

	inline char* NetDummyPacket::GetReadPos() const
	{
		return readPos_;
	}

	inline int NetDummyPacket::DeqData(char* const dest, const int size)
	{
		char* localReadPos = readPos_;
		MoveBodyReadPos(size);
		memmove(dest, localReadPos, size);
		return size;
	}

	inline int NetDummyPacket::GetSize()
	{
		return static_cast<int>(end_ - readPos_);
	}


}
#endif