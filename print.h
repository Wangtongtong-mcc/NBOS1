#define BACKSPACE 0x08
#define LINEFEED 0x0a
#define CARRIAGE_RETURN 0x0d
#define VEDIO_POSITION 0xc00b8000
#define CON_ATTR 0x70
#define LAST_CURSOR 2000

void print(unsigned char * p);
void printn(unsigned int number);
void clear_screen();