#include <Arduino.h>
#include <SPI.h>
#include <LoRa.h>
#include <LoRa.h>

// Constants
#define MAX_MESSAGE_SIZE 255 // Maximum message size
#define ACK_TIMEOUT_MS 1000	 // Timeout for waiting for ACK (in milliseconds)
#define MAX_RETRIES 3		 // Maximum number of retries for sending a message

// Variables
byte sequenceNumber = 0;		 // Current sequence number
byte expectedSequenceNumber = 0; // Expected sequence number for incoming messages
bool waitingForAck = false;		 // Flag to indicate if waiting for ACK
byte retryCount = 0;			 // Retry count for the current message

String readSerial();
void sendMessage(byte sequenceAndChunk, String message);
void waitForAck();
void serialEvent();

void setup()
{
	// Initialize LoRa module
	if (!LoRa.begin(915E6)) {
		while (1); // Failed to initialize LoRa module, halt
	}

	Serial.begin(9600);
}

void loop()
{
	if (!waitingForAck)
	{
		// Read incoming message from serial
		String input = readSerial();

		// Split the message into chunks
		int numChunks = ceil(input.length() / (float)MAX_MESSAGE_SIZE);
		for (int chunkNum = 0; chunkNum < numChunks; chunkNum++)
		{
			// Prepare the message chunk
			byte sequenceNumberAndChunkNum = (sequenceNumber << 4) | chunkNum;
			String chunk = input.substring(chunkNum * MAX_MESSAGE_SIZE, (chunkNum + 1) * MAX_MESSAGE_SIZE);

			// Send the message chunk
			sendMessage(sequenceNumberAndChunkNum, chunk);
			waitForAck();
		}

		// Increment the sequence number
		sequenceNumber++;
	}
}

void sendMessage(byte sequenceAndChunk, String message)
{
	// Prepare the message packet
	byte packet[255];
	packet[0] = sequenceAndChunk;
	message.getBytes(packet + 1, min(MAX_MESSAGE_SIZE, message.length()));

	// Send the packet via LoRa
	LoRa.beginPacket();
	LoRa.write(packet, message.length() + 1);
	LoRa.endPacket();
}

void waitForAck()
{
	waitingForAck = true;
	retryCount = 0;

	// Wait for ACK or timeout
	unsigned long startTime = millis();
	while (waitingForAck)
	{
		if (millis() - startTime > ACK_TIMEOUT_MS)
		{
			// ACK timeout occurred
			if (retryCount < MAX_RETRIES)
			{
				// Retry sending the message
				retryCount++;
				return;
			}
			else
			{
				// Maximum retries reached, give up on this message
				waitingForAck = false;
				return;
			}
		}

		// Check for incoming ACK
		if (LoRa.parsePacket())
		{
			byte ack = LoRa.read();
			if (ack == expectedSequenceNumber)
			{
				// ACK received, move to the next expected sequence number
				expectedSequenceNumber++;
				waitingForAck = false;
			}
		}
	}
}

String readSerial()
{
	String input;
	while (Serial.available())
	{
		input += (char)Serial.read();
	}
	return input;
}

void serialEvent()
{
	while (Serial.available())
	{
		byte incoming = Serial.read();
		LoRa.write(incoming);
	}
}
