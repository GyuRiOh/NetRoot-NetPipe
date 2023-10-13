
#ifndef __SINGLETON__
#define __SINGLETON__

#include <Windows.h>
#include <stdlib.h>

namespace server_baby
{

	template<typename DATA>
	class Singleton
	{
	protected:
		explicit Singleton() {}
		virtual ~Singleton() {}

		static DATA* instance_;
		static RTL_SRWLOCK mutex_;

	public:
		static DATA* GetInstance()
		{
			if (instance_)
				return instance_;

			AcquireSRWLockExclusive(&mutex_);
			if (instance_ == nullptr)
			{
				DATA* instance = (DATA*)malloc(sizeof(DATA));
				if (instance == nullptr)
				{
					int* p = nullptr;
					*p = 100;
				}
				//생성자 대신 호출하는 초기화 함수
				instance->Initialize();
				instance_ = instance;
				atexit(DestroyInstance);
			}
			ReleaseSRWLockExclusive(&mutex_);

			return instance_;
		}

		//최초에 한번 이 친구를 호출해야 한다.
		static DATA* GetInstance(int logLevel)
		{
			if (instance_)
				return instance_;

			AcquireSRWLockExclusive(&mutex_);
			if (instance_ == nullptr)
			{
				DATA* instance = (DATA*)malloc(sizeof(DATA));
				if (instance == nullptr)
				{
					int* p = nullptr;
					*p = 100;
				}
				//생성자 대신 호출하는 초기화 함수
				instance->Initialize();
				instance_ = instance;
				atexit(DestroyInstance);
			}
			ReleaseSRWLockExclusive(&mutex_);

			return instance_;
		}

		static void DestroyInstance()
		{
			instance_->Destroy();
			free(instance_);
			instance_ = nullptr;
		}
	};

	template<typename DATA>
	DATA* Singleton<DATA>::instance_ = nullptr;

	template<typename DATA>
	RTL_SRWLOCK Singleton<DATA>::mutex_;
}

#endif