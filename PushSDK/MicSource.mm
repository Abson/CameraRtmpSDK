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

@interface InterruptionHandler : NSObject
{
@public
//    std::weak_ptr<PushSDK::Apple::MicSource> _source;
    PushSDK::Apple::MicSource* _source;
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
    PushSDK::Apple::MicSource* mc = static_cast<PushSDK::Apple::MicSource*>(inRefCon);

    AudioBuffer buffer;
    buffer.mData = NULL;
    buffer.mDataByteSize = 0;
    buffer.mNumberChannels = 2;

    AudioBufferList buffers;
    buffers.mNumberBuffers = 1;
    buffers.mBuffers[0] = buffer;

    OSStatus status = AudioUnitRender(mc->audioUnit(), ioActionFlags, inTimeStamp, inBusNumber, inNumberFrames, &buffers);

    if (!status) {
        mc->inputCallback((uint8_t*)buffers.mBuffers[0].mData, buffers.mBuffers[0].mDataByteSize, inNumberFrames);
    }

    return status;
}


namespace PushSDK { namespace Apple {

    MicSource::MicSource(double sampleRate , int channelCount, ExcludeAudioUnitCallback excludeAudioUnit)
    : m_sampleRate(sampleRate), m_channelCount(channelCount), m_excludeAudioUnit(excludeAudioUnit)
    {
    }

    MicSource::~MicSource()
    {
        stop();
    }

    void
    MicSource::start()
    {
        AVAudioSession* session = [AVAudioSession sharedInstance];

        __block MicSource* bThis = this;

        PermissionBlock permission = ^(BOOL granted) {

            if (granted) {
                [session setCategory:AVAudioSessionCategoryPlayAndRecord withOptions:AVAudioSessionCategoryOptionMixWithOthers | AVAudioSessionCategoryOptionDefaultToSpeaker error:nil];
                [session setActive:true error:nil];

                AudioComponentDescription acd = {
                    kAudioUnitType_Output,
                    kAudioUnitSubType_RemoteIO,
                    kAudioUnitManufacturer_Apple,
                    0,
                    0
                };

                bThis->m_component = AudioComponentFindNext(NULL, &acd);

                AudioComponentInstanceNew(bThis->m_component, &bThis->m_audioUnit);

                uint32_t flagOne = 1;
                AudioUnitSetProperty(bThis->m_audioUnit, kAudioOutputUnitProperty_EnableIO, kAudioUnitScope_Input, 1, &flagOne, sizeof(flagOne));

                AudioStreamBasicDescription desc;
                desc.mSampleRate = bThis->m_sampleRate;
                desc.mFormatID = kAudioFormatLinearPCM;
                desc.mFormatFlags = (kAudioFormatFlagIsSignedInteger | kAudioFormatFlagsNativeEndian | kAudioFormatFlagIsPacked);
                desc.mChannelsPerFrame = bThis->m_channelCount;
                desc.mBitsPerChannel = 16;
                desc.mFramesPerPacket = 1;
                desc.mBytesPerFrame = desc.mBitsPerChannel / 8 * m_channelCount;
                desc.mBytesPerPacket = desc.mBytesPerFrame * desc.mFramesPerPacket;

                AURenderCallbackStruct cb;
                cb.inputProcRefCon = this;
                cb.inputProc = handleAudioInputBuffer;

                AudioUnitSetProperty(bThis->m_audioUnit, kAudioUnitProperty_StreamFormat, kAudioUnitScope_Output, 1, &desc, sizeof(desc));
                AudioUnitSetProperty(bThis->m_audioUnit, kAudioOutputUnitProperty_SetInputCallback, kAudioUnitScope_Global, 1, &cb, sizeof(cb));

                bThis->m_interruptionHandler = [[InterruptionHandler alloc] init];
                bThis->m_interruptionHandler->_source = this;

                [[NSNotificationCenter defaultCenter] addObserver:bThis->m_interruptionHandler selector:@selector(handleInterruption:) name:AVAudioSessionInterruptionNotification object:nil];

                AudioUnitInitialize(bThis->m_audioUnit);

                OSStatus ret = AudioOutputUnitStart(bThis->m_audioUnit);
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
    MicSource::inputCallback(uint8_t *data, size_t data_size, int inNumberFrames)
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
            output->pushBuffer(data, data_size, md);
        }
    }

    void
    MicSource::stop()
    {
        if(m_audioUnit) {
            [[NSNotificationCenter defaultCenter] removeObserver:m_interruptionHandler];
            m_interruptionHandler = NULL;

            AudioOutputUnitStop(m_audioUnit);
            AudioComponentInstanceDispose(m_audioUnit);
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
}
}

