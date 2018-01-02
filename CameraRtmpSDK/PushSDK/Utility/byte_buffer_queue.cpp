//
// Created by 何剑聪 on 2017/12/29.
// Copyright (c) 2017 Abson. All rights reserved.
//

#include "byte_buffer_queue.h"
#include <assert.h>


namespace push_sdk {namespace buffer {

    void BufferQueue::Write(const uint8_t* bytes, size_t len) {

      buffer_.append((char*)bytes, len);
    }

    uint8_t* BufferQueue::Read(size_t len) {

      assert(buffer_.length() >= len);

      std::string copy_str(buffer_.substr(0, len));
      buffer_.erase(buffer_.begin(), buffer_.begin()+len);
      const char *result = copy_str.c_str();

      return (uint8_t*)result;
    }
  }
}
