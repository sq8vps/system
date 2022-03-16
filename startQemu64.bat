diskutil mbr writeboot -i "D:\Programy wlasnej roboty\system\boot.bin" "D:\Programy wlasnej roboty\system\os-image.img"
diskutil mbr gap -i "D:\Programy wlasnej roboty\system\loader.bin" "D:\Programy wlasnej roboty\system\os-image.img"
osfmount -a -t file -o rw -f os-image.img -m F:
copy loader\LDR32 F:\SYSTEM\LDR32
osfmount -d -m F:
qemu-system-x86_64 -L "D:\Program Files\qemu" os-image.img