diskutil mbr writeboot -i "C:\programy\system\boot.bin" "C:\programy\system\os-image.img"
diskutil mbr gap -i "C:\programy\system\loader.bin" "C:\programy\system\os-image.img"
osfmount -a -t file -o rw -f os-image.img -m F:
copy loader\LDR32 F:\SYSTEM\LDR32
osfmount -D -m F:
bochsdbg -f cfg64.bxrc -q
del os-image.img.lock