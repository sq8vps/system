diskutil mbr writeboot -i "C:\programy\system\output\boot.bin" "C:\programy\system\os-image.img"
diskutil mbr gap -i "C:\programy\system\output\loader.bin" "C:\programy\system\os-image.img"
osfmount -a -t file -o rw -f os-image.img -m F:
xcopy output\* F:\SYSTEM\ /E /Y
osfmount -D -m F:
qemu-system-i386 -monitor stdio -no-shutdown -no-reboot os-image.img
del os-image.img.lock