//
//  ABSSimpleSession.h
//  CameraRtmpSDK
//
//  Created by Abson on 16/9/5.
//  Copyright © 2016年 Abson. All rights reserved.
//

#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>

typedef NS_ENUM(NSInteger, ABSCameraState) {
    ABSCameraStateFront,
    ABSCameraStateBack
};

typedef NS_ENUM(NSInteger, ABSEncodeType) {
  ABSEncodeTypeOpus,
  ABSEncodeTypeAAC,
};

@class ABSSimpleSession;
@protocol ABSSessionDelegate <NSObject>

@optional

- (void)didAddCamearSource:(ABSSimpleSession*)session;

@end

@class ABSPreviewView;
@interface ABSSimpleSession : NSObject

@property (nonatomic, weak) id<ABSSessionDelegate> delegate;


/**
 Record Video Method

 @param videoSize
 @param fps
 @param bps
 @param useInterfaceOrientation
 @param cameraState
 @param previewFrame
 @return
 */
- (instancetype)initWithVideoSize:(CGSize)videoSize
                              fps:(int)fps
                          bitrate:(int)bps
          useInterfaceOrientation:(BOOL)useInterfaceOrientation
                      cameraState:(ABSCameraState)cameraState
                     previewFrame:(CGRect)previewFrame;


/**
 Record Audio

 @param rate
 @param startRecord
 @return
 */
- (instancetype)initWithAudioSampleRate:(float)rate
                                bitRate:(int)biteRate
                           channelCount:(int)count
                                 encode:(ABSEncodeType)type;

- (void)startVideoRecord;

- (void)startAudioRecord;

- (void)endAudioRecord;

- (void)endVidoeRecord;

- (ABSPreviewView *)previewView;

@end
