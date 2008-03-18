#/bin/bash
echo

CONFIG_NAME=ls2fdev
if [ "$#" -eq 1 ] && [ "$1" == "xgi" ];then
	echo "Build xgifb as video driver"
	CONFIG_NAME=ls2fdev_xgi
else
	echo "Build sisfb as video driver"
fi
echo
echo "Get the Revision number from svn"
Revision=`svn info|grep Revision:`
REV_NUM=${Revision##* }
echo "Revision:$REV_NUM"
echo

make mrproper && make ${CONFIG_NAME}_defconfig && make && rm -fr rel && mkdir rel && INSTALL_MOD_PATH=rel make modules_install && mkdir rel/boot && cp vmlinux rel/boot/ && cd rel && tar -cjf ../linux-2.6.23.1-r$REV_NUM-${CONFIG_NAME}.tar.bz2 boot lib

