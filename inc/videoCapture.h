#ifndef _VIDEO_CAPTURE_H_
#define _VIDEO_CAPTURE_H_

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/sysmacros.h>
#include <linux/videodev2.h>
#include <sys/mman.h>
#include <assert.h>

struct videoBuffer {
    void   *start;
    size_t  length;
};

typedef struct videoCapture
{
    int fileDescriptor;
    char * deviceName;
} videoCapture_t;

videoCapture_t          
*videoCapture_createVideoCapture(void);

struct stat             
videoCapture_getStatusDevice(videoCapture_t *video);

int                     
videoCapture_openDevice(videoCapture_t *video);

int                     
videoCapture_closeDevice(videoCapture_t *video);

struct v4l2_capability  
videoCapture_queryCapability(videoCapture_t *video);

struct v4l2_cropcap     
videoCapture_cropCap(videoCapture_t *video, __u32 type, struct v4l2_rect *bounds, struct v4l2_rect *defrect, struct v4l2_fract *pixelaspect);

struct v4l2_format 
videoCapture_setFormat(videoCapture_t *video, __u32 type, __u32 width , __u32 height, __u32 pixelformat, __u32 field, __u32 bytesperline, __u32 sizeimage, __u32 colorspace);

struct v4l2_format 
videoCapture_getFormat(videoCapture_t *video);

struct v4l2_requestbuffers 
videoCapture_setRequestBuffers(videoCapture_t *video, __u32 numberBuffer, __u32 type, __u32 memory);

struct v4l2_buffer 
videoCapture_setBuffer(videoCapture_t *video, __u32 index, __u32 type, __u32 memory);

struct v4l2_buffer 
videoCapture_setBufferCapture(videoCapture_t *video, __u32 index, __u32 type, __u32 memory);

void 
videoCapture_freeBuffer(videoCapture_t *video, struct videoBuffer **buffer, __u32 bufferSize);

int 
videoCapture_streamOn(videoCapture_t *video);

int 
videoCapture_streamOff(videoCapture_t *video);

void *
videoCapture_getImageTimeout(videoCapture_t *video, struct videoBuffer *frame, int sec, int usec, __u32 type, __u32 memory, size_t numberBuffer, int *sizeFrame);

void *
videoCapture_readFrame(videoCapture_t *video, struct videoBuffer *frame, __u32 type, __u32 memory, size_t numberBuffer, int *sizeFrame);
#endif
