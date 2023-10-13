
#define _WINSOCKAPI_
#include <Windows.h>
#include <process.h>
#include "SystemLogger.h"

using namespace server_baby;

void server_baby::SystemLogger::Initialize()
{
	isAPCThreadRunning_ = true;

	APCThread_.handle = (HANDLE)_beginthreadex(
		NULL,
		0,
		(_beginthreadex_proc_type)&APCThread,
		(LPVOID)this,
		0,
		(unsigned int*)&APCThread_.ID);

	if (APCThread_.handle == NULL)
	{
		int* p = nullptr;
		*p = 100;
	}

}

void server_baby::SystemLogger::LogText(const wchar_t* type, const int logLevel, const wchar_t* stringFormat, ...)
{

	WCHAR timeStamp[LOG_LEN] = { 0 };
	SetTimeStamp(timeStamp);
	wcscat_s(timeStamp, L" ");

	wchar_t message[LOG_LEN] = { 0 };

	va_list va;
	va_start(va, stringFormat);
	HRESULT hResult = StringCchVPrintfW(message,
		LOG_LEN,
		stringFormat,
		va);
	va_end(va);

	wcscat_s(timeStamp, message);

	if (FAILED(hResult))
	{
		FILE* stream = nullptr;
		//가변인자 로그 짤리면 로그 남기기
		WCHAR brokenLog[FILENAME_LEN] = { 0 };

		SetLogPath(brokenLog);
		wcscat_s(brokenLog, type);
		wcscat_s(brokenLog, L"_LOG_BROKEN.txt\0");

		while (!stream)
		{
			_wfopen_s(&stream, brokenLog, L"a+");
		}

		fwprintf_s(stream,
			L"%d, %ws \n", logLevel, timeStamp);
		fclose(stream);
	}

	switch (logLevel)
	{
	case LEVEL_APC:
		SaveAPCLogText(type, logLevel, timeStamp);
		break;
	case LEVEL_DEBUG:
	case LEVEL_ERROR:
	case LEVEL_SYSTEM:
		SaveLogText(type, logLevel, timeStamp);
		break;
	}

}

void server_baby::SystemLogger::LogCSV(const wchar_t* type, const int logLevel, const wchar_t* stringFormat, ...)
{

	WCHAR timeStamp[LOG_LEN] = { 0 };
	SetTimeStamp(timeStamp);
	wcscat_s(timeStamp, L", ");

	wchar_t message[LOG_LEN] = { 0 };

	va_list va;
	va_start(va, stringFormat);
	HRESULT hResult = StringCchVPrintfW(message,
		LOG_LEN,
		stringFormat,
		va);
	va_end(va);

	wcscat_s(timeStamp, message);

	//가변인자 로그 짤리면 그에 대한 로그 남기기
	if (FAILED(hResult))
	{
		FILE* stream = nullptr;
		WCHAR brokenLog[FILENAME_LEN] = { 0 };

		SetLogPath(brokenLog);
		wcscat_s(brokenLog, type);
		wcscat_s(brokenLog, L"_LOG_BROKEN.csv\0");

		while (!stream)
		{
			_wfopen_s(&stream, brokenLog, L"a+");
		}

		fwprintf_s(stream,
			L"%d, %ws \n", logLevel, timeStamp);
		fclose(stream);
	}

	switch (logLevel)
	{
	case LEVEL_APC:
		SaveAPCLogCsv(type, logLevel, timeStamp);
		break;

	case LEVEL_DEBUG:
	case LEVEL_ERROR:
	case LEVEL_SYSTEM:
		SaveLogCsv(type, logLevel, timeStamp);
		break;
	}
}

void server_baby::SystemLogger::SaveLogText(const wchar_t* type, const int logLevel, const wchar_t* message)
{
	FILE* stream = nullptr;
	WCHAR fileName[FILENAME_LEN] = { 0 };

	SetLogPath(fileName);
	wcscat_s(fileName, type);
	wcscat_s(fileName, L"_LOG.txt");

	while (stream == nullptr)
	{
		_wfopen_s(&stream, fileName, L"a+");
	}

	fwprintf_s(stream,
		L"LOGLEVEL = %d, %ws \n", logLevel, message);
	fclose(stream);
}

void server_baby::SystemLogger::SaveLogCsv(const wchar_t* type, const int logLevel, const wchar_t* message)
{
	FILE* stream = nullptr;
	WCHAR fileName[FILENAME_LEN] = { 0 };

	SetLogPath(fileName);
	wcscat_s(fileName, type);
	wcscat_s(fileName, L"_LOG.csv");

	while (!stream)
	{
		_wfopen_s(&stream, fileName, L"a+");
	}

	fwprintf_s(stream,
		L"%d, %ws \n", logLevel, message);
	fclose(stream);
}

void server_baby::SystemLogger::LogHex(const wchar_t* type, const int logLevel, const wchar_t* log, const int byteLen)
{
	FILE* stream = nullptr;
	WCHAR fileName[FILENAME_LEN] = { 0 };

	SetLogPath(fileName, L"Log_HEX");
	wcscat_s(fileName, type);
	wcscat_s(fileName, L"_LOG_HEX_.txt\0");

	while (stream == nullptr)
	{
		_wfopen_s(&stream, fileName, L"ab+"); //멀티스레드 대비용!
	}

	fwrite(log, byteLen, 1, stream);
	fclose(stream);

}

void server_baby::SystemLogger::Console(const wchar_t* type, const int logLevel, const wchar_t* stringFormat, ...)
{
	wchar_t message[LOG_LEN];

	//로그레벨 표시 기능 추가하기!

	va_list va;
	va_start(va, stringFormat);
	vswprintf(message, LOG_LEN, stringFormat, va);
	va_end(va);

	wprintf_s(L"[%ws]", type);
	wprintf_s(L"[%d] ", logLevel);
	wprintf_s(L"[%ws]\n", message);
}


void server_baby::SystemLogger::SaveAPCLogText(const wchar_t* type, const int logLevel, const wchar_t* message)
{

	APCLog* newLog = new APCLog;

	SetLogPath(newLog->fileName, L"APCLog_Text");
	wcscat_s(newLog->fileName, type);
	wcscat_s(newLog->fileName, L"_APC_LOG_.txt");

	wcscpy_s(newLog->message, message);
	wcscat_s(newLog->message, L"\n");

	apcTxt_.Enqueue(newLog);
	QueueUserAPC((PAPCFUNC)APCProc, APCThread_.handle, (ULONG_PTR)this);

}

void server_baby::SystemLogger::SaveAPCLogCsv(const wchar_t* type, const int logLevel, const wchar_t* message)
{

	APCLog* newLog = new APCLog;

	SetLogPath(newLog->fileName, L"APCLog_CSV");
	wcscat_s(newLog->fileName, type);
	wcscat_s(newLog->fileName, L"_APC_LOG_.csv");

	wcscpy_s(newLog->message, message);
	wcscat_s(newLog->message, L"\n");

	apcCsv_.Enqueue(newLog);
	QueueUserAPC((PAPCFUNC)APCProcCSV, APCThread_.handle, (ULONG_PTR)this);
}

void server_baby::SystemLogger::SetLogPath(WCHAR* path, const wchar_t* dir)
{
	WCHAR tempPath[FILENAME_LEN] = { 0 };
	WCHAR string[64] = { 0 };

	CreateDirectoryW(dir, NULL);
	GetCurrentDirectoryW(FILENAME_LEN, tempPath);
	CopyTimeStampForFileName(string);

	wcscat_s(tempPath, L"\\");
	wcscat_s(tempPath, dir);
	wcscat_s(tempPath, L"\\");
	wcscat_s(tempPath, string);

	memmove(path, tempPath, wcslen(tempPath) * 2);
}

void server_baby::SystemLogger::SetTimeStamp(wchar_t* pathBuffer)
{
	time_t timer;
	struct tm t;
	timer = time(NULL);
	localtime_s(&t, &timer);
	
	swprintf(pathBuffer, 256, L"[%d/%02d/%02d/%d/%d/%d]",
		t.tm_year + 1900,
		t.tm_mon + 1,
		t.tm_mday,
		t.tm_hour,
		t.tm_min,
		t.tm_sec);

}

void server_baby::SystemLogger::CopyTimeStampForFileName(wchar_t* pathBuffer)

{
	wchar_t string[64];
	time_t timer;
	struct tm t;
	timer = time(NULL);
	localtime_s(&t, &timer);

	swprintf(string, 64, L"%d%02d%02d_%02d_",
		t.tm_year + 1900,
		t.tm_mon + 1,
		t.tm_mday,
		t.tm_hour);

	wcscpy_s(pathBuffer, TIME_LEN, string);

}

DWORD server_baby::SystemLogger::APCThread(LPVOID arg)
{
	SystemLogger* logger = (SystemLogger*)arg;
	while (logger->isAPCThreadRunning_)
	{
		SleepEx(1000, true);
		Sleep(1000);
	}
	return 0;
}

void server_baby::SystemLogger::APCProc(ULONG_PTR arg)
{
	SystemLogger* logger = (SystemLogger*)arg;
	logger->MyAPCProc();
	return;
}

void server_baby::SystemLogger::APCProcCSV(ULONG_PTR arg)
{
	SystemLogger* logger = (SystemLogger*)arg;
	logger->MyAPCProcCSV();
	return;
}

void server_baby::SystemLogger::MyAPCProc()
{
	FILE* stream = nullptr;
	while (!apcTxt_.isEmpty())
	{
		APCLog* log = nullptr;
		apcTxt_.Dequeue(&log);

		if (_wfopen_s(&stream, log->fileName, L"a+") == 0)
		{
			fwprintf_s(stream, log->message);
			fclose(stream);
		}

		delete log;
	}

}

void server_baby::SystemLogger::MyAPCProcCSV()
{

	FILE* stream = nullptr;
	while (!apcCsv_.isEmpty())
	{
		APCLog* log = nullptr;
		apcCsv_.Dequeue(&log);

		if (_wfopen_s(&stream, log->fileName, L"a+") == 0)
		{
			fwprintf_s(stream, log->message);
			fclose(stream);
		}

		delete log;
	}

}
