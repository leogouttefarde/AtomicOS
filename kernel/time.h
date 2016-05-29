
#ifndef __TIME_H__
#define __TIME_H__

#define SCHEDFREQ 50

#include <stdint.h>

/**
 * Initialise le module.
 */
void init_time();

/**
 * Retourne le nombre de secondes écoulées depuis le démarrage du noyau.
 */
uint32_t get_time();

/**
 * Retourne dans *quartz la fréquence du quartz du système et
 * dans *ticks le nombre d'oscillations du quartz entre chaque interruption.
 */
void clock_settings(unsigned long *quartz, unsigned long *ticks);

/**
 * Retourne le nombre d'interruptions d'horloge depuis le démarrage du noyau.
 */
unsigned long current_clock();

#endif
