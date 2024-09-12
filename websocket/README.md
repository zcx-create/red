# 用基于epoll的reactor网络模型实现WebSocket

## 一、WebSocket 简介

上次用reactor实现了webserver，这次来实现WebSocket

什么是 WebSocket 呢？WebSocket 是一种网络传输协议，可在单个 TCP 连接上进行全双工通信。WebSocket 使得客户端和服务器之间的数据交换变得更加简单，**允许服务端主动向客户端推送数据**。在 WebSocket API 中，浏览器和服务器只需要完成一次握手，两者之间就可以建立持久性的连接，并进行双向数据传输。

WebSocket 通过 HTTP 端口 80 和 443 进行工作，并支持 HTTP 代理和中介，从而使其与 HTTP 协议兼容。 为了实现兼容性，WebSocket 握手使用 HTTP Upgrade 头从 HTTP 协议更改为 WebSocket 协议。

WebSocket 的主要特点包括：

（1）全双工通信：客户端和服务器可以同时发送和接收数据。

（2）长连接：一旦连接建立，除非主动关闭，否则连接会一直保持。

（3）低延迟：由于不需要频繁建立和关闭连接，数据传输的延迟较低。

（4）基于 TCP：WebSocket 协议是基于 TCP 协议的，所以具有可靠传输的特性。

WebSocket 通信的基本流程包括以下几个步骤：

（1）握手：客户端通过HTTP请求向服务器发起握手请求，请求升级协议为 WebSocket。

（2）协议升级：服务器接收到握手请求后，返回一个确认响应，协议升级为 WebSocket。

（3）数据传输：建立连接后，客户端和服务器之间可以直接进行数据传输。

（4）关闭连接：当通信结束时，任意一方可以发起关闭连接的请求，对方确认后连接关闭。

## 二、握手协议

WebSocket 是独立的、建立在 TCP 上的协议。Websocket 通过 HTTP/1.1 协议的 101 状态码进行握手。为了建立 Websocket 连接，需要通过浏览器发出请求，之后服务器进行回应，这个过程通常称为“握手”（Handshaking）。

### 例子

一个典型的Websocket握手请求如下

客户端请求：

```html
GET /chat HTTP/1.1
Host: server.example.com
Upgrade: websocket
Connection: Upgrade
Sec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==
Origin: http://example.com
Sec-WebSocket-Protocol: chat, superchat
Sec-WebSocket-Version: 13
```

服务器回应：

```html
HTTP/1.1 101 Switching Protocols
Upgrade: websocket
Connection: Upgrade
Sec-WebSocket-Accept: s3pPLMBiTxaQ9kYGzzhZRbK+xOo=
Sec-WebSocket-Protocol: chat
```

#### 字段说明

（1）Connection 必须设置 Upgrade，表示客户端希望连接升级。

（2）Upgrade 字段必须设置 Websocket，表示希望升级到 Websocket 协议。

（3）Sec-WebSocket-Key 是随机的字符串，服务器端会用这些数据来构造出一个 SHA-1 的信息摘要。把“Sec-WebSocket-Key”加上一个特殊字符串**“258EAFA5-E914-47DA-95CA-C5AB0DC85B11”**，然后计算 **SHA-1** 摘要，之后进行 **Base64** 编码，将结果做为“Sec-WebSocket-Accept”头的值，返回给客户端。如此操作，可以尽量避免普通 HTTP 请求被误认为 Websocket 协议。

（4）Sec-WebSocket-Version 表示支持的 Websocket 版本。RFC6455 要求使用的版本是 13，之前草案的版本均应当弃用。

（5）Origin 字段是必须的。如果缺少 origin 字段，WebSocket 服务器需要回复 HTTP 403 状态码（禁止访问）。

（6）其他一些定义在 HTTP 协议中的字段，如 Cookie 等，也可以在 Websocket 中使用。

## 三、安装 OpenSSL 库

在实现WebSocket之前，首先要安装 OpenSSL 库或者开发头文件

在Ubantu系统上，执行以下命令

```bash
sudo apt-get update
sudo apt-get install libssl-dev
```

在CentOS系统上，执行以下命令

```bash
sudo yum install openssl-devel
```

## 四、实现WebSocket

### 4.1 WebSocket 编程流程

![image-20240826154046428](https://github.com/user-attachments/assets/6386e1b4-27d0-417b-8949-0f74fd0bf2bc)


### 4.2 实现握手

![image-20240826154513551](https://github.com/user-attachments/assets/b47c1b58-5bec-4ddf-82df-4e916acf1787)


代码如下：

```c
int handshark(struct conn *c)
{
    char linebuf[1024] = {0};
    int idx = 0;
    char sec_data[128] = {0}; // 存储SHA1加密后的数据
    char sec_accept[32] = {0}; // 存储Base64编码后的数据
    do
    {
        memset(linebuf, 0, 1024);
        idx = readline(c->rbuffer, idx, linebuf);
        if (strstr(linebuf, "Sec-WebSocket-Key")) // 如果当前行包含"Sec-WebSocket-Key"
        {
            strcat(linebuf, GUID); // 将GUID附加到Sec-WebSocket-Key中
            SHA1(linebuf + WEBSOCK_KEY_LENGTH, strlen(linebuf + WEBSOCK_KEY_LENGTH), sec_data); // 进行SHA1
            base64_encode(sec_data, strlen(sec_data), sec_accept); // 进行Base64编码
            memset(c->wbuffer, 0, BUFFER_LENGTH);
            // 构建响应报文
            c->wlength = sprintf(c->wbuffer, "HTTP/1.1 101 Switching Protocols\r\n"
                                             "Upgrade: websocket\r\n"
                                             "Connection: Upgrade\r\n"
                                             "Sec-WebSocket-Accept: %s\r\n\r\n",
                                 sec_accept);
            printf("ws response : %s\n", c->wbuffer);
            break;
        }
    } while ((c->rbuffer[idx] != '\r' || c->rbuffer[idx + 1] != '\n') && idx != -1);
    return 0;
}
```

### 4.3 解析请求报文

主要是解析报文头部字段，WebSocket定义了三种头部类型

```c
// 标准WebSocket头部，适用于长度小于126字节的数据
struct _nty_ophdr
{
    unsigned char opcode : 4,         // 操作码，表示帧的类型
        rsv3 : 1, rsv2 : 1, rsv1 : 1, // 保留位，通常设置为0
        fin : 1;                      // 表示是否为消息的最后一帧
    unsigned char payload_length : 7, // 有效载荷的长度
        mask : 1;                     // 表示是否对有效载荷进行掩码
} __attribute__((packed));            // 确保编译器不会对其进行填充，确保结构体的内存布局与协议中的字节布局完全一致

// 扩展头部，适用于长度在126到65535字节之间的数据
struct _nty_websocket_head_126
{
    unsigned short payload_length; // 有效载荷的长度
    char mask_key[4]; // 掩码密钥，用于解码有效载荷
    unsigned char data[8]; // 有效载荷的前8个字节
} __attribute__((packed));

// 另一种扩展头部，适用于长度超过65535字节的数据
struct _nty_websocket_head_127
{
    unsigned long long payload_length; 
    char mask_key[4];
    unsigned char data[8];
} __attribute__((packed));
```

分别按照不同的头部类型，分别解析即可

### 4.4 发送响应报文

发送响应报文前需要先编码，为什么一定需要编码呢？因为 WebSocket 协议定义了一种特定的数据帧结构来优化数据传输。也是根据想要发送的报文头部字段来分别处理，具体代码如下：

```C
int encode_packet(char *buffer, char *mask, char *stream, int length)
{
    /* 初始化头部 */
    nty_ophdr head = {0};
    head.fin = 1;
    head.opcode = 1;
    int size = 0;

    if (length < 126) // <126 使用基本头部
    {
        head.payload_length = length;
        memcpy(buffer, &head, sizeof(nty_ophdr));
        size = 2;
    }
    else if (length < 0xffff) // 使用nty_websocket_head_126扩展头部
    {
        nty_websocket_head_126 hdr = {0};
        hdr.payload_length = length;
        memcpy(hdr.mask_key, mask, 4);

        memcpy(buffer, &head, sizeof(nty_ophdr));
        memcpy(buffer + sizeof(nty_ophdr), &hdr, sizeof(nty_websocket_head_126));
        size = sizeof(nty_websocket_head_126);
    }
    else // 使用nty_websocket_head_127扩展头部
    {
        nty_websocket_head_127 hdr = {0};
        hdr.payload_length = length;
        memcpy(hdr.mask_key, mask, 4);
        memcpy(buffer, &head, sizeof(nty_ophdr));
        memcpy(buffer + sizeof(nty_ophdr), &hdr, sizeof(nty_websocket_head_127));
        size = sizeof(nty_websocket_head_127);
    }
    memcpy(buffer + size, stream, length);
    return length + size;
}
```

## 五、最终效果

实现的 WebSocket 功能比较简单，就是 echo

编译命令：gcc -o websocket reactor.c websocket.c -lssl -lcrypto

客户端如下：

![image-20240826155610289](https://github.com/user-attachments/assets/7e84b90c-c056-4ccc-a8ee-2c7fddc553f5)


点击连接：

![image-20240826155640026](https://github.com/user-attachments/assets/20dcbc68-b31e-4c93-94b9-9666b396b263)


客户端这边显示已连接，服务端显示如下内容：

```html
request: GET / HTTP/1.1
Host: 192.168.38.130:2000
Connection: Upgrade
Pragma: no-cache
Cache-Control: no-cache
User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/128.0.0.0 Safari/537.36 Edg/128.0.0.0
Upgrade: websocket
Origin: null
Sec-WebSocket-Version: 13
Accept-Encoding: gzip, deflate
Accept-Language: zh-CN,zh;q=0.9,en;q=0.8,en-GB;q=0.7,en-US;q=0.6
Sec-WebSocket-Key: HPvMZYKH9S7FZCIC/vS3rQ==
Sec-WebSocket-Extensions: permessage-deflate; client_max_window_bits


ws response : HTTP/1.1 101 Switching Protocols
Upgrade: websocket
Connection: Upgrade
Sec-WebSocket-Accept: dP0DLbvKaGlzfYY1K6WDOnHGOo8=
```

客户端发送信息，可以正常收发

![image-20240826155828153](https://github.com/user-attachments/assets/efd00067-6f82-42b8-802f-0a57ab00c9a8)


完整代码见：
