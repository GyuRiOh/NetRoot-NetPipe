

#ifndef __PDH__MONITOR__
#define __PDH__MONITOR__

#include <Pdh.h>
#include <stdio.h>
#pragma comment(lib,"Pdh.lib")
#include "Singleton.h"

constexpr int PDH_ETHERNET_MAX = 8;

namespace server_baby 
{
	class ProcessMonitor
	{
	public:
		class CpuUsageForProcess : public Singleton<ProcessMonitor::CpuUsageForProcess>
		{
			CpuUsageForProcess(HANDLE process = INVALID_HANDLE_VALUE);
		public:
			void Initialize(HANDLE process = INVALID_HANDLE_VALUE);
			void Destroy() {}
			void UpdateCpuTime();

			float ProcessTotal()
			{
				return processTotal_;
			}

			float ProcessUser()
			{
				return processUser_;
			}

			float ProcessKernel()
			{
				return processKernel_;
			}
		private:
			HANDLE process_;
			ULARGE_INTEGER processLastKernel_;
			ULARGE_INTEGER processLastUser_;
			ULARGE_INTEGER processLastTime_;

			float processTotal_;
			float processUser_;
			float processKernel_;

			int numberOfProcessors_;
		};

		class MemoryForProcess : public Singleton<ProcessMonitor::MemoryForProcess>
		{
			struct Ethernet
			{
				bool use;
				WCHAR name_[128];
				PDH_HCOUNTER recvBytes;
				PDH_HCOUNTER sendBytes;
			};
			MemoryForProcess(){}
			void UsePDHEnumObject();
		public:
			void Initialize();
			void Destroy(){}
			void Update();

			double GetPrivateBytes(const WCHAR* const processName)
			{
				WCHAR path[256] = { 0 };
				swprintf_s(path, L"\\Process(%ws)\\Private Bytes", processName);

				PdhAddCounter(cpuQuery_, path, NULL, &privateBytes_);
				PdhCollectQueryData(cpuQuery_);
				PdhGetFormattedCounterValue(privateBytes_, PDH_FMT_DOUBLE, NULL, &privateBytesVal_);

				return privateBytesVal_.doubleValue;
			}

			double GetAvailableBytes()
			{
				return availableBytesVal_.doubleValue;
			}

			double GetNonPagedByteForProcess(const WCHAR* const processName)
			{
				WCHAR path[256] = { 0 };
				swprintf_s(path, L"\\Process(%ws)\\Private Bytes", processName);

				PdhAddCounter(cpuQuery_, L"\\Process(%ws)\\Pool Nonpaged Bytes", NULL, &nonPagedByteForProcess_);
				PdhCollectQueryData(cpuQuery_);
				PdhGetFormattedCounterValue(nonPagedByteForProcess_, PDH_FMT_DOUBLE, NULL, &nonPagedByteForProcessVal_);

				return nonPagedByteForProcessVal_.doubleValue;
			}

			double GetNonPagedByte()
			{
				return nonPagedByteVal_.doubleValue;
			}

			double GetTotalRecvBytes()
			{
				return totalRecvBytes_;
			}

			double GetTotalSendBytes()
			{
				return totalSendBytes_;
			}

			void ZeroEthernetValue()
			{
				totalRecvBytes_ = 0;
				totalSendBytes_ = 0;
			}

		private:
			PDH_HQUERY cpuQuery_;

			PDH_HCOUNTER privateBytes_;
			PDH_HCOUNTER availableBytes_;
			PDH_HCOUNTER nonPagedByteForProcess_;
			PDH_HCOUNTER nonPagedByte_;

			PDH_FMT_COUNTERVALUE privateBytesVal_;
			PDH_FMT_COUNTERVALUE availableBytesVal_;
			PDH_FMT_COUNTERVALUE nonPagedByteForProcessVal_;
			PDH_FMT_COUNTERVALUE nonPagedByteVal_;


		private:
			Ethernet ethernetStruct_[PDH_ETHERNET_MAX]; //랜카드 별 pdh정보
			double totalRecvBytes_ = 0;
			double totalSendBytes_ = 0;

			bool err_ = false;
			WCHAR* cur_ = nullptr;
			WCHAR* counters_ = nullptr;
			WCHAR* interfaces_ = nullptr;
			DWORD counterSize_ = 0;
			DWORD interfaceSize_ = 0;
			WCHAR query_[1024] = { 0, };
			PDH_FMT_COUNTERVALUE recvVal_;
			PDH_FMT_COUNTERVALUE sendVal_;

		};
	};

	class HardwareMonitor 
	{
	public:
		class CpuUsageForProcessor : public Singleton<HardwareMonitor::CpuUsageForProcessor>
		{
			CpuUsageForProcessor(HANDLE process = INVALID_HANDLE_VALUE);
		public:	
			void Initialize(HANDLE process = INVALID_HANDLE_VALUE);
			void Destroy(){}
			void UpdateCpuTime();

			float ProcessorTotal()
			{
				return processorTotal_;
			}

			float ProcessorUser()
			{
				return processorUser_;
			}

			float ProcessorKernel()
			{
				return processorKernel_;
			}
		private:
			HANDLE process_;


			ULARGE_INTEGER processorLastKernel_;
			ULARGE_INTEGER processorLastUser_;
			ULARGE_INTEGER processorLastIdle_;

			float processorTotal_;
			float processorUser_;
			float processorKernel_;

			int numberOfProcessors_;
		};


	};
}

#endif