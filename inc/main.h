#include "videoCapture.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netdb.h>

#include <pthread.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>

#define BUFFLEN 1024
