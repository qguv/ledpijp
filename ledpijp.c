#include "apa102.h"
#include "fake102.h" /* DEBUG */

#include <signal.h>	/* signal */
#include <stdio.h>	/* perror */
#include <stdlib.h>	/* exit */
#include <time.h>	/* nanosleep */

uint8_t frames[360][SPI_DATA_BYTES];

static volatile int sigint_sent;

static void sigint_handler(int _)
{
	(void) _;
	sigint_sent = 1;
}

int main(void)
{
	int err;

	/* register ^C handler */
	sigint_sent = 0;
	signal(SIGINT, sigint_handler);

	/* generate rainbow frames */
	for (int i = 0; i < 360; i++)
		led_rainbow(i, i + 180, frames[i]);

	struct timespec sleep_time;
	sleep_time.tv_sec = 0;
	sleep_time.tv_nsec = 10000000;

	err = led_setup();
	if (err)
		exit(err);

	for (int i = 0; !sigint_sent; i++, i %= 360) {
		err = led_blit(frames[i], 1);
		if (err)
			goto handle_err;
		nanosleep(&sleep_time, NULL);
	}

	return led_teardown();

handle_err:
	led_teardown();
	return err;
}
