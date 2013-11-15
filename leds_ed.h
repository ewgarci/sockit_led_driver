#include <linux/ioctl.h>
 
typedef struct
{
    unsigned char leds_set;
} leds_arg_t;


#define LEDS_MAGIC 'l'
#define BLINK_LEDS _IO(LEDS_MAGIC, 0)
#define ON_LEDS _IO(LEDS_MAGIC, 1)
#define OFF_LEDS _IO(LEDS_MAGIC, 2)
#define READ_LEDS _IOR(LEDS_MAGIC, 3, leds_arg_t *)
#define SET_LEDS _IOW(LEDS_MAGIC, 4, leds_arg_t *)
 
 
