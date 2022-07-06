#include "videoCapture.h"


static void 
errno_exit(const char *s)
{
    fprintf(stderr, "%s error %d, %s\n", s, errno, strerror(errno));
    exit(EXIT_FAILURE);
}

static int 
xioctl(int fh, int request, void *arg)
{
    int r;

    do {
        r = ioctl(fh, request, arg);
    } while (-1 == r && EINTR == errno);

    return r;
}

videoCapture_t *
videoCapture_createVideoCapture(void)
{
    videoCapture_t *video = (videoCapture_t *)malloc(sizeof(videoCapture_t));
    video->deviceName = "/dev/video0";
    video->fileDescriptor = -1;
    return video;
}

struct stat 
videoCapture_getStatusDevice(videoCapture_t *video)
{
    struct stat st;

    if (stat(video->deviceName, &st) == -1)
    {
        fprintf(stderr, "Cannot identify: %s (%s)\n", video->deviceName, strerror(errno));
        exit(EXIT_FAILURE);
    }

    printf("ID of Camera Device:  [%jx,%jx]\n", (uintmax_t) major(st.st_dev),
        (uintmax_t) minor(st.st_dev));
    printf("File type:                ");

    switch (st.st_mode & S_IFMT) {
    case S_IFBLK:  printf("block device\n");            break;
    case S_IFCHR:  printf("character device\n");        break;
    case S_IFDIR:  printf("directory\n");               break;
    case S_IFIFO:  printf("FIFO/pipe\n");               break;
    case S_IFLNK:  printf("symlink\n");                 break;
    case S_IFREG:  printf("regular file\n");            break;
    case S_IFSOCK: printf("socket\n");                  break;
    default:       printf("unknown?\n");                break;
    }

    printf("I-node number:            %ju\n", (uintmax_t) st.st_ino);

    printf("Mode:                     %jo (octal)\n",
                   (uintmax_t) st.st_mode);

    printf("Link count:               %ju\n", (uintmax_t) st.st_nlink);
    printf("Ownership:                UID=%ju   GID=%ju\n",
            (uintmax_t) st.st_uid, (uintmax_t) st.st_gid);

    printf("Preferred I/O block size: %jd bytes\n",
            (intmax_t) st.st_blksize);
    printf("File size:                %jd bytes\n",
            (intmax_t) st.st_size);
    printf("Blocks allocated:         %jd\n",
                   (intmax_t) st.st_blocks);

    printf("Last status change:       %s", ctime(&st.st_ctime));
    printf("Last file access:         %s", ctime(&st.st_atime));
    printf("Last file modification:   %s", ctime(&st.st_mtime));
    return st;
}

int 
videoCapture_openDevice(videoCapture_t *video)
{
    video->fileDescriptor = open(video->deviceName, O_RDWR | O_NONBLOCK, 0);
    return video->fileDescriptor;
}

int 
videoCapture_closeDevice(videoCapture_t *video)
{
    return close(video->fileDescriptor);
}

struct v4l2_capability 
videoCapture_queryCapability(videoCapture_t *video)
{
    struct v4l2_capability cap;

    if (xioctl(video->fileDescriptor, VIDIOC_QUERYCAP, &cap) == -1) 
    {
        if (EINVAL == errno) 
        {
            fprintf(stderr, "%s is no V4L2 device\n", video->deviceName);
            exit(EXIT_FAILURE);
        } 
        else 
        {
            errno_exit("VIDIOC_QUERYCAP");
        }
    }

    printf("Driver:                 %s\n", cap.driver);
    printf("Device:                 %s\n", cap.card);
    printf("Bus info:               %s\n", cap.bus_info);
    printf("Version:                %u.%u.%u\n", (cap.version >> 16) & 0xFF, 
        (cap.version >> 8) & 0xFF, cap.version & 0xFF);

    return cap;
}

struct v4l2_cropcap     
videoCapture_cropCap(videoCapture_t *video, __u32 type, struct v4l2_rect *bounds, struct v4l2_rect *defrect, struct v4l2_fract *pixelaspect)
{
    struct v4l2_cropcap     cropcap;
    struct v4l2_crop        crop;

    memset(&cropcap, 0, sizeof(cropcap));

    cropcap.type = type;
    if (bounds != NULL)
    {
        cropcap.bounds = *bounds;
    } 

    if (defrect != NULL)
    {
        cropcap.defrect = *defrect;
    }

    if (pixelaspect != NULL)
    {
        cropcap.pixelaspect = *pixelaspect;
    }

    if (xioctl(video->fileDescriptor, VIDIOC_CROPCAP, &cropcap) == 0)
    {
        crop.type = type;
        crop.c = cropcap.defrect;

        if (xioctl(video->fileDescriptor, VIDIOC_S_CROP, &crop))
        {
            switch (errno) 
            {
            case EINVAL:
                /* Cropping not supported. */
                fprintf(stderr, "Cropping not supported\n");
                break;
            default:
                /* Errors ignored. */
                fprintf(stderr, "Error Occured when Crop Cap\n");
                break;
            }
        }
    }
    else 
    {
        fprintf(stderr, "Error Occured when Crop Cap\n");
    }
    return cropcap;
}

struct v4l2_format 
videoCapture_setFormat(videoCapture_t *video, __u32 type, __u32 width , __u32 height, __u32 pixelformat, __u32 field, __u32 bytesperline, __u32 sizeimage, __u32 colorspace)
{
    struct v4l2_format fmt;
    memset(&fmt, 0, sizeof(fmt));
    fmt.type = type;
    fmt.fmt.pix.width = width;
    fmt.fmt.pix.height = height;
    fmt.fmt.pix.pixelformat = pixelformat;
    fmt.fmt.pix.field = field;
    fmt.fmt.pix.bytesperline = bytesperline;
    fmt.fmt.pix.sizeimage = sizeimage;
    fmt.fmt.pix.colorspace = colorspace;
    if (xioctl(video->fileDescriptor, VIDIOC_S_FMT, &fmt) == -1)
    {
        fprintf(stderr, "Setting Format Failed\n");
        exit(EXIT_FAILURE);
    }
    return fmt;
}

struct v4l2_format 
videoCapture_getFormat(videoCapture_t *video)
{
    struct v4l2_format fmt;
    memset(&fmt, 0, sizeof(fmt));
    if (xioctl(video->fileDescriptor, VIDIOC_G_FMT, &fmt) == -1)
    {
        fprintf(stderr, "Get Format Failed\n");
        exit(EXIT_FAILURE);
    }
    return fmt;
}

struct v4l2_requestbuffers 
videoCapture_setRequestBuffers(videoCapture_t *video, __u32 numberBuffer, __u32 type, __u32 memory)
{
    struct v4l2_requestbuffers req;
    memset(&req, 0, sizeof(req));
    req.count = numberBuffer;
    req.type = type;
    req.memory = memory;
    if (xioctl(video->fileDescriptor, VIDIOC_REQBUFS, &req) == -1)
    {
        if (EINVAL == errno) 
        {
            fprintf(stderr, "%s does not support memory mapping\n", video->deviceName);
        } 
        else 
        {
            fprintf(stderr, "Request Buffer failed\n");
        }
        exit(EXIT_FAILURE);
    }
    return req;
}

struct v4l2_buffer 
videoCapture_setBuffer(videoCapture_t *video, __u32 index, __u32 type, __u32 memory)
{
    struct v4l2_buffer buf;
    buf.index = index;
    buf.type = type;
    buf.memory = memory;
    if (xioctl(video->fileDescriptor, VIDIOC_QUERYBUF, &buf) == -1)
    {
        fprintf(stderr, "Query Buffer Failed\n");
        exit(EXIT_FAILURE);
    }
    return buf;
}

void 
videoCapture_freeBuffer(videoCapture_t *video, struct videoBuffer **buffer, __u32 bufferSize)
{
    for(int i = 0; i < bufferSize; i++)
    {
        if (munmap((*buffer)[i].start, (*buffer)[i].length) == -1)
        {
            fprintf(stderr, "Free Memory Map Failed\n");
            exit(EXIT_FAILURE);
        }
    }
    free(*buffer);
}

struct v4l2_buffer 
videoCapture_setBufferCapture(videoCapture_t *video, __u32 index, __u32 type, __u32 memory)
{
    struct v4l2_buffer buf;
    memset(&buf, 0, sizeof(buf));
    buf.type = type;
    buf.memory = memory;
    buf.index = index;
    if (xioctl(video->fileDescriptor, VIDIOC_QBUF, &buf) == -1)
    {
        fprintf(stderr, "Setting Buffer for Capturing Failed\n");
        exit(EXIT_FAILURE);
    }
    return buf;
}

int 
videoCapture_streamOn(videoCapture_t *video)
{
    enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (xioctl(video->fileDescriptor, VIDIOC_STREAMON, &type) == -1)
    {
        return 0;
    }
    return 1;
}

int 
videoCapture_streamOff(videoCapture_t *video)
{
    enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (xioctl(video->fileDescriptor, VIDIOC_STREAMOFF, &type) == -1)
    {
        return 0;
    }
    return 1;
}



void *
videoCapture_getImageTimeout(videoCapture_t *video, struct videoBuffer *frame, int sec, int usec, __u32 type, __u32 memory, size_t numberBuffer, int *sizeFrame)
{
    void *tmp;
    while (1)
    {
        fd_set fds;
        struct timeval tv;
        int r;

        FD_ZERO(&fds);
        FD_SET(video->fileDescriptor, &fds);

        tv.tv_sec = sec;
        tv.tv_usec = usec;

        r = select(video->fileDescriptor + 1, &fds, NULL, NULL, &tv);

        if (r == -1)
        {
            if (errno == EINTR) 
                continue;
            fprintf(stderr, "Select function failed\n");
            exit(EXIT_FAILURE);
        }

        if (r == 0)
        {
            fprintf(stderr, "select timeout\n");
            exit(EXIT_FAILURE);
        }
        
        tmp = videoCapture_readFrame(video, frame, type, memory, numberBuffer, sizeFrame);
        if (tmp != NULL)
        {
            return tmp;
        }
    }
}

void *
videoCapture_readFrame(videoCapture_t *video, struct videoBuffer *frame, __u32 type, __u32 memory, size_t numberBuffer, int *sizeFrame)
{
    struct v4l2_buffer buf;
    buf.type = type;
    buf.memory = memory;

    if (xioctl(video->fileDescriptor, VIDIOC_DQBUF, &buf) == -1)
    {
        switch (errno)
        {
        case EAGAIN:
            return NULL;
        case EIO:
        default:
            fprintf(stderr, "Read Frame Error\n");
            return NULL;
        }
    }

    assert(buf.index < numberBuffer);

    *sizeFrame = buf.bytesused;

    if (xioctl(video->fileDescriptor, VIDIOC_QBUF, &buf) == -1)
    {
        fprintf(stderr, "Process Image failed\n");
        return NULL;
    }

    return frame[buf.index].start;
}
