/**    
    @file   touch.h
    @author dongyi kim
    @brief  about touch event on panels
*/
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
#include <signal.h>
#include <unistd.h>
#include <set>


#define  FBDEV_FILE         "/dev/fb0"
#define  INPUT_DEVICE_LIST  "/proc/bus/input/devices"
#define  EVENT_STR      "/dev/input/event"
#define  MAX_BUFF       200
#define EVENT_NUM       2
int     screen_width;
int     screen_height;
int     bits_per_pixel;
int     line_length;

#define MAX_TOUCH_X 0x740
#define MAX_TOUCH_Y 0x540

using namespace std;

pthread_t listener = NULL;
set<void(*)(touch::touch_event)> callbacks;


int fb_fd = NULL;
int fp    = NULL;
int prev_x, prev_y;
int cur_x, cur_y;

int timer = 0;
pthread_cond_t timer_cond;
pthread_mutex_t timer_mutex;
pthread_mutex_t callback_mutex;
pthread_mutex_t counter_mutex;


// create new touch event with coordinate
// and call all of callback functions in the set 
void notify_callback(int x, int y)
{
    pthread_mutex_lock(&callback_mutex);

    prev_x = cur_x;
    prev_y = cur_y;
    cur_x = x;
    cur_y = y;
    
    //create new touch_event istance
    touch::touch_event e(cur_x, cur_y, prev_x, prev_y);
    
    //to all callback functions 
    set<void(*)(touch::touch_event)>::iterator it = callbacks.begin();
    while(it != callbacks.end())
    {
        //call functions
        (*it)(e);
        it++;
    }
    
    pthread_mutex_unlock(&callback_mutex);
}

//run count-down to check touch_up event occur 
void *counter(void *nullable)
{
    while(1)
    {
        usleep(50000);
        
        //lock timer with mutex to prevent from being on race-condition
        pthread_mutex_lock(&timer_mutex);
        if(timer > 0)
        {
            timer = max(0, timer-50);
            //printf("timer : %d\n", timer);
            //if timer off after touch event 
            if(timer == 0)
            {
        //free timer value while call-back function running    
                pthread_mutex_unlock(&timer_mutex);
      
            //notify to all callback functions that new touch_up event occured       
                notify_callback(-1, -1);
                
                //lock timer 
                pthread_mutex_lock(&timer_mutex);
            }
        }
        pthread_mutex_unlock(&timer_mutex);
    }
}


//read coordinate from panel 
void read_coordinate()
{
    while(1)
    {
        int readSize;
        struct input_event event;
        int lcd_x, lcd_y;

        readSize = read(fp, &event, sizeof(event));
        //printf("check read Size : %d(%d)\n", readSize, sizeof(event));
        if ( readSize == sizeof(event) )
        {
            if( event.type == EV_ABS )
            {
                if (event.code == ABS_MT_POSITION_X )
                {
                    lcd_x = event.value*screen_width/MAX_TOUCH_X;
                }
                else if ( event.code == ABS_MT_POSITION_Y )
                {
                    lcd_y = event.value*screen_height/MAX_TOUCH_Y;
                }
            }
            else if ((event.type == EV_SYN) && (event.code == SYN_REPORT ))
            {
                //when touch event listened,
                //set timer to 200ms
                //timer_mutex should be used to prevent from being on race condition
                pthread_mutex_lock(&timer_mutex);
               
                //if no touch event occur in 200ms, that's touch_up event 
                timer = 200;
                pthread_mutex_unlock(&timer_mutex);
                
                //convert axis panel-base to image-base
                //and create new touch-event
                notify_callback(  lcd_y, screen_width - lcd_x-1);
                break;
            }
        }
    }

}

// start to listen touch-event from device
void *listen(void *nullable)
{
    pthread_t counter_thread;
    pthread_cond_init(&timer_cond, NULL);
    pthread_mutex_init(&timer_mutex, NULL);
    pthread_mutex_init(&callback_mutex, NULL);
    pthread_mutex_init(&counter_mutex, NULL);
    prev_x = prev_y = cur_x = cur_y = -1;
    pthread_create(&counter_thread, NULL, &counter, NULL);
    while(1)
    {
        //read touched coordinate
        read_coordinate();
        //usleep(50);
    }
}


namespace touch
{
    /**
     * initialize touch-sensor
     * init() function should be called only once
     */
    int init()
    {
        char eventFullPathName[100];
        struct  fb_var_screeninfo fbvar;
        struct  fb_fix_screeninfo fbfix;
        unsigned char   *fb_mapped;
        int     mem_size;

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

        //start listener thread
        pthread_create(&listener, NULL, &listen, NULL);
        return 0;
    }

    //add callback function with pointer
    void add_callback(void (*fptr)(touch::touch_event))
    {
        callbacks.insert(fptr);
    }

    //remove callback function pointer
    void remove_callback(void (*fptr)(touch::touch_event))
    {
        callbacks.erase(fptr);
    }
}