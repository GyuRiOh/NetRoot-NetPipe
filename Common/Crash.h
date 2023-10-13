
#ifndef __CRASH_DUMP__
#define __CRASH_DUMP__
#define _WINSOCKAPI_
#include <Windows.h>
#include <stdlib.h>
#include <new.h>
#include <atomic>
#include <minidumpapiset.h>
#include "Singleton.h"
#include "SystemLogger.h"
#pragma comment(lib,"Dbghelp.lib")

namespace server_baby
{
	class CrashDump : public Singleton<CrashDump>
	{
	public:
		explicit CrashDump()
		{
			dumpCount_ = 0;

			_invalid_parameter_handler old_handler, new_handler;
			new_handler = myInvalidParameterHandler;

			//CRT 함수에 null포인터 등을 넣었을 때.
			old_handler = _set_invalid_parameter_handler(new_handler);

			//CRT 오류 메시지 표시 중단. 바로 덤프로 남도록
			_CrtSetReportMode(_CRT_WARN, 0);

			//CRT 오류 메시지 표시 중단. 바로 덤프로 남도록
			_CrtSetReportMode(_CRT_ASSERT, 0);

			//CRT 오류 메시지 표시 중단. 바로 덤프로 남도록
			_CrtSetReportMode(_CRT_ERROR, 0);

			_CrtSetReportHook(_custom_Report_hook);

			//------------------------------------------------------
			// pure virtual function called 에러 핸들러를 사용자 정의 함수로 우회시킨다.
			//------------------------------------------------------

			_set_purecall_handler(myPurecallHandler);

			SetHandlerDump();

			//New 에러 핸들러
			std::new_handler new_alloc_handler;
			new_alloc_handler = MyNewHandler;
			set_new_handler(new_alloc_handler);

		}

		static void __cdecl MyNewHandler()
		{
			SystemLogger::GetInstance()->LogText(L"New Allocate", LEVEL_SYSTEM, L"New Allocation Failed!! Threw Bad Alloc");
			//   throw bad_alloc();  
			Crash();
			return;
		}

		

		void Initialize()
		{

			dumpCount_ = 0;

			_invalid_parameter_handler old_handler, new_handler;
			new_handler = myInvalidParameterHandler;

			//CRT 함수에 null포인터 등을 넣었을 때.
			old_handler = _set_invalid_parameter_handler(new_handler);

			//CRT 오류 메시지 표시 중단. 바로 덤프로 남도록
			_CrtSetReportMode(_CRT_WARN, 0);

			//CRT 오류 메시지 표시 중단. 바로 덤프로 남도록
			_CrtSetReportMode(_CRT_ASSERT, 0);

			//CRT 오류 메시지 표시 중단. 바로 덤프로 남도록
			_CrtSetReportMode(_CRT_ERROR, 0);

			_CrtSetReportHook(_custom_Report_hook);

			//------------------------------------------------------
			// pure virtual function called 에러 핸들러를 사용자 정의 함수로 우회시킨다.
			//------------------------------------------------------

			_set_purecall_handler(myPurecallHandler);

			SetHandlerDump();
		}

		void Destroy()
		{

		}

		static void Crash(void)
		{
			GetInstance();
			int* p = nullptr;
			*p = 100;
		}

		static LONG WINAPI MyExceptionFilter(__in PEXCEPTION_POINTERS pExceptionPointer)
		{
			int workingMemory = 0;
			SYSTEMTIME nowTime;

			long dumpCount = InterlockedIncrement(&dumpCount_);

			//-----------------------------------------------
			// 현재 날짜와 시간을 알아온다.
			//-----------------------------------------------

			WCHAR filename[MAX_PATH];

			GetLocalTime(&nowTime);
			swprintf_s(filename, L"DUMP_%d%02d%02d_%02d.%02d.%02d_%d_.dmp",
				nowTime.wYear,
				nowTime.wMonth,
				nowTime.wDay,
				nowTime.wHour,
				nowTime.wMinute,
				nowTime.wSecond,
				dumpCount);

			wprintf(L"\n\n\n!!! Crash Error !!! %d.%d.%d / %d:%d:%d \n",
				nowTime.wYear,
				nowTime.wMonth,
				nowTime.wDay,
				nowTime.wHour,
				nowTime.wMinute,
				nowTime.wSecond);

			wprintf(L"Now Save dump file.. \n");

			HANDLE dumpFile = ::CreateFile(
				filename,
				GENERIC_WRITE,
				FILE_SHARE_WRITE,
				NULL,
				CREATE_ALWAYS,
				FILE_ATTRIBUTE_NORMAL,
				NULL);

			if (dumpFile != INVALID_HANDLE_VALUE)
			{
				_MINIDUMP_EXCEPTION_INFORMATION miniDumpExceptionInformation;

				miniDumpExceptionInformation.ThreadId = ::GetCurrentThreadId();
				miniDumpExceptionInformation.ExceptionPointers = pExceptionPointer;
				miniDumpExceptionInformation.ClientPointers = TRUE;

				MiniDumpWriteDump(
					GetCurrentProcess(),
					GetCurrentProcessId(),
					dumpFile,
					MiniDumpWithFullMemory,
					&miniDumpExceptionInformation,
					NULL,
					NULL);

				CloseHandle(dumpFile);
				wprintf(L"CrashDump Save Finish!");

			}

			return EXCEPTION_EXECUTE_HANDLER;
		}

		static void SetHandlerDump()
		{
			SetUnhandledExceptionFilter(MyExceptionFilter);
		}

		//Invalid Parameter handler
		static void myInvalidParameterHandler(const wchar_t* expression, const wchar_t* function, const wchar_t* file, unsigned int line, uintptr_t pReserved)
		{
			Crash();
		}

		static int _custom_Report_hook(int ireposttype, char* message, int* returnvalue)
		{
			Crash();
			return true;
		}

		static void myPurecallHandler(void)
		{
			Crash();
		}

		static long dumpCount_;
	};

}

#endif