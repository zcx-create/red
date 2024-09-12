#include "server.h"

#include <stdio.h>
#include <string.h>

#include <openssl/sha.h>
#include <openssl/pem.h>
#include <openssl/bio.h>
#include <openssl/evp.h>

// GUID是特定值，与客户端的Sec-WebSocket-Key一起，通过SHA-1算法生成一个新值发送回客户端，完成握手，确保连接的安全
#define GUID "258EAFA5-E914-47DA-95CA-C5AB0DC85B11"

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

typedef struct _nty_websocket_head_127 nty_websocket_head_127;
typedef struct _nty_websocket_head_126 nty_websocket_head_126;
typedef struct _nty_ophdr nty_ophdr;

// 将输入字符串进行Base64编码
int base64_encode(char *in_str, int in_len, char *out_str)
{
    BIO *b64, *bio;
    BUF_MEM *bptr = NULL;
    size_t size = 0;

    if (in_str == NULL || out_str == NULL)
        return -1;

    b64 = BIO_new(BIO_f_base64()); // 创建Base64过滤BIO对象
    bio = BIO_new(BIO_s_mem()); // 创建内存对象，存储中间结果
    bio = BIO_push(b64, bio); // 形成BIO链

    BIO_write(bio, in_str, in_len); // 将输入字符写入BIO链
    BIO_flush(bio); // 刷新BIO链

    BIO_get_mem_ptr(bio, &bptr); // 获取内存BIO的指针bptr，指向编码后的数据
    memcpy(out_str, bptr->data, bptr->length); // 将编码后的数据从内存BIO复制到输出字符串out_str
    out_str[bptr->length - 1] = '\0';
    size = bptr->length;

    BIO_free_all(bio); // 释放所有的BIO对象，防止内存泄漏
    return size;
}

// 从allbuf中读取一行文本并存储到linebuf
int readline(char *allbuf, int level, char *linebuf)
{
    int len = strlen(allbuf);

    for (; level < len; ++level)
    {
        if (allbuf[level] == '\r' && allbuf[level + 1] == '\n')
            return level + 2;
        else
            *(linebuf++) = allbuf[level];
    }

    return -1;
}

// 解码操作
void demask(char *data, int len, char *mask)
{
    int i;
    for (i = 0; i < len; ++i)
        *(data + i) ^= *(mask + (i % 4));
}

// 解析数据包
char *decode_packet(unsigned char *stream, char *mask, int length, int *ret)
{
    nty_ophdr *hdr = (nty_ophdr *)stream; // 将输入流的前sizeof(nty_ophdr)字节解释为nty_ophdr类型的头部
    unsigned char *data = stream + sizeof(nty_ophdr); // 将data指针移动到头部之后的位置
    int size = 0;
    int start = 0;
    int i = 0;

    /* 根据头部信息中的掩码位来判断数据包的格式*/
    if ((hdr->mask & 0x7F) == 126)
    {
        nty_websocket_head_126 *hdr126 = (nty_websocket_head_126 *)data;
        size = hdr126->payload_length;
        for (i = 0; i < 4; ++i)
        {
            mask[i] = hdr126->mask_key[i];
        }
        start = 8;
    }
    else if ((hdr->mask & 0x7F) == 127)
    {
        nty_websocket_head_127 *hdr127 = (nty_websocket_head_127 *)data;
        size = hdr127->payload_length;
        for (i = 0; i < 4; ++i)
        {
            mask[i] = hdr127->mask_key[i];
        }
        start = 14;
    }
    else // 使用标准头部结构
    {
        size = hdr->payload_length;
        memcpy(mask, data, 4);
        start = 6;
    }
    *ret = size;
    demask(stream + start, size, mask); // 解码
    return stream + start;
}

// 将原始数据编码成数据包
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

#define WEBSOCK_KEY_LENGTH 19

// 握手
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

// 处理接收到的请求，根据连接状态进行握手和数据解码
int ws_request(struct conn *c)
{
    printf("request: %s\n", c->rbuffer);

    if (c->status == 0)
    {
        handshark(c);
        c->status = 1;
    }
    else if (c->status == 1)
    {
        char mask[4] = {0};
        int ret = 0;
        c->payload = decode_packet(c->rbuffer, c->mask, c->rlength, &ret);
        printf("data : %s , length : %d\n", c->payload, ret);
        c->wlength = ret;
        c->status = 2;
    }
    return 0;
}

// 处理要发送的响应，根据连接状态进行数据编码
int ws_response(struct conn *c)
{
    if (c->status == 2)
    {
        c->wlength = encode_packet(c->wbuffer, c->mask, c->payload, c->wlength);
        c->status = 1;
    }
    return 0;
}
