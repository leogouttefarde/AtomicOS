
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <cpu.h>
#include <segment.h>
#include "screen.h"
#include "time.h"
#include "process.h"
#include "interrupts.h"

#define QUARTZ 0x1234DD


// Déclaration du traitant asm
void traitant_IT_32();

// Variables internes
static uint32_t g_tics = 0;
static uint32_t g_secs = 0;


static void read_rtc();


// Affiche le temps en haut à droite
static inline void affiche_temps(char *chaine)
{
	const uint8_t taille = strlen(chaine);
	const uint8_t lig = 0;
	uint8_t col = NB_COLS - taille;

	for (uint32_t i = 0; i < taille; i++)
		ecrit_car(lig, col++, chaine[i]);
}

// Initialise la fréquence d'horloge
static inline void init_freq(const uint32_t freq)
{
	const uint16_t freq_sig = QUARTZ / freq;

	outb(0x34, 0x43);
	outb(freq_sig, 0x40);
	outb(freq_sig >> 8, 0x40);
}

/**
 * Initialise le module.
 */
void init_time()
{
	init_traitant_IT(32, traitant_IT_32);
	init_freq(SCHEDFREQ);
	masque_IRQ(0, false);

	// Crashes gdb somehow, enable later
	if (0)	read_rtc();
}

/**
 * Retourne le nombre de secondes écoulées depuis le démarrage du noyau.
 */
uint32_t get_time()
{
	return g_secs;
}

// Fonction du traitant de l'interruption horloge
void tic_PIT(void)
{
	// Acquittement de l'interruption
	outb(0x20, 0x20);

	// Incrémentation des secondes
	if (!(++g_tics % SCHEDFREQ)) {
		g_secs++;
	}

	char temps[9];
	uint32_t mns = g_secs / 60;
	uint32_t hrs = mns / 60;

	sprintf(temps, "%02d:%02d:%02d", hrs % 24, mns % 60, g_secs % 60);
	affiche_temps(temps);

	ordonnance();
}

/**
 * Retourne dans *quartz la fréquence du quartz du système et
 * dans *ticks le nombre d'oscillations du quartz entre chaque interruption.
 */
void clock_settings(unsigned long *quartz, unsigned long *ticks)
{
	*quartz = QUARTZ;
	*ticks = QUARTZ / SCHEDFREQ;
}

/**
 * Retourne le nombre d'interruptions d'horloge depuis le démarrage du noyau.
 */
unsigned long current_clock()
{
	return g_tics;
}


// Change this each year!
#define CURRENT_YEAR        2016

// Set by ACPI table parsing code if possible
static int century_register = 0x00;

static unsigned char second;
static unsigned char minute;
static unsigned char hour;
static unsigned char day;
static unsigned char month;
static unsigned int year;

enum {
	cmos_address = 0x70,
	cmos_data    = 0x71
};

static int get_update_in_progress_flag()
{
	outb(0x0A, cmos_address);
	return (inb(cmos_data) & 0x80);
}

static unsigned char get_RTC_register(int reg)
{
	outb(reg, cmos_address);
	return inb(cmos_data);
}

// Source : http://wiki.osdev.org/CMOS#Reading_All_RTC_Time_and_Date_Registers
static void read_rtc()
{
	unsigned char century;
	unsigned char last_second;
	unsigned char last_minute;
	unsigned char last_hour;
	unsigned char last_day;
	unsigned char last_month;
	unsigned char last_year;
	unsigned char last_century;
	unsigned char registerB;

	// Note: This uses the "read registers until you get the same values twice in a row" technique
	//       to avoid getting dodgy/inconsistent values due to RTC updates

	while (get_update_in_progress_flag());                // Make sure an update isn't in progress
	second = get_RTC_register(0x00);
	minute = get_RTC_register(0x02);
	hour = get_RTC_register(0x04);
	day = get_RTC_register(0x07);
	month = get_RTC_register(0x08);
	year = get_RTC_register(0x09);
	if(century_register != 0) {
		century = get_RTC_register(century_register);
	}

	do {
		last_second = second;
		last_minute = minute;
		last_hour = hour;
		last_day = day;
		last_month = month;
		last_year = year;
		last_century = century;
 
		while (get_update_in_progress_flag());           // Make sure an update isn't in progress
		second = get_RTC_register(0x00);
		minute = get_RTC_register(0x02);
		hour = get_RTC_register(0x04);
		day = get_RTC_register(0x07);
		month = get_RTC_register(0x08);
		year = get_RTC_register(0x09);
		if(century_register != 0) {
			century = get_RTC_register(century_register);
		}
	} while( (last_second != second) || (last_minute != minute) || (last_hour != hour) ||
		   (last_day != day) || (last_month != month) || (last_year != year) ||
		   (last_century != century) );
 
	registerB = get_RTC_register(0x0B);

	// Convert BCD to binary values if necessary

	if (!(registerB & 0x04)) {
		second = (second & 0x0F) + ((second / 16) * 10);
		minute = (minute & 0x0F) + ((minute / 16) * 10);
		hour = ( (hour & 0x0F) + (((hour & 0x70) / 16) * 10) ) | (hour & 0x80);
		day = (day & 0x0F) + ((day / 16) * 10);
		month = (month & 0x0F) + ((month / 16) * 10);
		year = (year & 0x0F) + ((year / 16) * 10);
		if(century_register != 0) {
			century = (century & 0x0F) + ((century / 16) * 10);
		}
	}

	// Convert 12 hour clock to 24 hour clock if necessary

	if (!(registerB & 0x02) && (hour & 0x80)) {
		hour = ((hour & 0x7F) + 12) % 24;
	}

	// Calculate the full (4-digit) year

	if(century_register != 0) {
		year += century * 100;
	} else {
		year += (CURRENT_YEAR / 100) * 100;
		if(year < CURRENT_YEAR) year += 100;
	}

	g_secs = second + ((hour+2) * 60 + minute) * 60;
}

