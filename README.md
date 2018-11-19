该工程是基于`https://github.com/Baidu-AIP/sdk-demo`的c版本。github上的是c++版本，而且依赖于`jsoncpp`库以及`c++11`特性，不太适合交叉编译。这个demo使用了轻量级的json库`cJSON`而且完全用c语言实现，已经在2440开发板上跑通。如果需要移植到其他平台，只需要修改下交叉编译工具即可，非常方便。

## 交叉编译依赖库
本工程只依赖于curl库，用来发送https请求，使用duos的REST API。因为是https，所以，在编译`curl`之前，还需要编译`openssl`库。
- 下载openssl源代码
```bash
wget https://www.openssl.org/source/openssl-1.1.0i.tar.gz
```
- 解压，编译，安装。
```bash
tar -xzf openssl-1.1.0i.tar.gz
cd openssl-1.1.0i
mkdir install
./config shared --prefix=`pwd`/install       # shared选项表示生成动态库，--prefix 参数为欲安装之目录
make
make install
```

编译完成之后，把头文件和库复制到交叉工具链里，同时也复制到开发板上。接下来，编译`curl`:

- 下载:https://curl.haxx.se/download.html
- 编译:
```bash
./configure --host=arm-linux CC=arm-linux-gcc CXX=arm-linux-g++ --with-ssl --enable-shared --enable-static --disable-dict --disable-ftp --disable-imap --disable-ldap --disable-ldaps --disable-pop3 --disable-proxy --disable-rtsp --disable-smtp --disable-telnet --disable-tftp --disable-zlib --without-ca-bundle --without-gnutls --without-libidn --without-librtmp --without-libssh2 --without-nss --without-zlib --prefix=$PWD/install
make
make install
```
- 把头文件和库复制到交叉工具链里，同时也复制到开发板上。


## 编译工程
- 在`asr_simu.c`中有提示需要你填写`AK` `SK`的地方，相应填写你申请到的`AK` `SK`。
- 修改`Makefile`中的交叉编译工具对应你的平台。
- make
- 将生成的`asr_simu`下载到开发板测试。


## 测试脚本
声卡驱动已经移植ok,基于`ALSA`框架，而且`alsa-lib` `alsa-util`都已经交叉编译安装完毕，这里就不在赘述。同时，因为从服务器下载下来的是mp3文件，如果要播放的话，还需要交叉编译`madplayer`。此处，也不在赘述。
```bash
#!/bin/sh


while :
do
        echo "start recording......"
        arecord -Dhw:0,0 -r16000 -f S16_LE -c 2 test.wav &
        read aaa
        killall arecord
        echo "start uploading......"
        ./asr_simu test.wav
        echo "start playing......"
        madplay -o wav:- test.mp3 | aplay
        rm -rf test.wav test.pcm test.mp3
done
```

测试效果很简单。就是，你说一句话，服务器会echo你刚才说的。其他复杂功能可以后续添加。

