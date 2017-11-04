#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>

#define BUF_SIZE 255


//app defines
#define CTRL_DATA 0x00
#define CTRL_START 0x01
#define CTRL_END 0x02

//protocol defines
#define FLAG 0x7E

#define STUFF 0x7D
#define STUFF_E 0x5E
#define STUFF_D 0x5D

#define ADDR_RECEIVER 0x01
#define ADDR_SENDER 0x03

#define INF_0 0x00
#define INF_2 0x02

#define CTRL_SET 0x03
#define CTRL_UA 0x07
#define CTRL_RR_0 0x01
#define CTRL_RR_1 0x21
#define CTRL_REJ_0 0x05
#define CTRL_REJ_1 0x25
#define CTRL_DISC 0x0B

#define BAUDRATE B38400
#define _POSIX_SOURCE 1 /* POSIX compliant source */
#define FALSE 0
#define TRUE 1

#define WAIT_TIME 3

