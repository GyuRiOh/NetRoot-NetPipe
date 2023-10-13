
#ifndef __SIZED__MEMORY__POOL__
#define __SIZED__MEMORY__POOL__

#include "Singleton.h"
#include "SizedMemTLS.h"
#include <unordered_map>

namespace server_baby
{

	class SizedMemoryPool : public Singleton<SizedMemoryPool>
	{
		enum BucketSize
		{
			bucket1 = 1 << 3,
			bucket2 = 1 << 4,
			bucket3 = 1 << 5,
			bucket4 = 1 << 6,
			bucket5 = 1 << 7,
			bucket6 = 1 << 8,
			bucket7 = 1 << 9,
			bucket8 = 1 << 10,
			bucket9 = 1 << 11,
			bucket10 = 1 << 12,
			bucket11 = 1 << 13,
			bucket12 = 1 << 14,
			bucket13 = 1 << 15,

		};
		SizedMemoryPool() {};
	public:
		void Initialize()
		{

			poolMap_[0] = new SizedMemTLS<bucket1>();
			poolMap_[bucket1] = poolMap_[0];
			poolMap_[bucket2] = new SizedMemTLS<bucket2>();
			poolMap_[bucket3] = new SizedMemTLS<bucket3>();
			poolMap_[bucket4] = new SizedMemTLS<bucket4>();
			poolMap_[bucket5] = new SizedMemTLS<bucket5>();
			poolMap_[bucket6] = new SizedMemTLS<bucket6>();
			poolMap_[bucket7] = new SizedMemTLS<bucket7>();
			poolMap_[bucket8] = new SizedMemTLS<bucket8>();
			poolMap_[bucket9] = new SizedMemTLS<bucket9>();
			poolMap_[bucket10] = new SizedMemTLS<bucket10>();
			poolMap_[bucket11] = new SizedMemTLS<bucket11>();
			poolMap_[bucket12] = new SizedMemTLS<bucket12>();
			poolMap_[bucket13] = new SizedMemTLS<bucket13>();
		}

		void Destroy()
		{
			delete poolMap_[0];
			delete poolMap_[bucket2];
			delete poolMap_[bucket3];
			delete poolMap_[bucket4];
			delete poolMap_[bucket5];
			delete poolMap_[bucket6];
			delete poolMap_[bucket7];
			delete poolMap_[bucket8];
			delete poolMap_[bucket9];
			delete poolMap_[bucket10];
			delete poolMap_[bucket11];
			delete poolMap_[bucket12];
			delete poolMap_[bucket13];

			poolMap_.clear();
		}

		char* Alloc(DWORD size);
		bool Free(LPVOID pData);

		void DeleteTLS();

	private:
		DWORD GetBucketSize(DWORD digit)
		{
			digit |= digit >> 1;
			digit |= digit >> 2;
			digit |= digit >> 4;
			digit |= digit >> 8;
			digit |= digit >> 16;

			return (digit + 1);
		}


	private:
		static std::unordered_map<DWORD, LPVOID> poolMap_;
	};

	
}

#endif