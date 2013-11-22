# if KERNELRELEASE is defined, we've been invoked from the
# kernel build system and can use its language.
ifneq (${KERNELRELEASE},)
#	obj-m := hello.o memTest.o ofcd.o
	obj-m := leds_ed.o

else
#	KERNEL_SOURCE := /usr/src/kernels/2.6.18-371.1.2.el5-x86_64
	KERNEL_SOURCE := /home/admin/Desktop/linux-altera-3.7
#	KERNEL_SOURCE := /home/admin/Desktop/linux-3.7.1
	PWD := $(shell pwd)

default: module led_ctl

module:
	${MAKE} -C ${KERNEL_SOURCE} SUBDIRS=${PWD} modules

clean:
	${MAKE} -C ${KERNEL_SOURCE} SUBDIRS=${PWD} clean
endif	
