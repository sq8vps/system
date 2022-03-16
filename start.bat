diskutil mbr writeboot -i "C:\Users\quati\Desktop\system\boot.bin" "C:\Users\quati\Desktop\system\os-image.img"
diskutil mbr gap -i "C:\Users\quati\Desktop\system\loader.bin" "C:\Users\quati\Desktop\system\os-image.img"
osfmount -a -t file -o rw -f os-image.img -m F:
copy loader\LDR32 F:\SYSTEM\LDR32
osfmount -D -m F:
bochs -f cfg.bxrc -q
del os-image.img.lock