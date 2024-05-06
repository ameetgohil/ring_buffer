#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/mutex.h>
#include <linux/uaccess.h>
#include <linux/ioctl.h>
#include <linux/slab.h>
#include <linux/mm.h>

#define DEVICE_NAME "ringbuf"
#define BUF_SIZE 1024
#define RINGBUF_IOC_MAGIC 'q'
#define RINGBUF_IOCRESET  _IO(RINGBUF_IOC_MAGIC, 0)
#define RINGBUF_IOCGETSTATUS _IOR(RINGBUF_IOC_MAGIC, 1, int)

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Ameet Gohil");
MODULE_DESCRIPTION("Ring buffer module");
MODULE_VERSION("0.01");

static int major;
static struct cdev c_dev;
static struct class *cl;
static char buffer[BUF_SIZE];
static int write_ptr = 0;
static int read_ptr = 0;
static struct mutex ringbuf_mutex;

static int ring_open(struct inode *inode, struct file *file);
static int ring_close(struct inode *inode, struct file *file);
static ssize_t ring_read(struct file *file, char __user *user, size_t size, loff_t *loff);
static ssize_t ring_write(struct file *file, const char __user *user, size_t size, loff_t *loff);
long ring_ioctl(struct file *file, unsigned int cmd, unsigned long arg);
static int ring_mmap(struct file *file, struct vm_area_struct *vma);

static struct file_operations pugs_fops = 
{
    .owner = THIS_MODULE,
    .open = ring_open,
    .release = ring_close,
    .read = ring_read,
    .write = ring_write,
    .unlocked_ioctl = ring_ioctl,
    .mmap = ring_mmap
};

static int ring_mmap(struct file *file, struct vm_area_struct *vma) {
    size_t size = vma->vm_end - vma->vm_start;

    if(buffer) {
        unsigned long pfn = vmalloc_to_pfn(buffer);

        //Remap-pfn-range will map the range of pages starting at pfn to user space
        if(remap_pfn_range(vma, vma->vm_start, pfn, size, vma->vm_page_prot) < 0) {
            return -EAGAIN;
        }
    } else {
        return -ENOMEM;
    }
    return 0;
}
long ring_ioctl(struct file *file, unsigned int cmd, unsigned long arg) {
    switch(cmd) {
        case RINGBUF_IOCRESET:
            mutex_lock(&ringbuf_mutex);
            write_ptr = 0;
            read_ptr = 0;
            memset(buffer, 0, BUF_SIZE);
            mutex_unlock(&ringbuf_mutex);
            break;
        case RINGBUF_IOCGETSTATUS:
            return put_user((write_ptr - read_ptr + BUF_SIZE) % BUF_SIZE, (int __user *)arg);
        default:
            return -ENOTTY;
    }
    return 0;
}

static int ring_open(struct inode *inode, struct file *file)
{
    return 0;
};

static int ring_close(struct inode *inode, struct file *file)
{
    return 0;
}

static ssize_t ring_read(struct file *file, char __user *user, size_t size, loff_t *loff) {
    size_t bytes_read = 0;
    mutex_lock(&ringbuf_mutex);

    while(bytes_read < size && read_ptr != write_ptr) {
        if(copy_to_user(user + bytes_read, &buffer[read_ptr], 1) != 0){
            mutex_unlock(&ringbuf_mutex);
            return -EFAULT;
        }
        read_ptr = (read_ptr + 1) % BUF_SIZE;
        bytes_read++;
    }
    mutex_unlock(&ringbuf_mutex);
    return bytes_read;
}

static ssize_t ring_write(struct file *file, const char __user *user, size_t size, loff_t *loff){
    size_t bytes_written = 0;

    mutex_lock(&ringbuf_mutex);
    while(bytes_written < size) {
        if((write_ptr + 1) % BUF_SIZE == read_ptr) {
            mutex_unlock(&ringbuf_mutex);
            return bytes_written > 0 ? bytes_written : -EAGAIN;
        }

        if(copy_from_user(&buffer[write_ptr], user + bytes_written, 1) != 0) {
            mutex_unlock(&ringbuf_mutex);
            return -EFAULT;
        }


        write_ptr = (write_ptr + 1) % BUF_SIZE;
        bytes_written++;
    }
    //printk(KERN_INFO "%s\n", buffer);
    mutex_unlock(&ringbuf_mutex);
    return bytes_written;
};

static int __init mod_init(void) {
    int ret;
    struct device *dev_ret;

    //Allocate a major number for the device
    if((ret = alloc_chrdev_region(&major, 0, 1, DEVICE_NAME)) < 0) {
        return ret;
    }

    cdev_init(&c_dev, &pugs_fops);

    // Add the charatacter device to the system
    if((ret = cdev_add(&c_dev, major, 1)) < 0) {
        unregister_chrdev_region(major, 1);
        return ret;
    }

    // Create a class structure for the device
    if(IS_ERR(cl = class_create("char"))) {
        cdev_del(&c_dev);
        unregister_chrdev_region(major, 1);
        return PTR_ERR(cl);
    }
    //class_set_owner(cl, THIS_MODULE);

    if(IS_ERR(dev_ret = device_create(cl, NULL, major, NULL, DEVICE_NAME))) {
        class_destroy(cl);
        cdev_del(&c_dev);
        unregister_chrdev_region(major, 1);
        return PTR_ERR(dev_ret);
    }

    mutex_init(&ringbuf_mutex);

    printk(KERN_INFO "Ring buffer device registered, major %d\n", major);
    return 0;
}

static void __exit mod_exit(void) {
    device_destroy(cl, major);
    class_destroy(cl);
    cdev_del(&c_dev);
    unregister_chrdev_region(major, 1);
    printk(KERN_INFO "Exiting ring buffer module\n");
}

module_init(mod_init);
module_exit(mod_exit);



