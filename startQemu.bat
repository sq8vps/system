call make_initrd.bat
osfmount -a -t file -o rw -f os-image.img -m F:
xcopy output\* F:\SYSTEM\ /E /Y
copy /y grub.cfg F:\BOOT\GRUB\
osfmount -D -m F:
qemu-system-i386 -s -S -monitor stdio -no-shutdown -no-reboot -smp 8 -d unimp os-image.img
del os-image.img.lock