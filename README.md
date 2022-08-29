<img align="left" src="https://github.com/hunsrus/telegram-bot-scripts/blob/main/telegram-bot.png" width="120px">

# telegram-bot-scripts
Simple telegram bot utility that listens for commands and runs scripts
# Dependencies
```
apt install libssl-dev libboost-all-dev ffmpeg
```
Also [spchcat](https://github.com/petewarden/spchcat) is needed for speech recognition. You can install it following the [install notes](https://github.com/petewarden/spchcat/blob/main/notebooks/install.ipynb). To use the speech recognition capabilities read the corresponding section.
# Configuration file
To set the available parameters, you should copy te ```config-sample``` file and rename it as ```config```. There you can customize the fields and add the information regarding your own Telegram Bot.
# Speech recognition
After installing ```spchcat```, you will need to configure the desired language. If yours is one of the [supported languages](https://github.com/petewarden/spchcat#language-support), all you have to do is add the corresponding name to the config file. In the case of an unsupported language, you will need to put a pretrained model for it in the language model folder (ie, for Argentine spanish:  ```/etc/spchcat/models/es_AR```). There you should add a ```.scorer``` file, a ```.tflite``` file and an ```alphabet.txt``` file containing in each line the unicode codepoint (UTF-encoded) associated with a numeric label.
# Building

```
cd build
cmake ..
./buildnrun.sh
```
