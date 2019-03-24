#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/gpio.h>
#include <linux/delay.h>
#include <linux/uaccess.h>
#include <linux/types.h>
//#include <linux/hrtimer.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Yevhen Fastiuk");
MODULE_DESCRIPTION("Kernel module for using WS2812 LED");

#define DEV_NAME	"ws2812"
#define CLASS_NAME	"led"

#define GPIO_SET_VALUE_DELTA	130

#define TRESET_US	60
#define T0H_NS		(400 - GPIO_SET_VALUE_DELTA)
#define T0L_NS		(850 - GPIO_SET_VALUE_DELTA)
#define T1H_NS		(800 - GPIO_SET_VALUE_DELTA)
#define T1L_NS		(450 - GPIO_SET_VALUE_DELTA)

// FIXME: Hardcoded for now. Will be changed in the future
#define GPIO_PIN 18
#define LED_COUNT 3

/* High precision nanodelay */
#define hpr_set(timer, delay) \
	timer = ktime_get_ns();\
	timer += (delay);

#define hpr_wait(timer) \
	while (ktime_get_ns() < timer);

static int major_num;
static struct class* class;
static struct device* dev;
static uint8_t usercount;
static uint8_t led_data[LED_COUNT * 3];

static void led_sync(void)
{
	uint8_t mask = 0;
	DEFINE_SPINLOCK(spin_lock);
	uint32_t spin_lock_flags;
	uint64_t hpr_timer;

	/* GPIO speed workaround */
	spin_lock_irqsave(&spin_lock, spin_lock_flags);
	for (int i = 0; i < 1000000; ++i) {
		gpio_set_value(GPIO_PIN, 0);
	}
	spin_unlock_irqrestore(&spin_lock, spin_lock_flags);

	/* Sending 3 bytes per LED */
	spin_lock_irqsave(&spin_lock, spin_lock_flags);
	for (int i = 0; i < sizeof(led_data); ++i) {
		mask = 1;
		for (int j = 0; j < 8; ++j) {
			if (led_data[i] & mask) {
				hpr_set(hpr_timer, T1H_NS);
				gpio_set_value(GPIO_PIN, 1);
				hpr_wait(hpr_timer);

				hpr_set(hpr_timer, T1L_NS);
				gpio_set_value(GPIO_PIN, 0);
				hpr_wait(hpr_timer);
			} else {
				hpr_set(hpr_timer, T0H_NS);
				gpio_set_value(GPIO_PIN, 1);
				hpr_wait(hpr_timer);

				hpr_set(hpr_timer, T0L_NS);
				gpio_set_value(GPIO_PIN, 0);
				hpr_wait(hpr_timer);
			}
			mask <<= 1;
		}
	}
	spin_unlock_irqrestore(&spin_lock, spin_lock_flags);

	/* Wait Treset time before sending new sequence */
	udelay(TRESET_US);
}

static int dev_open(struct inode *inode, struct file *file)
{
	// FIXME: Temp. To avoid working on synchronization
	if (usercount) {
		return -EFAULT;
	}

	++usercount;
	return 0;
}

static int dev_release(struct inode *inode, struct file *file)
{
// FIXME: Temp. For testing purposes.
	led_sync();
// ENDFIX

	--usercount;
	return 0;
}

static ssize_t dev_read(struct file *file, char * buff, size_t len, loff_t *off)
{
	int ret;

	if (*off >= sizeof(led_data)) {
		return 0;
	}

	// TODO: Check size of buff
	ret = copy_to_user(buff, led_data, sizeof(led_data));
	if (!ret) {
		*off += sizeof(led_data);
		return sizeof(led_data);
	} else {
		return -EFAULT;
	}

	return 0;
}

static ssize_t dev_write(struct file *file, const char *buff, size_t len, loff_t *off)
{
	int ret;

	if (*off >= sizeof(led_data)) {
		return 0;
	}

	// TODO: Check size of buff
	ret = copy_from_user(led_data, sizeof(led_data), buff);
	if (!ret) {
		*off += sizeof(led_data);
		return sizeof(led_data);
	} else {
		return -EFAULT;
	}

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
	int res;

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

	res = gpio_request(GPIO_PIN, DEV_NAME);
	if (res) {
		return -EFAULT;
	}

	res = gpio_direction_output(GPIO_PIN, 0);
	if (res) {
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

	gpio_free(GPIO_PIN);
	printk(KERN_INFO DEV_NAME ": deinit done\n");
}

module_init(mod_init);
module_exit(mod_exit);
