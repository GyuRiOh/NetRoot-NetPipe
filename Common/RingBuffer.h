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
	// ���� �����ͷ� �ܺο��� �ѹ濡 �а�, �� �� �ִ� ����.
	// (������ ���� ����)
	//
	// ���� ť�� ������ ������ ���ܿ� �ִ� �����ʹ� �� -> ó������ ���ư���
	// 2���� �����͸� ��ų� ���� �� ����. �� �κп��� �������� ���� ���̸� �ǹ�
	//
	// Parameters: ����.
	// Return: (int)��밡�� �뷮.
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
	// �����Ͱ� ��� �迭
	// size : 8193
	char data_[eBUFFER_SIZE + 1] = { 0 };

	// ������
	// size : 8
	char* beginPoint_;

	// ����
	// size : 8
	char* endPoint_;

	// �б� ������
	// size : 8
	char* readPos_;

	// ���� �� ������
	// size : 4
	int bufferSize_;

	// ���� ������
	// size : 8
	alignas(64) char* writePos_;

};
}