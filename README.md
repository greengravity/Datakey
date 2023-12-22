## Datastorage
# What is it?
In short, it is a physical offline-device to store secret information like account data or banking informations.
While mobile apps can do things like this, its always possible to have some spyware installed with the last gaming app or whatever.
Because of that i dont trust mobiles too much and so i decided to make this little project.

#How it Works?
It is pretty simple, the device have 6 Buttons for input and a 1.7" Display for output.
Just enter your pincode and you will be able to browse through your entries.

Every entry have a name and up to 5 additional tokens to store informations like username/password or other things.
You can add/delete or modify entries directly on the device, without the need of a mobile-device or computer.

The tokens itself are entered with a virtual keyboard on the device.
Its like entering the name with a gamecontroller on a gamingconsole.

Everything entered will be encrypted with a masterkey and stored on a micro-sd card.
The masterkey is a secret that only the user know, if it get lost, you may not be able to restore your data anymore.

Usually the masterkey is stored in the device, so you dont need to carry it around.
But if you or someone else mess up the pincode verification, the masterkey get deleted from the device (for security reasons).
Thats why it is important to have the masterkey stored or memorized safely.

The Device can also act as an keyboard.
After you connected the device to your pc via usb, you can select an entry/token and push it to the pc.
like entering the data with a keyboard.
This type of communication is pretty much one-way, so the connected system cannot control the output of the device.
It should be safe to use it like that.

#Features
128-Bit AES-CBC encryption over all entries
Offline input/output
USB-Keyboard function to push data to connected systems
USB-Charging
Long standby because of low power consumption ( about 2 years - 8.5müAmp usage with 150mAh Battery )
Usage of Fat32 filesystem, so you can backup your data on your PC, this is safe because the data is completely encrypted, but make sure the masterkey stay separated from the data (dont store the masterkey on the pc too)
TETRIS :)

#What do you need to start?
The Device
An micro sd-card, formated with a Fat32 filesystem.
A micro-usb cable to charge the device

Enter the sd-card to the slot on the side of the device and open it.
You will be questioned about the masterkey.
Enter one, the longer the better.
Then you will be questioned about your pincode.
This looks like a game, but it is actually just a disguise as an additional security layer.
Enter one and Reenter it, you can use all 6 buttons for it.
You should end up on an empty screen showing the entrylist.
Gratulations, you can now add/modify entries in your list, using the context menus.

#Build the Device
To build the device you need a bunch of things

The electrical components (see components.csv in the Docs)
The Printboard (see datakeyboard/ for the kicad project or datakeyboard/gerber for the gerberfiles to send to your pcb-maker)
The MPLAB X IDE from microchip as well as an programmer device like the PICKit to build and programm the pic24fj128gb204 microcontroller
A 3D Printer to print the housing.
A tiny piece of thin copper or iron and something to cut it, it become the inlay for the housing to trigger the pogopins when open or close the cap.

A possibility to solder SMD-Parts
Checkout c0pperdragon and his amazing HotPlate project on github for this (also available on tindie)











 
