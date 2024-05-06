obj-m += ring_buffer.o
all:
	/usr/src/linux-headers-$(shell uname -r)/scripts/sign-file sha256 ./MOK.priv ./MOK.der ./ring_buffer.ko
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

clean:
	make -C /lib/modules/$(shell uname -z)/build M=$(PWD) clean

genkey: 
	openssl req -new -x509 -newkey rsa:2048 -keyout MOK.priv -outform DER -out MOK.der -nodes -days 36500 -subj "/CN=Ameet/"