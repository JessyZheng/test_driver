/**
 * @file serialport_driver.h
 * @author jessy
 * @brief 
 * @version 0.1
 * @date 2021-11-09
 * 
 * @copyright Copyright (c) 2021
 * 
 */
#ifndef SERIALPORTDRIVER_H
#define SERIALPORTDRIVER_H

#include <iostream>
#include <mutex>
#include "json.hpp"

class SerialPortDriver
{
public:
    std::string             driver_name;
    std::string             dev_name;
    std::string             device_id;
private:
    bool                    initialled = false;
private:
    int                     speed;
    int                     fd;
    std::mutex              lock;
    bool                    select_mode;

public:
    SerialPortDriver(const std::string& dev_name, int speed, bool select_mode=true):
            driver_name("SerialPort[" + dev_name + "]"),
            dev_name(dev_name),
            speed(speed),
            fd(-1),
            select_mode(select_mode)
    {};

    SerialPortDriver(std::string device_id, nlohmann::json params=nullptr)
    {
        this->device_id = device_id;

        if (params == nullptr)
        {
            dev_name = "";
            driver_name = "";
            speed = 111;
        }
        else
        {
            dev_name = "";
            driver_name = "";
            speed = 111;
        }

        select_mode = true;
        fd = -1;
        if(Init() != 0)
        {
            std::cout << this->device_id << " init failed" << std::endl;
            exit(1);
        }
    };

    ~SerialPortDriver();

    bool EqualInstance(const std::string& dev_name, int speed)
    {
        return this->dev_name == dev_name && this->speed == speed;
    };

    bool EqualInstance(std::string device_id)
    {
        return this->device_id == device_id;
    };

public:
    int Init(void);

    void TakeLock() { lock.lock();   }
    void GiveLock() { lock.unlock(); }

    int SendMessage(uint8_t *msg_buf, uint32_t msg_len);
    int RecvMessage(uint8_t *msg_buf, uint32_t msg_len);

    int SendMessage(uint8_t *msg_buf, uint32_t *msg_len, uint32_t timeout);
    int RecvMessage(uint8_t *msg_buf, uint32_t *msg_len, uint32_t timeout);
};

#endif //SERIALPORTDRIVER_H
