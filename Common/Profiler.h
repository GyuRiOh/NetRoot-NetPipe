#pragma once


#ifndef __PROFILER__ //#PRO_BEGIN / PRO_END 는 매크로를 사용해서 컴파일 제외가 되도록 한다.
#define __PROFILER__

#include<WinDef.h>

//프로파일 매크로
#define PRO_BEGIN(TagName) server_baby::ProfileBegin(TagName)
#define PRO_END(TagName)  server_baby::ProfileEnd(TagName)

//크래시 매크로
#define CRASH() do{\
int* a = nullptr;\
*a = 100;\
} while(0)


namespace server_baby
{

	void ProfileInit();
	void ProfileBegin(const WCHAR* szName);
	void ProfileEnd(const WCHAR* szName);
	void ProfileDataOutText();
	void ProfileReset(void);
	void ProfilerControl(void);

	class Profile
	{

	public:
		Profile(const WCHAR* tag) /*: _tag(tag)*/
		{
			wcscpy_s(tag_, tag);
			ProfileBegin(tag);
		}

		~Profile()
		{
			ProfileEnd(tag_);
		}

		WCHAR tag_[256];

	};

	struct PROFILE_SAMPLE
	{
		WCHAR sampleName[64] = { 0 }; // 프로파일 샘플 이름.
		DWORD threadID; // 스레드 ID

		LARGE_INTEGER profileTime; // 프로파일 샘플 실행 시간.

		__int64 total; // 전체 사용시간 카운터 Time. (출력시 호출회수로 나누어 평균 구함)
		__int64 min[2] = { INT_MAX }; // 최소 사용시간 카운터 Time. (초단위로 계산하여 저장 / [0] 가장최소 [1] 다음 최소 [2])
		__int64 max[2] = { 0 }; // 최대 사용시간 카운터 Time. (초단위로 계산하여 저장 / [0] 가장최대 [1] 다음 최대 [2])

		__int64 call; // 누적 호출 횟수.

		PROFILE_SAMPLE()
		{
			threadID = 0;
			profileTime.QuadPart = 0;
			total = 0;
			call = 0;	
		}

		PROFILE_SAMPLE(const WCHAR* name, DWORD ID)
		{
			wcscpy_s(sampleName, name);
			threadID = ID;
			profileTime.QuadPart = 0;
			total = 0;
			call = 0;
		}
	};

}


#endif