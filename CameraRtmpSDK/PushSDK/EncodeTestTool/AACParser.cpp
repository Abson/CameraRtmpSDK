//
//  AACParser.cpp
//  CameraRtmpSDK
//
//  Created by Abson on 2016/11/26.
//  Copyright © 2016年 Abson. All rights reserved.
//

#include "AACParser.hpp"
#include <array>

// 一般情况下ADTS的头信息都是7个字节，分为2部分：
// adts_fixed_header();
// adts_variable_header();
namespace PushSDK { namespace ffmpeg {

    static std::array<std::string, 4> profile_array{"Main", "LC", "SSR", "unknown"};
    static std::array<std::string, 13> sampling_frequency_array{"96000Hz", "88200Hz", "64000Hz", "48000Hz", "44100Hz", "32000Hz", "24000Hz", "22050Hz", "16000Hz", "12000Hz", "11025Hz", "8000Hz", "unknown"};

    void
    AACParser::simplest_aac_parser(const uint8_t *buf, int len)
    {
        static int cnt = 0;
        get_adts_frame(buf, len, [=](std::shared_ptr<ADTS> adts){
            printf("%5d| %8s|  %8s| %5d|\n", cnt, profile_array[adts->header->fixe_h->profile].c_str(),
                   sampling_frequency_array[adts->header->fixe_h->sampling_frequency_index].c_str(),
                   adts->header->variable_h->acc_frame_length);
            cnt++;
        });
    }

    int
    AACParser::get_adts_frame(const uint8_t* buf, int len, CaptureADTSCallback callback)
    {
        int size = 0;
        if (!buf || len < 7) {
            return -1;
        }

        while (1) {
            if (len < 7) { // aac 头部为7个字节
                break;
            }

            if ((buf[0] == 0xff) && (buf[1] == 0xf0)) { // 通过同步头找到一个A DTS

                size |= (buf[3] & 0x03) << 11; // high 2 bit
                size |= buf[4] << 3; // middle 8 bit
                size |= (buf[5] & 0xe0) >> 5; // low 3 bit

                std::shared_ptr<ADTS> adts = create_adts(buf, size);
                callback(adts);
            }
            ++buf;
            len--;
        }

        return 0;
    }

    std::shared_ptr<ADTS>
    AACParser::create_adts(const uint8_t *buf, int adts_len)
    {
        ADTS_Fiex_Header fiex_header;
        fiex_header.syncword = (buf[0] << 4) | (buf[1] & 0xf0 >> 4);
        // 第 2 个字节
        fiex_header.id = (buf[1] & 0x08) >> 3; // 1 bit
        fiex_header.layer = (buf[1] & 0x6) >> 1; // 2 bit
        fiex_header.protenction_absent = buf[1] & 0x01; // 1 bit
        // 第 3 个字节
        fiex_header.profile = (buf[2] & 0xc0) >> 6; // 2 bit
        fiex_header.sampling_frequency_index = (buf[2] & 0x3c) >> 2;
        fiex_header.private_bit = (buf[2] & 0x02) >> 1; // 1 bit
        uint8_t channel_configuration = '\0';
        channel_configuration |= (buf[2] & 0x01) << 2; // high 1 bit
        // 第 4 个字节
        channel_configuration |= (buf[3] & 0xc0) >> 6; // low 2 bit
        fiex_header.channel_configuration = channel_configuration;
        fiex_header.original_copy = (buf[3] & 0x20) >> 5; // 1 bit
        fiex_header.home = (buf[3] & 0x10 ) >> 4; // 1 bit

        ADTS_Variable_Header var_header;
        var_header.copyright_identification_bit = (buf[3] & 0x08) >> 3; // 1 bit
        var_header.copyright_dientification_start = (buf[3] & 0x04) >> 2; // 1 bit
        int acc_frame_length = 0;
        acc_frame_length |= buf[3] & 0x03 << 1; // hight 2 bit
        // 第 5 个字节
        acc_frame_length |= buf[4] << 3; // middle 8 bit
        // 第 6 个字节
        acc_frame_length |= (buf[5] & 0xe0) >> 5; // low 3 bit
        var_header.acc_frame_length = acc_frame_length;
        int adts_buffer_fullness = 0;
        adts_buffer_fullness |= (buf[5] & 0x1f) << 6; // hight 5 bit;
        // 第 7 个字节
        adts_buffer_fullness |= (buf[6] & 0xfC) >> 2; // low 6 bit
        var_header.adts_buffer_fullness = adts_buffer_fullness;
        var_header.number_off_raw_data_blocks_in_frame = (buf[6] & 0x03); // 2 bit

        ADTS_Header header;
        header.fixe_h = &fiex_header;
        header.variable_h = &var_header;

        std::shared_ptr<ADTS> adts = std::make_shared<ADTS>();
        adts->header = &header;
        adts->size = adts_len;
        adts->data = new uint8_t[adts->size]();
        memcmp(adts->data, buf + 7, adts->size - 7);
        return adts;
    }
}
}
















