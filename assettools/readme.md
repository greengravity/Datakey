Assettools for generation of images/textsets/fontsets specially adapted for use with different keyboardlayouts

An asset consists of a characterset, which will directly extracted from a keyboardlayout file (.klc),
as well as translateable texts
The textfile must correspond with the available characters in the used set, but otherwise they are 2 different things

To explain better, many keyboardlayouts may use the same english textual information
like english_en-US and english_en-UK, where english stands for the language and en-US and en-UK stand
for the corresponding keyboardlayout

The assetfile will also hold icons that are independet from text and overall the same

To create a new asset, there are some things to do

1) create a folder for the asset in ./locals/fonts with the name of the keyboardlayout like bg-BG
2) put a file with the name kblayout.klc in it. The file can be created with Microsoft Keyboard Layout Creator 1.4, an has the localization info
3) Add a translationfile into the .locals/text folder if needed. Copy the default.json as Template and overwrite the text you want to change
4) Add the previous information to the ./locals/locals.json file (should explain itself)
5) Run the kb_to_font.js with the name defined in the locals.json file (a.e. node kb_to_font german_de-DE).
   You should now have a keycodes.json file as well as a font.bmfc file in the locals/font/?german_de-DE? folder.
6) Run the program bmfont64.exe or download and run the correct program for your machine from www.AngelCode.com.
   (at this point, many many thanks for this great fontmap generator. I tried a bunch of generators and this one give a really clean font, even with small resolutions)
   load the font.bmfc file and export the font to the same folder (save Bitmap font as...)
    You should now have a font.fnt and a font_0.png file in the folder
7) Create a patterns.json file in the same folder or copy it from another folder and adapt it to match your characters
8) Run assetgen with the name defined in the locals.json (a.e. node assetgen german_de-DE).
    Now you have created the assetfiles for your language
9) Open the mplab project with mplab from microchip
10) Add the assetfiles (.c and .h) to the corresponding assetfolders in the project
11) Open the asset.h and add the definitions and includes and set the assetpack to the new defined value
12) Compile and flash the device
