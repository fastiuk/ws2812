#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Yevhen Fastiuk");
MODULE_DESCRIPTION("Kernel module for using WS2812 LED");

static int __init ws2812_init(void)
{
	printk(KERN_INFO "%s: Hey!\n", __func__);
	return 0;
}

static void __exit ws2812_exit(void)
{
	printk(KERN_INFO "%s: Bye!\n", __func__);
}

module_init(ws2812_init);
module_exit(ws2812_exit);
