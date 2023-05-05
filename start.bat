diskutil mbr writeboot -i "C:\programy\system\boot.bin" "C:\programy\system\os-image.img"
diskutil mbr gap -i "C:\programy\system\loader.bin" "C:\programy\system\os-image.img"
osfmount -a -t file -o rw -f os-image.img -m F:
copy output\* F:\SYSTEM\
osfmount -D -m F:
bochs -f cfg.bxrc -q
del os-image.img.lock