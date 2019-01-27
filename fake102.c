#include "config.h"
#include "apa102.h"

#include <fcntl.h>		/* open */
#include <stdio.h>		/* dprintf */
#include <sys/stat.h>		/* open */
#include <unistd.h>		/* close */
#include <stdint.h>		/* uint8_t */

static char *html_header = "<!doctype html><html><head><style>body { background-color: #333; } .frame { border-color: gray; border-style: solid; border-width: 1px 1px 10px 1px; margin: 20px; display: inline-block; } .led { height: 1em; width: 0.5em; display: inline-block; }</style></head><body>";
static char *html_frame_start = "<div class=\"frame\">";
static char *html_led = "<div class=\"led\" style=\"background-color: #%02x%02x%02x;\"></div>";
static char *html_frame_end = "</div><br />";
static char *html_footer = "</body></html>";

int fake_blit(uint8_t *spi_data, int frames)
{
	int fd = open("/tmp/fake102.html", O_WRONLY | O_CREAT);
	dprintf(fd, html_header);

	for (int f = 0; f < frames; f++) {
		dprintf(fd, html_frame_start);
		uint8_t *frame = spi_data + f*SPI_DATA_BYTES + 4;
		for (int i = 0; i < 4*LEDS; i += 4)
			dprintf(fd, html_led, frame[i+3], frame[i+2], frame[i+1]);
		dprintf(fd, html_frame_end);
	}

	dprintf(fd, html_footer);

	close(fd);

	return 0;
}
