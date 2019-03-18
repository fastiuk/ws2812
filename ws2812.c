#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/gpio.h>
#include <linux/delay.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Yevhen Fastiuk");
MODULE_DESCRIPTION("Kernel module for using WS2812 LED");

#define DEV_NAME	"ws2812"
#define CLASS_NAME	"led"

static int major_num;
static struct class* class = NULL;
static struct device* dev = NULL;

static int dev_open(struct inode *inode, struct file *file)
{
// FIXME: Temp. For testing purposes. 
#define GPIO_PIN 18
	int res;

	res = gpio_request(GPIO_PIN, DEV_NAME);
	if (res) {
		return -EFAULT;
	}

	res = gpio_direction_output(GPIO_PIN, 0);
	if (res) {
		return -EFAULT;
	}

	gpio_set_value(GPIO_PIN, 1);
	udelay(1000000);
	gpio_set_value(GPIO_PIN, 0);
	udelay(1000000);
	gpio_set_value(GPIO_PIN, 1);
	udelay(500000);
	gpio_set_value(GPIO_PIN, 0);
	udelay(500000);

	gpio_free(GPIO_PIN);
	
	return 0;
}

static int dev_release(struct inode *inode, struct file *file)
{
	return 0;
}

static ssize_t dev_read(struct file *file, char * buff, size_t len, loff_t *off)
{
	return 0;
}

static ssize_t dev_write(struct file *file, const char *buff, size_t len, loff_t *off)
{
	return 0;
}

static struct file_operations fops = {
	.open = dev_open,
	.read = dev_read,
	.write = dev_write,
	.release = dev_release,
};

static int __init mod_init(void)
{
	major_num = register_chrdev(0, DEV_NAME, &fops);
	if (major_num < 0) {
		printk(KERN_ALERT DEV_NAME ": chrdev register failed\n");
		return major_num;
	}

	class = class_create(THIS_MODULE, CLASS_NAME);
	if (!class) {
		unregister_chrdev(major_num, DEV_NAME);
		printk(KERN_ALERT DEV_NAME ": class create failed\n");
		return -EFAULT;
	}

	dev = device_create(class, NULL, MKDEV(major_num, 0), NULL, DEV_NAME);
	if (!dev) {
		class_destroy(class);
		unregister_chrdev(major_num, DEV_NAME);
		printk(KERN_ALERT DEV_NAME ": dev create failed\n");
		return -EFAULT;
	}

	printk(KERN_INFO DEV_NAME ": init done\n");
	return 0;
}

static void __exit mod_exit(void)
{
	device_destroy(class, MKDEV(major_num, 0));
	class_unregister(class);
	class_destroy(class);
	unregister_chrdev(major_num, DEV_NAME);
	printk(KERN_INFO DEV_NAME ": deinit done\n");
}

module_init(mod_init);
module_exit(mod_exit);
