diskutil mbr writeboot -i "D:\Programy wlasnej roboty\system\boot.bin" "D:\Programy wlasnej roboty\system\os-image.img"
diskutil mbr gap -i "D:\Programy wlasnej roboty\system\loader.bin" "D:\Programy wlasnej roboty\system\os-image.img"
bochsdbg -f cfg64.bxrc -q
del os-image.img.lock