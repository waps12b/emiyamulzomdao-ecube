//
// Created by parallels on 6/12/16.
//
#include "touch.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>     // for open/close
#include <fcntl.h>      // for O_RDWR
#include <sys/ioctl.h>  // for ioctl
#include <sys/mman.h>
#include <string.h>
#include <linux/input.h>
#include <linux/fb.h>   // for fb_var_screeninfo, FBIOGET_VSCREENINFO
#include <pthread.h>
#include <unistd.h>
#include <set>


#define	 FBDEV_FILE "/dev/fb0"
#define  INPUT_DEVICE_LIST	"/proc/bus/input/devices"
#define  EVENT_STR	"/dev/input/event"
#define  MAX_BUFF	200

int		screen_width;
int		screen_height;
int		bits_per_pixel;
int		line_length;

#define MAX_TOUCH_X	0x740
#define MAX_TOUCH_Y	0x540

using namespace std;

pthread_t listener = NULL;
set<void(*)(touch::touch_event)> callbacks;

int	fb_fd = NULL;
int fp    = NULL;
int readSize;
void* read_from_device(void* eptr){
    struct input_event *event = (input_event*)eptr;
    readSize = -1;
    readSize = read(fp, &event, sizeof(*event));
}
void read_coordinate(int &tx, int &ty){
    struct input_event event;
    int readSize;
    while(1)
    {
        pthread_t read_thread;
        pthread_create(&read_thread, NULL, &read_from_device, event);
        usleep(100);
        pthread_kill(read_thread, 1);
        if(readSize == -1)
        {
            tx = ty = -1;
            break;
        }

        if ( readSize == sizeof(event) )
        {
			//printf("type :%04X \n",event.type);
			//printf("code :%04X \n",event.code);
			//printf("value:%08X \n",event.value);
            if( event.type == EV_ABS )
            {
                if (event.code == ABS_MT_POSITION_X )
                {
                    tx = event.value*screen_width/MAX_TOUCH_X;
                }
                else if ( event.code == ABS_MT_POSITION_Y )
                {
                    ty = event.value*screen_height/MAX_TOUCH_Y;
                }
            }
            else if ((event.type == EV_SYN) && (event.code == SYN_REPORT ))
            {
                break;
            }
        }
    }

}

void *listen(void *nullable)
{
    int prev_x, prev_y;
    int cur_x, cur_y;
    prev_x = prev_y = cur_x = cur_y = -1;
    while(1)
    {
        prev_x = cur_x;
        prev_y = cur_y;

        //read touched coordinate
        read_coordinate(cur_x, cur_y);

        //generate touch event data;
        touch::touch_event e(cur_x, cur_y, prev_x, prev_y);

        //call callbacks;
        set<void(*)(touch::touch_event)>::iterator it = callbacks.begin();
        while(it != callbacks.end())
        {
            (*it)(e);
            it++;
        }

        //sleep for a second
        usleep(50);
    }
}


namespace touch{
    int init() {
        if (listener != NULL) {
            pthread_kill(listener, 1);
            listener = NULL;
        }

        if (fb_fd != NULL) {
            close(fb_fd);
            fb_fd = NULL;
        }

        if (fp != NULL) {
            close(fp);
            fp = NULL;
        }

        callbacks.clear();

        char eventFullPathName[100];
        struct  fb_var_screeninfo fbvar;
        struct  fb_fix_screeninfo fbfix;
        unsigned char   *fb_mapped;
        int		mem_size;

        sprintf(eventFullPathName,"%s%d",EVENT_STR, EVENT_NUM);

        fp = open( eventFullPathName, O_RDONLY);

        if (-1 == fp)
        {
            //printf("%s open fail\n",eventFullPathName);
            return -1;
        }

        if( access(FBDEV_FILE, F_OK) )
        {
            printf("%s: access error\n", FBDEV_FILE);
            close(fp);
            return -2;
        }

        if( (fb_fd = open(FBDEV_FILE, O_RDWR)) < 0)
        {
            //printf("%s: open error\n", FBDEV_FILE);
            close(fp);
            return -2;
        }

        if( ioctl(fb_fd, FBIOGET_VSCREENINFO, &fbvar) )
        {
            //printf("%s: ioctl error - FBIOGET_VSCREENINFO \n", FBDEV_FILE);
            close(fb_fd);
            close(fp);
            return -3;
        }

        if( ioctl(fb_fd, FBIOGET_FSCREENINFO, &fbfix) )
        {
            // printf("%s: ioctl error - FBIOGET_FSCREENINFO \n", FBDEV_FILE);
            close(fb_fd);
            close(fp);
            return -4;
        }

        screen_width    =   fbvar.xres;
        screen_height   =   fbvar.yres;
        bits_per_pixel  =   fbvar.bits_per_pixel;
        line_length     =   fbfix.line_length;

        //printf("screen_width : %d\n", screen_width);
        //printf("screen_height : %d\n", screen_height);
        //printf("bits_per_pixel : %d\n", bits_per_pixel);
        //printf("line_length : %d\n", line_length);

        mem_size    =   screen_width * screen_height * 4;

        pthread_create(&listener, NULL, &listen, NULL);

        return 0;
    }

    void add_callback(void (*fptr)(touch::touch_event)){
        callbacks.insert(fptr);
    }
}
