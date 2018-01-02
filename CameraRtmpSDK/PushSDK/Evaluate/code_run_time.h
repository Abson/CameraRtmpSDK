//
// Created by 何剑聪 on 2017/12/22.
// Copyright (c) 2017 Abson. All rights reserved.
//

#ifndef CAMERARTMPSDK_CODE_RUN_TIME_H
#define CAMERARTMPSDK_CODE_RUN_TIME_H

#include <vector>

#ifdef __MACH__
#include <mach/clock.h>
#include <mach/mach.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

int clock_gettime_ex(int clk_id, struct timespec* t);

#ifdef __cplusplus
}
#endif

#endif

namespace push_sdk { namespace evaluate {

    class code_run_time {
    public:
      code_run_time()  {}

      void StartClock() {
        clock_gettime_ex(CLOCK_PROCESS_CPUTIME_ID, &time_);
      };
      void EndClock() {
        struct timespec end_time = {0, 0};
        clock_gettime_ex(CLOCK_PROCESS_CPUTIME_ID, &end_time);
        result_ = {result_.tv_sec + (end_time.tv_sec - time_.tv_sec),
            result_.tv_nsec + (end_time.tv_nsec - time_.tv_nsec)};
      };

      // result time is ns(纳秒)
      long Result()  {
        long result = result_.tv_sec * 1000000000 + result_.tv_nsec;
        result_ = {0, 0};
        return result;
      }

      friend double AverageCodeRuntime(std::vector<long>& run_timers) {
        std::sort(run_timers.begin(), run_timers.end());

        run_timers.erase(run_timers.begin());
        run_timers.pop_back();

        long sum = 0;
        for (auto& time : run_timers) {
          sum += time;
        }
        double average = sum / run_timers.size();
        return average;
      }
    private:
      struct timespec time_;
      struct timespec result_;
    };

    double AverageCodeRuntime(std::vector<long>& run_timers);
  }
}





#endif //CAMERARTMPSDK_CODE_RUN_TIME_H
