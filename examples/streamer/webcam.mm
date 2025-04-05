// CameraCapture.mm

#import "webcam.h"

#include <iostream>

int frame = 0;

// The default nal separate is LENGTH!
void compressionOutputCallback(void *outputCallbackRefCon,
                               void *sourceFrameRefCon, OSStatus status,
                               VTEncodeInfoFlags infoFlags,
                               CMSampleBufferRef sampleBuffer) {

	if (!sampleBuffer || !CMSampleBufferDataIsReady(sampleBuffer)) {
		return;
	}

	CameraCapture *context = (CameraCapture *)outputCallbackRefCon;

	// Print the first 10 bytes of the frame
	CMBlockBufferRef blockBuffer = CMSampleBufferGetDataBuffer(sampleBuffer);
	size_t offset = 0;
	// Get H.264 data
	size_t totalLength;
	char *dataPointer;
	CMBlockBufferGetDataPointer(blockBuffer, 0, NULL, &totalLength,
	                            &dataPointer);

	// Print the first 10 bytes
	printf("Frame data: ");
	for (size_t i = 0; i < 10; i++) {
		printf("%02X ", (unsigned char)dataPointer[i]);
	}
	printf("\n");

	// print frame size in hex
	printf("Frame size: %08x bytes\n", totalLength);

	CMTime pts = CMSampleBufferGetPresentationTimeStamp(sampleBuffer);
	static CMTime baseTime;
	if (CMTIME_IS_INVALID(baseTime)) {
		baseTime = pts;
	}

	double time = CMTimeGetSeconds(pts);
	NSLog(@"Sample time: %f seconds (%llu)", CMTimeGetSeconds(pts), time);

	// Check for keyframe (IDR)
	CFArrayRef attachments =
	    CMSampleBufferGetSampleAttachmentsArray(sampleBuffer, true);

	BOOL isKeyFrame = !CFDictionaryContainsKey(
	    (CFDictionaryRef)CFArrayGetValueAtIndex(attachments, 0),
	    kCMSampleAttachmentKey_NotSync);

	if (isKeyFrame) {
		CMFormatDescriptionRef formatDesc =
		    CMSampleBufferGetFormatDescription(sampleBuffer);

		const uint8_t *spsData, *ppsData;
		size_t spsSize, ppsSize;
		size_t spsCount, ppsCount;
		int len = ntohl(spsSize);
		// Extract SPS
		CMVideoFormatDescriptionGetH264ParameterSetAtIndex(
		    formatDesc, 0, &spsData, &spsSize, &spsCount, NULL);

		// Append size to SPS
		char *spsDataWithSize = (char *)malloc(spsSize + 4);
		len = ntohl(spsSize);
		memcpy(spsDataWithSize, &len, 4);
		memcpy(spsDataWithSize + 4, spsData, spsSize);
		spsSize += 4;

		context.on_frame(NULL, spsDataWithSize, spsSize,
		                 time - (((double)1.0f) / 30.0f));

		// Extract PPS
		CMVideoFormatDescriptionGetH264ParameterSetAtIndex(
		    formatDesc, 1, &ppsData, &ppsSize, &ppsCount, NULL);

		char *ppsDataWithSize = (char *)malloc(ppsSize + 4);
		len = ntohl(ppsSize);
		memcpy(ppsDataWithSize, &len, 4);
		memcpy(ppsDataWithSize + 4, ppsData, ppsSize);
		ppsSize += 4;
		context.on_frame(NULL, ppsDataWithSize, ppsSize,
		                 time - (((double)1.0f) / 30.0f));

		printf("SPS size: %zu, PPS size: %zu\n", spsSize, ppsSize);
		printf("SPS count: %zu, PPS count: %zu\n", spsCount, ppsCount);

		char *frameWithPPSAndSPS =
		    (char *)malloc(totalLength + spsSize + ppsSize);
		memcpy(frameWithPPSAndSPS, spsDataWithSize, spsSize);
		memcpy(frameWithPPSAndSPS + spsSize, ppsDataWithSize, ppsSize);
		memcpy(frameWithPPSAndSPS + ppsSize + spsSize, dataPointer,
		       totalLength);

		// print first 10 bytes of sps and pps
		printf("SPS data: ");
		for (size_t i = 0; i < spsSize; i++) {
			printf("%02X ", (unsigned char)spsDataWithSize[i]);
		}
		printf("\n");

		printf("PPS data: ");
		for (size_t i = 0; i < ppsSize; i++) {
			printf("%02X ", (unsigned char)ppsDataWithSize[i]);
		}
		printf("\n");

		context.on_frame(NULL, dataPointer, totalLength, time);
		// context.on_frame(NULL, frameWithPPSAndSPS,
		//                  totalLength + spsSize + ppsSize, time);

		free(spsDataWithSize);
		free(ppsDataWithSize);
		free(frameWithPPSAndSPS);
	} else {
		context.on_frame(NULL, dataPointer, totalLength, time);
	}

	// free(frame);
}

@implementation CameraCapture {
	VTCompressionSessionRef compressionSession;
}

- (instancetype)init:(on_frame_cb_t *)on_frame {
	self = [super init];
	if (self) {
		self.on_frame = on_frame;
		[self setupEncodingSession];
		[self setupCapture];
	}
	return self;
}

static int g_fps = 30;

- (void)captureOutput:(AVCaptureOutput *)output
    didOutputSampleBuffer:(CMSampleBufferRef)sampleBuffer
           fromConnection:(AVCaptureConnection *)connection {
	static int frameCount = 0;
	CVImageBufferRef imageBuffer = CMSampleBufferGetImageBuffer(sampleBuffer);
	// CMTime presentationTimeStamp =
	//     CMSampleBufferGetPresentationTimeStamp(sampleBuffer);
	// if (count++ == 0) {
	// 	first_time = CMTimeGetSeconds(pts);
	// }

	CMTime presentationTimeStamp = CMTimeMake(frameCount++, g_fps);

	// Create a duration for the frame of 1/25
	CMTime frameDuration = CMTimeMake(1, g_fps);

	VTCompressionSessionEncodeFrame(self->compressionSession, imageBuffer,
	                                presentationTimeStamp, frameDuration, NULL,
	                                NULL, NULL);
}

- (void)setupEncodingSession {

	/**
	 * The following code is not yet implemented in the Objective-C version.
	 * The code is provided as a reference for the user to implement.
	 */
	int width = 1280;
	int height = 720;

	VTCompressionSessionCreate(NULL, width, height, kCMVideoCodecType_H264,
	                           NULL, NULL, NULL, compressionOutputCallback,
	                           (__bridge void *)(self),
	                           &self->compressionSession);

	int32_t bitrate = 2000000;
	CFNumberRef bitrateRef =
	    CFNumberCreate(NULL, kCFNumberSInt32Type, &bitrate);

	OSStatus status = VTSessionSetProperty(
	    self->compressionSession, kVTCompressionPropertyKey_ConstantBitRate,
	    kCFBooleanTrue);
	VTSessionSetProperty(self->compressionSession,
	                     kVTCompressionPropertyKey_AverageBitRate, bitrateRef);

	// Set encoding properties

	VTSessionSetProperty(self->compressionSession,
	                     kVTCompressionPropertyKey_RealTime, kCFBooleanTrue);
	VTSessionSetProperty(self->compressionSession,
	                     kVTCompressionPropertyKey_ProfileLevel,
	                     kVTProfileLevel_H264_Baseline_3_1);
	VTSessionSetProperty(self->compressionSession,
	                     kVTCompressionPropertyKey_AllowFrameReordering,
	                     kCFBooleanFalse);
	VTSessionSetProperty(compressionSession,
	                     kVTCompressionPropertyKey_H264EntropyMode,
	                     kVTH264EntropyMode_CAVLC);
	int keyframeInterval = 1;
	CFNumberRef maxKeyFrameInterval = CFNumberCreate(
	    kCFAllocatorDefault, kCFNumberSInt32Type, &keyframeInterval);
	VTSessionSetProperty(self->compressionSession,
	                     kVTCompressionPropertyKey_MaxKeyFrameInterval,
	                     maxKeyFrameInterval);
	// Prepare to encode
	VTCompressionSessionPrepareToEncodeFrames(self->compressionSession);
}

- (void)setupCapture {
	// self. is on objective
	self.session = [[AVCaptureSession alloc] init];

	[self.session beginConfiguration];

	self.session.sessionPreset = AVCaptureSessionPreset1280x720;

	AVCaptureDevice *camera =
	    [AVCaptureDevice defaultDeviceWithMediaType:AVMediaTypeVideo];

	// for (AVCaptureDeviceFormat *format in camera.formats) {
	// 	for (AVFrameRateRange *range in format.videoSupportedFrameRateRanges) {
	// 		if (range.minFrameRate <= desiredFPS &&
	// 		    desiredFPS <= range.maxFrameRate) {
	// 			camera.activeFormat = format;

	// 			NSLog(@"Frame rate set to %.2f fps", desiredFPS);
	// 			[camera unlockForConfiguration];
	// 			goto configuration_done;
	// 		}
	// 	}
	// }
	[camera lockForConfiguration:nil];

	camera.activeVideoMinFrameDuration = CMTimeMake(1, (int32_t)g_fps);
	camera.activeVideoMaxFrameDuration = CMTimeMake(1, (int32_t)g_fps);
	[camera unlockForConfiguration];

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
	output.videoSettings = [NSDictionary
	    dictionaryWithObject:
	        [NSNumber numberWithUnsignedInt:
	                      kCVPixelFormatType_420YpCbCr8BiPlanarVideoRange]
	                  forKey:(__bridge NSString *)
	                             kCVPixelBufferPixelFormatTypeKey];
	[self.session addOutput:output];

	[self.session commitConfiguration];
	[self.session startRunning];
}
@end
