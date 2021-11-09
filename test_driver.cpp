/**
 * @file test_driver.cpp
 * @author jessy
 * @brief       测试获取名称为serial_ttyUSB1的driver对象
 *          思路：  1.在获取的时候传入的参数为具体的驱动名称，再用该名称去配置文件中检索相应的驱动类，如SerialPortDriver
 *                 2.获取的对象为全局唯一对象
 *                 3.具体获取的思路参照driver_class.h中的说明
 * @version 0.1
 * @date 2021-11-09
 * 
 * @copyright Copyright (c) 2021
 * 
 */
#include <iostream>
#include "serialport_driver.h"
#include "driver_class.h"

static const std::string CONFIG_FILE = "config.json";

int main(void)
{
    std::string robot_config = CONFIG_FILE;

    if(RobotConfig::GetInstance().Init<nlohmann::json>(robot_config, true) == 0)
    {
        std::shared_ptr<SerialPortDriver> serial_port = GetDriverInstance<SerialPortDriver>("serial_ttyUSB1");
        serial_port->Init();
    }
    return 0;
}

DRIVER_REGISTER(SerialPortDriver);
