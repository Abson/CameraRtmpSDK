//
//  H264Parser.cpp
//  CameraRtmpSDK
//
//  Created by Abson on 2016/11/14.
//  Copyright © 2016年 Abson. All rights reserved.
//

#include "H264Parser.hpp"

#ifdef __cplusplus
extern "C"
{
#endif
  
#include <libavcodec/avcodec.h>
#ifdef __cplusplus
};
#endif

typedef enum {
    NALU_TYPE_SLICE    = 1,
    NALU_TYPE_DPA      = 2,
    NALU_TYPE_DPB      = 3,
    NALU_TYPE_DPC      = 4,
    NALU_TYPE_IDR      = 5,
    NALU_TYPE_SEI      = 6,
    NALU_TYPE_SPS      = 7, // 序列参数集
    NALU_TYPE_PPS      = 8, // 图像参数集
    NALU_TYPE_AUD      = 9,
    NALU_TYPE_EOSEQ    = 10,
    NALU_TYPE_EOSTREAM = 11,
    NALU_TYPE_FILL     = 12,
} NaluType;

typedef enum {
    NALU_PRIORITY_DISPOSABLE = 0,
    NALU_PRIRITY_LOW         = 1,
    NALU_PRIORITY_HIGH       = 2,
    NALU_PRIORITY_HIGHEST    = 3
} NaluPriority;

typedef struct {
    int startcodeprefix_len;      //! 4 for parameter sets and first slice in picture, 3 for everything else (suggested)
    unsigned len;                 //! Length of the NAL unit (Excluding the start code, which does not belong to the NALU)
    unsigned max_size;            //! Nal Unit Buffer size
    int forbidden_bit;            //! should be always FALSE
    int nal_reference_idc;        //! NALU_PRIORITY_xxxx
    int nal_unit_type;            //! NALU_TYPE_xxxx
    unsigned char *buf;                    //! contains the first byte followed by the EBSP
}NALU_t;

int info2=0, info3=0;

static int last = 0;

static int FindStartCode2 (const unsigned char *Buf){

//    printf("%d %d %d\n", Buf[0], Buf[1], Buf[2]);

//    if(Buf[0]!=0 || Buf[1]!=0 || Buf[2] !=1) return 0; //0x000001?
//    else return 1;
    if (Buf[0] == 0 && Buf[1] == 0 && Buf[2] == 1) {
        return 1;
    }
    return 0;
}

static int FindStartCode3 (const unsigned char *Buf){
//    printf("%d %d %d %d\n", Buf[0], Buf[1], Buf[2], Buf[3]);

//    if(Buf[0]!=0 || Buf[1]!=0 || Buf[2] !=0 || Buf[3] !=1) return 0;//0x00000001?
//    else return 1;
    if (Buf[0] == 0 && Buf[1] == 0 && Buf[2] == 0 && Buf[3] == 1) {
        return 1;
    }
    return 0;
}

namespace VideoCore { namespace Parser {

    int GetAnnexbNALU (NALU_t& nalu, const uint8_t* buf, int len);

    void
    H264Parser::simplest_h264_parser(uint8_t *buf, int len)
    {
        std::shared_ptr<NALU_t> nalu = std::make_shared<NALU_t>();
        int buffersize=100000;
        
        FILE *myout=stdout;
        int data_offset=0;
        int nal_num=0;
        nalu->max_size = buffersize;
        nalu->buf = new unsigned char[buffersize]();

        int i = 0;
        int data_lenth = 0;

        printf("-----+-------- NALU Table ------+---------+\n");
        printf(" NUM |    POS  |    IDC |  TYPE |   LEN   |\n");
        printf("-----+---------+--------+-------+---------+\n");
        last = 0;
//        uint8_t* buf = packet.buf->data;
//        uint8_t len = packet.buf->size;
        while (i < len) {
            buf += data_lenth;
            len -= data_lenth;
            data_lenth= VideoCore::Parser::GetAnnexbNALU(*nalu.get(), buf, len);
            i += data_lenth;

            char type_str[20]={0};
            switch(nalu->nal_unit_type){
                case NALU_TYPE_SLICE:sprintf(type_str,"SLICE");break;
                case NALU_TYPE_DPA:sprintf(type_str,"DPA");break;
                case NALU_TYPE_DPB:sprintf(type_str,"DPB");break;
                case NALU_TYPE_DPC:sprintf(type_str,"DPC");break;
                case NALU_TYPE_IDR:sprintf(type_str,"IDR");break;
                case NALU_TYPE_SEI:sprintf(type_str,"SEI");break;
                case NALU_TYPE_SPS:sprintf(type_str,"SPS");break;
                case NALU_TYPE_PPS:sprintf(type_str,"PPS");break;
                case NALU_TYPE_AUD:sprintf(type_str,"AUD");break;
                case NALU_TYPE_EOSEQ:sprintf(type_str,"EOSEQ");break;
                case NALU_TYPE_EOSTREAM:sprintf(type_str,"EOSTREAM");break;
                case NALU_TYPE_FILL:sprintf(type_str,"FILL");break;
            }
            char idc_str[20]={0};
            switch(nalu->nal_reference_idc>>5){
                case NALU_PRIORITY_DISPOSABLE:sprintf(idc_str,"DISPOS");break;
                case NALU_PRIRITY_LOW:sprintf(idc_str,"LOW");break;
                case NALU_PRIORITY_HIGH:sprintf(idc_str,"HIGH");break;
                case NALU_PRIORITY_HIGHEST:sprintf(idc_str,"HIGHEST");break;
            }

            fprintf(myout,"%5d| %8d| %7s| %6s| %8d|\n",nal_num,data_offset,idc_str,type_str,nalu->len);
            
            data_offset=data_offset+data_lenth;  
            
            nal_num++;
        }


        delete[] nalu->buf;
    }

    int GetAnnexbNALU (NALU_t& nalu, const uint8_t* buf, int len){
        int pos = 0;
        int start_code_found, rewind;

        if (3 > len) {
            return 0;
        }
        info2 = FindStartCode2(buf);
        if (info2 != 1) {
            if (4 > len) {
                return 0;
            }
            info3 = FindStartCode3(buf);
            if (info3 != 1) {
                return -1;
            }
            else {
                pos = 4;
                nalu.startcodeprefix_len = 4;
            }
        }
        else {
            pos = 3;
            nalu.startcodeprefix_len = 3;
        }
        start_code_found = 0;
        info2 = 0;
        info3 = 0;
        //        unsigned char* buf = new unsigned char[nalu->max_size]();
        //        buf[pos] = 1;

        while (false == start_code_found) {
            if (pos >= len) {
                nalu.len = (pos - 1) - nalu.startcodeprefix_len;
                memcpy(nalu.buf, &buf[nalu.startcodeprefix_len], nalu.len);
                nalu.forbidden_bit = nalu.buf[0] & 0x80;
                nalu.nal_reference_idc = nalu.buf[0] & 0x60; // 2 bit
                nalu.nal_unit_type = (nalu.buf[0]) & 0x1f;// 5 bit
                return pos - 1;
            }

            pos++;

            info3 = FindStartCode3(&(buf[pos - 4]));
            if (info3 != 1) {
                info2 = FindStartCode2(&(buf[pos - 3]));
            }
            start_code_found = (info2 == 1 || info3 == 1);
        }

        rewind = (info3 == 1) ? 4 : 3;

        nalu.len = (pos - rewind) - nalu.startcodeprefix_len;
        memcpy(nalu.buf, &buf[nalu.startcodeprefix_len], nalu.len);
//        printf("nalu 中的内容：\n");
//        for (int i = 0; i < nalu.len; i++) {
//            printf(" %zi", nalu.buf[i]);
//        }
//        printf("\n");
        nalu.forbidden_bit = nalu.buf[0] & 0x80;
        nalu.nal_reference_idc = nalu.buf[0] & 0x60; // 2 bit
        nalu.nal_unit_type = (nalu.buf[0]) & 0x1f;// 5 bit

//        printf("nalu.nal_unit_type : %d", (nalu.buf[0]) & 0x1f);

        last += pos - rewind;

        return (pos - rewind);
    }
 
}
}


