#include "apa102.h"

#ifdef DEBUG
#include "fake102.h"
#endif

#include <signal.h>	/* signal */
#include <stdio.h>	/* perror */
#include <stdlib.h>	/* exit */
#include <time.h>	/* nanosleep */

#define NUM_FRAMES 360

uint8_t frames[NUM_FRAMES][SPI_DATA_BYTES];
static volatile int doomed;
static volatile int spi_err;

static void sigint_handler(int _)
{
	(void) _;

	doomed = 1;
}

#ifndef DEBUG
static void send_frame(int sig, siginfo_t *si, void *uc)
{
	(void) sig;
	(void) si;
	(void) uc;

	if (doomed)
		return;

	static int frame = 0;
	spi_err = led_blit(frames[frame], 1);
	if (spi_err)
		doomed = 1;
	else
		frame = (frame + 1) % NUM_FRAMES;
}
#endif

int main(void)
{
	int err = 0;

	setbuf(stdout, NULL);

	/* register ^C handler */
	doomed = 0;
	signal(SIGINT, sigint_handler);

	printf("generating rainbows, please wait...");
	/* generate rainbow frames ahead of time */
	for (int i = 0; !doomed && i < 360; i++)
		led_rainbow(i, i + 180, frames[i]);
	printf("\r                                     ");
	if (doomed) {
		printf("\rrainbow generation aborted\n");
		exit(0);
	}

#ifdef DEBUG
	printf("\rwriting to /tmp/fake102.html ...");
	fake_blit((uint8_t *) frames, NUM_FRAMES);
	printf("\r                                ");
#else

	/* open LED file descriptors */
	printf("\ropening spi file descriptors...");
	err = led_setup();
	if (err)
		exit(err);

	printf("\rsetting up spi frame handler...");
	spi_err = 0;
	struct sigaction sa;
	sa.sa_flags = SA_SIGINFO;
	sa.sa_sigaction = send_frame;
	sigemptyset(&sa.sa_mask);
	sigaddset(&sa.sa_mask, SIGRTMIN);
	err = sigaction(SIGRTMIN, &sa, NULL);
	if (err == -1) {
		led_teardown();
		perror("couldn't register timer handler");
		return 10;
	}
	printf("\r                               ");

	printf("\rcreating spi frame timer...");
	timer_t timerid;
	struct sigevent se;
	struct itimerspec it;
	se.sigev_notify = SIGEV_SIGNAL;
	se.sigev_signo = SIGRTMIN;
	se.sigev_value.sival_ptr = &timerid;
	err = timer_create(CLOCK_BOOTTIME, &se, &timerid);
	if (err == -1) {
		led_teardown();
		perror("couldn't create timer");
		return 11;
	}

	printf("\rstarting spi frame timer...");
	it.it_value.tv_sec = it.it_interval.tv_sec = 0;
	it.it_value.tv_nsec = it.it_interval.tv_nsec = 10000000;
	err = timer_settime(timerid, 0, &it, NULL);
	if (err == -1) {
		led_teardown();
		perror("couldn't set timer");
		return 12;
	}
	printf("\r                           ");

	printf("\rentering busy loop...");
	while (!doomed);
	printf("\r                       ");

	if (spi_err) {
		led_teardown();
		return spi_err;
	}

	printf("\rtearing down...");
	err = led_teardown();
	printf("\r               ");
#endif

	printf("\r<3 ledpijp loves you!\n");
	return err;
}
