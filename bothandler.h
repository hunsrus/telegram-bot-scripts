#ifndef bothandler_h
#define bothandler_h

#include <iostream>
#include <string>
#include <list>
#include <fstream>
#include <cstdio>
#include <memory>
#include <stdexcept>
#include <array>

struct cmd
{
	std::string key;
	std::string script;
	std::string param;
};

class BotHandler
{
    private:
        std::list<cmd> available_commands;
        std::ifstream *configFile;
        std::string token, bot_name, lang, scripts_path;
    public:
        std::string getToken();
        std::string checknrun(std::string command);
        int download(std::string url, std::string file_name);
        int convert2wav(std::string file_name);
        int transcribe(std::string file_name);
        int configFileParser(std::string config_file_path);
        ~BotHandler();
};

#endif //bothandler_h