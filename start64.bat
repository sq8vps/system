diskutil mbr writeboot -i "C:\programy\system\boot.bin" "C:\programy\system\os-image.img"
diskutil mbr gap -i "C:\programy\system\loader.bin" "C:\programy\system\os-image.img"
bochs -f cfg64.bxrc -q
del os-image.img.lock