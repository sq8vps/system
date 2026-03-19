osfmount -a -t file -o rw -f os-image.img -m F:
xcopy image\* F:\SYSTEM\ /E /Y
copy /y grub.cfg F:\BOOT\GRUB\
osfmount -D -m F:
qemu-system-i386 -s -S -monitor stdio -no-shutdown -no-reboot -d unimp -smp 16 os-image.img
del os-image.img.lock