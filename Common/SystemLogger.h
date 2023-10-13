
#ifndef __SYSTEM__LOGGER__
#define __SYSTEM__LOGGER__

#include <stdlib.h>
#include <stdio.h>
#include <cstdarg>
#include <strsafe.h>
#include <xiosbase>
#include <wchar.h>
#include "Singleton.h"
#include "QueueWithLock.h"

namespace server_baby
{
	enum LogLevel
	{
		LEVEL_APC = 0,
		LEVEL_DEBUG,
		LEVEL_ERROR,
		LEVEL_SYSTEM,
		
	};

	enum LengthMax
	{
		FILENAME_LEN = 240,
		TIME_LEN = 256,
		LOG_LEN = 512,
	};

	class SystemLogger : public Singleton<SystemLogger>
	{

		struct ThreadInfo
		{
			HANDLE handle = INVALID_HANDLE_VALUE;
			unsigned int ID = NULL;
		};

		struct APCLog
		{
			wchar_t fileName[FILENAME_LEN] = { 0 };
			wchar_t message[LOG_LEN] = { 0 };
		};

		explicit SystemLogger() {}
	public:

		~SystemLogger() {}

		//최초 1회 반드시 실행해줘야 한다.
		void Initialize();

		inline void Destroy()
		{
			isAPCThreadRunning_ = false;
			CloseHandle(APCThread_.handle);
		}

		void LogText(const wchar_t* type, const int logLevel, const wchar_t* stringFormat, ...);
		void LogCSV(const wchar_t* type, const int logLevel, const wchar_t* stringFormat, ...);
		void LogHex(const wchar_t* type, const int logLevel, const wchar_t* log, const int byteLen);
		void Console(const wchar_t* type, const int logLevel, const wchar_t* stringFormat, ...);

	private:
		void SaveLogText(const wchar_t* type, const int logLevel, const wchar_t* message);
		void SaveLogCsv(const wchar_t* type, const int logLevel, const wchar_t* message);
		void SaveAPCLogText(const wchar_t* type, const int logLevel, const wchar_t* message);
		void SaveAPCLogCsv(const wchar_t* type, const int logLevel, const wchar_t* message);

		void SetLogPath(WCHAR* path, const wchar_t* dir = L"Log");
		void SetTimeStamp(wchar_t* pathBuffer);
		void CopyTimeStampForFileName(wchar_t* pathBuffer);

		static DWORD CALLBACK APCThread(LPVOID arg);
		static void CALLBACK APCProc(ULONG_PTR arg);
		static void CALLBACK APCProcCSV(ULONG_PTR arg);

		void MyAPCProc();
		void MyAPCProcCSV();

	private:
		QueueWithLock<APCLog*> apcTxt_;
		QueueWithLock<APCLog*> apcCsv_;

		ThreadInfo APCThread_;
		bool isAPCThreadRunning_ = true;

	};
}

#endif