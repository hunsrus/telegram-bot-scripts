#include <iostream>
#include <cstdio>
#include <memory>
#include <stdexcept>
#include <string>
#include <array>
#include <fstream>
#include "libtelegram/libtelegram.h"

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
	std::string response;
	std::string line, param_name, param_content, token, bot_name, lang;
	std::ifstream configFile("../config");
	if(configFile.is_open())
	{
		while(std::getline(configFile,line))
		{
			int div_pos = line.find_last_of("=");
			param_name = line.substr(0,div_pos);
			param_content = line.substr(div_pos+1);
			if(!param_name.compare("TOKEN")) token = param_content;
			else if(!param_name.compare("BOT_NAME")) bot_name = param_content;
			else if(!param_name.compare("LANGUAGE")) lang = param_content;
			else std::cout << "Unknown parameter in configuration file: " << line << std::endl;
		}
		configFile.close();
	}else std::cout << "Error reading configuration file" << std::endl;
  	telegram::sender sender(token);                                               // create a sender with our token for outgoing messages
  	telegram::listener::poll listener(sender);                                    // create a polling listener which will process incoming requests, polling using the sender
  	listener.set_callback_message([&](telegram::types::message const &message)
	{									   // we set a callback for receiving messages in native format, using a lambda for convenience
		auto message_chat_id = message.chat.id;
    		std::string const message_from(message.from ? message.from->first_name : "Unknown sender"); // some fields, such as message.from, are optional
	    	if(message.text == "Battery")
		{
			response = exec("scripts/battery-status/battery-status.sh");
			sender.send_message(message.chat.id, response); 
		}
		else if (message.text == "Start")
		{
			response = exec("scripts/serial-commander/sc start");
			sender.send_message(message.chat.id, response);
		}
		else if (message.text == "Prender led")
		{
			response = exec("scripts/serial-commander/sc builtin_on");
			if(!response.compare("Successful connection to /dev/ttyUSB0")) response  = "Ya te lo prendo maestro";	//Si la operación sale bien, cambia la respuesta por otro mensaje. Si sale mal, muestra el error correspondiente que ya se encuentra en el string response.
			sender.send_message(message.chat.id, response);
		}
		else if (message.text == "Apagar led")
		{
			response = exec("scripts/serial-commander/sc builtin_off");
			if(!response.compare("Successful connection to /dev/ttyUSB0")) response  = "Ya te lo apago maestro";
			sender.send_message(message.chat.id, response);
		}


		if(message.voice)
		{
			sender.send_chat_action(message_chat_id, telegram::sender::chat_action_type::TYPING); // show the user they should wait, we're typing
			auto const &file_optional(sender.get_file((*message.voice).file_id));
			if(!file_optional)   // we might not get a file back, so handle this gracefully	
			{                                                    		
				sender.send_message(message_chat_id, "Something went wrong and I couldn't download the file.");
			    return;
			}
			auto const &file(*file_optional);
			//auto file_blob(file.download(token)); // ------------------------ stream.open() NOT WORKING
			//momentarily using wget (no crossplatform)
			std::string url = file.get_url(token).to_string();
			std::string command = "wget "+url;
			//std::string url = std::string("api.telegram.org/file/bot" + token + "/"+ *file.file_path);
			//std::string const filename(std::string("voice_" + file.file_id + ".oga")); // give it a unique filename
			//std::ofstream filestream(std::string("voice_" + file.file_id + ".oga"), std::ios::binary); // make sure we write in binary mode
			
			std::string file_name = (*file.file_path).substr((*file.file_path).find_last_of("/")+1);
			
			//filestream.write(file_blob.data(), file_blob.size());                     // write the binary blob to disk
			if(system(command.c_str()))
			{
				sender.send_message(message_chat_id, "Error descargando el archivo de voz");
			}else
			{
				std::string file_wav = file_name.substr(0,file_name.find_last_of(".")+1)+"wav";
				command = "ffmpeg -i "+file_name+" "+file_wav;
				system(command.c_str());
				command = "/usr/local/bin/spchcat "+file_wav+" >> transcript";
				system(command.c_str());
				std::string transcript;
				std::ifstream transcriptFile("transcript");
				if(transcriptFile.is_open())
				{
					std::getline(transcriptFile,transcript);
				}else transcript = "Error leyendo archivo de transcripción";
				sender.send_message(message_chat_id, transcript);
				command = "rm "+file_name+" "+file_wav+" transcript";
				system(command.c_str());
			}
		}
//		std::string const reply(message_from + " sent \"" + *message.text + "\" to chat id " + std::to_string(message.chat.id)); // each element is a native type - the chat id is an integer
//	    	sender.send_message(message.chat.id, reply);                                // send our reply from within the callback
  	});
	listener.set_num_threads(1);
	listener.run();                                                               // launch the listener - this call blocks until the listener is terminated
	return EXIT_SUCCESS;
}
