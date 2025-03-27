// CameraCapture.mm

#import "webcam.h"

#include <iostream>

// The default nal separate is LENGTH!
void compressionOutputCallback(void *outputCallbackRefCon,
                               void *sourceFrameRefCon, OSStatus status,
                               VTEncodeInfoFlags infoFlags,
                               CMSampleBufferRef sampleBuffer) {
	if (!sampleBuffer || !CMSampleBufferDataIsReady(sampleBuffer)) {
		return;
	}

	// Here, handle the encoded H.264 sample buffer (e.g., write to file or
	// stream)
	int num_samples = CMSampleBufferGetNumSamples(sampleBuffer);

	std::cout << "Got frame encoded!! Sample count: " << num_samples
	          << std::endl;
	int sample_size = CMSampleBufferGetSampleSize(sampleBuffer, 0);
	std::cout << "Sample size: " << sample_size;

	// print sample size as hex
	printf(" (%08X) minus 4 (%08x)\n", sample_size, sample_size - 4);

	// Print the first 10 bytes of the frame
	CMBlockBufferRef blockBuffer = CMSampleBufferGetDataBuffer(sampleBuffer);
	size_t length = CMBlockBufferGetDataLength(blockBuffer);
	size_t offset = 0;
	char data[10] = {0};
	CMBlockBufferCopyDataBytes(blockBuffer, offset, 10, data);
	for (size_t i = 0; i < 10; i++) {
		printf("%02X ", (unsigned char)data[i]);
	}
	printf("\n");
}

@implementation CameraCapture {
	VTCompressionSessionRef compressionSession;
}

- (instancetype)init {
	self = [super init];
	if (self) {
		[self setupEncodingSession];
		[self setupCapture];
	}
	return self;
}

- (void)captureOutput:(AVCaptureOutput *)output
    didOutputSampleBuffer:(CMSampleBufferRef)sampleBuffer
           fromConnection:(AVCaptureConnection *)connection {

	CVImageBufferRef imageBuffer = CMSampleBufferGetImageBuffer(sampleBuffer);
	CMTime pts = CMSampleBufferGetPresentationTimeStamp(sampleBuffer);
	VTCompressionSessionEncodeFrame(self->compressionSession, imageBuffer, pts,
	                                kCMTimeInvalid, NULL, NULL, NULL);
}

- (void)setupEncodingSession {

	/**
	 * The following code is not yet implemented in the Objective-C version.
	 * The code is provided as a reference for the user to implement.
	 */

	int width = 1280, height = 720;

	VTCompressionSessionCreate(NULL, width, height, kCMVideoCodecType_H264,
	                           NULL, NULL, NULL, compressionOutputCallback,
	                           (__bridge void *)(self),
	                           &self->compressionSession);

	// Set encoding properties
	VTSessionSetProperty(self->compressionSession,
	                     kVTCompressionPropertyKey_RealTime, kCFBooleanTrue);
	VTSessionSetProperty(self->compressionSession,
	                     kVTCompressionPropertyKey_ProfileLevel,
	                     kVTProfileLevel_H264_Main_AutoLevel);
	VTSessionSetProperty(self->compressionSession,
	                     kVTCompressionPropertyKey_AllowFrameReordering,
	                     kCFBooleanFalse);

	// Prepare to encode
	VTCompressionSessionPrepareToEncodeFrames(self->compressionSession);
}

- (void)setupCapture {
	// self. is on objective
	self.session = [[AVCaptureSession alloc] init];

	[self.session beginConfiguration];

	if ([self.session canSetSessionPreset:AVCaptureSessionPreset1280x720]) {
		self.session.sessionPreset = AVCaptureSessionPreset1280x720;

	} else {
		// Handle the failure.
		std::cerr << "Failed to set session preset" << std::endl;
	}

	AVCaptureDevice *camera =
	    [AVCaptureDevice defaultDeviceWithMediaType:AVMediaTypeVideo];

	double desiredFPS = 30.0;
	[camera lockForConfiguration:nil];
	for (AVCaptureDeviceFormat *format in camera.formats) {
		for (AVFrameRateRange *range in format.videoSupportedFrameRateRanges) {
			if (range.minFrameRate <= desiredFPS &&
			    desiredFPS <= range.maxFrameRate) {
				camera.activeFormat = format;
				camera.activeVideoMinFrameDuration =
				    CMTimeMake(1, (int32_t)desiredFPS);
				camera.activeVideoMaxFrameDuration =
				    CMTimeMake(1, (int32_t)desiredFPS);
				NSLog(@"Frame rate set to %.2f fps", desiredFPS);
				[camera unlockForConfiguration];
				goto configuration_done;
			}
		}
	}

configuration_done:
	NSError *error = nil;
	AVCaptureDeviceInput *input =
	    [AVCaptureDeviceInput deviceInputWithDevice:camera error:&error];
	if (error) {
		NSLog(@"Camera error: %@", error.localizedDescription);
		return;
	}
	[self.session addInput:input];

	// https://developer.apple.com/library/archive/documentation/AudioVideo/Conceptual/AVFoundationPG/Articles/04_MediaCapture.html#//apple_ref/doc/uid/TP40010188-CH5-SW6
	AVCaptureVideoDataOutput *output = [[AVCaptureVideoDataOutput alloc] init];
	[output
	    setSampleBufferDelegate:self
	                      queue:dispatch_queue_create("WebCameraQueue",
	                                                  DISPATCH_QUEUE_SERIAL)];

	[self.session addOutput:output];

	[self.session commitConfiguration];
	[self.session startRunning];
}
@end
