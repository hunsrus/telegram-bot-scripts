#include <iostream>
#include <cstdio>
#include <memory>
#include <stdexcept>
#include <string>
#include <array>
#include <fstream>
#include <list>
#include "libtelegram/libtelegram.h"

struct cmd
{
	std::string key;
	std::string script;
	std::string param;
};

std::string checknrun(std::string command, std::list<cmd> available_commands, std::string scripts_path);
int download(std::string url, std::string file_name);
int convert2wav(std::string file_name);
int transcribe(std::string file_name);

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

int main(void)
{
	std::string response, command;
	std::string line, param_name, param_content, token, bot_name, lang, scripts_path, aux_keywords, aux_script, aux_param;
	std::list<cmd> available_commands;
	std::ifstream configFile("../config");
	if(configFile.is_open())
	{
		while(std::getline(configFile,line))
		{
			int div_pos = line.find_last_of("=");
			param_name = line.substr(0,div_pos);
			param_content = line.substr(div_pos+1);
			if(line.empty());	//ignore empty lines
			else if(!param_name.compare("TOKEN")) token = param_content;
			else if(!param_name.compare("BOT_NAME")) bot_name = param_content;
			else if(!param_name.compare("LANGUAGE")) lang = param_content;
			else if(!param_name.compare("SCRIPTS_PATH")) scripts_path = param_content;
			else if(!param_name.compare("COMMAND"))
			{
				std::cout << "Configuring command..." << std::endl;
				while(line.compare("}"))
				{
					if(std::getline(configFile,line))
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
				cmd aux_cmd = {
					aux_keywords,
					aux_script,
					aux_param
				};
				available_commands.push_front(aux_cmd);
				std::cout << "Command configuration successful: \""+aux_cmd.key+"\"" << std::endl;
				std::cout << "Script: \""+scripts_path+aux_cmd.script+"\"" << std::endl;
				std::cout << "Parameters: \""+aux_cmd.param+"\"" << std::endl;
			}else
			{
				std::cout << "Unknown parameter in configuration file: " << line << std::endl;
				return EXIT_FAILURE;
			}
		}
		configFile.close();
	}else
	{
		std::cout << "Error reading configuration file" << std::endl;
		return EXIT_FAILURE;
	}
  	telegram::sender sender(token);                                               // create a sender with our token for outgoing messages
  	telegram::listener::poll listener(sender);                                    // create a polling listener which will process incoming requests, polling using the sender
  	listener.set_callback_message([&](telegram::types::message const &message)	  // we set a callback for receiving messages in native format, using a lambda for convenience
	{
		auto message_chat_id = message.chat.id;
    	std::string const message_from(message.from ? message.from->first_name : "Unknown sender"); // some fields, such as message.from, are optional
	    if(message.text) command = *message.text;
		else if(message.voice)
		{
			sender.send_chat_action(message_chat_id, telegram::sender::chat_action_type::TYPING); // show the user they should wait, we're typing
			auto const &file_optional(sender.get_file((*message.voice).file_id));
			if(!file_optional)   // we might not get a file back, so handle this gracefully	
			{                                                    		
				sender.send_message(message_chat_id, "Something went wrong and I couldn't download the file.");
			    return;
			}
			auto const &file(*file_optional);
			std::string url = file.get_url(token).to_string();
			std::string file_name = (*file.file_path).substr((*file.file_path).find_last_of("/")+1);
			std::string file_wav = file_name.substr(0,file_name.find_last_of(".")+1)+"wav";

			if(!download(url, file_name))
			{
				convert2wav(file_name);
				transcribe(file_wav);
				std::ifstream transcriptFile("transcript");
				if(transcriptFile.is_open())
				{
					std::getline(transcriptFile,command);
					transcriptFile.close();
					if(!command.empty()) command.resize(command.length()-1);		//removes last unwanted space character
				}else sender.send_message(message_chat_id, "Error reading transcript file");
				//sender.send_message(message_chat_id, transcript);
				system(std::string("rm "+file_name+" "+file_wav+" transcript").c_str());
			}else sender.send_message(message_chat_id, "Error downloading voice message");
		}
		response = checknrun(command,available_commands,scripts_path);
		sender.send_message(message.chat.id, response);
  	});
	listener.set_num_threads(1);
	listener.run();                                                               // launch the listener - this call blocks until the listener is terminated
	return EXIT_SUCCESS;
}

int download(std::string url, std::string file_name)	//Download file in executable current directory
{
	std::cout << "Downloading file \"" << file_name << "\" from " << url << std::endl;
	return system(std::string("wget "+url).c_str());
}

int convert2wav(std::string file_name)
{
	std::string file_wav = file_name.substr(0,file_name.find_last_of(".")+1)+"wav";
	std::cout << "Converting file \"" << file_name << "\" to \"" << file_wav << "\"" << std::endl;
	return system(std::string("ffmpeg -y -i "+file_name+" "+file_wav).c_str());		//-y overwrite always (in case something went wrong deleting the files)
}

int transcribe(std::string file_name)
{
	std::cout << "Transcribing file \"" << file_name << "\" into transcript file" << std::endl;
	return system(std::string("/usr/local/bin/spchcat "+file_name+" >> transcript").c_str());
}

std::string checknrun(std::string command, std::list<cmd> available_commands, std::string scripts_path)
{
	std::string response = "No entiendo que quiere decir \""+command+"\" bro";
	if(command.empty()) response = "Error de transcripción. Intentá nuevamente con un audio más largo.";
	else for (std::list<cmd>::iterator it = available_commands.begin(); it != available_commands.end(); it++)
	{
		if (command == it->key)
			response = exec(std::string(scripts_path+it->script+" "+it->param).c_str());
	}
	return response;
}