//
//  MicSource.m
//  CameraRtmpSDK
//
//  Created by Abson on 2016/11/23.
//  Copyright © 2016年 Abson. All rights reserved.
//

#include "MicSource.h"
#import <AVFoundation/AVFoundation.h>
#include "IAudioMixer.hpp"
#include <libavdevice/avdevice.h>

@interface InterruptionHandler : NSObject
{
@public
//    std::weak_ptr<PushSDK::Apple::MicSource> _source;
  push_sdk::Apple::MicSource* _source;
}

- (void)handleInterruption:(NSNotification*)aNotification;
@end

@implementation InterruptionHandler

- (void)handleInterruption:(NSNotification *)aNotification
{

}

@end


#define SYSTEM_VERSION_GREATER_THAN_OR_EQUAL_TO(v) ([[[UIDevice currentDevice] systemVersion] compare:v options:NSNumericSearch] != NSOrderedAscending)

static OSStatus handleAudioInputBuffer(
    void *							inRefCon,
    AudioUnitRenderActionFlags *	ioActionFlags,
    const AudioTimeStamp *			inTimeStamp,
    UInt32							inBusNumber,
    UInt32							inNumberFrames,
    AudioBufferList * __nullable	ioData
)
{
  push_sdk::Apple::MicSource* mc = static_cast<push_sdk::Apple::MicSource*>(inRefCon);

  AudioBuffer buffer;
  buffer.mData = NULL;
  buffer.mDataByteSize = 0;
  buffer.mNumberChannels = 0;

  AudioBufferList buffers;
  buffers.mNumberBuffers = 1;
  buffers.mBuffers[0] = buffer;

  OSStatus status = AudioUnitRender(mc->audioUnit(), ioActionFlags, inTimeStamp, inBusNumber, inNumberFrames, &buffers);

  if (!status) {
    mc->inputCallback((uint8_t*)buffers.mBuffers[0].mData, buffers.mBuffers[0].mDataByteSize, inNumberFrames);
  }

  return status;
}


namespace push_sdk { namespace Apple {

    constexpr static int kAudioBitPerChannel = 16;
    constexpr static int kAudioBitPerByte = 8;

    MicSource::MicSource(double sampleRate , int channelCount, ExcludeAudioUnitCallback excludeAudioUnit)
        : m_sampleRate(sampleRate), m_channelCount(channelCount), m_excludeAudioUnit(excludeAudioUnit), audio_unit_()
    {
      AudioUnitInitialize(audio_unit_);
    }

    MicSource::~MicSource()
    {
      stop();
    }

    void
    MicSource::start()
    {
      AVAudioSession* session = [AVAudioSession sharedInstance];
      [session setCategory:AVAudioSessionCategoryPlayAndRecord withOptions:AVAudioSessionCategoryOptionMixWithOthers | AVAudioSessionCategoryOptionDefaultToSpeaker error:nil];

      [session setPreferredSampleRate:m_sampleRate error:nil];
      [session setPreferredInputNumberOfChannels:m_channelCount error:nil];
      [session setPreferredIOBufferDuration:0.002 error:nil];
      [session setActive:true error:nil];

      __block MicSource* bThis = this;

      PermissionBlock permission = ^(BOOL granted) {

        if (granted) {


          AudioComponentDescription acd = {
              kAudioUnitType_Output,
              kAudioUnitSubType_RemoteIO,
              kAudioUnitManufacturer_Apple,
              0,
              0
          };

          bThis->m_component = AudioComponentFindNext(NULL, &acd);

          AudioComponentInstanceNew(bThis->m_component, &bThis->audio_unit_);

          uint32_t flagOne = 1;
          AudioUnitSetProperty(bThis->audio_unit_, kAudioOutputUnitProperty_EnableIO, kAudioUnitScope_Input, 1, &flagOne, sizeof(flagOne));
          AudioUnitSetProperty(bThis->audio_unit_, kAudioOutputUnitProperty_EnableIO, kAudioUnitScope_Input, 0, &flagOne, sizeof(flagOne));

          AudioStreamBasicDescription desc;
          desc.mSampleRate = m_sampleRate;
          desc.mFormatID = kAudioFormatLinearPCM;
          desc.mFormatFlags = (kAudioFormatFlagIsSignedInteger | kAudioFormatFlagIsPacked);
          desc.mChannelsPerFrame = static_cast<UInt32>(bThis->m_channelCount);
          desc.mBitsPerChannel = kAudioBitPerChannel;
          desc.mFramesPerPacket = 1;
          desc.mBytesPerFrame = desc.mBitsPerChannel / kAudioBitPerByte * desc.mChannelsPerFrame;
          desc.mBytesPerPacket = desc.mBytesPerFrame * desc.mFramesPerPacket;

          AudioUnitSetProperty(bThis->audio_unit_, kAudioUnitProperty_StreamFormat, kAudioUnitScope_Output, 1, &desc, sizeof(desc));
          AudioUnitSetProperty(bThis->audio_unit_, kAudioUnitProperty_StreamFormat, kAudioUnitScope_Input, 0, &desc, sizeof(desc));

          AURenderCallbackStruct cb;
          cb.inputProcRefCon = bThis;
          cb.inputProc = handleAudioInputBuffer;

          AudioUnitSetProperty(bThis->audio_unit_, kAudioOutputUnitProperty_SetInputCallback, kAudioUnitScope_Global, 1, &cb, sizeof(cb));

          bThis->m_interruptionHandler = [[InterruptionHandler alloc] init];
          bThis->m_interruptionHandler->_source = bThis;

          [[NSNotificationCenter defaultCenter] addObserver:bThis->m_interruptionHandler selector:@selector(handleInterruption:) name:AVAudioSessionInterruptionNotification object:nil];


          OSStatus ret = AudioOutputUnitStart(bThis->audio_unit_);
          if(ret != noErr) {
            std::cout << "Failed to start microphone!" << "\n";
          }
        }
      };

      if(SYSTEM_VERSION_GREATER_THAN_OR_EQUAL_TO(@"7.0")) {
        [session requestRecordPermission:permission];
      }
      else {
        permission(true);
      }
    }

    void
    MicSource::inputCallback(uint8_t *data, size_t data_size, uint32_t inNumberFrames)
    {
      auto output = m_output.lock();

      if (output) {
        AudioBufferMetadata md {};
        md.setData(m_sampleRate,
            16,
            m_channelCount,
            kAudioFormatFlagIsSignedInteger| kAudioFormatFlagIsPacked,
            2 * m_channelCount ,
            inNumberFrames,
            false,
            false);

        /* Write to file to test pcm whether available */
        WritePCMFileTest(data, data_size, 10);

        output->PushBuffer(data, data_size, md);
      }
    }

    void
    MicSource::stop()
    {
      if(audio_unit_) {
        [[NSNotificationCenter defaultCenter] removeObserver:m_interruptionHandler];
        m_interruptionHandler = NULL;

        AudioOutputUnitStop(audio_unit_);
        AudioComponentInstanceDispose(audio_unit_);
      }

      auto output = m_output.lock();
      if (output) {
        output->stop();
      }
    }

    void
    MicSource::setOutput(std::shared_ptr<IOutput> output)
    {
      m_output = output;
    }

    void
    MicSource::WritePCMFileTest(uint8_t *data, size_t data_size, int time) {
      short* pcm_data = (short*)data;
      NSMutableData *resData = [NSMutableData data];
      [resData appendBytes:pcm_data length:data_size];
      static int count = 0;
      count++;
      NSLog(@"buffer size: %zi index count %d",data_size, count);
      static NSMutableData* resAllData = [NSMutableData data];
      static NSString* mfileRes = [NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES)[0] stringByAppendingPathComponent:@"res_audio.pcm"];
      static int total_count = static_cast<int>(time / 0.002);
      if (count < total_count) {
        [resAllData appendData:resData];
      }else if(count == total_count){
        [resAllData writeToFile:mfileRes atomically:YES];
        NSLog(@"resAllData write to file");
      }
    }
  }
}