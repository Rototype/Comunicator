#define _GNU_SOURCE
/************************** INCLUDE SYSTEM SECTION **********************************/
#include <math.h>             
#include <byteswap.h>
#include <termios.h>         
#include <time.h>	           
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <stdarg.h>
#include <unistd.h>
#include <resolv.h>
#include <signal.h>
#include <pthread.h>
#include <syslog.h>
#include <dirent.h>
#include <getopt.h>

#include <sqlite3.h>          

#include <net/if.h>  
#include <netinet/in.h>
#include <arpa/inet.h>

#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <sys/ipc.h>          
#include <sys/shm.h>          

#include <linux/i2c-dev.h>
#include <linux/serial.h>
#include <linux/spi/spidev.h>


