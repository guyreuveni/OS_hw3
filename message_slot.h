#ifndef CHARDEV_H
#define CHARDEV_H

#include <linux/ioctl.h>


#define MAJOR_NUM 235

#define MSG_SLOT_CHANNEL _IOW(MAJOR_NUM, 0, unsigned int)

#define DEVICE_NAME "message_slot_device"

typedef struct massage_slot_file
{
    unsigned int minor;
    unsigned long owners;
    struct massage_slot_file* next;  
    struct channel* channels;    
} massage_slot_file;

typedef struct channel
{
    unsigned long channel_num;
    ssize_t word_len;
    char* data;
    struct channel* next;
} channel;

#endif
