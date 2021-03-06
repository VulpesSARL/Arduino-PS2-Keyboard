#include"FoxPS2Keyboard.h"

#define TransistorIRQPin  6
#define TransistorDataPin 7
#define DataPin 4
#define IRQPin 3


#define BUFFER_SIZE 45
static volatile uint8_t buffer[BUFFER_SIZE];
static volatile uint8_t head, tail;

static volatile uint8_t SendByteState;
static volatile byte SendBits[10];
static volatile uint8_t disableisr;
static volatile uint8_t fulldisableisr;
static volatile uint16_t waitcounter;

#pragma GCC push_options
#pragma GCC optimize ("O2")

void KeyboardISR() //FALLING EDGE
{
	if (fulldisableisr != 0)
		return;

	if (disableisr != 0)
	{
		SendByteState++;

		switch (SendByteState)
		{
		case 0:
		case 1:
		case 2:
		case 3:
		case 4:
		case 5:
		case 6:
		case 7:
		case 8:
		case 9:
			digitalWrite(TransistorDataPin, SendBits[SendByteState]);
			break;
		case 10:
			digitalWrite(TransistorDataPin, 0);
			break;
		case 11:
			waitcounter = 0;
			while (digitalRead(DataPin) != 0)
			{
				waitcounter++;
				if (waitcounter > 32767)
				{
					disableisr = 0;
					return;
				}
			};
			waitcounter = 0;
			while (digitalRead(DataPin) != 1)
			{
				waitcounter++;
				if (waitcounter > 32767)
				{
					disableisr = 0;
					return;
				}
			};
			disableisr = 0;
			break;
		}

		return;
	}

	static uint8_t bitcount = 0;
	static uint8_t incoming = 0;
	static uint32_t prev_ms = 0;
	uint32_t now_ms;
	uint8_t n, val;

	val = digitalRead(DataPin);
	now_ms = millis();
	if (now_ms - prev_ms > 250)
	{
		bitcount = 0;
		incoming = 0;
	}
	prev_ms = now_ms;
	n = bitcount - 1;
	if (n <= 7)
	{
		incoming |= (val << n);
	}
	bitcount++;
	if (bitcount == 11)
	{
		uint8_t i = head + 1;
		if (i >= BUFFER_SIZE) i = 0;
		if (i != tail)
		{
			buffer[i] = incoming;
			head = i;
		}
		bitcount = 0;
		incoming = 0;
	}
}

#pragma GCC pop_options

byte FoxPS2Keyboard::GetScancode()
{
	uint8_t c, i;

	i = tail;
	if (i == head) return 0;
	i++;
	if (i >= BUFFER_SIZE) i = 0;
	c = buffer[i];
	tail = i;
	return (c);
}

byte FoxPS2Keyboard::GetParity(byte b)
{
	byte count = 0;
	for (byte i = 0; i < 8; i++)
	{
		if (((b >> i) & 0x1) == 1)
			count++;
	}
	return ((count % 2) == 1 ? 0 : 1);
}

void FoxPS2Keyboard::SendByte(byte b)
{
	SendBits[0] = 1;

	for (byte j = 0; j < 8; j++)
		SendBits[j + 1] = ((b >> j) & 0x1) == 0x1 ? 0x0 : 0x1;

	SendBits[9] = GetParity(b) == 0x1 ? 0x0 : 0x1;

	SendByteState = 0;
	fulldisableisr = 1;
	digitalWrite(TransistorIRQPin, 1);
	delayMicroseconds(100);
	digitalWrite(TransistorDataPin, 1);
	delayMicroseconds(100);
	digitalWrite(TransistorIRQPin, 0);
	disableisr = 1;
	fulldisableisr = 0;

	while (disableisr != 0);

	delay(10);
}

FoxPS2Keyboard::FoxPS2Keyboard()
{
	pinMode(IRQPin, INPUT_PULLUP);
	pinMode(DataPin, INPUT_PULLUP);
	pinMode(TransistorDataPin, OUTPUT);
	pinMode(TransistorIRQPin, OUTPUT);

	digitalWrite(TransistorDataPin, 0);
	digitalWrite(TransistorIRQPin, 0);

	disableisr = 0;
	head = 0;
	tail = 0;
	fulldisableisr = 0;
	attachInterrupt(digitalPinToInterrupt(IRQPin), KeyboardISR, FALLING);
}

bool FoxPS2Keyboard::SendByteWithConfirm(byte b, byte ACK, byte Tries, unsigned short Wait)
{
	unsigned long wait = millis();
	byte Test = 0;
	byte dat = 0;

	do
	{
		SendByte(b);
		while (1)
		{
			dat = GetScancode();
			if (dat == ACK)
				return(true);

			if (millis() - wait > Wait) //Sek
			{
				Test++;
				break;
			}
		}
	} while (Test < Tries);

	return(false);
}

bool FoxPS2Keyboard::SetKeyboardLights(byte NumLock, byte CapsLock, byte ScrollLock)
{
	byte b = 0;
	if (NumLock != 0)
		b |= 0x2;
	if (CapsLock != 0)
		b |= 0x4;
	if (ScrollLock != 0)
		b |= 0x1;

	if (SendByteWithConfirm(0xED, 0xFA, 1, 500) == false) //Change Lights
		return(false);

	if (SendByteWithConfirm(b, 0xFA, 1, 500) == false) //Lights data
		return(false);

	return(true);
}

bool FoxPS2Keyboard::WaitForKeyboard()
{
	unsigned long wait = millis();
	byte Test = 0;

	if (SendByteWithConfirm(0xFF, 0xAA, 5, 2000) == false) //Reset Keyboard
		return(false);

	if (SendByteWithConfirm(0xEE, 0xEE, 1, 500) == false) //Echo Test (connectivity test)
		return(false);

	if (SendByteWithConfirm(0xF4, 0xFA, 1, 500) == false) //Enable Keyboard
		return(false);

	return(true);
}