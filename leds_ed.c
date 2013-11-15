#include <linux/module.h>
#include <linux/version.h>
#include <linux/kernel.h> /* printk() */
#include <linux/types.h> /* size_t */
#include <linux/kdev_t.h>
#include <linux/fs.h> /* everything... */
#include <linux/device.h>
#include <linux/errno.h> /* error codes */
#include <linux/cdev.h>
#include <linux/delay.h>
#include <asm/system.h> /* cli(), *_flags */
#include <asm/uaccess.h> /* copy_from/to_user */
#include <asm/io.h> /* copy_from/to_user */
#include <linux/platform_device.h>
#include <linux/of.h>
#include "leds_ed.h" /* copy_from/to_user */

#define HW_REGS_BASE ( 0xfc000000 )
#define HW_REGS_SPAN ( 0x04000000 )
#define HW_REGS_MASK ( HW_REGS_SPAN - 1 )

#define ALT_GPIO1_OFST  ( 0xff709000 )
#define ALT_GPIO_SWPORTA_DDR_OFST       ( 0x4 )
#define ALT_GPIO_SWPORTA_DR_OFST        ( 0x0 )

#define GPIO1_DIR_REGISTER  (ALT_GPIO1_OFST + ALT_GPIO_SWPORTA_DDR_OFST)
#define GPIO1_DATA_REGISTER (ALT_GPIO1_OFST + ALT_GPIO_SWPORTA_DR_OFST)

#define BIT_LED_0   (0x01000000)
#define BIT_LED_1   (0x02000000)
#define BIT_LED_2   (0x04000000)
#define BIT_LED_3   (0x08000000)

#define BIT_LED_ALL  (BIT_LED_0 | BIT_LED_1 | BIT_LED_2 | BIT_LED_3)

static void *leds_reg; 
static dev_t first; // Global variable for the first device number 
static struct cdev c_dev; // Global variable for the character device structure
static struct class *cl; // Global variable for the device class
unsigned char leds_set = 0;

static void write_to_leds(void);

static void write_to_leds(void)
{
      iowrite32((leds_set << 6*4), ( leds_reg + ( ( u32 )( GPIO1_DATA_REGISTER ) & ( u32 )( HW_REGS_MASK ) ) ));
}

static long my_ioctl(struct file *f, unsigned int cmd, unsigned long arg)
{
    leds_arg_t l;
 
    switch (cmd)
    {
        case READ_LEDS:
            l.leds_set = leds_set;
            if (copy_to_user((leds_arg_t *)arg, &l, sizeof(leds_arg_t)))
            {
                return -EACCES;
            }
            break;
        case OFF_LEDS:
            leds_set = 0;
      	    write_to_leds();
            break;
        case ON_LEDS:
	    leds_set = 0xF;
      	    write_to_leds();
            break;
        case BLINK_LEDS:
	    leds_set = 0;
      	    write_to_leds();
	    msleep(500);
	    leds_set = 0x1;
      	    write_to_leds();
	    msleep(500);
	    leds_set = 0x2;
      	    write_to_leds();
	    msleep(500);
	    leds_set = 0x4;
      	    write_to_leds();
	    msleep(500);
	    leds_set = 0x8;
      	    write_to_leds();
	    msleep(500);
	    leds_set = 0xF;
      	    write_to_leds();
	    msleep(500);
	    leds_set = 0;
      	    write_to_leds();
            break;
        case SET_LEDS:
            if (copy_from_user(&l, (leds_arg_t *)arg, sizeof(leds_arg_t)))
            {
                return -EACCES;
            }
            leds_set = l.leds_set;
      	    write_to_leds();
            break;
        default:
            return -EINVAL;
    }
 
    return 0;
}


static int my_open(struct inode *i, struct file *f)
{
  printk(KERN_INFO "Eddy LED Driver: open()\n");
  return 0;
}

static int my_close(struct inode *i, struct file *f)
{
  printk(KERN_INFO "Eddy LED Driver: close()\n");
  return 0; 
}

static ssize_t my_read(struct file *f, char __user *buf, size_t
  len, loff_t *off)
{
  printk(KERN_INFO "Eddy LED Driver: read()\n");
  if (copy_to_user(buf, &leds_set, 1) != 0)
      return -EFAULT;

  if (*off == 0) { 
    *off+=1; 
    return len; 
  } else { 
    return 0; 
  }  
}

static ssize_t my_write(struct file *f, const char __user *buf,
  size_t len, loff_t *off)
{
  printk(KERN_INFO "Eddy LED Driver: write()\n");
  if (copy_from_user(&leds_set, (buf + len - 1),1) != 0)
      return -EFAULT;
  else
      /* Turn on all LEDs  */
      write_to_leds();
      return 1;
}

static struct file_operations leds_fops =
{
  .owner = THIS_MODULE,
  .open = my_open,
  .release = my_close,
  .read = my_read,
  .write = my_write,
  .unlocked_ioctl = my_ioctl
};
 
static int altr_gpio_probe(struct platform_device *pdev)
{
        int err;
        struct resource *iomem;

  printk(KERN_INFO "Eddy LED Driver: getting resource \n");
        iomem = platform_get_resource(pdev, IORESOURCE_MEM, 0);
        if (!iomem) {
                err = -EINVAL;
                goto err_mem;
        }
  printk(KERN_INFO "Eddy LED Driver: got resource \n");

        if (!request_mem_region(iomem->start, resource_size(iomem),
                "EddyLED")) {
                err = -EBUSY;
                goto err_mem;
        }
  printk(KERN_INFO "Eddy LED Driver: got mem region\n");

        leds_reg = ioremap(iomem->start, resource_size(iomem));
        if (!leds_reg) {
                err = -ENOMEM;
                goto err_ioremap;
        }
  printk(KERN_INFO "Eddy LED Driver: io remaped\n");

        return 0;
/*
  if (alloc_chrdev_region(&first, 0, 1, "EddyLED") < 0)
  {
    return -1;
  }
  
  if ((cl = class_create(THIS_MODULE, "chardrv")) == NULL)
  {
    unregister_chrdev_region(first, 1);
    return -1;
  }

  if (device_create(cl, NULL, first, NULL, "EddyLED") == NULL)
  {
    class_destroy(cl);
    unregister_chrdev_region(first, 1);
    return -1;
  }
  
  cdev_init(&c_dev, &leds_fops);
  if (cdev_add(&c_dev, first, 1) == -1)
  {
    device_destroy(cl, first);
    class_destroy(cl);
    unregister_chrdev_region(first, 1);
    return -1;
  }
  printk(KERN_INFO "Eddy LED Driver: getting ready to write dir\n");

  iowrite32( BIT_LED_ALL, ( leds_reg + ( ( u32 )( GPIO1_DIR_REGISTER ) & ( u32 )( HW_REGS_MASK ) ) ) );
  printk(KERN_INFO "Eddy LED Driver: wrote output\n");

  return 0;
*/
err_ioremap:
        release_mem_region(iomem->start, resource_size(iomem));
err_mem:
        printk(KERN_ERR ": Failed to register GPIOs: %d\n", err);

        return err;
}

static int altr_gpio_remove(struct platform_device *pdev)
{
        struct resource *iomem = platform_get_resource(pdev, IORESOURCE_MEM, 0);
  device_destroy(cl, first);
  class_destroy(cl);
  unregister_chrdev_region(first, 1);
        iounmap(leds_reg);
        release_mem_region(iomem->start, resource_size(iomem));

        return 0;
}

static struct of_device_id altr_gpio_of_match[] = {
	{ .compatible = "snps,dw-gpio-1.0", "snps,dw-gpio", "altr,pio-1.0",},
	{},
};

MODULE_DEVICE_TABLE(of, altr_gpio_of_match);

static struct platform_driver altr_gpio_driver = {
	.driver = {
		.name	= "gpio",
		.owner	= THIS_MODULE,
		.of_match_table = of_match_ptr(altr_gpio_of_match),
	},
	.probe		= altr_gpio_probe,
	.remove		= altr_gpio_remove,
};

static int __init leds_ed_init(void) 
{
  printk(KERN_INFO "Eddy LED Driver: registered\n");
/*
  if (platform_driver_register(&altr_gpio_driver)< 0)
  {
    return -1;
  }
*/
///*
  printk(KERN_INFO "Eddy LED Driver: registered2\n");
 
  if ((leds_reg = ioremap(HW_REGS_BASE, HW_REGS_SPAN)) == NULL)
  {
    printk(KERN_ERR "Mapping LED failed\n");
    return -1;
  }
//*/

  if (alloc_chrdev_region(&first, 0, 1, "EddyLED") < 0)
  {
    return -1;
  }
  
  if ((cl = class_create(THIS_MODULE, "chardrv")) == NULL)
  {
    unregister_chrdev_region(first, 1);
    return -1;
  }

  if (device_create(cl, NULL, first, NULL, "EddyLED") == NULL)
  {
    class_destroy(cl);
  printk(KERN_INFO "Eddy LED Driver: got resource \n");
    unregister_chrdev_region(first, 1);
    return -1;
  }
  
  cdev_init(&c_dev, &leds_fops);
  if (cdev_add(&c_dev, first, 1) == -1)
  {
    device_destroy(cl, first);
    class_destroy(cl);
    unregister_chrdev_region(first, 1);
    return -1;
  }
  printk(KERN_INFO "Eddy LED Driver: getting ready to write direction\n");
  /* Set GPIO direction to output */
  iowrite32( BIT_LED_ALL, ( leds_reg + ( ( u32 )( GPIO1_DIR_REGISTER ) & ( u32 )( HW_REGS_MASK ) ) ) );
  printk(KERN_INFO "Eddy LED Driver: wrote output\n");

  return 0;
}
 
static void __exit leds_ed_exit(void)
{
  /* Turn off all  LEDs  */
  /*iowrite32( 0x00000000, ( leds + ( ( u32 )( GPIO1_DATA_REGISTER ) & ( u32 )( HW_REGS_MASK ) ) ) );*/
  cdev_del(&c_dev);
  device_destroy(cl, first);
  class_destroy(cl);
  unregister_chrdev_region(first, 1);
  platform_driver_unregister(&altr_gpio_driver);
  printk(KERN_INFO "Eddy LED Driver: unregistered\n");
}


/*module_platform_driver(altr_gpio_driver);*/
/*
static int __init altr_gpio_init(void)
{
	return platform_driver_register(&altr_gpio_driver);
}
subsys_initcall(altr_gpio_init);

static void __exit altr_gpio_exit(void)
{
	platform_driver_unregister(&altr_gpio_driver);
}
module_exit(altr_gpio_exit);*/

module_init(leds_ed_init);
module_exit(leds_ed_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Eddy G");
MODULE_DESCRIPTION("Character Driver Test");
