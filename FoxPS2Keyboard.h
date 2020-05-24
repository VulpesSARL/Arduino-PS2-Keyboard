#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <Arduino.h>


class FoxPS2Keyboard
{
public:
	FoxPS2Keyboard();
	static byte GetScancode();
	static void SendByte(byte b);
	static bool WaitForKeyboard();
	static bool FoxPS2Keyboard::SendByteWithConfirm(byte b, byte ACK, byte Tries, unsigned short Wait);
	static bool FoxPS2Keyboard::SetKeyboardLights(byte NumLock, byte CapsLock, byte ScrollLock);
private:
	static byte FoxPS2Keyboard::GetParity(byte b);
};
