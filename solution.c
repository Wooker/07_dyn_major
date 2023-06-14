#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/kernel.h>
#include <linux/uaccess.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/device.h>
#include <linux/kdev_t.h>
#include <linux/cdev.h>

static char *node_name;
module_param(node_name, charp, S_IRUSR);
MODULE_PARM_DESC(node_name, "Name of the node");

static bool read = false;
static unsigned int major;

static ssize_t solution_read(struct file *filp, char __user *buf, size_t count, loff_t *offset)
{
  int result, len; 
  char sum_string[15]; // Allocate a buffer to hold the sum as a string
  printk(KERN_INFO "kernel_mooc In read");

  if (read) {
    printk(KERN_INFO "kernel_mooc Already read");

    read = false;
    return 0;
  }
  
  len = snprintf(sum_string, sizeof(sum_string), "%d\n", major); // Convert sum to string

  if (*offset >= len)
      return 0; // End of file

  result = copy_to_user(buf, sum_string, len); 
  if ( result != 0) {
    printk(KERN_ERR "kernel_mooc copy_to_user returned 0");
    return -EFAULT; // Failed to copy sum to user space
  }

  *offset += len;
  read = true;
  return len;
}

// Register operations
static struct file_operations solution_fops = {
  .owner = THIS_MODULE,
  .read = solution_read,
};

static struct cdev my_cdev;
static dev_t dev;
static struct class *class = NULL;

static int __init solution_init(void)
{
  int result;

  // Register the character device driver
  result = alloc_chrdev_region(&dev, 0, 1, node_name);
  if (result < 0) {
      printk(KERN_ERR "kernel_mooc Failed to allocate solution module %d\n", result);
      return result;
  }

  major = MAJOR(dev);

  cdev_init(&my_cdev, &solution_fops);
  cdev_add(&my_cdev, dev, 1);

	class = class_create(THIS_MODULE, node_name);
	if (IS_ERR(class)) {
    printk(KERN_ERR "kernel_mooc class_create error\n");
		result = PTR_ERR(class);
    return result;
	}

  device_create(class, NULL, dev, NULL, node_name);

  printk(KERN_INFO "kernel_mooc Solution module is loaded\n");
  return 0;
}

static void __exit solution_exit(void)
{
  // Unregister the character device driver
  device_destroy(class, dev);
  class_destroy(class);
  cdev_del(&my_cdev);
  unregister_chrdev(dev, node_name);

  printk(KERN_INFO "kernel_mooc Solution module is unloaded\n");
}

module_init(solution_init);
module_exit(solution_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Zakhar Semenov");
MODULE_DESCRIPTION("Solution");