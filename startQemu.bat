cd output
7z a -ttar initrd.tar drivers config.ndb mount.ndb
cd ..
osfmount -a -t file -o rw -f os-image.img -m F:
xcopy output\* F:\SYSTEM\ /E /Y
osfmount -D -m F:
qemu-system-i386 -s -S -monitor stdio -no-shutdown -no-reboot -smp 8 os-image.img
del os-image.img.lock