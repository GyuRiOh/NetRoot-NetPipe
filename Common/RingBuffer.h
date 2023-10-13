#pragma once
#include <Windows.h>
#include "SystemLogger.h"
#include "Crash.h"

using namespace std;

namespace server_baby
{

class RingBuffer
{
	enum Size
	{
		eBUFFER_SIZE = 8191
	};

public:

	explicit RingBuffer(void);
	RingBuffer(int iBufferSize);
	~RingBuffer();

	long long GetUseSize(void);
	long long GetFreeSize(void);

	/////////////////////////////////////////////////////////////////////////
	// 버퍼 포인터로 외부에서 한방에 읽고, 쓸 수 있는 길이.
	// (끊기지 않은 길이)
	//
	// 원형 큐의 구조상 버퍼의 끝단에 있는 데이터는 끝 -> 처음으로 돌아가서
	// 2번에 데이터를 얻거나 넣을 수 있음. 이 부분에서 끊어지지 않은 길이를 의미
	//
	// Parameters: 없음.
	// Return: (int)사용가능 용량.
	////////////////////////////////////////////////////////////////////////
	long long DirectEnqueueSize(void);
	long long DirectDequeueSize(void);

	int	Enqueue(char* chpData, int iSize);
	int	Dequeue(char* chpDest, int iSize);
	int	Peek(char* chpDest, int iSize);

	void MoveWritePos(int iSize);
	void MoveReadPos(int iSize);

	bool isEmpty() const
	{
		bool empty = (writePos_ == readPos_);
		return empty;
	}

	bool isFull() const
	{
		bool full = (writePos_ == readPos_ - 1);
		return full;
	}

	int GetBufferSize(void) const
	{
		int buffer_size = bufferSize_;
		return buffer_size;
	}
	
	void ClearBuffer(void)
	{
		readPos_ = beginPoint_;
		writePos_ = beginPoint_;
	} 

	char* GetWriteBufferPtr(void) const
	{
		return writePos_;
	} 

	char* GetReadBufferPtr(void) const
	{
		return readPos_;
	}

	char* GetBeginPoint(void) const
	{
		return beginPoint_;
	}

	char* GetEndPoint(void) const
	{
		return endPoint_;
	}

	char* DEBUG_GetMiddlePoint(void) const
	{
		return (beginPoint_ + 500);
	}

private :
	void Error(const WCHAR* message);

private:
	// 데이터가 담긴 배열
	// size : 8193
	char data_[eBUFFER_SIZE + 1] = { 0 };

	// 시작점
	// size : 8
	char* beginPoint_;

	// 끝점
	// size : 8
	char* endPoint_;

	// 읽기 포인터
	// size : 8
	char* readPos_;

	// 버퍼 총 사이즈
	// size : 4
	int bufferSize_;

	// 쓰기 포인터
	// size : 8
	alignas(64) char* writePos_;

};
}