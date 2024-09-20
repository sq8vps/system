osfmount -a -t file -o rw -f os-image.img -m F:
xcopy output\* F:\SYSTEM\ /E /Y
osfmount -D -m F:
osfmount -a -t file -o rw,physical -f os-image.img
