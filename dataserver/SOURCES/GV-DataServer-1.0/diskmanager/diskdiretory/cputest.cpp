#include "cpu_metrics.h"
#include "tbsys.h"
using namespace tfs::dataserver;
using namespace std;

int main()
{
    CpuMetrics cpuinfo;
    while(1)
    {
        cpuinfo.summary();
        CpuMetrics::UsageInfo usage = cpuinfo.get_usage(0);
        printf("Cpu(s): %f%%us,%f%%sy,%f%%ni,%f%%id\n",usage.u,usage.s,usage.n,usage.i);
        sleep(1);
    }
    return 0;
}