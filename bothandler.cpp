#include "bothandler.h"

std::string exec(const char* cmd) {
    std::array<char, 128> buffer;
    std::string result;
    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd, "r"), pclose);
    if (!pipe) {
        throw std::runtime_error("popen() failed!");
    }
    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
        result += buffer.data();
    }
    return result;
}

std::string BotHandler::getToken(void)
{
    return this->token;
}

int BotHandler::download(std::string url, std::string file_name)	//Download file in executable current directory
{
	std::cout << "Downloading file \"" << file_name << "\" from " << url << std::endl;
	return system(std::string("wget "+url).c_str());
}

int BotHandler::convert2wav(std::string file_name)
{
	std::string file_wav = file_name.substr(0,file_name.find_last_of(".")+1)+"wav";
	std::cout << "Converting file \"" << file_name << "\" to \"" << file_wav << "\"" << std::endl;
	return system(std::string("ffmpeg -y -i "+file_name+" "+file_wav).c_str());		//-y overwrite always (in case something went wrong deleting the files)
}

int BotHandler::transcribe(std::string file_name)
{
	std::cout << "Transcribing file \"" << file_name << "\" into transcript file" << std::endl;
	return system(std::string("/usr/local/bin/spchcat "+file_name+" >> transcript").c_str());
}

std::string BotHandler::checknrun(std::string command)
{
	std::string param_value, value, bands, param_mod;
	int param_pos, param_end;
	param_pos = command.find("resistencia de ");
	if(param_pos != std::string::npos)
	{
		param_pos += 15;
		param_end = command.find_first_of(" ",param_pos);
		if(param_end != std::string::npos)
			param_value = command.substr(param_pos,param_end-param_pos);
		value = param_value;
		command.replace(param_pos,param_end-param_pos,"$value");
	}
	param_pos = command.find("$value de ");
	if(param_pos != std::string::npos)
	{
		param_pos += 10;
		param_end = command.length();
		if(param_end != std::string::npos)
			param_value = command.substr(param_pos,param_end-param_pos);
		bands = param_value;
		command.replace(param_pos,param_end-param_pos,"$bands");
	}
	
	std::string response = "No entiendo que quiere decir \""+command+"\" bro";
	if(command.empty()) response = "Error de transcripción. Intentá nuevamente con un audio más largo.";
	else for (std::list<cmd>::iterator it = this->available_commands.begin(); it != this->available_commands.end(); it++)
	{
		if (command == it->key)
		{
			param_mod = it->param;
			param_pos = param_mod.find_first_of("$");
			if(param_pos != std::string::npos)
			{
				param_end = param_mod.find_first_of(" ",param_pos);
				param_mod.replace(param_pos,param_end-param_pos,value);
				
				param_pos = param_mod.find_first_of("$",param_end);
				if(param_pos != std::string::npos)
				{
					param_end = param_mod.length();
					param_mod.replace(param_pos,param_end-param_pos,bands);
				}
			}
			response = exec(std::string(this->scripts_path+it->script+" "+param_mod).c_str());
		}
	}
	return response;
}

int BotHandler::configFileParser(std::string config_file_path)
{
	this->configFile = new std::ifstream(config_file_path);
	std::string line, param_name, param_content, aux_keywords, aux_script, aux_param;
    cmd aux_cmd;
	if(this->configFile->is_open())
	{
		while(std::getline(*(this->configFile),line))
		{
			int div_pos = line.find_last_of("=");
			param_name = line.substr(0,div_pos);
			param_content = line.substr(div_pos+1);
			if(line.empty());	//ignore empty lines
			else if(!param_name.compare("TOKEN")) this->token = param_content;
			else if(!param_name.compare("BOT_NAME")) this->bot_name = param_content;
			else if(!param_name.compare("LANGUAGE")) this->lang = param_content;
			else if(!param_name.compare("SCRIPTS_PATH")) this->scripts_path = param_content;
			else if(!param_name.compare("COMMAND"))
			{
				std::cout << "Configuring command..." << std::endl;
				while(line.compare("}"))
				{
					if(std::getline(*(this->configFile),line))
					{
						div_pos = line.find_last_of("=");
						param_name = line.substr(0,div_pos);
						param_content = line.substr(div_pos+1);
						if(!param_name.compare("KEYWORDS")) aux_keywords = param_content;
						else if(!param_name.compare("SCRIPT")) aux_script = param_content;
						else if(!param_name.compare("PARAMETERS")) aux_param = param_content;
					}else
					{
						std::cout << "Unknown parameter in configuration file: " << line << std::endl;
						return EXIT_FAILURE;
					}
				}
				aux_cmd = {
					aux_keywords,
					aux_script,
					aux_param
				};
				this->available_commands.push_front(aux_cmd);
				std::cout << "Command configuration successful: \""+aux_cmd.key+"\"" << std::endl;
				std::cout << "Script: \""+this->scripts_path+aux_cmd.script+"\"" << std::endl;
				std::cout << "Parameters: \""+aux_cmd.param+"\"" << std::endl;
			}else
			{
				std::cout << "Unknown parameter in configuration file: " << line << std::endl;
				return EXIT_FAILURE;
			}
		}
		this->configFile->close();
	}else
	{
		std::cout << "Error reading configuration file" << std::endl;
		return EXIT_FAILURE;
	}
    return EXIT_SUCCESS;
}

BotHandler::~BotHandler()
{
    if(this->configFile) delete(this->configFile);
}