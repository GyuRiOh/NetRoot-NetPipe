
#include <corecrt_wstdio.h>
#include "RingBuffer.h"

using namespace std;

namespace server_baby
{

	RingBuffer::RingBuffer(void)
	{
		bufferSize_ = eBUFFER_SIZE;
		writePos_ = data_;
		readPos_ = data_;
		beginPoint_ = data_;
		endPoint_ = data_ + eBUFFER_SIZE + 1;
	}


	RingBuffer::RingBuffer(int bufferSize)
	{
		bufferSize_ = bufferSize;
		writePos_ = data_;
		readPos_ = data_;
		beginPoint_ = data_;
		endPoint_ = data_ + bufferSize_ + 1;

	}

	RingBuffer::~RingBuffer(){ }

	void RingBuffer::Error(const WCHAR* message)
	{
		SystemLogger::GetInstance()->LogText(L"RingBuffer", LEVEL_SYSTEM, message);
		CrashDump::Crash();
	}

	//끝점 - 시작점 문제

	long long RingBuffer::GetUseSize(void)
	{
		long long useSize;
		long long diff = writePos_ - readPos_;

		if (diff > 0)
			useSize = diff;
		else if (diff == 0)
			useSize = 0;
		else
			useSize = bufferSize_ + diff + 1;

		return useSize;
	}

	long long RingBuffer::GetFreeSize(void)
	{
		long long freeSize;
		long long diff = writePos_ - readPos_;

		if (diff > 0)
			freeSize = bufferSize_ - diff;
		else if (diff == 0)
			freeSize = bufferSize_;
		else
			freeSize = (- diff - 1);

		return freeSize;
	}

	long long RingBuffer::DirectEnqueueSize(void)
	{
		long long enqSize;
		long long diff = writePos_ - readPos_;

		if (diff == 0) //큐가 비어있을 때
		{
			enqSize = endPoint_ - writePos_;
			if (readPos_ == writePos_ && writePos_ == beginPoint_) //큐가 처음 지점일 때
				enqSize--;
		}
		else if (diff > 0) //writePos가 readPos보다 앞서있음
			enqSize = endPoint_ - writePos_;
		else //writePos가 readPos보다 뒤에 있음
			enqSize = -diff - 1;

		return enqSize;
	}

	long long RingBuffer::DirectDequeueSize(void)
	{
		long long  deqSize;
		char* localWritePos = writePos_;
		char* localReadPos = readPos_;
		long long diff = localWritePos - localReadPos;

		if (diff >= 0)
			deqSize = diff;
		else
			deqSize = endPoint_ - localReadPos;

		return deqSize;
	}

	int RingBuffer::Enqueue(char* data, int size)
	{
		long long freeSize = GetFreeSize();

		if (freeSize < size)
		{
			Error(L"Enq error : not enough space to enq");
			return -1;
		}
		//유저 끊어주기 후작업 필요

		long long directEnqSize = DirectEnqueueSize();

		if (directEnqSize >= size)
			memmove(writePos_, data, size);
		else
		{
			memmove(writePos_, data, directEnqSize);
			memmove(beginPoint_, data + directEnqSize, size - directEnqSize);
		}

		MoveWritePos(size);

		return size;
	}

	int RingBuffer::Dequeue(char* dest, int size)
	{
		long long useSize = GetUseSize();

		if (useSize < size)
		{
			Error(L"Deq error : not enough data to deq");
			return -1;
		}

		long long directDeqSize = DirectDequeueSize();

		if (directDeqSize >= size)
			memmove(dest, readPos_, size);
		else
		{
			memmove(dest, readPos_, directDeqSize);
			memmove(dest + directDeqSize, beginPoint_, size - directDeqSize);
		}

		MoveReadPos(size);

		return size;
	}

	int RingBuffer::Peek(char* dest, int size)
	{
		long long useSize = GetUseSize();

		if (useSize < size)
		{
			Error(L"Peek error : Not enough data to peek");
			return -1;
		}

		long long directDeqSize = DirectDequeueSize();

		if (directDeqSize >= size)
			memmove(dest, readPos_, size);
		else
		{
			memmove(dest, readPos_, directDeqSize);
			memmove(dest + directDeqSize, beginPoint_, size - directDeqSize);
		}

		return size;
	}


	void RingBuffer::MoveWritePos(int size)
	{
		char* localWritePos = writePos_;
		char* localReadPos = readPos_;
		long long directEnqSize = DirectEnqueueSize();

		if (size < directEnqSize)
			localWritePos += size;
		else if (size > directEnqSize)
			localWritePos = beginPoint_ + (size - directEnqSize);
		else //if (size == direct_enq_size)
		{
			if (localWritePos + size == endPoint_)
				localWritePos = beginPoint_;
			else if (localWritePos + size == localReadPos - 1)
				localWritePos += size;
		}

		writePos_ = localWritePos;

	}

	void RingBuffer::MoveReadPos(int size)
	{
		char* localWritePos = writePos_;
		char* localReadPos = readPos_;
		long long directDeqSize = DirectDequeueSize();

		if (size < directDeqSize)
			localReadPos += size;
		else if (size > directDeqSize)
			localReadPos = beginPoint_ + (size - directDeqSize);
		else //if (size == direct_deq_size)
		{
			if (localReadPos + size == endPoint_)
				localReadPos = beginPoint_;
			else if (localReadPos + size == localWritePos)
				localReadPos += size;

		}

		readPos_ = localReadPos;

	}



}
