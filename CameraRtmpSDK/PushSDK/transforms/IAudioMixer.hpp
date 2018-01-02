
#ifndef __IAduioMixer_hpp__
#define __IAduioMixer_hpp__

#include "IMetaData.hpp"

namespace push_sdk {
    enum  {
        kAudioMetadaFrequencyInHz,
        kAudioMetadaBitsPerChannel,
        kAudioMetadataChannelCount,
        kAudioMetadataFlags,
        kAudioMetadataBytesPerFrame,
        kAudioMetadataNumberFrames,
        kAudioMetadataUsesOSStruct,
        kAudioMetadataLoops,
        // kAudioMetadataSource
    };

    typedef  MetaData<'soun ', double, int, int, int, int, int, bool, bool> AudioBufferMetadata;
}

#endif
