#include <iostream>
#include <fstream>
#include <string>
using namespace std;

#define READ 0
#define WRITE 1
#define DEBUG 2

#define MSG_MAX 195075
#define IMG_MAX 195126

void crypt(char str[MSG_MAX]);

int main()
{
	int mode = 2;
	char *source_path = 0;
	char *message_path = "payload.txt";

	cout << "Image Messenger" << endl;
	cout << endl;
	cout << "Usage: [MODE] [PATH] [MESSAGE] [KEY]" << endl;
	cout << "MODE:" << endl;
	cout << "	Extract Message		: 0" << endl;
	cout << "	Inject message		: 1" << endl;
	cout << "PATH:" << endl;
	cout << "	Source path		: source_image.bmp" << endl;
	cout << "	Format			: 24 bit BMP" << endl;
	cout << "	Max size		: 255 x 255" << endl;
	cout << "MESSAGE:" << endl;
	cout << "	Source path		: payload.txt" << endl;
	cout << "	Format			: ASCII" << endl;
	cout << "	Max size		: image size / 3" << endl;
	cout << "KEY:" << endl;
	cout << "	Source path		: encryption_key.txt" << endl;
	cout << "	Format			: ASCII" << endl;
	cout << "	Max size		: 1000 char" << endl;
	cout << "" << endl;
	cout << "Select Mode [ 0 / 1 ] : ";
	cin >> mode;
	cout << "" << endl;

	if (mode == 0)
		source_path = "injected_image.bmp";
	else if (mode == 1)
		source_path = "source_image.bmp";

	string input_message;
	char message[MSG_MAX] = {};
	int msg_len = 0;

	if (mode == WRITE || mode == DEBUG)
	{

		// Open message.
		ifstream message_file;
		message_file.open(message_path);
		if (!message_file.is_open())
		{
			cout << "Failure: Open message file.\a" << endl;
			cout << message_path << " not found." << endl;
			cout << "" << endl;
			system("pause");
			return -1;
		}
		message_file.seekg(0, ios::end);
		streampos message_size = message_file.tellg();
		message_file.seekg(0, ios::beg);
		//message = new char[(int)message_size];
		message_file.read(message, message_size);
		message_file.close();

		msg_len = message_size;

		crypt(message);
	}


	char buff[IMG_MAX];

	// Open image.
	ifstream image;
	image.open(source_path, ios::in | ios::binary);
	if (!image.is_open())
	{
		cout << "Failure: Open image file.\a" << endl;
		cout << source_path << " not found." << endl;
		cout << "" << endl;
		system("pause");
		return -1;
	}
	image.seekg(0, ios::end);
	int size = image.tellg();
	image.seekg(0, ios::beg);
	image.read(buff, size);
	image.close();

	if (msg_len > (size / 3))
	{
		cout << "Failure: Message too long.\a" << endl;
		cout << "" << endl;
		system("pause");
		return -1;
	}

	// TODO Little endian. Add proper cast.
	int header_size = buff[14] + 14;			// 14 - 17  Header_remaining + header_behind 
	int pixelmap_offset = header_size;			// 10 - 13	
	int witdh = buff[18];						// 18 - 21
	int height = buff[22];						// 22 - 25
	int bytes_per_pixel = 3;
	int row_padding_size = 3;

	int clr_byte = 0;
	int msg_cursor = 0;
	char msg_ch = message[msg_cursor];
	int bit_index = 0;
	unsigned char bit = 1;

	char read_message[MSG_MAX] = {};
	int read_message_index = 0;
	unsigned char read_ch = 0;

	// Loop RGB values.
	for (int cursor = pixelmap_offset; cursor < size; cursor++) // cursor += bytes_per_pixel
	{
		// Write.
		if (mode == WRITE || mode == DEBUG)
		{
			buff[cursor] &= ~1;
			if ((msg_ch & bit) != 0)
				buff[cursor] |= 1;
		}

		// Read.
		if (mode == READ || mode == DEBUG)
		{
			if ((buff[cursor] & 1) != 0)
				read_ch |= bit;
		}

		// Next bit.
		bit_index++;
		bit <<= 1;

		// Next byte.
		if (bit_index == 8)
		{
			bit_index = 0;
			bit = 1;
			msg_cursor++;
			if (msg_cursor < MSG_MAX)
				msg_ch = message[msg_cursor];
			else
				msg_ch = 0;

			if (read_message_index < MSG_MAX)
				read_message[read_message_index] = read_ch;
			read_message_index++;
			read_ch = 0;
		}

		clr_byte++;
		// Skip padding.
		if ((clr_byte % (height * bytes_per_pixel)) == 0)
		{
			cursor += row_padding_size;
		}
	}

	// Create read message file.
	if (mode == READ)
	{
		crypt(read_message);

		char *output_path = "extracted_message.txt";
		int output_size = strlen((char *)read_message);
		//int output_size = MSG_MAX;
		ofstream output(output_path, ios::out | ios::binary);
		if (!output.is_open())
		{
			cout << "Failure: Create message file.\a" << endl;
			cout << "" << endl;
			system("pause");
			return -1;
		}

		output.write((char *)read_message, output_size);
		output.close();

		cout << "Success: Message extracted." << endl;
		cout << "New file: extracted_message.txt" << endl;
		cout << endl;
		//cout << "Message:" << endl;
		//cout << (char *)read_message << endl;
		system("pause");
	}

	// Create modified image.
	if (mode == WRITE)
	{
		char *output_path = "injected_image.bmp";
		ofstream output(output_path, ios::out | ios::binary);
		if (!output.is_open())
		{
			cout << "Failure: Create injected image.\a" << endl;
			cout << "" << endl;
			system("pause");
			return -1;
		}
		output.write((char *)buff, size);
		output.close();

		cout << "Success: Message injected." << endl;
		cout << "New file: " << output_path << endl;
		cout << endl;
		system("pause");
	}

	return 0;
}

void crypt(char str[MSG_MAX])
{
	char *encryption_path = "encryption_key.txt";

	ifstream encryption_file(encryption_path, ios::in);
	if (!encryption_file.is_open())
	{
		cout << "Failure: Open encryption key.\a" << endl;
		cout << "" << endl;
		system("pause");
		exit(-1);
	}
	encryption_file.seekg(0, ios::end);
	int key_len = encryption_file.tellg();
	char *key = new char[key_len];
	encryption_file.seekg(0, ios::beg);
	encryption_file.read(key, key_len);
	encryption_file.close();

	int key_index = 0;
	for (int i = 0; i < MSG_MAX; i++)
	{
		str[i] ^= key[key_index];

		key_index += 3; // shuffle
		if (key_index > key_len)
			key_index = 0;
	}
}