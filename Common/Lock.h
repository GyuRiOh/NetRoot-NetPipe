#pragma once
#include <Windows.h>


namespace server_baby
{

	class SRWLockObject
	{
		enum Setting
		{
			INITIALIZED = 0xabcdef12
		};
		RTL_SRWLOCK lock;
		unsigned long initFlag_;

	public:
		SRWLockObject()
		{
			Initialize();
		}

		inline void Initialize()
		{
			if (initFlag_ == INITIALIZED)
				return;

			InitializeSRWLock(&lock);
			initFlag_ = INITIALIZED;
		}

		inline void Lock_Exclusive()
		{
			AcquireSRWLockExclusive(&lock);
		}

		inline void Unlock_Exclusive()
		{
			ReleaseSRWLockExclusive(&lock);
		}

		inline void Lock_Shared()
		{
			AcquireSRWLockShared(&lock);
		}

		inline void Unlock_Shared()
		{
			ReleaseSRWLockShared(&lock);
		}
	};

}