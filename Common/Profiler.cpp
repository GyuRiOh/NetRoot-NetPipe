// Profiler.cpp : 이 파일에는 'main' 함수가 포함됩니다. 거기서 프로그램 실행이 시작되고 종료됩니다.
//

#include <process.h>
#include <iostream>
#include <Windows.h>
#include <time.h>
#include <unordered_map>
#include <conio.h>
#include "Profiler.h"

#pragma comment(lib,"winmm.lib")

using namespace server_baby;
using namespace std;

namespace server_baby
{

	constexpr int MAX_INDEX = 128;
	constexpr int MAX_FILENAME = 256;

	struct hash_str
	{
		size_t operator()(const WCHAR* s) const
		{
			size_t v = 0;
			while (WCHAR c = *s++)
			{
				v = (v << 6) + (v << 16) - v + c;
				return hash<int>()(v);
			}
		};
	};

	struct equal_str
	{
		bool operator () (const WCHAR* a, const WCHAR* b) const
		{
			return wcscmp(a, b) == 0;
		};
	};

	struct Index
	{
		bool _useFlag = false;
		DWORD _threadID = 0;
		unordered_map<const WCHAR*, PROFILE_SAMPLE*, hash_str, equal_str>* _mapPointer = nullptr;

	};

	Index g_IndexArray[MAX_INDEX];
	unsigned long long g_TLSIndex;


	void SetCurrentTimeToFilePath(WCHAR* fileName)
	{
		WCHAR string[MAX_FILENAME];
		time_t timer;
		struct tm t;
		timer = time(NULL);
		localtime_s(&t, &timer);

		wsprintf(string, L"Profiler_%d%02d%02d_%02d_MemTLS_Test.csv", t.tm_year + 1900, t.tm_mon + 1, t.tm_mday, t.tm_hour);

		wcscpy_s(fileName, sizeof(string), string);

		return;
	}

	void RetrieveProfile(DWORD* pThreadID, unordered_map<const WCHAR*, PROFILE_SAMPLE*, hash_str, equal_str>** pProfileMap)
	{
		//스레드ID를 받고, 그걸로 인덱스를 검색한다.
		//TLS에서 프로파일 맵 포인터를 받는다.
		DWORD threadID = GetCurrentThreadId();

		//프로파일 정보(이것도 TLS에 넣음)를 이름과 대비하여 가리킬 map이 필요하다.
		//map포인터를 가리킬 슬롯을 받는다.
		unordered_map<const WCHAR*, PROFILE_SAMPLE*, hash_str, equal_str>* profileMap =
			(unordered_map<const WCHAR*, PROFILE_SAMPLE*, hash_str, equal_str>*)TlsGetValue(g_TLSIndex);

		//profileMap이 널포인터였으면 최초로 할당된 것이다.
		if (profileMap == nullptr)
		{
			//ProfileInit에서 할당받지 않았다면 문제가 있다.
			//크래시낸다.
			CRASH();
		}

		*pProfileMap = profileMap;
		*pThreadID = threadID;

	}

	 //전역배열 멤셋 0
	void ProfileInit()
	{
		timeBeginPeriod(1);

		LARGE_INTEGER freq;
		QueryPerformanceFrequency(&freq);

		//해상도가 바뀌면 크래시낸다.
		if (freq.QuadPart != 10000000)
			CRASH();

		//스레드ID를 받는다.
		DWORD threadID = GetCurrentThreadId();

		//스레드ID와 대응되는 unordered_map 포인터를 가리킬 슬롯을 찾아야한다.
		//Index를 받고, 스레드 ID와 대응되는 전역 map에 넣는다.
		if (InterlockedCompareExchange(&g_TLSIndex, TlsAlloc(), NULL) == NULL)
		{
			if (g_TLSIndex == TLS_OUT_OF_INDEXES)
			{
				//TLS Alloc 함수가 비트 플래그 배열로부터 프리 상태인 플래그를 찾지 못했다.
				CRASH();
			}
		}

		//프로파일 정보(이것도 TLS에 넣음)를 이름과 대비하여 가리킬 map이 필요하다.
		//map포인터를 가리킬 슬롯을 받는다.
		unordered_map<const WCHAR*, PROFILE_SAMPLE*, hash_str, equal_str>* pIndexMap = 
			(unordered_map<const WCHAR*, PROFILE_SAMPLE*, hash_str, equal_str>*)TlsGetValue(g_TLSIndex);

		//threadSample이 널포인터였으면 최초로 할당된 것이다.
		if (pIndexMap == nullptr)
		{
			if (GetLastError() != ERROR_SUCCESS)
			{
				errno_t errCode = GetLastError();
				CRASH();
			}

			//초기화. 데이터 세팅 작업 한다.
			//필요시 ZeroMemory로 세팅해주면 좋은데, 일단은 구조체 생성자로 초기화를 해놓았다.
			//불필요한 Memcpy를 줄이기 위해 넘어간다.
			int retval = TlsSetValue(g_TLSIndex, new unordered_map<const WCHAR*, PROFILE_SAMPLE*, hash_str, equal_str>);

			//TlsSetValue에서 에러가 발생했을 시 
			//체크하는 코드
			if (retval == 0)
			{
				errno_t errCode = GetLastError();
				CRASH();
			}

			//다시 포인터를 받았는데 NULL이면 그냥 크래시 내버린다.
			pIndexMap = (unordered_map<const WCHAR*, PROFILE_SAMPLE*, hash_str, equal_str>*)TlsGetValue(g_TLSIndex);
			if (pIndexMap == nullptr)
				CRASH();

		}
		else
		{
			//최초로 할당받은 것이 아니라면 로직의 결함이 있다.
			//디버깅을 용이하게 하기 위해 CRASH낸다.
			CRASH();
		}

		for (int i = 0; i < MAX_INDEX; i++)
		{
			if (g_IndexArray[i]._useFlag == true)
				continue;

			g_IndexArray[i]._useFlag = true;
			g_IndexArray[i]._threadID = threadID;
			g_IndexArray[i]._mapPointer = pIndexMap;

			break;

		}
	}

	void ProfileBegin(const WCHAR* name)
	{


		DWORD threadID;
		unordered_map<const WCHAR*, PROFILE_SAMPLE*, hash_str, equal_str>* profileMap = nullptr;
		RetrieveProfile(&threadID, &profileMap);

		//profileMap에 name : PROFILE_SAMPLE로 세팅을 해준다.
		//할당이 제대로 안 되었으면 알아서 CRASH를 내 줄 것이다.		
		unordered_map<const WCHAR*, PROFILE_SAMPLE*, hash_str, equal_str>::iterator samplePair = profileMap->find(name);
		if (samplePair == profileMap->end())
		{
			PROFILE_SAMPLE* newSample = new PROFILE_SAMPLE(name, threadID);
			profileMap->insert(make_pair(name, newSample)); 
			samplePair = profileMap->find(name);
		}

		PROFILE_SAMPLE* sample = samplePair->second;

		//공용체 start에 현재 시간 값을 받는다.
		LARGE_INTEGER start;
		QueryPerformanceCounter(&start);
		sample->profileTime.QuadPart = start.QuadPart;

	}


	void ProfileEnd(const WCHAR* name)
	{
		//공용체 end에 현재 시간 값을 받는다.
		LARGE_INTEGER end;
		QueryPerformanceCounter(&end);

		DWORD threadID;
		unordered_map<const WCHAR*, PROFILE_SAMPLE*, hash_str, equal_str>* profileMap = nullptr;
		RetrieveProfile(&threadID, &profileMap);

		//ProfileMap에서 name으로 프로필샘플을 찾는다.
		//없으면 결함. 크래시 낸다.
		unordered_map<const WCHAR*, PROFILE_SAMPLE*, hash_str, equal_str>::iterator samplePair = profileMap->find(name);
		if (samplePair == profileMap->end())
			CRASH();

		PROFILE_SAMPLE* threadSample = samplePair->second;
		//threadSample이 널포인터였으면 최초로 할당된 것이다.
		if (threadSample == nullptr)
		{
			//Profile End에서는 threadSample이 널포인터가 나오면 안된다.
			//반드시 최초 1회는 세팅이 되어있어야 한다.
			//뭔가 로직이 꼬인 것이므로.
			//크래시를 낸다.
			CRASH();

		}

		threadSample->profileTime.QuadPart = ((end.QuadPart) - threadSample->profileTime.QuadPart);
		threadSample->total += threadSample->profileTime.QuadPart;

		if (threadSample->profileTime.QuadPart > threadSample->max[0])
		{
			long long temp = threadSample->max[0];
			threadSample->max[1] = temp;
			threadSample->max[0] = threadSample->profileTime.QuadPart;
		}
		else if (threadSample->profileTime.QuadPart > threadSample->max[1])
		{
			threadSample->max[1] = threadSample->profileTime.QuadPart;
		}

		if (threadSample->profileTime.QuadPart < threadSample->min[0])
		{
			long long temp = threadSample->min[0];
			threadSample->min[1] = temp;
			threadSample->min[0] = threadSample->profileTime.QuadPart;
		}
		else if (threadSample->profileTime.QuadPart < threadSample->min[1])
		{
			threadSample->min[1] = threadSample->profileTime.QuadPart;
		}

		threadSample->call++;


	}

	void ProfileDataOutText()
	{
		WCHAR fileName[MAX_FILENAME] = { 0 };
		FILE* stream = nullptr;
		LARGE_INTEGER freq;

		SetCurrentTimeToFilePath(fileName);
		QueryPerformanceFrequency(&freq);

		DWORD threadID;
		unordered_map<const WCHAR*, PROFILE_SAMPLE*, hash_str, equal_str>* profileMap = nullptr;
		RetrieveProfile(&threadID, &profileMap);

		while (stream == nullptr)
		{
			_wfopen_s(&stream, fileName, L"ab"); //멀티스레드 대비용!
		}

		fwprintf_s(stream, L"ThreadID, Name, Average, Min[0], Min[1], Max[0], Max[1], Call\n");
		for (unordered_map<const WCHAR*, PROFILE_SAMPLE*, hash_str, equal_str>::iterator  iter = profileMap->begin(); 
			iter != profileMap->end(); 
			++iter)
		{
			PROFILE_SAMPLE* threadSample = iter->second;
			if (threadSample->call < 5)
			{
				fwprintf(stream, L"%u, %s,Lack of Data\n", 
					threadSample->threadID, 
					threadSample->sampleName);
				continue;
			}

			//printf_s("Thread ID : %u, before_iTotalTime : %.5llf, iTotalTime : %.5llf, iCall : %lld, MaxMinAverage : %llf \n",
			//	threadSample->_threadID,
			//	(double)threadSample->_total,
			//	((double)threadSample->_total - (double)threadSample->_max[0] - (double)threadSample->_max[1] - (double)threadSample->_min[0] - (double)threadSample->_min[1]),
			//	threadSample->_call,
			//	(double)threadSample->_total / (double)threadSample->_call);


			double average =
				((double)threadSample->total - 
					(double)threadSample->max[0] - 
					(double)threadSample->max[1] - 
					(double)threadSample->min[0] - 
					(double)threadSample->min[1]) 	/ 
				((double)threadSample->call - 4.0);

			fwprintf(stream, L"%u, %s, %.5llf ms, %.5llf ms, %.5llf ms, %.5llf ms, %.5llf ms, %I64d \n", 
				threadSample->threadID,
				threadSample->sampleName,
				average / freq.QuadPart * 1000,
				(double)threadSample->min[0] / freq.QuadPart * 1000,
				(double)threadSample->min[1] / freq.QuadPart * 1000,
				(double)threadSample->max[0] / freq.QuadPart * 1000,
				(double)threadSample->max[1] / freq.QuadPart * 1000,
				threadSample->call);

		}
		fclose(stream);

		timeEndPeriod(1);
	}

	void ProfileReset(void)
	{
		//공용체 start에 현재 시간 값을 받는다.
		LARGE_INTEGER start;
		QueryPerformanceCounter(&start);

		//모든 스레드의 ID 및 인덱스를 받는다.
		//TLS에서 프로파일 맵 포인터를 받는다.
		//애매한 부분 존재한다.. 좀더 생각해보자.

		for (int i = 0; i < MAX_INDEX;i++)
		{
			if (g_IndexArray[i]._useFlag == false)
				break;

			unordered_map<const WCHAR*, PROFILE_SAMPLE*, hash_str, equal_str>* profileMap = g_IndexArray[i]._mapPointer;

			//profileIndexMap이 널포인터였으면 최초로 할당된 것이다.
			if (profileMap == nullptr)
			{
				//Profile Reset에서는 threadSample이 널포인터가 나오면 안된다.
				//반드시 최초 1회는 세팅이 되어있어야 한다.
				//뭔가 로직이 꼬인 것이므로.
				//크래시를 낸다.
				errno_t errCode = GetLastError();
				CRASH();

			}

			for (unordered_map<const WCHAR*, PROFILE_SAMPLE*, hash_str, equal_str>::iterator iterMap = profileMap->begin();
				iterMap != profileMap->end();
				++iterMap)
			{
				PROFILE_SAMPLE* threadSample = iterMap->second;
				//threadSample이 널포인터였으면 최초로 할당된 것이다.
				if (threadSample == nullptr)
				{
					//Profile Reset에서는 threadSample이 널포인터가 나오면 안된다.
					//반드시 최초 1회는 세팅이 되어있어야 한다.
					//뭔가 로직이 꼬인 것이므로.
					//크래시를 낸다.
					CRASH();

				}

				threadSample->profileTime.QuadPart = start.QuadPart;
				threadSample->max[0] = 0;
				threadSample->max[1] = 0;
				threadSample->min[0] = INT_MAX;
				threadSample->min[1] = INT_MAX;
				threadSample->total = 0;
				threadSample->call = 0;
			}
		}

	}

	void ProfilerControl(void)
	{ 

		if (_kbhit())
		{
			WCHAR ControlKey = _getwch();

			//키보드 제어 허용
			if (L'r' == ControlKey || L'R' == ControlKey)
			{
				ProfileReset();
				wprintf(L"Control Mode : RESET Profiler !!");

			}


		}
	}

}