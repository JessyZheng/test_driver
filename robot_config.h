/**
 * @file robot_config.h
 * @author jessy
 * @brief   主要是用来读写配置文件(json格式)，后期可拓展其他格式文件
 *          思路：1.将所有json配置文件的内容加载到同一个json对象中
 *               2.对json对象进行嵌套读写指定key的value
 * @version 0.1
 * @date 2021-11-09
 * 
 * @copyright Copyright (c) 2021
 * 
 */
#ifndef ROBOT_CONFIG_H
#define ROBOT_CONFIG_H

#include <string>
#include <list>
#include <map>
#include <mutex>
#include <fstream>
#include "json.hpp"

class RobotConfig{
public:
    /*该函数完成对所有配置文件的加载，并将内容做拷贝，全部存取在一个对象中*/
    template<typename _Tc>
    int Init(std::string config_file, bool verbose = false)
    {
        if(typeid(_Tc) == typeid(nlohmann::json))
        {
            m_kubot_config_json = nullptr;
            // 将配置文件内容加载到输入流对象中
            std::ifstream config_ifs(config_file);
            if (!config_ifs)
            {
                if(verbose)  std::cout << "Load \"" << config_file << "\" failed, not found!";
                return -1;
            }
            if(verbose)  std::cout << "Load \"" << config_file << "\"";

            try
            {
                //将输入流对象重定向到配置文件json对象中
                config_ifs >> m_kubot_config_json;
            }
            catch (std::exception &e)
            {
                std::cout << "Parse config file " << config_file << " failed:" << std::endl << e.what();
                config_ifs.close();
                exit(1);
            }
            //将输入流关闭
            config_ifs.close();

            // 读取所有包含的配置文件，这样可以完成配置文件的拓展
            if (m_kubot_config_json["include_files"] != nullptr &&
                m_kubot_config_json["include_files"].is_array())
            {
                //遍历所有需要加载的配置文件
                for(auto& file : m_kubot_config_json["include_files"])
                {
                    nlohmann::json include_config;
                    //将配置文件加载到输入流中
                    std::ifstream include_ifs(file);
                    if (!include_ifs)
                    {
                        if(verbose) std::cout << "Load " << file << " failed, not found!";
                        return -1;
                    }
                    if(verbose) std::cout << "Load " << file << std::endl;

                    try
                    {
                        //将输入流重定向到json对象中
                        include_ifs >> include_config;
                    }
                    catch (std::exception& e)
                    {
                        include_ifs.close();
                        std::cout << "Parse config file " << file << " failed:" << std::endl << e.what();
                        exit(1);;
                    }
                    //关闭输入流
                    include_ifs.close();
                    //将json对象做合并处理，所有的配置都会保存在一个json对象中
                    m_kubot_config_json.merge_patch(include_config);
                }
            }
        }
        else
        {
            std::cout << "Unsupported config type" << std::endl;
            exit(1);
        }

        //完成配置文件内容读取和拷贝
        m_init_done = true;
        return 0;
    }

    template<typename _Tp, typename _Tc, typename... _Args>
    const _Tp GetConfig(std::string path, _Args&&... __args)
    {
        _Tp value = _Tp();

        if (!m_init_done)
        {
            return value;
        }

        if(typeid(_Tc) == typeid(nlohmann::json))
        {
            pthread_rwlock_rdlock(&m_kubot_config_lock);
            //获取key对应的value
            nlohmann::json& kubot_config = getJsonCfg(m_kubot_config_json, path, std::forward<_Args>(__args)...);
            try
            {
                //value格式转换
                value = kubot_config.get<_Tp>();
            }
            catch (nlohmann::json::type_error& e)
            {
                std::cout << e.what() << std::endl;
                std::cout << "Key: ";

                //嵌套打印Key
                dumpJsonKey(path, std::forward<_Args>(__args)...);
                pthread_rwlock_unlock(&m_kubot_config_lock);
                exit(1);
            }
            catch (nlohmann::json::out_of_range & e)
            {
                std::cout << e.what() << std::endl;
                std::cout << "Key: ";

                //嵌套打印Key
                dumpJsonKey(path, std::forward<_Args>(__args)...);
                pthread_rwlock_unlock(&m_kubot_config_lock);
                exit(1);
            }
            pthread_rwlock_unlock(&m_kubot_config_lock);
        }
        else
        {
            std::cout << "Unsupported config type" << std::endl;
            exit(1);
        }

        return value;
    }

    template<typename _Tp, typename _Tc>
    const _Tp GetConfig(std::vector<std::string>& path)
    {
        _Tp value = _Tp();

        if (!m_init_done)
        {
            return value;
        }

        if(typeid(_Tc) == typeid(nlohmann::json))
        {
            pthread_rwlock_rdlock(&m_kubot_config_lock);
            nlohmann::json& kubot_config = getJsonCfg(m_kubot_config_json, path);
            try
            {
                value = kubot_config.get<_Tp>();
            }
            catch (nlohmann::json::type_error& e)
            {
                std::cout << e.what() << std::endl;
                std::cout << "Key: ";
                for(auto point : path)
                {
                    std::cout << "[" << point << "]";
                }
                std::cout << std::endl;
                pthread_rwlock_unlock(&m_kubot_config_lock);
                exit(1);
            }
            catch (nlohmann::json::out_of_range & e)
            {
                std::cout << e.what() << std::endl;
                std::cout << "Key: ";
                for(auto point : path)
                {
                    std::cout << "[" << point << "]";
                }
                std::cout << std::endl;
                pthread_rwlock_unlock(&m_kubot_config_lock);
                exit(1);
            }
            pthread_rwlock_unlock(&m_kubot_config_lock);
        }
        else
        {
            std::cout << "Unsupported config type" << std::endl;
            exit(1);
        }

        return value;
    }

    template<typename _Tc>
    int UpdateConfig(std::string path, nlohmann::json& config)
    {
        if (!m_init_done)
        {
            std::cout << "The configuration file has not been initialized" << std::endl;
            exit(1);
        }

        if(typeid(_Tc) == typeid(nlohmann::json))
        {
            pthread_rwlock_rdlock(&m_kubot_config_lock);
            try
            {
                m_kubot_config_json.merge_patch(config);
            }
            catch(const std::exception& e)
            {
                std::cout << e.what() << std::endl;
                pthread_rwlock_unlock(&m_kubot_config_lock);
                exit(1);
            }
            pthread_rwlock_unlock(&m_kubot_config_lock);
        }
        else
        {
            std::cout << "Unsupported config type" << std::endl;
            exit(1);
        }

        return 0;
    }

    static RobotConfig& GetInstance()
    {
        //静态变量(单例)
        static RobotConfig s_kubot_config;
        return s_kubot_config;
    }

private:
    //最后一次打印
    template <typename T>
    void dumpJsonKey(T arg)
    {
        std::cout <<"["<< arg<<"]" << std::endl;
    }
    //嵌套打印
    template <typename T, typename... Args>
    void dumpJsonKey(T arg, Args... args)
    {
        std::cout <<"["<< arg<<"]";
        dumpJsonKey(args...);
    }

    //最后一次调用获取键值对信息
    template <typename T>
    nlohmann::json& getJsonCfg(nlohmann::json& config, T arg){
        return config[arg];
    }
    
    //嵌套调用
    template <typename T, typename ... Args>
    nlohmann::json& getJsonCfg(nlohmann::json& config, T arg, Args ... args){
        return getJsonCfg(config[arg], args...);
    }

    nlohmann::json& getJsonCfg(nlohmann::json& config, std::vector<std::string> path){
        if (path.size() == 1)
        {
            return config[path[0]];
        }

        return getJsonCfg(config[path[0]], std::vector<std::string>{path.begin()+1, path.end()});
    }

private:
    bool                m_init_done = false;
    nlohmann::json      m_kubot_config_json;
    pthread_rwlock_t    m_kubot_config_lock;
    RobotConfig(){
        pthread_rwlockattr_t attr;
        pthread_rwlockattr_setkind_np(&attr, PTHREAD_RWLOCK_PREFER_WRITER_NONRECURSIVE_NP);
        pthread_rwlock_init(&m_kubot_config_lock, &attr);
    };
};

template<typename _Tp, typename... _Args>
const _Tp GetConfig(std::string path, _Args&&... __args)
{
    return RobotConfig::GetInstance().GetConfig<_Tp, nlohmann::json>(path, std::forward<_Args>(__args)...);
}

template<typename _Tp>
const _Tp GetConfig(std::vector<std::string> path)
{
    return RobotConfig::GetInstance().GetConfig<_Tp, nlohmann::json>(path);
}

template<typename _Tp, typename... _Args>
const _Tp GetDeviceConfig(_Args&&... __args)
{
    return GetConfig<_Tp>("device_list", std::forward<_Args>(__args)...);
}

template <typename T1, typename T2>
void setJsonCfg(nlohmann::json& config, T1 arg1, T2 arg2){
    config[arg1] = arg2;
}
template <typename T1, typename T2, typename ... Args>
void setJsonCfg(nlohmann::json& config, T1 arg1, T2 arg2, Args ... args){
    setJsonCfg(config[arg1], arg2, args...);
}

template<typename... _Args>
void SetKubotConfig(nlohmann::json& config, _Args&&... __args)
{
    setJsonCfg(config, std::forward<_Args>(__args)...);
}

#endif //ROBOT_CONFIG_H
