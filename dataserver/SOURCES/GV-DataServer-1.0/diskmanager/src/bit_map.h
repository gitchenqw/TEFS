
#ifndef TFS_DATASERVER_BITMAP_H_
#define TFS_DATASERVER_BITMAP_H_

#include <inttypes.h> 
#include <string.h>

namespace tfs
{
  namespace dataserver
  {
    class BitMap
    {
      public:
        BitMap(const bool set_flag = false);
        BitMap(const uint32_t item_count, const bool set_flag = false);
        BitMap(const BitMap& rhs);
        BitMap& operator=(const BitMap& rhs);
        ~BitMap();

        void mount(const uint32_t item_count, const char* bitmap_data, const bool mount_flag = true);
        bool alloc(const uint32_t item_count, const bool set_flag);

        void copy(const uint32_t slot_count, const char* bitmap_data);
        bool test(const uint32_t pos) const;
        void set(const uint32_t pos);
        void reset(const uint32_t pos);

        inline void reset_all()
        {
          memset(data_, 0, slot_count_ * sizeof(char));
          set_count_ = 0;
        }

        inline char* get_data() const
        {
          return data_;
        }

        inline uint32_t get_set_count() const
        {
          return set_count_;
        }

        inline uint32_t get_slot_count() const
        {
          return slot_count_;
        }

        inline uint32_t get_item_count() const
        {
          return item_count_;
        }

      public:
        static const uint32_t SLOT_SIZE = 8 * sizeof(char);
        static const unsigned char BITMAPMASK[SLOT_SIZE];
        static const uint32_t INVALID_INDEX = 0xFFFFFFFF;

      private:
        uint32_t item_count_;          //total bit,maybe not the multiple = SLOT_SIZE
        uint32_t slot_count_;          //bitmap size,bytes
        mutable uint32_t set_count_;  //bit=1   number
        char* data_;                  //bitmap
        bool mount_;
    };

  }
}
#endif //TFS_DATASERVER_BITMAP_H_
