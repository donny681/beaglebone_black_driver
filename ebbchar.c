/*
 * @Author: your name
 * @Date: 2021-08-09 15:53:11
 * @LastEditTime: 2021-08-10 09:52:56
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: /beagleboard/driver/bbbchar/ebbchar.c
 */
#include <linux/init.h>    // Macros used to mark up functions e.g. __init __exit
#include <linux/module.h>  // Core header for loading LKMs into the kernel
#include <linux/device.h>  // Header to support the kernel Driver Model
#include <linux/kernel.h>  // Contains types, macros, functions for the kernel
#include <linux/fs.h>      // Header for the Linux file system support
#include <linux/uaccess.h> // Required for the copy to user function
#include <linux/major.h>
#include <linux/cdev.h>
#define DEVICE_NAME "ebbchar" ///< The device will appear at /dev/ebbchar using this value
#define CLASS_NAME "ebb"      ///< The device class -- this is a character device driver
#define GLOBALMEM_SIZE 0x1000
MODULE_LICENSE("GPL");                                        ///< The license type -- this affects available functionality
MODULE_AUTHOR("Derek Molloy");                                ///< The author -- visible when you use modinfo
MODULE_DESCRIPTION("A simple Linux char driver for the BBB"); ///< The description -- see modinfo
MODULE_VERSION("0.1");                                        ///< A version number to inform users

struct CDEV_DEV
{
    struct cdev cdev_template;
    struct class *class;
    struct device *device;
    dev_t devid;
    int major;
    int minor;
    unsigned char mem[GLOBALMEM_SIZE];
    short size_of_message;
};
struct CDEV_DEV cdev_dev;

static int numberOpens = 0;

struct globalmem_dev *globalmem_devp;


static ssize_t cdev_read(struct file *filep, char *buffer, size_t len, loff_t *offset){
   int error_count = 0;
//    char   message[32] = {0};
   // copy_to_user has the format ( * to, *from, size) and returns 0 on success
   error_count = copy_to_user(buffer, cdev_dev.mem, cdev_dev.size_of_message);

   if (error_count==0){            // if true then have success
      printk(KERN_INFO "EBBChar: Sent %d characters to the user\n", cdev_dev.size_of_message);
      return (cdev_dev.size_of_message=0);  // clear the position to the start and return 0
   }
   else {
      printk(KERN_INFO "EBBChar: Failed to send %d characters to the user\n", error_count);
      return -EFAULT;              // Failed -- return a bad address message (i.e. -14)
   }
}

static ssize_t cdev_write(struct file *filep, const char __user*buffer, size_t len, loff_t *offset)
{
    // char message[32] = {0};
    int ret = copy_from_user(cdev_dev.mem,buffer,len);
    if(ret!=0){
        printk(KERN_ERR"copy_from_user error");
        return -EFAULT;
    }

    cdev_dev.size_of_message = len;
    printk(KERN_INFO "EBBChar: Received %s characters from the user\n",cdev_dev.mem );
    return len;
}

static int cdev_open(struct inode *inodep, struct file *filep)
{
    numberOpens++;
    printk(KERN_INFO "EBBChar: Device has been opened %d time(s)\n", numberOpens);
    return 0;
}

static int cdev_release(struct inode *inodep, struct file *filep)
{
    printk(KERN_INFO "EBBChar: Device successfully closed\n");
    return 0;
}
static const struct file_operations file_fops = {
    .open = cdev_open,
    .read = cdev_read,
    .release = cdev_release,
    .write = cdev_write,
};


static int __init ebbchar_init(void)
{
    printk(KERN_INFO "EBBChar: Initializing the EBBChar LKM\n");
    int ret = 0;
    cdev_dev.devid = MKDEV(cdev_dev.major, 0);
    if (cdev_dev.major)
    {
        ret = register_chrdev_region(cdev_dev.devid, 1, DEVICE_NAME);
    }
    else
    {
        ret = alloc_chrdev_region(&cdev_dev.devid, 0, 1, DEVICE_NAME);
        cdev_dev.major = MAJOR(cdev_dev.devid);
    }
    if (ret < 0)
    {
        printk(KERN_WARNING "unable to get major %d", cdev_dev.major);
        goto chrdev_region_error;
    }
    /*  创建字符设备  */
    cdev_init(&cdev_dev.cdev_template, &file_fops);
    cdev_dev.cdev_template.owner = THIS_MODULE;
    ret = cdev_add(&cdev_dev.cdev_template, cdev_dev.devid, 1);
    if (ret)
    {
        goto chrdev_region_error;
    }
    cdev_dev.class = class_create(THIS_MODULE, DEVICE_NAME);
    if (IS_ERR(cdev_dev.class))
    {
        cdev_del(&cdev_dev.cdev_template);
        unregister_chrdev_region(cdev_dev.devid, 1);
        printk(KERN_INFO "EBBChar: class_create error %ld", PTR_ERR(cdev_dev.class));
        return PTR_ERR(cdev_dev.class);
    }
    cdev_dev.device = device_create(cdev_dev.class, NULL, cdev_dev.devid, NULL, DEVICE_NAME);
    if (IS_ERR(cdev_dev.device))
    {
        class_destroy(cdev_dev.class);
        cdev_del(&cdev_dev.cdev_template);
        unregister_chrdev_region(cdev_dev.devid, 1);
        printk(KERN_INFO "EBBChar: device_create error %ld", PTR_ERR(cdev_dev.class));
        return PTR_ERR(cdev_dev.device);
    }
    printk(KERN_INFO "EBBChar: Initializing  char device %d successfully\n", cdev_dev.major);
    return 0;

chrdev_region_error:
    unregister_chrdev_region(cdev_dev.devid, 1);
    printk(KERN_INFO "EBBChar: Initializing  char device %d error\n", cdev_dev.major);
    return ret;
}

static void __exit ebbchar_exit(void)
{
    printk(KERN_INFO "EBBChar: Goodbye from the LKM!\n");
    device_destroy(cdev_dev.class, cdev_dev.devid);
    class_destroy(cdev_dev.class);
    cdev_del(&cdev_dev.cdev_template);
    unregister_chrdev_region(cdev_dev.devid, 1);
}

module_init(ebbchar_init);
module_exit(ebbchar_exit);