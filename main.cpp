#include "libtelegram/libtelegram.h"
#include "bothandler.h"

int main(void)
{
	BotHandler bh;
	std::string response, command, param_value;
	bh.configFileParser("../config");
	
  	telegram::sender sender(bh.getToken());                                       // create a sender with our token for outgoing messages
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
			std::string url = file.get_url(bh.getToken()).to_string();
			std::string file_name = (*file.file_path).substr((*file.file_path).find_last_of("/")+1);
			std::string file_wav = file_name.substr(0,file_name.find_last_of(".")+1)+"wav";

			if(!bh.download(url, file_name))
			{
				bh.convert2wav(file_name);
				bh.transcribe(file_wav);
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
		
		//converts entire command to lowercase
		std::for_each(command.begin(), command.end(), [](char & c){
    		c = ::tolower(c);
		});

		response = bh.checknrun(command);
		sender.send_message(message.chat.id, response);
  	});
	listener.set_num_threads(1);
	listener.run();                                                               // launch the listener - this call blocks until the listener is terminated
	return EXIT_SUCCESS;
}