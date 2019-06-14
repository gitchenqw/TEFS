// get info from /proc/net/dev,caculate the machine's bandwidth usage;

#include <stdlib.h>
#include <string.h>
#include "cpu_metrics.h"
#include "common/internal.h"

namespace tfs
{
  namespace dataserver
  {
    using namespace common;

    BandMetrics::BandMetrics()
    {
       memset(&Band_data_, 0, sizeof(BandInfo));
       fp_ = NULL;
       refresh();
    }

    BandMetrics::~BandMetrics()
    {
      if (fp_ != NULL)
        fclose(fp_);
      fp_ = NULL;
    }

    int BandMetrics::refresh()
    {
      if (!fp_)
      {
        fp_ = fopen("/proc/net/dev", "r");
        if (!fp_)
        {
          valided_ = false;
          return TFS_ERROR;
        }
      }
      rewind(fp_);
      fflush(fp_);

      char* line = NULL;
      size_t len = 0;
      ssize_t read;
      int32_t num = 0;
      while ((read = getline(&line, &len, fp_)) != -1)
      {
        if (strstr(line, INTERFACE))
        {
            num
                = sscanf(
                    line,
                    INTERFACE": %" PRI64_PREFIX "u %" PRI64_PREFIX "u %" PRI64_PREFIX "u %" PRI64_PREFIX "u %" PRI64_PREFIX "u %" PRI64_PREFIX "u %" PRI64_PREFIX "u %" PRI64_PREFIX "u"
                          "%" PRI64_PREFIX "u %" PRI64_PREFIX "u %" PRI64_PREFIX "u %" PRI64_PREFIX "u %" PRI64_PREFIX "u %" PRI64_PREFIX "u %" PRI64_PREFIX "u %" PRI64_PREFIX "u",
                    Band_data_.rbytes,Band_data_.rpackets,Band_data_.rerrs,Band_data_.rdrop,Band_data_.rfifo,Band_data_.rframe,Band_data_.rcompressed,Band_data_.rmulticast,
                    Band_data_.tbytes,Band_data_.tpackets,Band_data_.terrs,Band_data_.tdrop,Band_data_.tfifo,Band_data_.tframe,Band_data_.tcompressed,Band_data_.tmulticast
                    );
            if (num < 16)
            {
              continue;
            }
          valided_ = true;
        }
      }
      if (line)
        ::free(line);
      return 0;
    }

    int CpuMetrics::save()
    {
      if (index >= cpu_count_)
        return TFS_ERROR;
#define save_m(a)  do { cpu_data_.a##_sav = cpu_data_.a;  } while (0);
      save_m(rbytes);
      save_m(tbytes);
      return TFS_SUCCESS;
    }

    int CpuMetrics::calc(UsageInfo& ret)
    {
#define calc_frme(a)  do { a##_frme = cpu_data_.a - cpu_data_.a##_sav; } while(0);
#define calc_usage(x, a) do {x.a = (float)a##_frme * scale; } while(0);
      if (index >= cpu_count_)
        return TFS_ERROR;
      float scale;
      //calc bytes received and sended in interval
      uint64_t rbytes_frme,tbytes_frme;
      calc_frme(rbytes);
      calc_frme(tbytes);
      
      //calc speed in 1s
      rbytes_frme = 1/timeinterval*rbytes_frme;
      tbytes_frme = 1/timeinterval*tbytes_frme;
      
      //calc the rate
      scale = 100.0 / MAX_BANDWIDTH;
      calc_usage(ret, rbytes);
      calc_usage(ret, tbytes);
      return TFS_SUCCESS;
    }

    int CpuMetrics::summary()
    {
      refresh();
      calc();
      save();
      return TFS_SUCCESS;
    }

  }
}
