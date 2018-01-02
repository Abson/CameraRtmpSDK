//
// Created by 何剑聪 on 2017/12/22.
// Copyright (c) 2017 Abson. All rights reserved.
//

#include "code_run_time.h"

int clock_gettime_ex(int clk_id, struct timespec* t)
{
  clock_serv_t cclock;
  mach_timespec_t mts;
  host_get_clock_service(mach_host_self(), CALENDAR_CLOCK, &cclock);
  clock_get_time(cclock, &mts);
  mach_port_deallocate(mach_task_self(), cclock);
  t->tv_sec = static_cast<__darwin_time_t>(mts.tv_sec);
  t->tv_nsec = mts.tv_nsec;
  return 0;
}

