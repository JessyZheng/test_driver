/**
 * @file serialport_driver.cpp
 * @author jessy
 * @brief 
 * @version 0.1
 * @date 2021-11-09
 * 
 * @copyright Copyright (c) 2021
 * 
 */
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <termios.h>
#include <pthread.h>
#include <string>
#include <iostream>
#include <iomanip>
#include <chrono>
#include <sys/file.h>


#include "serialport_driver.h"

SerialPortDriver::~SerialPortDriver()
{
    if (!initialled)
        return;

    close(fd);
}

int SerialPortDriver::Init()
{
    std::cout << "SerialPortDriver::Init " << driver_name << std::endl;
    if (initialled)
        return 0;

    int ret, sfd;

    //打开串口
    ret = open(dev_name.c_str(), O_RDWR | O_NOCTTY | O_NDELAY);
    if (ret < 0)
    {
        std::cout << "open device failed(" << strerror(errno) << ")!" << std::endl;
        return -1;
    }
    else
    {
        sfd = ret;
    }

    //初始化的时候锁住设备
    if(flock(sfd, LOCK_EX | LOCK_NB) != 0)
    {
        std::cout << "flock(fd=" << std::dec << (int)sfd << ",LOCK_EX | LOCK_NB) failed(" << strerror(errno) << ")!" << std::endl;
        goto init_error_handle;
    }

    //恢复串口为阻塞状态
    if(fcntl(sfd, F_SETFL, 0) != 0)
    {
        std::cout << "fcntl(fd=" << std::dec << (int)sfd << ",F_SETFL,0) failed(" << strerror(errno) << ")!" << std::endl;
        goto init_error_handle;
    }

    //串口配置
    struct termios options;
    bzero(&options, sizeof(options));

#if 0
    //获取当前串口的属性
    if (tcgetattr(sfd, &options) != 0)
    {
        std::cout <<  "serial port(" << dev_name << ") tcgetattr(fd=" << sfd << ") failed(" << strerror(errno) << ")!";
        goto init_error_handle;
    }
#endif

    //波特率配置
    speed_t opt_speed;
    switch (speed)
    {
        case 50:      opt_speed = B50;      break;
        case 75:      opt_speed = B75;      break;
        case 110:     opt_speed = B110;     break;
        case 134:     opt_speed = B134;     break;
        case 150:     opt_speed = B150;     break;
        case 200:     opt_speed = B200;     break;
        case 300:     opt_speed = B300;     break;
        case 600:     opt_speed = B600;     break;
        case 1200:    opt_speed = B1200;    break;
        case 1800:    opt_speed = B1800;    break;
        case 2400:    opt_speed = B2400;    break;
        case 4800:    opt_speed = B4800;    break;
        case 9600:    opt_speed = B9600;    break;
        case 19200:   opt_speed = B19200;   break;
        case 38400:   opt_speed = B38400;   break;
        case 57600:   opt_speed = B57600;   break;
        case 115200:  opt_speed = B115200;  break;
        case 230400:  opt_speed = B230400;  break;
        case 460800:  opt_speed = B460800;  break;
        case 500000:  opt_speed = B500000;  break;
        case 576000:  opt_speed = B576000;  break;
        case 921600:  opt_speed = B921600;  break;
        case 1000000: opt_speed = B1000000; break;
        case 1152000: opt_speed = B1152000; break;
        case 1500000: opt_speed = B1500000; break;
        case 2000000: opt_speed = B2000000; break;
        case 2500000: opt_speed = B2500000; break;
        case 3000000: opt_speed = B3000000; break;
        case 3500000: opt_speed = B3500000; break;
        case 4000000: opt_speed = B4000000; break;
        default:
            std::cout << "unsupported baud rate(" << speed << ")!";
            goto init_error_handle;
    }
    //设置输入速度
    if (cfsetispeed(&options, opt_speed) != 0)
    {
        std::cout << "cfsetispeed(speed=" << speed << ") failed(" << strerror(errno) << ")!";
        goto init_error_handle;
    }
    //设置输出速度
    if (cfsetospeed(&options, opt_speed) != 0)
    {
        std::cout << "cfsetospeed(speed=" << speed << ") failed(" << strerror(errno) << ")!";
        goto init_error_handle;
    }

    //设置控制模式
    options.c_cflag = opt_speed | CS8 | CLOCAL | CREAD;
    //设置输入模式
    options.c_iflag = IGNPAR; //忽略奇偶校验错误的字符
    //设置输出模式
    options.c_oflag = 0;
    //设置本地模式
    options.c_lflag = 0;
    //设置特殊的控制字符，此处为等待时间和最小接受字符
    options.c_cc[VTIME] = 0;
    options.c_cc[VMIN] = 0;

    //清空输入缓冲区 TCIFLUSH
    if (tcflush(sfd, TCIFLUSH) != 0)
    {
        std::cout << "tcflush(fd=" << std::dec << (int)sfd << ") failed(" << strerror(errno) << ")!";
        goto init_error_handle;
    }

    //设置发送的属性。激活新配置（立刻）
    if (tcsetattr(sfd, TCSANOW, &options) != 0)
    {
        std::cout <<"tcsetattr(fd=" << std::dec << (int)sfd << ") failed(" << strerror(errno) << ")!";
        goto init_error_handle;
    }

    //文件句柄拷贝
    fd = sfd;

    initialled = true;
    return 0;

    init_error_handle:
    close(sfd);

    return -1;
}

int SerialPortDriver::SendMessage(uint8_t *msg_buf, uint32_t msg_len)
{
    int ret;

    if (fd < 0)
    {
        std::cout << "not initialed!";
        return -1;
    }

#if 1
    //等待所有输出都被发送
    ret = tcdrain(fd);
    if (ret == -1)
    {
        std::cout << "tcflush failed(" << strerror(errno) << ")!";
        return -1;
    }
#endif

    ret = write(fd, msg_buf, msg_len);
    if (ret == -1)
    {
        std::cout << "write(n=" << msg_len << ") failed(" << strerror(errno) << ")!";
        return -1;
    }

    return ret;
}

int SerialPortDriver::SendMessage(uint8_t *msg_buf, uint32_t *msg_len, uint32_t timeout)
{
    int ret, err = 0;

    uint8_t *_msg_buf = msg_buf;
    uint32_t _msg_len = *msg_len;
    uint32_t _msg_cnt = 0;

    std::chrono::steady_clock::time_point t_start = std::chrono::steady_clock::now();

    while (_msg_len)
    {
        ret = SendMessage(_msg_buf, _msg_len);
        if (ret == -1)
        {
            err = -1;
            break;
        }

        _msg_buf += ret;
        _msg_len -= ret;
        _msg_cnt += ret;
        if (_msg_len == 0)
            break;

        if (std::chrono::steady_clock::now() - t_start >= std::chrono::milliseconds(timeout))
        {
            std::cout <<  "send message(" << std::dec << (int)*msg_len << ") serial port(" << dev_name << ") timeout(" << std::dec << (int)timeout << ")!";
            err = 1;
            break;
        }
    }

    *msg_len = _msg_cnt;

//串口调试打印开关
#if 0
    printf("TX >> ");
    for (uint32_t i = 0; i < *msg_len; i++)
    {
        printf("%02x ", msg_buf[i]);
    }
    printf("\n");
#endif
    return err;
}

int SerialPortDriver::RecvMessage(uint8_t *msg_buf, uint32_t msg_len)
{
    int ret;

    if (fd < 0)
    {
        std::cout << "not initialed!";
        return -1;
    }

    ret = read(fd, msg_buf, msg_len);
    if (ret == -1)
    {
        std::cout << "read(n=" << std::dec << (int)msg_len << ") failed(" << strerror(errno) << ")!";
        return -1;
    }

    return ret;
}

int SerialPortDriver::RecvMessage(uint8_t *msg_buf, uint32_t *msg_len, uint32_t timeout)
{
    int ret, err = 0;

    uint8_t *_msg_buf = msg_buf;
    uint32_t _msg_len = *msg_len;
    uint32_t _msg_cnt = 0;

    std::chrono::steady_clock::time_point t_deadline = std::chrono::steady_clock::now() + std::chrono::microseconds(timeout * 1000);

    while (_msg_len)
    {
        if ((select_mode && timeout != 0) || timeout > 200)
        {
            /* low cpu usage but high latency */
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(t_deadline - std::chrono::steady_clock::now());
            if (duration.count() <= 0)
            {
                std::cout << "recv message(" << std::dec << (int)*msg_len << ") timeout(" << std::dec << (int)timeout << ")!";
                err = 1;
                break;
            }

            fd_set rfds;
            {
                FD_ZERO(&rfds);
                FD_SET(fd, &rfds);
            }

            struct timeval tv;
            {
                tv.tv_sec = (duration.count() / 1000);
                tv.tv_usec = ((duration.count() % 1000) * 1000);
            }

            ret = select(FD_SETSIZE, &rfds, nullptr, nullptr, &tv);
            if (ret == -1)
            {
                perror("select error: ");
                err = -1;
                break;
            }
            else
            if (ret == 0 || !FD_ISSET(fd, &rfds))
            {
                continue;
            }
        }
        {
            ret = RecvMessage(_msg_buf, _msg_len);
            if (ret == -1)
            {
                perror("RecvMessage error: ");
                err = -1;
                break;
            }

            _msg_buf += ret;
            _msg_len -= ret;
            _msg_cnt += ret;
            if (_msg_len == 0)
                break;

            if (timeout == 0)
            {
                err = 1;
                break;
            }
            else
            if (std::chrono::steady_clock::now() >= t_deadline)
            {
                std::cout << "recv message(" << std::dec << (int)*msg_len << ") timeout(" << std::dec << (int)timeout << ")!";
                err = 1;
                break;
            }
        }
    }

    *msg_len = _msg_cnt;

//串口接收调试打印开关
#if 0
    printf("RX << ");
    for (uint32_t i = 0; i < *msg_len; i++)
    {
        printf("%02x ", msg_buf[i]);
    }
    printf("\n");
#endif
    return err;
}