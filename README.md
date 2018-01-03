### A iOS project be used for recording video stream in real time

This project was created base in ffmpeg, which code with c++ as far as possible. By sampling and resampling the video stream, we encode the video/audio stream to H.264/AAC.
And also, we create H.264/AAC parser to help us understand that structure.
hope it can help you.

### Blog to Introduction

[利用FFmpeg 开发音视频流（三）——将视频 YUV 格式编码成 H264](http://simplecodesky.com/2016/08/18/%E5%88%A9%E7%94%A8FFmpeg-%E5%BC%80%E5%8F%91%E9%9F%B3%E8%A7%86%E9%A2%91%E6%B5%81-3/)

[深入浅出理解视频编码H264结构](http://simplecodesky.com/2016/11/15/%E6%B7%B1%E5%85%A5%E6%B5%85%E5%87%BA%E7%90%86%E8%A7%A3%E8%A7%86%E9%A2%91%E7%BC%96%E7%A0%81H264%E7%BB%93%E6%9E%84/)

i did write a blog to introduce this project , hope it can help you to comprehend it.

### How to Use
**If you want to test Vido (H.264编码)**

```object-c
self.session = [[ABSSimpleSession alloc] initWithVideoSize:CGSizeMake(640, 480)
                                                       fps:30
                                                   bitrate:1000000
                                   useInterfaceOrientation:true
                                               cameraState:ABSCameraStateFront previewFrame:self.view.bounds];
self.session.delegate = self;
[self.view addSubview:self.session.previewView];

[self.session startVideoRecord];

// close it 
[self.session endVidoeRecord];
```

**if you want to test audio for AAC (AAC编码)**

```object-c
self.session = [[ABSSimpleSession alloc] initWithAudioSampleRate:44100. channelCount:2];
self.session.delegate = self;
[self.session startAudioRecord];

// close it 
[self.session endAudioRecord];
```
**if you want to test audio for Opus (Opus编码)**
```
file ABSSimpleSession.m
modify the method of the file for ABSSimpleSession

- (void)startAudioRecord
{
  NSString* filePath = [self randomOpusPath];

  const char* file_path = [filePath cStringUsingEncoding:NSUTF8StringEncoding];
  opusEncoder_ = std::make_shared<push_sdk::ffmpeg::OpusEncoder>(self.audioSampleRate, self.audioChannelCount, 96000, file_path);
  micSource_->setOutput(opusEncoder_);
  micSource_->start();
}
```


**if you want to test amix（混音） of ffmpeg**
```
NSString* path1 = [[NSBundle mainBundle] pathForResource:@"audio1.wav" ofType:nil];
NSString* path2 = [[NSBundle mainBundle] pathForResource:@"audio2.wav" ofType:nil];
NSString* outputPath = [NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, true).lastObject 
stringByAppendingPathComponent:@"audio127.wav"];

const char* input1 = [path1 cStringUsingEncoding:NSUTF8StringEncoding];
const char* input2 = [path2 cStringUsingEncoding:NSUTF8StringEncoding];
const char* output = [outputPath cStringUsingEncoding:NSUTF8StringEncoding];

std::vector<std::string> inputs{std::string(input1), std::string(input2)};

PushSDK::ffmpeg::audio_mixer mixer(inputs, output);
mixer.StartMixAudio();
std::cout<< "Running time is: "<<static_cast<double>(end_time-start_time)/CLOCKS_PER_SEC*1000<<"ms"<< std::endl;//
```

### Update
2017.11.1 Update project to runnable and and modify test API, enjoy it.

2018.1.2 fix audio encode bug, make it encode correctly，enjoy it.

### feature
As far as this moment, This project looks very confusing，i will take more time make it more usable. hope you can supports me, leaving you stars, Thanks.
