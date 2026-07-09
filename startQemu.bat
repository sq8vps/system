cd initrd
tar cf ..\image\initrd.tar *
cd ..
osfmount -a -t file -o rw -f os-image.img -m F:
xcopy image\* F:\SYSTEM\ /E /Y
copy /y grub.cfg F:\BOOT\GRUB\
osfmount -D -m F:
qemu-system-i386 -s -S -monitor stdio -no-shutdown -no-reboot -d unimp -usb -smp 8 os-image.img
del os-image.img.lock