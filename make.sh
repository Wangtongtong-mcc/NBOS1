#rm -rf /Users/wangtongtong/CLionProjects/NBOS1/target/*

#REMOTEPATH=/home/xiaohaizi/wangtongtong/tmp/NBOS1
REMOTEPATH=/Users/wangqingfeng/tmp/NBOS1

LOCALHOST=wangtongtong@192.168.0.113
LOCALPATH=/Users/wangtongtong/CLionProjects/NBOS1

#REMOTEBASE=/home/xiaohaizi/wangtongtong/tmp
REMOTEBASE=/Users/wangqingfeng/tmp



COMMAND="rm -rf $REMOTEPATH; \
    scp -r $LOCALHOST:$LOCALPATH $REMOTEBASE/;\
    cd $REMOTEPATH;\
    /usr/bin/make;\
    scp -r $REMOTEPATH/target $LOCALHOST:$LOCALPATH/;
    "
#ssh xiaohaizi@192.168.0.108 ${COMMAND}

#ssh wangqingfeng@192.168.0.112 ${COMMAND}


dd if=target/bootsect.bin of=/Users/wangtongtong/opt/bochs/60hd.img bs=512 count=1 conv=notrunc
dd if=target/loader.bin of=/Users/wangtongtong/opt/bochs/60hd.img bs=512 count=5 seek=1 conv=notrunc
dd if=target/kernel.bin of=/Users/wangtongtong/opt/bochs/60hd.img bs=512 count=200 seek=6 conv=notrunc
dd if=target/shell.bin of=/Users/wangtongtong/opt/bochs/60hd.img bs=512 count=11 seek=357 conv=notrunc
dd if=target/mypro.bin of=/Users/wangtongtong/opt/bochs/60hd.img bs=512 count=11 seek=372 conv=notrunc

