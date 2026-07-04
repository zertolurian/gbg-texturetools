# 🤖GBG Texture Tools🎨

A collection of various tools which can be used to import images into Textures into the 2021 Nintendo Switch game **Game Builder Garage** (a.k.a. **GBG** in English, a.k.a. **はじプロ** in Japan)!

🎥 **[Introduction/Tutorial by VideoDojo (slightly outdated)](https://youtu.be/PTDiTS-Exjk?si=DSy7ZaHgRSA1d4rG)**  
This video by VideoDojo explains how the original tools (**GBG Texture Builder** by Scrubz and **AGTD** by Borri) can be used. Since then, new and improved tools have been created, but the general gist is still mostly the same!  
*(These original tools can still be found under the `legacy/` folder within this repository.)*

The current latest tools are the **GBG Texture Converter** by YOSSYくん, and **AGTD2** by Zert.  

**GBG Texture Converter** has expanded image converting capabilities, and is the recommended tool to output the latest AGTD2 format.  

**AGTD2** is an improved verion of AGTD with faster drawing speed, a more efficient texture encoding algorithm to allow more textures at a time, as well as the capability to edit the other settings of each Texture Nodon. The old CSV format and SD Card functionality are currently not supported. AGTD2 is incompatible with the original GBG Texture Builder and the original AGTD format. The latest version is available within this repository.

🚀 **[Launch YOSSYくん's GBG Texture Converter](https://yossy-kun.github.io/GBG-Texture-Converter/)** [(GitHub repository)](https://github.com/yossy-kun/GBG-Texture-Converter)

---

## 📂 Repository Structure

* **`AGTD2.ino`** — The script to paste your AGTD2 CSV into and then upload to your Arduino board to import into GBG.
* **`GBG_Texture_Builder_AGTD2.html`** — A quick edit of v1.2 to output the AGTD2 format by Zert. Please use YOSSYくん's tool instead.
* **`Import_Textures_from_GBG_v1.1.htm`** — The original Screenshot->Texture tool by Scrubz.
* **`legacy/`** — Legacy tools folder
* **`├── AGTD_v0.7-beta.ino`** — The original AGTD script by Borri.
* **`├── GBG_Texture_Builder_v1.2.htm`** — The original Texture->CSV by Scrubz.
* **`└── GBG_Texture_Builder_v1.2_CLIPBOARD.html`** — Added clipboard functionality by Voxy.

---

## 💡 What's new in AGTD2?

**Added Functionality**  
By reusing a little bit of code from **[ArduiNodon](https://github.com/zertolurian/gbg-arduinodon)**, each Texture's Face, Size, Position, and Rotation settings may be automatically set as well!

**Faster Drawing**  
All the delays within the original script were meticulously tested to find out which ones and how much were actually necessary to prevent mis-clicks.  
The algorithm was also significantly improved by allowing vertical line drawing, as well as prioritizing the most abundant colors first to maximize the number of lines drawn.

**More Textures**  
By using a self-desriptive CSV encoding, more than 5 textures can now be uploaded at a time! Instead of storing only 1 pixel into each byte for a total of 4096 bytes per texture, the new format now encodes all the colors used within each texture at the start of the CSV, allowing us to store more than 1 pixel per byte depending on the number of colors used. (Around 20k bytes of memory are available per run)
| Colors | Total Bytes | Available Memory ÷ Total Bytes |
|:------:|:-----------:|:------------------------------:|
| 1      | 3–29        | >512                           |
| 2      | 516–542     | 39                             |
| 3–4    | 1029–1056   | 19                             |
| 5–8    | 1543–1572   | 13                             |
| 9–16   | 2059–2092   | 9                              |
| 17–32  | 2579–2620   | 7                              |
| 33–64  | 3107–3164   | 6                              |
| 65–118 | 3641–3720   | 5                              |

A lot of the code was also trimmed down in order to save more memory, which includes the removal of the MouseTo library dependency. (So you don't need to install any external libraries anymore either!)

---

## ⚙️ How it Works

Within the *Programming Screen* of **Game Builder Garage**, USB mouse controls are natively supported.

The **Arduino** is a programmable microcontroller, and there are a few models that can pretend to be a USB mouse.

Through careful scripting and automation, we can reverse-engineer any Texture into a long list of mouse movements and clicks. This project simply automates the entire process of converting a bunch of Nodons into a series of mouse commands that would result in the same configuration of Nodons within GBG.

Since this only sends a one-way string of mouse commands, it cannot react to anything unexpected that happens within the game like the user accidentally nudging the mouse, nor can it make anything the player cannot make themselves, such as other colors.

---

## 🛠️ Getting Started

**[It is recommended to watch this Introduction/Tutorial by VideoDojo first. It talks about the original tools, but it's still really good!](https://youtu.be/PTDiTS-Exjk?si=7BVb8NnknOZa9iBn)**

Follow these steps to import Textures from your browser into GBG itself!:

### 1. Convert your image to CSV
1. Open [YOSSYくん's GBG Texture Converter](https://yossy-kun.github.io/GBG-Texture-Converter/) or your local copy of `GBG_Texture_Builder_v1.3.html` in a compatible browser.
2. Upload your image, and play with the settings until you get the desired look of your Texture.
3. Copy the CSV of the Texture/s into your clipboard.

### 2. Hardware Preparation
1. Connect your Arduino board to your computer using a USB cable. **(This script only works for boards that natively support mouse emulation, such as the Arduino Leonardo with its ATmega32u4; Arduino UNO is *NOT* supported)**
2. *(Optional)* Connect a button that connects GND to *any* pin from 8-13.

### 3. Upload to the Arduino
1. Open the `AGTD2.ino` file inside the Arduino IDE.
2. Select your correct Board and Port from the IDE tools menu.
3. *(Optional)* Change the settings within the Config at the top of the script based on your preferences, such as Delay.
4. *(If you didn't connect a button)* Enable *No Button Mode* from within the Config.
5. Paste your code exported from step 1 into the **DATA IMAGE SPACE** (it's hard to miss!).
7. Click **Upload** to flash the code to your microcontroller. Ensure that the console says that the sketch has been uploaded successfully, and then unplug the Arduino from your computer.

### 3. Importing into GBG
1. Connect your Arduino to your Switch through the Dock's USB-A port (or directly to the Switch's USB-C port, if you have a USB-A to USB-C adapter; this is required if you are using a Switch Lite).
5. *(If you didn't connect a button)* Wait for 30 seconds (or whichever delay you put into the config), and then the script should start running automatically. Once it is finished, unplug the Arduino or else it will run the script again after another set delay.
6. *(If you connected a button)* Press the button, and the script should start running immediately..
7. Wait for the script to finish, and make sure that none of the controls are disturbed during this process or else the script will make a mistake, and you would need to re-run the script again.
8. Voila! Your images are now in GBG!🎉

---

## 📜 License & Support

**AI Disclosure:** AGTD2 and Texture_Builder_AGTD2 were partially built using AI.

This project is open-source and free to use, modify, and share within the Game Builder Garage community. If you run into issues or have ideas for new features, feel free to DM @zertolurian on Discord!

