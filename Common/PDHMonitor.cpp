
#define _WINSOCKAPI_

#include "PDHMonitor.h"
#include <Windows.h>
#include <strsafe.h>

server_baby::ProcessMonitor::CpuUsageForProcess::CpuUsageForProcess(HANDLE process)
{
	if (process == INVALID_HANDLE_VALUE)
		process_ = GetCurrentProcess();

	SYSTEM_INFO systemInfo;
	GetSystemInfo(&systemInfo);
	numberOfProcessors_ = systemInfo.dwNumberOfProcessors;

	processTotal_ = 0;
	processUser_ = 0;
	processKernel_ = 0;

	processLastUser_.QuadPart = 0;
	processLastKernel_.QuadPart = 0;
	processLastTime_.QuadPart = 0;

	UpdateCpuTime();

}

void server_baby::ProcessMonitor::CpuUsageForProcess::Initialize(HANDLE process)
{
	if (process == INVALID_HANDLE_VALUE)
		process_ = GetCurrentProcess();

	SYSTEM_INFO systemInfo;
	GetSystemInfo(&systemInfo);
	numberOfProcessors_ = systemInfo.dwNumberOfProcessors;

	processTotal_ = 0;
	processUser_ = 0;
	processKernel_ = 0;

	processLastUser_.QuadPart = 0;
	processLastKernel_.QuadPart = 0;
	processLastTime_.QuadPart = 0;

	UpdateCpuTime();

}

void server_baby::HardwareMonitor::CpuUsageForProcessor::Initialize(HANDLE process)
{
	if (process == INVALID_HANDLE_VALUE)
		process_ = GetCurrentProcess();

	SYSTEM_INFO systemInfo;
	GetSystemInfo(&systemInfo);
	numberOfProcessors_ = systemInfo.dwNumberOfProcessors;

	processorTotal_ = 0;
	processorUser_ = 0;
	processorKernel_ = 0;

	processorLastUser_.QuadPart = 0;
	processorLastKernel_.QuadPart = 0;
	processorLastIdle_.QuadPart = 0;

	UpdateCpuTime();

}

//500ms~1000ms 단위 호출이 적절함
void server_baby::ProcessMonitor::CpuUsageForProcess::UpdateCpuTime()
{
	ULARGE_INTEGER kernel;
	ULARGE_INTEGER user;
	ULARGE_INTEGER none;
	ULARGE_INTEGER nowTime;
	
	ULONGLONG kernelDiff;
	ULONGLONG userDiff;
	ULONGLONG timeDiff;
	ULONGLONG total;


	GetSystemTimeAsFileTime((LPFILETIME)&nowTime);
	GetProcessTimes(process_,
		(LPFILETIME)&none,
		(LPFILETIME)&none,
		(LPFILETIME)&kernel,
		(LPFILETIME)&user);

	timeDiff = nowTime.QuadPart - processLastTime_.QuadPart;
	userDiff = user.QuadPart - processLastUser_.QuadPart;
	kernelDiff = kernel.QuadPart - processLastKernel_.QuadPart;

	total = kernelDiff + userDiff;

	processTotal_ = (float)((double)total / (double)numberOfProcessors_ / (double)timeDiff * 100.0f);
	processKernel_ = (float)((double)kernelDiff / (double)numberOfProcessors_ / (double)timeDiff * 100.0f);
	processUser_ = (float)((double)userDiff / (double)numberOfProcessors_ / (double)timeDiff * 100.0f);

	processLastTime_ = nowTime;
	processLastKernel_ = kernel;
	processLastUser_ = user;



}

server_baby::HardwareMonitor::CpuUsageForProcessor::CpuUsageForProcessor(HANDLE process)
{
	if (process == INVALID_HANDLE_VALUE)
		process_ = GetCurrentProcess();

	SYSTEM_INFO systemInfo;
	GetSystemInfo(&systemInfo);
	numberOfProcessors_ = systemInfo.dwNumberOfProcessors;

	processorTotal_ = 0;
	processorUser_ = 0;
	processorKernel_ = 0;

	processorLastUser_.QuadPart = 0;
	processorLastKernel_.QuadPart = 0;
	processorLastIdle_.QuadPart = 0;

	UpdateCpuTime();
}

void server_baby::HardwareMonitor::CpuUsageForProcessor::UpdateCpuTime()
{
	ULARGE_INTEGER idle;
	ULARGE_INTEGER kernel;
	ULARGE_INTEGER user;

	if (GetSystemTimes((PFILETIME)&idle, (PFILETIME)&kernel, (PFILETIME)&user) == false)
		return;

	ULONGLONG kernelDiff = kernel.QuadPart - processorLastKernel_.QuadPart;
	ULONGLONG userDiff = user.QuadPart - processorLastUser_.QuadPart;
	ULONGLONG idleDiff = idle.QuadPart - processorLastIdle_.QuadPart;
	ULONGLONG total = kernelDiff + userDiff;

	if (total == 0)
	{
		processorUser_ = 0.0f;
		processorKernel_ = 0.0f;
		processorTotal_ = 0.0f;
	}
	else
	{
		processorTotal_ = (float)((((double)total - (double)idleDiff) / (double)total) * 100.0f);
		processorUser_ = (float)(((double)userDiff / (double)total) * 100.0f);
		processorKernel_ = (float)((((double)kernelDiff - (double)idleDiff) / (double)total) * 100.0f);
	}

	processorLastKernel_ = kernel;
	processorLastUser_ = user;
	processorLastIdle_ = idle;

}

void server_baby::ProcessMonitor::MemoryForProcess::Initialize()
{
	PdhOpenQuery(NULL, NULL, &cpuQuery_);
	PdhAddCounter(cpuQuery_, L"\\Memory\\Available MBytes", NULL, &availableBytes_);
	PdhAddCounter(cpuQuery_, L"\\Memory\\Pool Nonpaged Bytes", NULL, &nonPagedByte_);

	UsePDHEnumObject();
	PdhCollectQueryData(cpuQuery_);
}

void server_baby::ProcessMonitor::MemoryForProcess::Update()
{
	PdhCollectQueryData(cpuQuery_);
	PdhGetFormattedCounterValue(availableBytes_, PDH_FMT_DOUBLE, NULL, &availableBytesVal_);
	PdhGetFormattedCounterValue(nonPagedByte_, PDH_FMT_DOUBLE, NULL, &nonPagedByteVal_);

	for (int iCnt = 0; iCnt < PDH_ETHERNET_MAX; iCnt++)
	{
		if (ethernetStruct_[iCnt].use)
		{
			PDH_STATUS Status = PdhGetFormattedCounterValue(ethernetStruct_[iCnt].recvBytes,
				PDH_FMT_DOUBLE, NULL, &recvVal_);
			if (Status == 0)
				totalRecvBytes_ += recvVal_.doubleValue;

			Status = PdhGetFormattedCounterValue(ethernetStruct_[iCnt].sendBytes,
				PDH_FMT_DOUBLE, NULL, &sendVal_);
			if (Status == 0)
				totalSendBytes_ += sendVal_.doubleValue;
		}

	}
}

void server_baby::ProcessMonitor::MemoryForProcess::UsePDHEnumObject()
{
	PdhEnumObjectItems(
		NULL,
		NULL,
		L"Network Interface",
		counters_,
		&counterSize_,
		interfaces_,
		&interfaceSize_,
		PERF_DETAIL_WIZARD,
		0);

	counters_ = new WCHAR[counterSize_];
	interfaces_ = new WCHAR[interfaceSize_];

	if (PdhEnumObjectItems(
		NULL,
		NULL,
		L"Network Interface",
		counters_,
		&counterSize_,
		interfaces_,
		&interfaceSize_,
		PERF_DETAIL_WIZARD,
		0) != ERROR_SUCCESS)
	{
		delete[] counters_;
		delete[] interfaces_;
		return;
	}

	cur_ = interfaces_;
	
	for (int i = 0; *cur_ != L'\0' && i < PDH_ETHERNET_MAX; cur_ += wcslen(cur_) + 1, i++)
	{
		ethernetStruct_[i].use = true;
		ethernetStruct_[i].name_[0] = L'\0';
		wcscpy_s(ethernetStruct_[i].name_, cur_);
		query_[0] = L'\0';
		StringCbPrintf(query_, sizeof(WCHAR) * 1024, L"\\Network Interface(%s)\\Bytes Received/sec", cur_);
		PdhAddCounter(cpuQuery_, query_, NULL, &ethernetStruct_[i].recvBytes);
		query_[0] = L'\0';
		StringCbPrintf(query_, sizeof(WCHAR) * 1024, L"\\Network Interface(%s)\\Bytes Sent/sec", cur_);
		PdhAddCounter(cpuQuery_, query_, NULL, &ethernetStruct_[i].sendBytes);
	}

}
