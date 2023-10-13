#pragma once


#ifndef __PROFILER__ //#PRO_BEGIN / PRO_END �� ��ũ�θ� ����ؼ� ������ ���ܰ� �ǵ��� �Ѵ�.
#define __PROFILER__

#include<WinDef.h>

//�������� ��ũ��
#define PRO_BEGIN(TagName) server_baby::ProfileBegin(TagName)
#define PRO_END(TagName)  server_baby::ProfileEnd(TagName)

//ũ���� ��ũ��
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
		WCHAR sampleName[64] = { 0 }; // �������� ���� �̸�.
		DWORD threadID; // ������ ID

		LARGE_INTEGER profileTime; // �������� ���� ���� �ð�.

		__int64 total; // ��ü ���ð� ī���� Time. (��½� ȣ��ȸ���� ������ ��� ����)
		__int64 min[2] = { INT_MAX }; // �ּ� ���ð� ī���� Time. (�ʴ����� ����Ͽ� ���� / [0] �����ּ� [1] ���� �ּ� [2])
		__int64 max[2] = { 0 }; // �ִ� ���ð� ī���� Time. (�ʴ����� ����Ͽ� ���� / [0] �����ִ� [1] ���� �ִ� [2])

		__int64 call; // ���� ȣ�� Ƚ��.

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