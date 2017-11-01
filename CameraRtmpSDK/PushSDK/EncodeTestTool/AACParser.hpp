//
//  AACParser.hpp
//  CameraRtmpSDK
//
//  Created by Abson on 2016/11/26.
//  Copyright © 2016年 Abson. All rights reserved.
//

#ifndef AACParser_hpp
#define AACParser_hpp

#include <iostream>

/*
 #sampling_frequency_index
 There are 13 supported frequencies:
 0: 96000 Hz
 1: 88200 Hz
 2: 64000 Hz
 3: 48000 Hz
 4: 44100 Hz
 5: 32000 Hz
 6: 24000 Hz
 7: 22050 Hz
 8: 16000 Hz
 9: 12000 Hz
 10: 11025 Hz
 11: 8000 Hz
 12: 7350 Hz
 13: Reserved
 14: Reserved
 15: frequency is written explictly
 */

/*
 #channel_configuration
 0: Defined in AOT Specifc Config
 1: 1 channel: front-center
 2: 2 channels: front-left, front-right
 3: 3 channels: front-center, front-left, front-right
 4: 4 channels: front-center, front-left, front-right, back-center
 5: 5 channels: front-center, front-left, front-right, back-left, back-right
 6: 6 channels: front-center, front-left, front-right, back-left, back-right, LFE-channel
 7: 8 channels: front-center, front-left, front-right, side-left, side-right, back-left, back-right, LFE-channel
 8-15: Reserved
 */

namespace PushSDK { namespace ffmpeg {

    static const int ADTS_HEADER_SIZE = 56; // bits
    static const int ADTS_FIXED_HEADER_SIZE = 28; // bits
    static const int ADTS_VARIABEL_HEADER_SIZE = 28; // bits

    typedef struct ADTS_Fiex_Header{
        short syncword; // 12 bits #同步头 总是0xFFF, all bits must be 1，代表着一个ADTS帧的开始
        uint8_t id; // 1 bits  #MPEG Version: 0 for MPEG-4, 1 for MPEG-2
        uint8_t layer; // 2 bits #always: '00'
        uint8_t protenction_absent; // 1 bits #
        uint8_t profile; // 2 bits #表示使用哪个级别的AAC，有些芯片只支持AAC LC 。在MPEG-2 AAC中定义了3种：
        uint8_t sampling_frequency_index; // 4 bits # 表示使用的采样率下标，通过这个下标在 Sampling Frequencies[ ]数组中查找得知采样率的值。
        uint8_t private_bit; // 1 bits
        uint8_t channel_configuration; // 3 bits # 表示声道数
        uint8_t original_copy; // 1 bits
        uint8_t home; // 1 bits;
    }ADTS_Fiex_Header;

    typedef struct ADTS_Variable_Header{
        uint8_t copyright_identification_bit; // 1 bits
        uint8_t copyright_dientification_start; // 1 bits;
        short acc_frame_length; // 13 bits
        short adts_buffer_fullness; // 11 bits;
        uint8_t number_off_raw_data_blocks_in_frame; // 2 bits
    }ADTS_Variable_Header;

    typedef struct ADTS_Header{
        ADTS_Fiex_Header* fixe_h;
        ADTS_Variable_Header* variable_h;
    }ADTS_Header;

    typedef struct ADTS{
        ADTS_Header* header;
        uint8_t* data; // 原始码流数据
        int size; // 一个ADTS帧的长度包括ADTS头和AAC原始流.

        ~ADTS(){
            delete[] data;
        };
    }ADTS;

    using CaptureADTSCallback = std::function<void(std::shared_ptr<ADTS>)>;

    class AACParser {
    public:
        static void simplest_aac_parser(const uint8_t* buf, int len);

    private:
        static int get_adts_frame(const uint8_t* buf, int len, CaptureADTSCallback callback);
        static std::shared_ptr<ADTS> create_adts(const uint8_t* buf, int adts_len);

    private:
        uint8_t* m_aacbuffer;
    };
}
}

#endif /* AACParser_hpp */
