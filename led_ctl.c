#include <stdio.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/ioctl.h>
 
#include "leds_ed.h"
 
void read_vars(int fd)
{
    leds_arg_t l;
 
    if (ioctl(fd, READ_LEDS, &l) == -1)
    {
        perror("ioctl Read");
    }
    else
    {
        printf("Leds set to : %x\n", l.leds_set);
    }
}
void off_vars(int fd)
{
    if (ioctl(fd, OFF_LEDS) == -1)
    {
        perror("ioctl OFF");
    }
}
void on_vars(int fd)
{
    if (ioctl(fd, ON_LEDS) == -1)
    {
        perror("ioctl ON");
    }
}
void blink_vars(int fd)
{
    if (ioctl(fd, BLINK_LEDS) == -1)
    {
        perror("ioctl BLINK");
    }
}
/*void set_vars(int fd)
{
    int v;
    leds_arg_t l;
 
    printf("Enter Status: ");
    scanf("%d", &v);
    getchar();
    l.leds_set = leds_set;
 
    if (ioctl(fd, SET_LEDS, &l) == -1)
    {
        perror("ioctl SET");
    }
}*/
 
int main(int argc, char *argv[])
{
    char *file_name = "/dev/EddyLED";
    int fd;
    enum
    {
        e_read,
        e_on,
        e_off,
	e_blink
    } option;
 
    if (argc == 1)
    {
        option = e_blink;
    }
    else if (argc == 2)
    {
        if (strcmp(argv[1], "-on") == 0)
        {
            option = e_on;
        }
        else if (strcmp(argv[1], "-off") == 0)
        {
            option = e_off;
        }
        else if (strcmp(argv[1], "-read") == 0)
        {
            option = e_read;
        }
        else if (strcmp(argv[1], "-blink") == 0)
        {
            option = e_blink;
        }
        else
        {
            fprintf(stderr, "Usage: %s [-on | -off | -read | -blink]\n", argv[0]);
            return 1;
        }
    }
    else
    {
        fprintf(stderr, "Usage: %s [-on | -off | -read | -blink]\n", argv[0]);
        return 1;
    }
    fd = open(file_name, O_RDWR);
    if (fd == -1)
    {
        perror("Leds open");
        return 2;
    }
 
    switch (option)
    {
        case e_read:
            read_vars(fd);
            break;
        case e_off:
            off_vars(fd);
            break;
        case e_on:
            on_vars(fd);
            break;
        case e_blink:
            blink_vars(fd);
            break;
        default:
            break;
    }
 
    close (fd);
 
    return 0;
}
