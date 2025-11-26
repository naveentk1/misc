// simple_driver.c - Basic character device driver
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/uaccess.h>
#include <linux/dma-mapping.h>

#define DEVICE_NAME "my_device"
#define BUFFER_SIZE 4096

static int major_number;
static struct class* my_class = NULL;
static struct device* my_device = NULL;
static char* kernel_buffer = NULL;
static dma_addr_t dma_handle;

static int device_open(struct inode *inode, struct file *file) {
    printk(KERN_INFO "MyDevice: Device opened\n");
    return 0;
}

static int device_release(struct inode *inode, struct file *file) {
    printk(KERN_INFO "MyDevice: Device closed\n");
    return 0;
}

static ssize_t device_read(struct file *filp, char __user *buffer, 
                          size_t length, loff_t *offset) {
    int bytes_to_copy = min(length, (size_t)BUFFER_SIZE);
    
    if (copy_to_user(buffer, kernel_buffer, bytes_to_copy)) {
        return -EFAULT;
    }
    
    printk(KERN_INFO "MyDevice: Read %d bytes\n", bytes_to_copy);
    return bytes_to_copy;
}

static ssize_t device_write(struct file *filp, const char __user *buffer,
                           size_t length, loff_t *offset) {
    int bytes_to_copy = min(length, (size_t)BUFFER_SIZE);
    
    if (copy_from_user(kernel_buffer, buffer, bytes_to_copy)) {
        return -EFAULT;
    }
    
    printk(KERN_INFO "MyDevice: Written %d bytes\n", bytes_to_copy);
    return bytes_to_copy;
}

static struct file_operations fops = {
    .open = device_open,
    .release = device_release,
    .read = device_read,
    .write = device_write,
};

static int __init mydriver_init(void) {
    // Allocate DMA buffer
    kernel_buffer = dma_alloc_coherent(NULL, BUFFER_SIZE, 
                                      &dma_handle, GFP_KERNEL);
    if (!kernel_buffer) {
        printk(KERN_ALERT "MyDevice: DMA allocation failed\n");
        return -ENOMEM;
    }
    
    // Register character device
    major_number = register_chrdev(0, DEVICE_NAME, &fops);
    if (major_number < 0) {
        printk(KERN_ALERT "MyDevice: Failed to register device\n");
        dma_free_coherent(NULL, BUFFER_SIZE, kernel_buffer, dma_handle);
        return major_number;
    }
    
    // Create device node
    my_class = class_create(THIS_MODULE, "my_device_class");
    my_device = device_create(my_class, NULL, 
                             MKDEV(major_number, 0), NULL, DEVICE_NAME);
    
    printk(KERN_INFO "MyDevice: Loaded with major number %d\n", major_number);
    return 0;
}

static void __exit mydriver_exit(void) {
    device_destroy(my_class, MKDEV(major_number, 0));
    class_destroy(my_class);
    unregister_chrdev(major_number, DEVICE_NAME);
    dma_free_coherent(NULL, BUFFER_SIZE, kernel_buffer, dma_handle);
    printk(KERN_INFO "MyDevice: Unloaded\n");
}

module_init(mydriver_init);
module_exit(mydriver_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Your Name");