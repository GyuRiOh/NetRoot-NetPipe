#include <Windows.h>
#include "SizedMemoryPool.h"

using namespace server_baby;
using namespace std;

unordered_map<DWORD, LPVOID> SizedMemoryPool::poolMap_;

char* SizedMemoryPool::Alloc(DWORD size)
{
	DWORD bucketSize = NULL;
		
	if (size > bucket13)
		CrashDump::Crash();

	if (size > bucket1)
		bucketSize = GetBucketSize(size);
	else if (size > 0)
		bucketSize = bucket1;
	else
		CrashDump::Crash();

	switch (bucketSize)
	{
	case bucket1:
	{
		SizedMemTLS<bucket1>* memtls = reinterpret_cast<SizedMemTLS<bucket1>*>(poolMap_[bucket1]);
		return memtls->Alloc();
	}
	case bucket2:
	{
		SizedMemTLS<bucket2>* memtls = reinterpret_cast<SizedMemTLS<bucket2>*>(poolMap_[bucket2]);
		return memtls->Alloc();
	}
	case bucket3:
	{
		SizedMemTLS<bucket3>* memtls = reinterpret_cast<SizedMemTLS<bucket3>*>(poolMap_[bucket3]);
		return memtls->Alloc();
	}
	case bucket4:
	{
		SizedMemTLS<bucket4>* memtls = reinterpret_cast<SizedMemTLS<bucket4>*>(poolMap_[bucket4]);
		return memtls->Alloc();
	}
	case bucket5:
	{
		SizedMemTLS<bucket5>* memtls = reinterpret_cast<SizedMemTLS<bucket5>*>(poolMap_[bucket5]);
		return memtls->Alloc();
	}
	case bucket6:
	{
		SizedMemTLS<bucket6>* memtls = reinterpret_cast<SizedMemTLS<bucket6>*>(poolMap_[bucket6]);
		return memtls->Alloc();
	}
	case bucket7:
	{
		SizedMemTLS<bucket7>* memtls = reinterpret_cast<SizedMemTLS<bucket7>*>(poolMap_[bucket7]);
		return memtls->Alloc();
	}
	case bucket8:
	{
		SizedMemTLS<bucket8>* memtls = reinterpret_cast<SizedMemTLS<bucket8>*>(poolMap_[bucket8]);
		return memtls->Alloc();
	}
	case bucket9:
	{
		SizedMemTLS<bucket9>* memtls = reinterpret_cast<SizedMemTLS<bucket9>*>(poolMap_[bucket9]);
		return memtls->Alloc();
	}
	case bucket10:
	{
		SizedMemTLS<bucket10>* memtls = reinterpret_cast<SizedMemTLS<bucket10>*>(poolMap_[bucket10]);
		return memtls->Alloc();
	}
	case bucket11:
	{
		SizedMemTLS<bucket11>* memtls = reinterpret_cast<SizedMemTLS<bucket11>*>(poolMap_[bucket11]);
		return memtls->Alloc();
	}
	case bucket12:
	{
		SizedMemTLS<bucket12>* memtls = reinterpret_cast<SizedMemTLS<bucket12>*>(poolMap_[bucket12]);
		return memtls->Alloc();
	}
	case bucket13:
	{
		SizedMemTLS<bucket13>* memtls = reinterpret_cast<SizedMemTLS<bucket13>*>(poolMap_[bucket13]);
		return memtls->Alloc();
	}
	default:
	{
		return nullptr;
	}
	}

}

bool SizedMemoryPool::Free(LPVOID pData)
{

	DWORD* size = (DWORD*)((char*)pData - 16);
	switch (*size)
	{
	case bucket1:
	{
		SizedMemTLS<bucket1>* memtls = reinterpret_cast<SizedMemTLS<bucket1>*>(poolMap_[bucket1]);
		return memtls->Free((char*)pData);
	}
	case bucket2:
	{
		SizedMemTLS<bucket2>* memtls = reinterpret_cast<SizedMemTLS<bucket2>*>(poolMap_[bucket2]);
		return memtls->Free((char*)pData);
	}
	case bucket3:
	{
		SizedMemTLS<bucket3>* memtls = reinterpret_cast<SizedMemTLS<bucket3>*>(poolMap_[bucket3]);
		return memtls->Free((char*)pData);
	}
	case bucket4:
	{
		SizedMemTLS<bucket4>* memtls = reinterpret_cast<SizedMemTLS<bucket4>*>(poolMap_[bucket4]);
		return memtls->Free((char*)pData);
	}
	case bucket5:
	{
		SizedMemTLS<bucket5>* memtls = reinterpret_cast<SizedMemTLS<bucket5>*>(poolMap_[bucket5]);
		return memtls->Free((char*)pData);
	}
	case bucket6:
	{
		SizedMemTLS<bucket6>* memtls = reinterpret_cast<SizedMemTLS<bucket6>*>(poolMap_[bucket6]);
		return memtls->Free((char*)pData);
	}
	case bucket7:
	{
		SizedMemTLS<bucket7>* memtls = reinterpret_cast<SizedMemTLS<bucket7>*>(poolMap_[bucket7]);
		return memtls->Free((char*)pData);
	}
	case bucket8:
	{
		SizedMemTLS<bucket8>* memtls = reinterpret_cast<SizedMemTLS<bucket8>*>(poolMap_[bucket8]);
		return memtls->Free((char*)pData);
	}
	case bucket9:
	{
		SizedMemTLS<bucket9>* memtls = reinterpret_cast<SizedMemTLS<bucket9>*>(poolMap_[bucket9]);
		return memtls->Free((char*)pData);
	}
	case bucket10:
	{
		SizedMemTLS<bucket10>* memtls = reinterpret_cast<SizedMemTLS<bucket10>*>(poolMap_[bucket10]);
		return memtls->Free((char*)pData);
	}
	case bucket11:
	{
		SizedMemTLS<bucket11>* memtls = reinterpret_cast<SizedMemTLS<bucket11>*>(poolMap_[bucket11]);
		return memtls->Free((char*)pData);
	}
	case bucket12:
	{
		SizedMemTLS<bucket12>* memtls = reinterpret_cast<SizedMemTLS<bucket12>*>(poolMap_[bucket12]);
		return memtls->Free((char*)pData);
	}
	case bucket13:
	{
		SizedMemTLS<bucket13>* memtls = reinterpret_cast<SizedMemTLS<bucket13>*>(poolMap_[bucket13]);
		return memtls->Free((char*)pData);
	}
	default:
	{
		CrashDump::Crash();
		return false;
	}
	}
}

void server_baby::SizedMemoryPool::DeleteTLS()
{
	SizedMemTLS<bucket1>* memtls1 = reinterpret_cast<SizedMemTLS<bucket1>*>(poolMap_[bucket1]);
	SizedMemTLS<bucket2>* memtls2 = reinterpret_cast<SizedMemTLS<bucket2>*>(poolMap_[bucket2]);
	SizedMemTLS<bucket3>* memtls3 = reinterpret_cast<SizedMemTLS<bucket3>*>(poolMap_[bucket3]);
	SizedMemTLS<bucket4>* memtls4 = reinterpret_cast<SizedMemTLS<bucket4>*>(poolMap_[bucket4]);
	SizedMemTLS<bucket5>* memtls5 = reinterpret_cast<SizedMemTLS<bucket5>*>(poolMap_[bucket5]);
	SizedMemTLS<bucket6>* memtls6 = reinterpret_cast<SizedMemTLS<bucket6>*>(poolMap_[bucket6]);
	SizedMemTLS<bucket7>* memtls7 = reinterpret_cast<SizedMemTLS<bucket7>*>(poolMap_[bucket7]);
	SizedMemTLS<bucket8>* memtls8 = reinterpret_cast<SizedMemTLS<bucket8>*>(poolMap_[bucket8]);
	SizedMemTLS<bucket9>* memtls9 = reinterpret_cast<SizedMemTLS<bucket9>*>(poolMap_[bucket9]);
	SizedMemTLS<bucket10>* memtls10 = reinterpret_cast<SizedMemTLS<bucket10>*>(poolMap_[bucket10]);
	SizedMemTLS<bucket11>* memtls11 = reinterpret_cast<SizedMemTLS<bucket11>*>(poolMap_[bucket11]);
	SizedMemTLS<bucket12>* memtls12 = reinterpret_cast<SizedMemTLS<bucket12>*>(poolMap_[bucket12]);
	SizedMemTLS<bucket13>* memtls13 = reinterpret_cast<SizedMemTLS<bucket13>*>(poolMap_[bucket13]);

	memtls1->RemoveRemainders();
	memtls2->RemoveRemainders();
	memtls3->RemoveRemainders();
	memtls4->RemoveRemainders();
	memtls5->RemoveRemainders();
	memtls6->RemoveRemainders();
	memtls7->RemoveRemainders();
	memtls8->RemoveRemainders();
	memtls9->RemoveRemainders();
	memtls10->RemoveRemainders();
	memtls11->RemoveRemainders();
	memtls12->RemoveRemainders();
	memtls13->RemoveRemainders();
}
