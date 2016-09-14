#include <linux/module.h>
#include <linux/gpio.h>
#include <linux/delay.h>
#include <linux/kernel.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/hrtimer.h>
#include <linux/ktime.h>
#include <linux/device.h>
#include <linux/kdev_t.h>
#include <linux/interrupt.h> 
#include <linux/sched.h>
#include <linux/errno.h>
#include <linux/cdev.h>
#include <linux/fs.h>

#define DEV_NAME		"hcsr04"

// Change these two lines to use differents GPIOs
#define HCSR04_ECHO		  59 
#define HCSR04_TRIGGER	58 
#define DEVICE_MAJOR    0
#define HCSR04_NR_DEVS  1

static int gpio_irq=-1;
static int valid_value = 0;

static ktime_t echo_start;
static ktime_t echo_end;

struct cdev hscr04_cdev;
int hcsr04_major = DEVICE_MAJOR;
int hcsr04_minor = 0;
int hcsr04_read_flag = 0;
int hcsr04_nr_devs = HCSR04_NR_DEVS;

dev_t devno;

// Interrupt handler on ECHO signal
static irqreturn_t gpio_isr(int irq, void *data)
{
	ktime_t ktime_dummy;

	//gpio_set_value(HCSR04_TEST,1);
  if(hcsr04_read_flag  == 0) return IRQ_HANDLED;
	if (valid_value==0) {
		ktime_dummy=ktime_get();
		if (gpio_get_value(HCSR04_ECHO)==1) {
			echo_start=ktime_dummy;
		} else {
			echo_end=ktime_dummy;
			valid_value=1;
		}
	}

	return IRQ_HANDLED;
}

static int hcsr04_open(struct inode *inode, struct file *filp){
	int rtc = 0;
	
	printk(KERN_INFO "HC-SR04 driver v0.1 device open.\n");
	rtc=gpio_request(HCSR04_TRIGGER,"TRIGGER");
	if (rtc!=0) {
		printk(KERN_INFO "Error %d\n",__LINE__);
		goto fail;
	}

	rtc=gpio_request(HCSR04_ECHO,"ECHO");
	if (rtc!=0) {
		printk(KERN_INFO "Error %d\n",__LINE__);
		goto fail;
	}

	rtc=gpio_direction_output(HCSR04_TRIGGER,0);
	if (rtc!=0) {
		printk(KERN_INFO "Error %d\n",__LINE__);
		goto fail;
	}

	rtc=gpio_direction_input(HCSR04_ECHO);
	if (rtc!=0) {
		printk(KERN_INFO "Error %d\n",__LINE__);
		goto fail;
	}

	rtc=gpio_to_irq(HCSR04_ECHO);
	if (rtc<0) {
		printk(KERN_INFO "Error %d\n",__LINE__);
		goto fail;
	} else {
		gpio_irq=rtc;
	}

	rtc = request_irq(gpio_irq, gpio_isr, IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING , "hc-sr04.trigger", NULL);

	if(rtc) {
		printk(KERN_ERR "Unable to request IRQ: %d\n", rtc);
		goto fail;
	}


	return 0;

fail:
	return -1;
}

// This function is called when you write something on /sys/class/hcsr04/value
static ssize_t hcsr04_write(struct file *filp, const char *buf, size_t count, loff_t *f_ops) {
	printk(KERN_INFO "Buffer len %d bytes\n", count);
        return count;
}

// This function is called when you read /sys/class/hcsr04/value

static ssize_t hcsr04_read(struct file *filp, char __user *buff,size_t count, loff_t *offp){
//(struct class *class, struct class_attribute *attr, char *buf) {
	int counter;

	hcsr04_read_flag = 1;
	// Send a 10uS impulse to the TRIGGER line
	gpio_set_value(HCSR04_TRIGGER,1);
	udelay(20);
	gpio_set_value(HCSR04_TRIGGER,0);
	valid_value=0;

	counter=0;
	while (valid_value==0) {
		// Out of range
		if (++counter>23200) {
			return sprintf(buff, "%d\n", -1);;
		}
		udelay(1);
	}
	hcsr04_read_flag = 0;
	//printk(KERN_INFO "Sub: %lld\n", ktime_to_us(ktime_sub(echo_end,echo_start)));
	return sprintf(buff, "%lld\n", ktime_to_us(ktime_sub(echo_end,echo_start)));;
}

static ssize_t hcsr04_close(struct inode *inode, struct file *filp){
    int rtc;
    gpio_free(HCSR04_ECHO);
    gpio_free(HCSR04_TRIGGER);
    rtc=gpio_to_irq(HCSR04_ECHO);
    free_irq(rtc, NULL);
    printk(KERN_INFO "HC-SR04 driver v0.2 closed\n");
    return 0;
}

static struct file_operations dev_fops = {
	.owner		= THIS_MODULE,
	.open		= hcsr04_open,
	.release	= hcsr04_close,
	.write =  hcsr04_write,
	.read		= hcsr04_read,
};


static int __init hcsr04_init(void){
	int rtc;
	
	printk(KERN_INFO "HC-SR04 driver v0.2 initializing.\n");

/* register to who? */
	if (hcsr04_major){
			devno = MKDEV(hcsr04_major, hcsr04_minor);
			rtc = register_chrdev_region(devno, hcsr04_nr_devs, DEV_NAME);
	}
	else
	{
			rtc = alloc_chrdev_region(&devno, hcsr04_minor, hcsr04_nr_devs, DEV_NAME); /* get devno */
			hcsr04_major = MAJOR(devno); /* get major */
			hcsr04_minor = MINOR(devno); /* get minor*/
	}
	if (rtc < 0)
	{
  	printk(KERN_INFO " %s can't get major %d\n", DEV_NAME, hcsr04_major);
		goto fail;
	}
        printk(KERN_INFO "hcsr04 major %d  minor %d\n", hcsr04_major,hcsr04_minor);
  cdev_init(&hscr04_cdev, &dev_fops);
	hscr04_cdev.owner = THIS_MODULE;	
	rtc = cdev_add(&hscr04_cdev, devno, 1);
	if (rtc < 0)
	{
		printk(KERN_INFO " %s cdev_add failure\n", DEV_NAME);
		goto fail;
	}

	return 0;

fail:
	return -1;
}

static void __exit hcsr04_exit(void){
	cdev_del(&hscr04_cdev);
	unregister_chrdev_region(devno,1);
	printk(KERN_INFO "Mode unregister");
}
module_init(hcsr04_init);
module_exit(hcsr04_exit);

MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR("PHL");
MODULE_DESCRIPTION("Driver for HC-SR04 ultrasonic sensor");
