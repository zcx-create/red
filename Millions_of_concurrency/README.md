# 4台虚拟机实现百万并发

准备了4台虚拟机来模拟百万并发，一台当服务器，三台当客户端，服务器的内存为6G，客户端的内存均为4G

服务端采用了基于epoll的reactor网络模型，开20个端口来提供客户端访问

打印消息为：每1000个连接，打印一条信息以及所用的耗时

先来看最终结果图

<img width="239" alt="百万并发达成" src="https://github.com/user-attachments/assets/66375e06-b4cf-4d2d-861f-b0cf11992bd2">

最终跑到了102万个连接，百万并发达成，下面详细讲解可能出现的问题

## 1、编译错误

直接编译服务端代码

```shell
gcc -o server server.c
```

可能会出现下面的类似错误

![image-20240822124133009](https://github.com/user-attachments/assets/041ab9ec-a1e7-41af-b11b-48fcd93442ee)


只需要添加编译选项即可，改为下面这样即可编译成功

```shell
gcc -mcmodel=medium -o server server.c
```

## 2、Too many open files

如图所示：

![image-20240817145810633](https://github.com/user-attachments/assets/c73b4e3c-3baa-473e-a424-462e463d433c)


其实就是可以打开的文件受到限制，通过ulimit -a选项查看

```shell
ulimit -a
```

![image-20240817150130019](https://github.com/user-attachments/assets/b26e90b9-5e78-45cd-9748-76ed6d67e68c)


可以看到open files的值为1024

修改方法：ulimit -n 1048576，注意服务端和客户端都要修改

```shell
ulimit -n 1048576
```

![image-20240817150307875](https://github.com/user-attachments/assets/1d685c08-97a4-4479-ad49-81633493d5c4)


注意：此方法修改open files只是暂时的，在重启之后会重置为1024

## 3、Cannot assign requested address

如下所示：

![image-20240817153102103](https://github.com/user-attachments/assets/8e23e3ed-e84e-4579-9ca8-a4417e63f3d6)


其实就是五元组不够。五元组是什么呢？就是服务器IP、客户端IP、服务器端口、客户端端口、协议。具体来说，这里主要是端口受限，可用端口为1024-65535，在我的虚拟机上，系统默认的可用端口为32768-60999，通过如下指令查看

```shell
cat /proc/sys/net/ipv4/ip_local_port_range
```

<img width="344" alt="端口受限" src="https://github.com/user-attachments/assets/151bc549-2e95-44db-8547-948750868c0c">


永久修改端口范围的方法

①打开/etc/sysctl.conf文件

```shell
vim /etc/sysctl.conf
```

②在文件中添加或修改：

```shell
net.ipv4.ip_local_port_range = 1024 65535
```

③保存并退出，然后运行

```shell
sysctl -p
```

一般的朋友在上述操作后是可以连接到百万的，如果做不到，说明还有限制没有打开

## 4、NAT 表溢出问题

因为我的网络是用的NAT，并发连接数过多可能会导致NAT表溢出，从而导致新的连接失败。这个问题比较独特，在运行的时候甚至不会报错，就一直卡在那里，然后显示连接超时，我费了老大劲才搞定

查看NAT表的大小：

```shell
sysctl net.netfilter.nf_conntrack_max
```

![image-20240818162314882](https://github.com/user-attachments/assets/728f9149-7bad-401c-8377-3419a985f665)


默认限制为65535

可以通过增加NAT表的大小来缓解这一问题，指令如下

```shell
sysctl -w net.netfilter.nf_conntrack_max=1048576
```

## 5、总结

百万并发对epoll来说是小菜一碟，问题在于如何解除所有系统限制，可以打开的文件数量限制和五元组的限制是所有人都会遇到的问题，NAT表溢出是比较独特的问题，这两份代码是可以跑到百万并发的（在物理机性能不要太低的情况下），每个人系统的独特限制可能会不一样，如果你出现了新的问题，可以在评论区留言讨论！
