osfmount -a -t file -o rw -f os-image.img -m F:
xcopy image\* F:\SYSTEM\ /E /Y
copy /y grub.cfg F:\BOOT\GRUB\
osfmount -D -m F:
del os-image.img.lock
bochsgdb -f cfggdb.bxrc -q
del os-image.img.lock