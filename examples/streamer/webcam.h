// objcpp header file for webcam.mm
// implement it
#import <AVFoundation/AVFoundation.h>
#import <VideoToolbox/VideoToolbox.h>

typedef void (on_frame_cb_t)(void* context, char* data, int size, double sample_time_seconds);


@interface CameraCapture
: NSObject <AVCaptureVideoDataOutputSampleBufferDelegate>
@property(nonatomic, strong) AVCaptureSession *session;
@property(nonatomic) on_frame_cb_t* on_frame;
@end
