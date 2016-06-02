
#include "interrupts.h"
#include "screen.h"
#include <cpu.h>
#include <stdint.h>
#include <stdio.h>
#include <vesa.h>
#define outportb(a, b) outb(b, a)
typedef uint8_t byte;
typedef int8_t sbyte;
typedef int32_t dword;

//Mouse.inc by SANiK
//License: Use as you wish, except to cause damage
byte mouse_cycle=0;     //unsigned char
int32_t mouse_byte[4] = {0,0,0,0};    //signed char
int32_t mouse_x=40;         //signed char
int32_t mouse_y=12;         //signed char

int32_t x = 0;
int32_t y = 0;

void traitant_IT_44();



// void mouse_handler(void *r)//struct regs *r)
// {
// 	outb(0x20, 0xA0);
// 	outb(0x20, 0x20);

// 	(void)r;
// 	static unsigned char cycle = 0;
// 	static char mouse_bytes[3];
//     unsigned char status = inb(0x64);

//     printf("Mouse handler, status = 0x%X\n", status);


// 	mouse_bytes[cycle++] = inb(0x60);int x=0, y=0;
// 			printf("%d pressed\n", mouse_bytes[cycle-1]);

//      status = inb(0x64);
//     printf("Mouse handler, status = 0x%X\n", status);



// 	if (cycle == 3) { // if we have all the 3 bytes...
// 		cycle = 0; // reset the counter
// 		// do what you wish with the bytes, this is just a sample
// 		if ((mouse_bytes[0] & 0x80) || (mouse_bytes[0] & 0x40))
// 			return; // the mouse only sends information about overflowing, do not care about it and return
// 		if (!(mouse_bytes[0] & 0x20))
// 			y |= 0xFFFFFF00; //delta-y is a negative value
// 		if (!(mouse_bytes[0] & 0x10))
// 			x |= 0xFFFFFF00; //delta-x is a negative value
// 		if (mouse_bytes[0] & 0x4)
// 			printf("Middle button is pressed!n");
// 		if (mouse_bytes[0] & 0x2)
// 			printf("Right button is pressed!n");
// 		if (mouse_bytes[0] & 0x1)
// 			printf("Left button is pressed!n");
// 		// do what you want here, just replace the puts's to execute an action for each button
// 		// to use the coordinate data, use mouse_bytes[1] for delta-x, and mouse_bytes[2] for delta-y
// 	}
// }

bool initm = true;


//Mouse functions
void mouse_handler(void *a_r)//struct regs *a_r) //struct regs *a_r (not used but just there)
{
	outb(0x20, 0x20);
	outb(0x20, 0xA0);

	(void)a_r;

	char packet = inb(0x60);

	// Qemu startup bug
	if (initm && packet == 0x41) {
		initm = false;
		return;
	}
	// printf("mouse says : 0x%X\n", packet);
	// printf("mouse says : 0x%X\n", (int)inb(0x60));
	// printf("mouse says : 0x%X\n", (int)inb(0x60));
	// printf("mouse says : 0x%X\n", (int)inb(0x60));

	switch(mouse_cycle)
	{
		case 0:
			mouse_byte[0]=packet;
			mouse_cycle++;
			break;
		case 1:
			mouse_byte[1]=packet;
			mouse_cycle++;
			break;
		case 2:
			mouse_byte[2]=packet;
			mouse_x = mouse_byte[1];
			mouse_y = mouse_byte[2];

			char car = '@';

			// if (mouse_byte[0] & 0x20) {
			// 	mouse_y = -mouse_y;
				// printf(" yneg ");
			// }
				// mouse_y |= 0xFFFFFF00; //delta-y is a negative value

			// if (mouse_byte[0] & 0x10) {
			// 	mouse_x = -mouse_x;
				// printf(" xneg ");
			// }
				// mouse_x |= 0xFFFFFF00; //delta-x is a negative value

			if (mouse_byte[0] & 0x4) {
				// printf("Middle button is pressed!n");
				set_bg_color(WHITE);
				car = ' ';
			}
			if (mouse_byte[0] & 0x2) {
				// printf("Right button is pressed!n");
				set_bg_color(RED);
				car = ' ';
			}
			if (mouse_byte[0] & 0x1) {
				// printf("Left button is pressed!n");
				set_bg_color(BLUE);
				car = ' ';
			}

			// const int min = 0;


			// if (mouse_x > min && x < 79) {
			// 	// printf("RIGHT\n");
			// 	x++;
			// }

			// else if (mouse_x < -min && x >0) {
			// 	// printf("LEFT\n");
			// 	x--;
			// }

			// if (mouse_y < -min && y < 24) {
			// 	// printf("DOWN\n");
			// 	y++;
			// }

			// else if (mouse_y > min && y > 0) {
			// 	// printf("UP\n");
			// 	y--;
			// }

			x += mouse_x;
			y -= mouse_y;

			if (x > 1000)x=1000;
			if (y > 700)y=700;
			// if (x > 79)x=79;
			// if (y > 24)y=24;
			if (x < 0) x = 0;
			if (y < 0) y = 0;

			fill_rectangle(x, y, 5, 5, 0);
			fill_rectangle(x+1, y+1, 3, 3, 0xFFFFFFFF);
			ecrit_car(y, x, car);
			reset_color();

			// printf("mouse x %d  y %d\n", x, y);
			// printf("mouse x %d  y %d\n", mouse_x, mouse_y);
			// mouse_cycle++;
			mouse_cycle=0;
			break;
		// case 3:
		// 	mouse_byte[3]=inb(0x60);
		// 	mouse_cycle=0;
		// 	break;
	}

}

void mouse_wait(byte a_type) //unsigned char
{
	dword _time_out=100000; //unsigned int
	if(a_type==0)
	{
		while(_time_out--) //Data
		{
			if((inb(0x64) & 1)==1)
			{
				return;
			}
		}
		return;
	}
	else
	{
		while(_time_out--) //Signal
		{
			if((inb(0x64) & 2)==0)
			{
				return;
			}
		}
		return;
	}
}

void mouse_write(byte a_write) //unsigned char
{
	//Wait to be able to send a command
	mouse_wait(1);
	//Tell the mouse we are sending a command
	outportb(0x64, 0xD4);
	//Wait for the final part
	mouse_wait(1);
	//Finally write
	outportb(0x60, a_write);
}

byte mouse_read()
{
	//Get's response from mouse
	mouse_wait(0); 
	return inb(0x60);
}

void init_mouse()
{
	byte _status;  //unsigned char

	//Enable the auxiliary mouse device
	mouse_wait(1);
	outportb(0x64, 0xA8);
	
	//Enable the interrupts
	mouse_wait(1);
	outportb(0x64, 0x20);
	mouse_wait(0);
	_status=(inb(0x60) | 2);
	mouse_wait(1);
	outportb(0x64, 0x60);
	mouse_wait(1);
	outportb(0x60, _status);
	
	//Tell the mouse to use default settings
	mouse_write(0xF6);
	mouse_read();  //Acknowledge
	
	//Enable the mouse
	mouse_write(0xF4);
	mouse_read();  //Acknowledge

	// Setup the mouse handler
	init_traitant_IT_user(32 + 12, (int)traitant_IT_44);
	masque_IRQ(2, false);
	masque_IRQ(12, false);
}
