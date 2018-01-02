//
// Created by 何剑聪 on 2017/12/28.
// Copyright (c) 2017 Abson. All rights reserved.
//

//
//  MicSource.h
//  CameraRtmpSDK
//
//  Created by Abson on 2016/11/23.
//  Copyright © 2016年 Abson. All rights reserved.
//

#include <iostream>
#import <CoreAudioKit/CoreAudioKit.h>
#import "IOutput.hpp"

#ifndef __Apple_MicSource__h
#define __Apple_MicSource__h

@class InterruptionHandler;
namespace push_sdk { namespace Apple {

    class MicSource : std::enable_shared_from_this<MicSource> {

      using ExcludeAudioUnitCallback = std::function< void (AudioUnit&)>;

    public:
      MicSource(double sampleRate = 44100., int channelCount = 2, ExcludeAudioUnitCallback excludeAudioUnit = nullptr);

      ~MicSource();

      void start();

      void stop();

      void setOutput(std::shared_ptr<IOutput> output);

      const AudioUnit& audioUnit() const { return audio_unit_; }

      void inputCallback(uint8_t* data, size_t data_size, uint32_t inNumberFrames);
    private:
      double m_sampleRate;
      int m_channelCount;

      InterruptionHandler* m_interruptionHandler;

      AudioComponent m_component;
      AudioUnit audio_unit_;

      ExcludeAudioUnitCallback m_excludeAudioUnit;

      std::weak_ptr<IOutput> m_output;


      void WritePCMFileTest(uint8_t *data, size_t data_size, int time);
    };
  }
}


#endif