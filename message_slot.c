#undef __KERNEL__
#define __KERNEL__
#undef MODULE
#define MODULE

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/string.h>
#include <linux/slab.h>

MODULE_LICENSE("GPL");

#include "message_slot.h"

/**************************-declarations-**********************************/
massage_slot_file* search_massage_slot(unsigned int minor);
void insert_massage_slot(massage_slot_file* msg_slot);
void delete_massage_slot(massage_slot_file* msg_slot);
channel* search_channel(unsigned long num, massage_slot_file* msg_slot);
void insert_channel(massage_slot_file* msg_slot, channel* chan);
void delete_channels(massage_slot_file* msg_slot);
void buff_to_chan(ssize_t len, const char* buff, channel* chan);
static massage_slot_file* head;

/****************-search and insert functions for both structs-***********************/
massage_slot_file* search_massage_slot(unsigned int minor){/*returns a masage slot struct, NULL if not initialized yet*/
    massage_slot_file* curr = head;
    while(!(curr == NULL)){
        if((curr->minor) == minor){
            return curr;
        }
        curr = curr->next;
    }
    return NULL;
}

void insert_massage_slot(massage_slot_file* msg_slot){/*insert a new massage_slot to linked list*/
    massage_slot_file* curr_head = head;
    msg_slot->next = curr_head;
    head = msg_slot;
    return;
}

void delete_massage_slot(massage_slot_file* msg_slot){/*delete massage slot and its channels, knowing a massage slot with this minor exists*/
    delete_channels(msg_slot);
    kfree(msg_slot);
    return;
}

channel* search_channel(unsigned long num, massage_slot_file* msg_slot){/*returns a channel struct, NULL if not initialized yet*/
    channel* curr = msg_slot->channels;
    if (curr == NULL){
        return NULL;
    }
    while(!(curr->next == NULL)){
        if((curr->channel_num) == num){
            return curr;
        }
        curr = curr->next;
    }
    return NULL;
}

void insert_channel(massage_slot_file* msg_slot, channel* chan){/*insert a new channel to channels of msg_slot*/
    channel* curr_head = msg_slot->channels;
    chan->next = curr_head;
    msg_slot->channels = chan;
    return;
}

void delete_channels(massage_slot_file* msg_slot){
    channel* chan = msg_slot->channels;
    channel* next = NULL;
    while(!(chan == NULL)){/*free all open channels*/
        next = chan->next;
        kfree(chan->data);
        kfree(chan);
        chan = next;
    }
    return;
}

void buff_to_chan(ssize_t len, const char* buff, channel* chan){
    ssize_t i;
    for(i=0; i<len; i++){
        (chan->data)[i] = buff[i];
    }
}

/****************-Device functions-***********************/
static int device_open( struct inode* inode,
                        struct file*  file )
{
    unsigned int minor = iminor(inode);
    massage_slot_file* msg_slot = search_massage_slot(minor);

    if(msg_slot == NULL){/*first time seeing this msg_slot so create new one*/
        msg_slot = (massage_slot_file*) kmalloc(sizeof(massage_slot_file), GFP_KERNEL);
        if (msg_slot == NULL){/*failed allocating memory*/
            return -ENOMEM;
        }
        msg_slot->minor = minor;
        msg_slot->channels = NULL;
        msg_slot->owners = 0;
        insert_massage_slot(msg_slot);
    }

    msg_slot->owners = msg_slot->owners + 1; /*one more owner added to the file*/

    return 0;
}

//---------------------------------------------------------------------
static ssize_t device_write( struct file*       file,
                             const char __user* buffer,
                             size_t             length,
                             loff_t*            offset )
{
  char* buff;
  ssize_t i;
  if(file->private_data == NULL || buffer == NULL){/*no open channel for this file or buffer is NULL*/
    return -EINVAL;
  }
  
  if(length == 0 || length > 128){ //message lenght invalid
    return -EMSGSIZE;
  }

  buff = (char*) kmalloc(128, GFP_KERNEL);/*allocate a buffer for atomic write*/
    if(buff == NULL){/*failed allocating memory*/
        return -ENOMEM;
    }

  for( i = 0; i < length; ++i) {
    if(!(get_user(buff[i], &buffer[i]) == 0)){
        kfree(buff);
        return -EFAULT;
    };
  }
  buff_to_chan(i, buffer, (channel*)file->private_data); /*copy from buff to the channel*/
  ((channel*)(file->private_data))->word_len = i; /*update to length of current word of the channel*/

  // free buff and return the number of input characters used
  kfree(buff);
  return i;
}


//---------------------------------------------------------------------
static ssize_t device_read( struct file* file,
                            char __user* buffer,
                            size_t       length,
                            loff_t*      offset )
{
    ssize_t n, i;
    char* word;
    if(((channel*)file->private_data) == NULL || buffer == NULL){/*no open channel for this file or buffer is NULL*/
        return -EINVAL;
    }
    n = ((channel*)(file->private_data))->word_len;
    if(n == 0){/*no word in the channel*/
        return -EWOULDBLOCK;
    }
    if(length < n){/*buffer is too short for the word saved in the channel*/
        return -ENOSPC;
    }
    
    /*implement main function details*/
    word = ((channel*)(file->private_data))->data;
    for(i=0; i<n; ++i){
        if(!(put_user(word[i], &buffer[i]) == 0)){
            return -EFAULT;
        }
    }

    return i;
}

//----------------------------------------------------------------------
static long device_ioctl( struct   file* file,
                          unsigned int   ioctl_command_id,
                          unsigned long  ioctl_param )
{
  unsigned int minor;
  massage_slot_file* msg_slot;
  channel* chan;

  // got wrong parameter - error
  if(!(MSG_SLOT_CHANNEL == ioctl_command_id) || ioctl_param == 0){
    return -EINVAL;
  }

  minor = iminor(file->f_inode);
  msg_slot = search_massage_slot(minor);
  chan = search_channel(ioctl_param, msg_slot);

  if(chan == NULL){ /*no channel with this number opened yet, create new one*/
    chan = (channel*) kmalloc(sizeof(channel), GFP_KERNEL);
    if(chan == NULL){/*error when allocating memory*/
        return -ENOMEM;
    }

    chan->channel_num = ioctl_param;
    chan->data = (char*) kmalloc(128, GFP_KERNEL);
    if(chan->data == NULL){/*failed allocating memory*/
        kfree(chan);
        return -ENOMEM;
    }
    chan->word_len = 0;
    insert_channel(msg_slot, chan);
  }

  file->private_data = chan; /*set the channel to the message slot file*/

  return 0;
}


//==================== DEVICE SETUP ============================= /*code skeleton taken from 6th recitation and modified*/

// The File Operations
struct file_operations Fops = {
  .owner	  = THIS_MODULE, 
  .read           = device_read,
  .write          = device_write,
  .open           = device_open,
  .unlocked_ioctl = device_ioctl,
};

//---------------------------------------------------------------
// Initialize module
static int __init simple_init(void)
{
  int rc = -1;

  // Register driver capabilities
  rc = register_chrdev( MAJOR_NUM, DEVICE_NAME, &Fops );

  // Negative values signify an error
  if( rc < 0 ) {
    printk( KERN_ALERT "registraion failed \n");
    return -1;
  }
  head = NULL;
  return 0;
}

//---------------------------------------------------------------
static void __exit simple_cleanup(void)
{
    massage_slot_file* curr = head;
    massage_slot_file* next = NULL;
    while(!(curr == NULL)){/*delete all open massage slots*/
        next = curr->next;
        delete_massage_slot(curr);
        curr = next;
    }

    // Unregister the device
    // Should always succeed
    unregister_chrdev(MAJOR_NUM, DEVICE_NAME);
}

//---------------------------------------------------------------
module_init(simple_init);
module_exit(simple_cleanup);

//========================= END OF FILE =========================
