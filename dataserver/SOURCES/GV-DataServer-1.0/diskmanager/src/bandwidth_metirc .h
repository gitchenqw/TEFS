
#ifndef TFS_DATASERVER_BANDMETRICS_H_
#define TFS_DATASERVER_BANDMETRICS_H_

#include <stdio.h>
#include <stdint.h>
#include <sys/types.h>

namespace tfs
{
  namespace dataserver
  {
    class BandMetrics
    {
    public:
        #define  INTERFACE "eth5"
        static const  uint64_t MAX_BANDWIDTH = 1024*1024/8;
        struct BandInfo
        {// as represented in /proc/net/dev receive and transimit
            uint64_t rbytes,rpackets,rerrs,rdrop,rfifo,rframe,rcompressed,rmulticast;
            uint64_t tbytes,tpackets,terrs,tdrop,tfifo,tcolls,tcarrier,tcompressed; 
            uint64_t rbytes_save,tbytes_save;
        };

        struct UsageInfo
        {
            float tbytes,rbytes; // percent
        };

      public:
        BandMetrics();
        ~BandMetrics();

      public:
        inline bool is_valid() const
        {
          return valided_;
        }

        int32_t summary();
        inline const UsageInfo& get_usage(const uint32_t index) const
        {
          return usage_data_;
        }

      private:
        bool valided_;
        FILE* fp_;
        BandInfo Band_data_;
        UsageInfo usage_data_;

        int save();
        int calc(UsageInfo& ret);
        int refresh();
    };

  }
}
#endif 
