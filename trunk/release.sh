#/bin/bash
make mrproper && make ls2fdev_defconfig && make && rm -fr rel && mkdir rel && INSTALL_MOD_PATH=rel make modules_install && mkdir rel/boot && cp vmlinux rel/boot/ && cd rel && tar -cjf linux-2.6.23.1-ls2f.tar.bz2 boot lib

