diskutil mbr writeboot -i "D:\programy\system\output\boot.bin" "D:\programy\system\os-image.img"
diskutil mbr gap -i "D:\programy\system\output\loader.bin" "D:\programy\system\os-image.img"
osfmount -a -t file -o rw -f os-image.img -m F:
copy output\* F:\SYSTEM\
osfmount -D -m F:
bochs -f cfg.bxrc -q
del os-image.img.lock