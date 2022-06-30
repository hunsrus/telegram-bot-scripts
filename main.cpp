#include <iostream>
#include <cstdio>
#include <memory>
#include <stdexcept>
#include <string>
#include <array>
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
	std::string const token("5286497545:AAHZfebdIGcD8O64XbOMDekiMN2GPyrCbnY");         // in practice you would probably read this from config file or database
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
			//auto file_blob(file.download(token)); ------------------------ NO FUNCIONA STREAM.OPEN()
			std::string url = file.get_url(token).to_string();
			//std::string url = std::string("api.telegram.org/file/bot" + token + "/"+ *file.file_path);
			sender.send_message(message_chat_id, url);
			std::string const filename(std::string("voice_" + file.file_id + ".ogg")); // give it a unique filename
			std::ofstream filestream(std::string("voice_" + file.file_id + ".ogg"), std::ios::binary); // make sure we write in binary mode
			
			/*filestream.write(file_blob.data(), file_blob.size());                     // write the binary blob to disk
			if(filestream.fail())
			{
				sender.send_message(message_chat_id, "Error descargando el archivo de voz");
			}else
			{
				sender.send_message(message_chat_id, "A la orden");
			}*/
		}
//		std::string const reply(message_from + " sent \"" + *message.text + "\" to chat id " + std::to_string(message.chat.id)); // each element is a native type - the chat id is an integer
//	    	sender.send_message(message.chat.id, reply);                                // send our reply from within the callback
  	});
	listener.set_num_threads(1);
	listener.run();                                                               // launch the listener - this call blocks until the listener is terminated
	return EXIT_SUCCESS;
}
