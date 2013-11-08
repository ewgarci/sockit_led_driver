#include <linux/module.h>
#include <linux/version.h>
#include <linux/kernel.h> /* printk() */
#include <linux/types.h> /* size_t */
#include <linux/kdev_t.h>
#include <linux/fs.h> /* everything... */
#include <linux/device.h>
#include <linux/errno.h> /* error codes */
#include <linux/cdev.h>
#include <asm/system.h> /* cli(), *_flags */
#include <asm/uaccess.h> /* copy_from/to_user */
#include <asm/io.h> /* copy_from/to_user */

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

static void __iomem *leds; 
static dev_t first; // Global variable for the first device number 
static struct cdev c_dev; // Global variable for the character device structure
static struct class *cl; // Global variable for the device class
unsigned char c;

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
  if (copy_to_user(buf, &c, 1) != 0)
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
  if (copy_from_user(&c,(buf + len - 1),1) != 0)
      return -EFAULT;
  else
      /* Turn on all LEDs  */
      iowrite32((c << 6*4), ( leds + ( ( u32 )( GPIO1_DATA_REGISTER ) & ( u32 )( HW_REGS_MASK ) ) ));
      return 1;
}

static struct file_operations leds_fops =
{
  .owner = THIS_MODULE,
  .open = my_open,
  .release = my_close,
  .read = my_read,
  .write = my_write
};
 
static int __init leds_ed_init(void) /* Constructor */
{
  printk(KERN_INFO "Eddy LED Driver: registered\n");
  
  if ((leds = ioremap(HW_REGS_BASE, HW_REGS_SPAN)) == NULL)
  {
    printk(KERN_ERR "Mapping LED failed\n");
    return -1;
  }

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
  /* Set GPIO direction to output */
  iowrite32( BIT_LED_ALL, ( leds + ( ( u32 )( GPIO1_DIR_REGISTER ) & ( u32 )( HW_REGS_MASK ) ) ) );

  return 0;
}
 
static void __exit leds_ed_exit(void) /* Destructor */
{
  /* Turn off all  LEDs  */
  /*iowrite32( 0x00000000, ( leds + ( ( u32 )( GPIO1_DATA_REGISTER ) & ( u32 )( HW_REGS_MASK ) ) ) );*/
  cdev_del(&c_dev);
  device_destroy(cl, first);
  class_destroy(cl);
  unregister_chrdev_region(first, 1);
  iounmap(leds);
  printk(KERN_INFO "Eddy LED Driver: unregistered\n");
}
 
module_init(leds_ed_init);
module_exit(leds_ed_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Eddy G");
MODULE_DESCRIPTION("Character Driver Test");
