
#include "interrupts.h"
#include "screen.h"
#include <cpu.h>
#include <stdint.h>
#include <stdio.h>
#include <vesa.h>

uint8_t cycle = 0;
int8_t packets[3];

int32_t mouse_x = 40;
int32_t mouse_y = 12;

int32_t x = 0;
int32_t y = 0;

void traitant_IT_44();

//Mouse functions
void mouse_handler()
{
	outb(0x20, 0x20);
	outb(0x20, 0xA0);

	char packet = inb(0x60);

	// Discard invalid packets
	if (!cycle && ((packet & 0xC0) || !(packet & 0x8))) {
		// printf("mouse error : 0x%X\n", packet);
		return;
	}

	// printf("mouse says : 0x%X\n", packet);

	switch (cycle) {
	case 0:
	case 1:
		packets[cycle]=packet;
		cycle++;
		break;

	case 2:
		packets[2]=packet;
		cycle = 0;

		mouse_x = packets[1];
		mouse_y = packets[2];


		char car = '@';

		// if (packets[0] & 0x20) {
		// 	mouse_y = -mouse_y;
			// printf(" yneg ");
		// }
			// mouse_y |= 0xFFFFFF00; //delta-y is a negative value

		// if (packets[0] & 0x10) {
		// 	mouse_x = -mouse_x;
			// printf(" xneg ");
		// }
			// mouse_x |= 0xFFFFFF00; //delta-x is a negative value

		if (packets[0] & 0x4) {
			// printf("Middle button is pressed!n");
			set_bg_color(WHITE);
			car = ' ';
		}
		if (packets[0] & 0x2) {
			// printf("Right button is pressed!n");
			set_bg_color(RED);
			car = ' ';
		}
		if (packets[0] & 0x1) {
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
		//fill_rectangle(x+1, y+1, 3, 3, 0xFFFFFFFF);
		ecrit_car(y, x, car);
		reset_color();

		// printf("mouse x %d  y %d\n", x, y);
		// printf("mouse x %d  y %d\n", mouse_x, mouse_y);
		// cycle++;
		break;
	// case 3:
	// 	packets[3]=inb(0x60);
	// 	cycle=0;
	// 	break;
	}
}

void mouse_wait(uint8_t a_type)
{
	uint32_t _time_out=100000;
	if(a_type==0)
	{
		while(_time_out--)
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
		while(_time_out--)
		{
			if((inb(0x64) & 2)==0)
			{
				return;
			}
		}
		return;
	}
}

void mouse_write(uint8_t a_write) //unsigned char
{
	//Wait to be able to send a command
	mouse_wait(1);
	//Tell the mouse we are sending a command
	outb(0xD4, 0x64);
	//Wait for the final part
	mouse_wait(1);
	//Finally write
	outb(a_write, 0x60);
}

uint8_t mouse_read()
{
	//Get's response from mouse
	mouse_wait(0); 
	return inb(0x60);
}

void init_mouse()
{
	uint8_t _status;  //unsigned char

	//Enable the auxiliary mouse device
	mouse_wait(1);
	outb(0xA8, 0x64);
	
	//Enable the interrupts
	mouse_wait(1);
	outb(0x20, 0x64);
	mouse_wait(0);
	_status=(inb(0x60) | 2);
	mouse_wait(1);
	outb(0x60, 0x64);
	mouse_wait(1);
	outb(_status, 0x60);
	
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
