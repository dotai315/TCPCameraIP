#include "main.h"

#define PORT 8080

int clientFileDescriptor;
videoCapture_t              *video;
struct v4l2_requestbuffers  req;
struct videoBuffer          *buffers;
struct stat                 st;
FILE                        *fp1;
int                         sizeFrame;

int main(void)
{
    int serverFileDescriptor = -1;
    struct sockaddr_in serverAddr;
    struct sockaddr_in clientAddr;
    size_t lenClientAddr;
    pid_t childProc;


    video = videoCapture_createVideoCapture();
    st = videoCapture_getStatusDevice(video);
    //Encoder                     en;
    //uint8_t                     *h264_buf = (uint8_t *) malloc(sizeof(uint8_t) * 480 * 360 * 2);

    if (!S_ISCHR(st.st_mode))
    {
        fprintf(stderr, "%s is no device\n", video->deviceName);
        exit(EXIT_FAILURE);
    }

    printf("Video Capture Open Device...\n");
    if (videoCapture_openDevice(video) == -1)
    {
        fprintf(stderr, "Cannot open '%s': %d, %s\n", video->deviceName, errno, strerror(errno));
        exit(EXIT_FAILURE);
    }

    printf("Video Capabilities...\n");
    struct v4l2_capability cap = videoCapture_queryCapability(video);
    if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE))
    {
        fprintf(stderr, "%s is no device capture\n", video->deviceName);
        exit(EXIT_FAILURE);
    }

    if (!(cap.capabilities & V4L2_CAP_STREAMING))
    {
        fprintf(stderr, "%s does not support streaming i/o\n", video->deviceName);
        exit(EXIT_FAILURE);
    }

    printf("Cropping...\n");
    videoCapture_cropCap(video, V4L2_BUF_TYPE_VIDEO_CAPTURE, NULL, NULL, NULL);
    
    printf("Formating...\n");
    videoCapture_setFormat(video, V4L2_BUF_TYPE_VIDEO_CAPTURE, 480, 360, V4L2_PIX_FMT_MJPEG, V4L2_FIELD_INTERLACED, 480 * 2, 480 * 360 * 2, 0);

    printf("Request Buffer...\n");
    req = videoCapture_setRequestBuffers(video, 4, V4L2_BUF_TYPE_VIDEO_CAPTURE, V4L2_MEMORY_MMAP);

    if (req.count < 2)
    {
        fprintf(stderr, "Insufficient buffer memory on %s\n", video->deviceName);
        exit(EXIT_FAILURE);
    }

    buffers = calloc(req.count, sizeof(*buffers));
    if (!buffers)
    {
        fprintf(stderr, "Out of memory\n");
        exit(EXIT_FAILURE);
    }

    printf("Setting Buffer with MMap Method...\n");

    for(int i = 0; i < req.count; i++)
    {
        struct v4l2_buffer buf = videoCapture_setBuffer(video, i, V4L2_BUF_TYPE_VIDEO_CAPTURE, V4L2_MEMORY_MMAP);
        buffers[i].length = buf.length;
        buffers[i].start = mmap(NULL, buf.length, PROT_READ | PROT_WRITE, MAP_SHARED, video->fileDescriptor, buf.m.offset);

        if (buffers[i].start == MAP_FAILED)
        {
            fprintf(stderr, "Memory Map Failed\n");
            exit(EXIT_FAILURE);
        }
    }

    printf("Start Capturing...\n");
    for(int i = 0; i < req.count; i++)
    {
        videoCapture_setBufferCapture(video, i, V4L2_BUF_TYPE_VIDEO_CAPTURE, V4L2_MEMORY_MMAP);
    }

    printf("Streaming On...\n");
    if (videoCapture_streamOn(video) == 0)
    {
        fprintf(stderr, "Turn on stream failed\n");
        exit(EXIT_FAILURE);
    }

    printf("Capturing...\n");
    printf("Creating socket...\n");
    serverFileDescriptor = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (serverFileDescriptor < 0)
    {
        fprintf(stderr, "Cannot create socket: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
    printf("Socket Create Successful...\n");

    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT);

    printf("Binding...\n");
    if (bind(serverFileDescriptor, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) != 0)
    {
        fprintf(stderr, "Could not bind: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
    printf("Bind Successful...\n");

    if (listen(serverFileDescriptor, 3) != 0)
    {
        fprintf(stderr, "Listen Failed: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
    printf("Listening...\n");
    while (1)
    {   
        // if (pthread_create(&clientThreads[indexClient++], NULL, stream, &clientFileDescriptor) != 0)
        // {
        //     fprintf(stderr, "Failed to create thread\n");
        // }
        // if (indexClient >= 50)
        // {
        //     indexClient = 0;

        //     while(indexClient < 50)
        //     {
        //         pthread_join(clientThreads[indexClient++], NULL);
        //     }

        //     indexClient = 0;
        // }
        lenClientAddr = sizeof(clientAddr);
        clientFileDescriptor = accept(serverFileDescriptor, (struct sockaddr *)&clientAddr, (socklen_t *)&lenClientAddr);
        if (clientFileDescriptor < 0)
        {
            fprintf(stderr, "Cannot accept client\n");
            continue;
        }
        printf("Connection accepted from %s:%d\n", inet_ntoa(clientAddr.sin_addr), ntohs(clientAddr.sin_port));
        if ((childProc = fork()) == 0)
        {
            close(serverFileDescriptor);

            while (1)
            {
                write(clientFileDescriptor, 
                        videoCapture_readFrame(video, buffers, V4L2_BUF_TYPE_VIDEO_CAPTURE, V4L2_MEMORY_MMAP, req.count, &sizeFrame),
                        sizeFrame
                    );
            }
        }
    }
    close(clientFileDescriptor);
    printf("\n");

    printf("Streaming Off...\n");
    if (videoCapture_streamOff(video) == 0)
    {
        fprintf(stderr, "Turn off stream failed\n");
        exit(EXIT_FAILURE);
    }
    fclose(fp1);
    //fclose(fp2);
    //h264_compress_uninit(&en);
    printf("Free Memory Map Device...\n");
    videoCapture_freeBuffer(video, &buffers, req.count);

    printf("Close Device...\n");
    if (videoCapture_closeDevice(video) == -1)
    {
        fprintf(stderr, "close error\n");
        exit(EXIT_FAILURE);
    }

    exit(EXIT_SUCCESS);
}
