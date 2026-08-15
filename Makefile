obj-m += bridge_control_kpm.o

KDIR ?= /lib/modules/$(shell uname -r)/build
export KBUILD_MODPOST_WARN=1

all:
	$(MAKE) -C $(KDIR) M=$(PWD) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) CC=$(CC) modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean
