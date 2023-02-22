

# README

## COS_Lab1

### 搭建一个最简操作系统

在bochs运行，启动后显示Hello World



BIOS会在通电后，检查软盘0面0磁道1扇区的内容，如果扇区以0xaa55结束，就认定为引导扇区，将这个扇区的512字节的数据加载到内存的07c00处，然后设置PC（程序计数器），跳转到内存07c00处开始执行代码

> 0xaa55以及07c00都是一种约定

编写汇编文件boot.asm，使用nasm将其生成二进制代码，一个512字节的二进制文件

```shell
nasm boot.asm -o boot.bin
```

可以反编译boot.bin

```shell
ndisasm boot.bin -o 0
```


生成一个a.img，作为虚拟软盘，大小有1.44m

```shell
bximage -> fd -> 1.44 -> a.img
```

将boot.bin写入软盘的第一个扇区

```shell
dd if=boot.bin of=a.img bs=512 count=1 conv=notrunc
```

配置虚拟机bochs，bochsrc

```shell
megs:32
display_library: sdl
floppya: 1_44=a.img, status=inserted
boot: floppy
```

启动bochs

```shell
bochs -f bochsrc
```

### 汇编语言练习

用汇编语言实现大整数的加法和乘法



使用nasm将.asm代码汇编过后得到二进制的目标文件，经过链接生成可执行文件，本次试验是32位的asm文件

```asm
# 64位MacOS
nasm -f macho64 main.asm -DDEBUG
ld -e _start main.o -macosx_version_min 10.13 -lSystem

# 32位Ubuntu
nasm -f elf32 main.asm
ld -m elf_i386 -o main main.o
```

## COS_Lab2

<<<<<<< HEAD
## COS_Lab3

## COS_Lab4

##
=======
使用C/Cpp和asm编写一个FAT12镜像查看工具，读取一个.img格式的文件并响应用户输入，asm文件用于输出，允许用户一直输入并且有错误提示。

用户输入

```shell
$ ls # 会输出根目录及其子目录的文件和目录列表。
/NJU/:
. .. ABOUT.TXT CS SOFTWARE 
/NJU/CS/:
. .. 
/NJU/SOFTWARE/:
. .. SE1.TXT SE2.TXT 
>ls
/:
HOUSE NJU ROLL.TXT LONG.TXT 
/HOUSE/:
. .. ROOM 
/HOUSE/ROOM/:
. .. 
/NJU/:
. .. ABOUT.TXT CS SOFTWARE 
/NJU/CS/:
. .. 
/NJU/SOFTWARE/:
. .. SE1.TXT SE2.TXT 
$ ls -l # 输出目录和文件的属性，目录则输入子目录树和子目录的文件数量，文件则输出大小
/ 2 2:
HOUSE 1 0
NJU 2 1
ROLL.TXT 1945
LONG.TXT 2256

/HOUSE/ 1 0:
. 
.. 
ROOM 0 0

/HOUSE/ROOM/ 0 0:
. 
.. 

/NJU/ 2 1:
. 
.. 
ABOUT.TXT 1390
CS 0 0
SOFTWARE 0 2

/NJU/CS/ 0 0:
. 
.. 

/NJU/SOFTWARE/ 0 2:
. 
.. 
SE1.TXT 48
SE2.TXT 49
$ ls -l /NJU
/NJU/:
. .. ABOUT.TXT CS SOFTWARE 
/NJU/CS/:
. .. 
/NJU/SOFTWARE/:
. .. SE1.TXT SE2.TXT 
$ ls -ll /NJU
/NJU/ 2 1:
. 
.. 
ABOUT.TXT 1390
CS 0 0
SOFTWARE 0 2

/NJU/CS/ 0 0:
. 
.. 

/NJU/SOFTWARE/ 0 2:
. 
.. 
SE1.TXT 48
SE2.TXT 49
$ ls /NJU
/NJU/:
. .. ABOUT.TXT CS SOFTWARE 
/NJU/CS/:
. .. 
/NJU/SOFTWARE/:
. .. SE1.TXT SE2.TXT 
$ cat test.txt


$ exit # 退出
```

## COS_Lab3

在Lab1搭建的nasm+bochs平台上完成一个接受键盘输入，回显到屏幕上的程序

- 支持数字和英文大小写
- 支持大小写锁定
- 支持回车换行和退格删除、支持空格和Tab
- 每隔20s清空屏幕
- 光标显示，跟随屏幕
- 使用Esc进入查找模式，查找模式不会清空屏幕
  - 查找按回车后，查到的关键词红色显示，屏蔽除了ESC之外的任何输入
  - 再按ESC，光标归位，文本回复白色
- control + z支持撤回操作，包含回车和Tab和删除，直到初始状态



参考《orange's一个操作系统的实现》第三章，第四章，第五章和第七章的内容

## COS_Lab4

在nasm+bochs实验平台上实现特定进程调度问题的模拟

- 添加系统调用，接受int 型参数milli_seconds ，调用此方法进程会在milli_seconds 毫秒内不被分配时间片
- 添加系统调用**打印字符串**，接受char* 型参数str
- 添加两个系统调用**执行信号量****PV** 操作，在此基础上模拟**读者写者问题**。
  - 三个读者，两个个写者，一个普通
    - A阅读消耗2个时间片
    - B、C阅读消耗3个时间片
    - D写消耗3个时间片
    - E写消耗4个时间片
  - ABCDE打印基本操作：读开始、写开始、读、写、读完成、 写完成，以及对应进程名字
  - F每隔一个时间片打印当前读或者写
  - 读者优先和写者优先
  - 解决进程饿死问题





