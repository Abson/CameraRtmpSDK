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

@class ABSSimpleSession;
@protocol ABSSessionDelegate <NSObject>

@optional

- (void)didAddCamearSource:(ABSSimpleSession*)session;

@end

@class ABSPreviewView;
@interface ABSSimpleSession : NSObject

@property (nonatomic, weak) id<ABSSessionDelegate> delegate;

- (instancetype)initWithVideoSize:(CGSize)videoSize
                              fps:(int)fps
                          bitrate:(int)bps
          useInterfaceOrientation:(BOOL)useInterfaceOrientation
                      cameraState:(ABSCameraState)cameraState
                     previewFrame:(CGRect)previewFrame;

- (void)startRecord;

- (void)endRecord;

- (ABSPreviewView *)previewView;

@end
