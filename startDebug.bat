diskutil mbr writeboot -i "C:\programy\system\output\boot.bin" "C:\programy\system\os-image.img"
diskutil mbr gap -i "C:\programy\system\output\loader.bin" "C:\programy\system\os-image.img"
osfmount -a -t file -o rw -f os-image.img -m F:
xcopy output\* F:\SYSTEM\ /E /Y
osfmount -D -m F:
bochsdbg -f cfg.bxrc -q
del os-image.img.lock