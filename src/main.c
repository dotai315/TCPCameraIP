#include "main.h"

pthread_t                   *streamThread;
struct addrinfo             *hints;
struct addrinfo             *rp;
struct addrinfo             **result;
int                         error;
int                         *serverSock; 
int                         *clientSock;
struct sockaddr             *clientAddr;
socklen_t                   *clientAddrLen;
int                         indexThread;
videoCapture_t              *video;
struct stat                 st;
int                         sizeFrame;
struct v4l2_requestbuffers  req;
struct videoBuffer          *buffers;

void handle_sigint(int sig)
{
    free(streamThread);
    free(hints);
    free(rp);
    free(*result);
    close(*serverSock);
    if (serverSock != NULL)
        free(serverSock);
    close(*clientSock);
    free(clientSock);
    free(clientAddr);
    if (videoCapture_streamOff(video) == 0)
    {
        fprintf(stderr, "Turn off stream failed\n");
        exit(EXIT_FAILURE);
    }
    videoCapture_freeBuffer(video, &buffers, req.count);
    if (videoCapture_closeDevice(video) == -1)
    {
        fprintf(stderr, "close error\n");
        exit(EXIT_FAILURE);
    }
    free(video);
    printf("End Program\n");
}
        
void handle_sigterm(int sig)
{
    free(streamThread);
    free(hints);
    free(rp);
    free(*result);
    close(*serverSock);
    close(*clientSock);
    free(clientSock);
    free(clientAddr);
    if (videoCapture_streamOff(video) == 0)
    {
        fprintf(stderr, "Turn off stream failed\n");
        exit(EXIT_FAILURE);
    }
    videoCapture_freeBuffer(video, &buffers, req.count);
    if (videoCapture_closeDevice(video) == -1)
    {
        fprintf(stderr, "close error\n");
        exit(EXIT_FAILURE);
    }
    free(video);
    printf("End Program\n");
}

void handle_sigquit(int sig)
{
    free(streamThread);
    free(hints);
    free(rp);
    free(*result);
    close(*serverSock);
    close(*clientSock);
    free(clientSock);
    free(clientAddr);
    if (videoCapture_streamOff(video) == 0)
    {
        fprintf(stderr, "Turn off stream failed\n");
        exit(EXIT_FAILURE);
    }
    videoCapture_freeBuffer(video, &buffers, req.count);
    if (videoCapture_closeDevice(video) == -1)
    {
        fprintf(stderr, "close error\n");
        exit(EXIT_FAILURE);
    }
    free(video);
    printf("End Program\n");
}

void handle_sigpipe(int sig)
{
    // do nothing
}

void *stream (void *param)
{
    int clientFd = *((int *)param);
    while (1)
    {
        if (write(clientFd, 
            videoCapture_readFrame(video, buffers, V4L2_BUF_TYPE_VIDEO_CAPTURE, V4L2_MEMORY_MMAP, req.count, &sizeFrame),
            sizeFrame
        ) == -1)
        {
            if (errno == EPIPE)
            {
                break;
            }
            else
            {
                continue;
            }
        }
    }
    return NULL;
}

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        fprintf(stderr, "usage: %s port\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    printf("Start Program\n");

    signal(SIGINT, handle_sigint);
    signal(SIGQUIT, handle_sigquit);
    signal(SIGTERM, handle_sigterm);
    signal(SIGPIPE, handle_sigpipe);

    video = videoCapture_createVideoCapture();
    st = videoCapture_getStatusDevice(video);

    if (!S_ISCHR(st.st_mode))
    {
        fprintf(stderr, "%s is no device\n", video->deviceName);
        exit(EXIT_FAILURE);
    }
    if (videoCapture_openDevice(video) == -1)
    {
        fprintf(stderr, "Cannot open '%s': %d, %s\n", video->deviceName, errno, strerror(errno));
        exit(EXIT_FAILURE);
    }
    
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

    videoCapture_cropCap(video, V4L2_BUF_TYPE_VIDEO_CAPTURE, NULL, NULL, NULL);
    videoCapture_setFormat(video, V4L2_BUF_TYPE_VIDEO_CAPTURE, 480, 360, V4L2_PIX_FMT_MJPEG, V4L2_FIELD_INTERLACED, 480 * 2, 480 * 360 * 2, 0);
    
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

    for(int i = 0; i < req.count; i++)
    {
        videoCapture_setBufferCapture(video, i, V4L2_BUF_TYPE_VIDEO_CAPTURE, V4L2_MEMORY_MMAP);
    }

    if (videoCapture_streamOn(video) == 0)
    {
        fprintf(stderr, "Turn on stream failed\n");
        exit(EXIT_FAILURE);
    }

    serverSock = (int *)malloc(sizeof(int));
    if (serverSock == NULL)
    {
        fprintf(stderr, "[ERROR] Not enough space to allocate socket file description");
        exit(EXIT_FAILURE);
    }
    else
    {
        printf("Allocated Socket FD\n");
    }

    hints = (struct addrinfo *)malloc(sizeof(struct addrinfo));
    if (hints == NULL)
    {
        fprintf(stderr, "[ERROR] Not enough memory!\n");
        exit(EXIT_FAILURE);
    }
    else
    {
        printf("Allocated Hints\n");
    }

    memset(hints, 0, sizeof(*hints));
    hints->ai_addr = NULL;
    hints->ai_canonname = NULL;
    hints->ai_family = AF_INET;
    hints->ai_flags = AI_PASSIVE;
    hints->ai_next = NULL;
    hints->ai_protocol = IPPROTO_TCP;
    hints->ai_socktype = SOCK_STREAM;

    result = (struct addrinfo **)malloc(sizeof(struct addrinfo *));
    if (result == NULL)
    {
        fprintf(stderr, "[ERROR] Not enough space\n");
        exit(EXIT_FAILURE);
    }
    else
    {
        printf("Allocated Getaddrinfo Result\n");
    }
 
    error = getaddrinfo(NULL, argv[1], hints, result);
    if (error != 0)
    {
        fprintf(stderr, "[ERROR] getaddrinfo: %s\n", gai_strerror(error));
        exit(EXIT_FAILURE);
    }
    else
    {
        printf("Getaddrinfo Success\n");
    }

    for (rp = *result; rp != NULL; rp = rp->ai_next)
    {
        *serverSock = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (*serverSock < 0)
        {
            continue;
        }
        else
        {
            printf("Create Socket Success\n");
        }

        if (bind(*serverSock, rp->ai_addr, rp->ai_addrlen) == 0)
        {
            printf("Bind Success\n");
            break;
        }

        close(*serverSock);
    }

    if (rp == NULL)
    {
        fprintf(stderr, "[ERROR] Could not bind\n");
        exit(EXIT_FAILURE);
    }    
    if (setsockopt(*serverSock, SOL_SOCKET, SO_REUSEPORT, &(int){1}, sizeof(int)) < 0)
        fprintf(stderr,"setsockopt(SO_REUSEADDR) failed");

    freeaddrinfo(*result);
    printf("Initial Program Success\n");

    if ((error = listen(*serverSock, 5)) != 0)
    {
        fprintf(stderr, "[ERROR] Listen Failed: %s\n", strerror(error));
        exit(EXIT_FAILURE);
    }
    else
    {
        printf("Listenning....\n");
    }

    indexThread = 0;
    streamThread = (pthread_t *)malloc(sizeof(pthread_t) * 5);
    if (streamThread == NULL)
    {
        fprintf(stderr, "[ERROR] NOt enough space to create thread\n");
        exit(EXIT_FAILURE);
    }
    else
    {
        printf("Created Thread For Program\n");
    }

    clientSock = (int *)malloc(sizeof(int));
    if (clientSock == NULL)
    {
        fprintf(stderr, "[ERROR] Not enough space to create client socket\n");
        exit(EXIT_FAILURE);
    }
    else
    {
        printf("Allocated Socket for Client\n");
    }

    while (true)
    {
        printf("Waiting for client connect\n");
        clientAddr = (struct sockaddr *)malloc(sizeof(struct sockaddr));
        if (clientAddr == NULL)
        {
            fprintf(stderr, "[ERROR] Not enough space memory\n");
            continue;
        }
        else
        {
            printf("Allocated client address\n");
        }

        clientAddrLen = (socklen_t *)malloc(sizeof(socklen_t));
        if (clientAddrLen == NULL)
        {
            fprintf(stderr, "[ERROR] Not enough space to allocated client address len\n");
            continue;
        }
        *clientAddrLen = (socklen_t)sizeof(clientAddr);
        *clientSock = accept(*serverSock, clientAddr, clientAddrLen);
        if (*clientSock < 0)
        {
            fprintf(stderr, "[ERROR] Cannot accept client: %s\n", strerror(errno));
            continue;
        }
        else
        {
            printf("Accepted Client\n");
        }

        if (pthread_create(streamThread + indexThread, NULL, stream, clientSock) != 0)
        {
            printf("Have Create Thread at Index: %d\n", indexThread);
        }

        indexThread++;
        if (indexThread >= 5)
        {
            indexThread = 0;
            while (indexThread < 5)
            {
                pthread_join(*(streamThread + indexThread), NULL);
                indexThread++;
            }
            indexThread = 0;
        }
    }

    exit(EXIT_SUCCESS);
}
