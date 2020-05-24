#include "FoxPS2Keyboard.h"

//http://www-ug.eecg.utoronto.ca/desl/nios_devices_SoC/datasheets/PS2%20Protocol.htm
//https://www.win.tue.nl/~aeb/linux/kbd/scancodes-1.html
//https://www.win.tue.nl/~aeb/linux/kbd/scancodes-12.html


FoxPS2Keyboard keyboard;
uint8_t dat;

void setup()
{
	Serial.begin(9600);
	delay(1000);

	Serial.write("Init\n");

	if (keyboard.WaitForKeyboard() == false)
	{
		Serial.write("Keyboard not found... Press F1 to continue\n");
		while (1);
	}

	Serial.write("Lights\n");

	keyboard.SetKeyboardLights(1, 0, 0);

	Serial.write("Completed\n");
}

void loop()
{
	dat = keyboard.GetScancode();
	if (dat != 0)
		Serial.print(dat, 16);
}
