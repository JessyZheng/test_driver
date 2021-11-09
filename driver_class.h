/**
 * @file driver_class.h
 * @author jessy
 * @brief 思路：1.先将现有的类进行注册操作，主要是将指向对应类的构造函数的指针存到map容器中，key值为类名
 *             2.在GetInstance操作中，当链表中不存在想获取的单例类对象时，就利用传入的类名称去匹配map容器中的类构造函数，
 *               构造出该类对象，以指针返回，并将改指针存入链表
 * @version 0.1
 * @date 2021-11-09
 * 
 * @copyright Copyright (c) 2021
 * 
 */
#ifndef DRIVER_CLASS_H
#define DRIVER_CLASS_H

#include <string>
#include <list>
#include <map>
#include <mutex>
#include "json.hpp"
#include "robot_config.h"

class DriverClass {
public:
    void* GetClass(std::string device_id, nlohmann::json params=nullptr)
    {
        std::string name;
        if (params == nullptr)
        {
            name = GetDeviceConfig<std::string>(device_id, "driver_name");
        }
        else
        {
            name = params["driver_name"].get<std::string>();
        }

        auto iter = m_class_map.find(name);
        if(iter == m_class_map.end())
            return nullptr;

        return iter->second(device_id, params);
    }

    bool IsShared(std::string device_id)
    {
        return GetDeviceConfig<bool>(device_id, "shared");
    }

    void RegistClass(std::string name, std::function<void*(std::string, nlohmann::json params)> method)
    {
        m_class_map.insert(make_pair(name, method));
    }

    static DriverClass& GetInstance()
    {
        static DriverClass s_driver_class;
        return s_driver_class;
    }

private:
    std::map<std::string, std::function<void*(std::string, nlohmann::json params)>> m_class_map;
private:
    DriverClass() {};
};

class RegisterAction{
public:
    RegisterAction(std::string name, std::function<void*(std::string, nlohmann::json params)> method)
    {
        DriverClass::GetInstance().RegistClass(name, method);
    }
};

//一类驱动的注册入口
#define DRIVER_REGISTER(class_name)\
    class_name* object_creator##class_name(std::string device_id, nlohmann::json params=nullptr){ \
        return new class_name(device_id, params); \
    } \
    RegisterAction g_creator_reigster##class_name( \
        #class_name, object_creator##class_name)

#define DRIVER_DECLARE(class_name)\
    class_name* object_declare##class_name(std::string device_id, nlohmann::json params=nullptr){ \
        return new class_name(device_id, params); \
    }

template<typename _Tp>
const std::shared_ptr<_Tp> GetDriverInstance(std::string device_id, nlohmann::json params=nullptr)
{
    static std::recursive_mutex instances_lock;
    static std::list<std::shared_ptr<_Tp>> instances;

    std::shared_ptr<_Tp> p_instance, n_instance;
    instances_lock.lock();  

    for(const auto& instance : instances)
    {
        if (!instance->EqualInstance(device_id))
            continue;
        else
        {
            p_instance = instance;
            break;
        }
    }
    if (p_instance == nullptr)
    {
        _Tp *pinstance = nullptr;
        try
        {
            pinstance = (_Tp *)DriverClass::GetInstance().GetClass(device_id, params);
        }
        catch (std::exception&  e)
        {
            std::cout << device_id << "get instance failed" << e.what();
        }

        if(pinstance != nullptr)
        {
            n_instance.reset(pinstance);
            instances.push_back(n_instance);
        }
        else
        {
            std::cout << device_id << " not registered" << std::endl;
        }
    }
    instances_lock.unlock();

    bool shared = DriverClass::GetInstance().IsShared(device_id);
    return shared ? (p_instance != nullptr ? p_instance : n_instance) : n_instance;
}

#endif


