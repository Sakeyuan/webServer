#include "Config.h"
#include <iostream>
Config::Config()
    : PORT(8080), THREAD_NUMS(8), MAX_TASK_NUMS(10000), 
    SQL_MAX_CONN(8), SQL_PORT(3306),SQL_HOST("localhost"),TRIGMODE(0)
{

    loadConfig();
}

Config::~Config()
{
}

void Config::removeBlank(string &s)
{
    string temp = "";
    for (char ch : s)
    {
        if (ch != ' ')
        {
            temp += ch;
        }
    }
    s = temp;
}

void Config::loadConfig()
{
    char buf[200];
    getcwd(buf, 200);
    char *last_occur = strrchr(buf, '/');
    *last_occur = '\0';
    strcat(buf, "/Config.conf");

    fstream file(buf, std::ios::in);
    if (!file.is_open())
    {
        std::cerr << "Error opening Config.conf: " << strerror(errno) << std::endl;
        return;
    }
    string line = "";
    while (getline(file, line))
    {
        removeBlank(line);
        int pos = line.find("=");
        if (pos == std::string::npos)
        {
            perror("Config parse error");
            return;
        }
        string key = line.substr(0, pos);
        string value = line.substr(pos + 1);
        if (key == "PORT")
        {
            PORT = std::stoul(value);
        }
        else if (key == "THREAD_NUMS")
        {
            THREAD_NUMS = std::stoi(value);
        }
        else if (key == "MAX_TASK_NUMS")
        {
            MAX_TASK_NUMS = std::stoi(value);
        }
        else if (key == "SQL_MAX_CONN")
        {
            SQL_MAX_CONN = std::stoi(value);
        }
        else if (key == "SQL_PORT")
        {
            SQL_PORT = std::stoi(value);
        }
        else if (key == "SQL_HOST")
        {
            SQL_HOST = std::stoi(value);
        }
        else if (key == "SQL_USER_NAME")
        {
            SQL_USER_NAME = value;
        }
        else if (key == "SQL_PASSWORD")
        {
            SQL_PASSWORD = value;
        }
        else if (key == "SQL_DATABASE_NAME")
        {
            SQL_DATABASE_NAME = value;
        }
        else if (key == "TRIGMODE")
        {
            TRIGMODE = stoi(value);
        }
        else if (key == "LINGER")
        {
            LINGER = stoi(value);
        }
        else
        {
            printf("UNKOWN KEY %s\n", key.c_str());
        }
    }
    file.close();
}

Config *Config::getInstance()
{
    static Config instance;
    return &instance;
}
