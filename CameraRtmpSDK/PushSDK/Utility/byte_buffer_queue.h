//
// Created by 何剑聪 on 2017/12/29.
// Copyright (c) 2017 Abson. All rights reserved.
//

#ifndef CAMERARTMPSDK_BYTE_BUFFER_QUEUE_H
#define CAMERARTMPSDK_BYTE_BUFFER_QUEUE_H


#include <queue>
#include <string>

namespace push_sdk {namespace buffer {

    class BufferQueue {

    public:
      BufferQueue() {}
      ~BufferQueue() {}

      void Write(const uint8_t *bytes, size_t len);

      uint8_t* Read(size_t len);

      size_t availableBytes() const {
          return buffer_.length();
      }

      bool ThrowAway() {
        buffer_.erase(buffer_.begin(), buffer_.end());
        return true;
      }

    private:
      std::string buffer_;
    };
}
}



#endif //CAMERARTMPSDK_BYTE_BUFFER_QUEUE_H
