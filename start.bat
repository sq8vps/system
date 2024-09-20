osfmount -a -t file -o rw -f os-image.img -m F:
copy output\* F:\SYSTEM\
osfmount -D -m F:
bochs -f cfg.bxrc -q
del os-image.img.lock