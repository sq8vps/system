diskutil mbr writeboot -i "D:\programy\system\output\boot.bin" "D:\programy\system\os-image.img"
diskutil mbr gap -i "D:\programy\system\output\loader.bin" "D:\programy\system\os-image.img"
osfmount -a -t file -o rw -f os-image.img -m F:
xcopy output\* F:\SYSTEM\ /E /Y
osfmount -D -m F:
qemu-system-i386 -s -S -monitor stdio -no-shutdown -no-reboot os-image.img
del os-image.img.lock