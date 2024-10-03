cd output
7z a -ttar initrd.tar drivers
cd ..
osfmount -a -t file -o rw -f os-image.img -m F:
xcopy output\* F:\SYSTEM\ /E /Y
osfmount -D -m F:
bochsdbg -f cfg.bxrc -q
del os-image.img.lock