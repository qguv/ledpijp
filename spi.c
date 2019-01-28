/* based heavily on the following resources:
 *
 * https://raspberry-projects.com/pi/programming-in-c/spi/using-the-spi-interface
 * http://hertaville.com/interfacing-an-spi-adc-mcp3008-chip-to-the-raspberry-pi-using-c.html
 */

#include <fcntl.h>		/* open */
#include <linux/spi/spidev.h>	/* SPI_ constants */
#include <stdint.h>		/* uint8_t */
#include <stdio.h>		/* perror */
#include <string.h>		/* memset */
#include <sys/ioctl.h>		/* ioctl */
#include <sys/stat.h>		/* open */
#include <sys/types.h>		/* open */
#include <unistd.h>		/* close */

#include "spi.h"

int spi_cs0_fd, spi_cs1_fd;
unsigned char spi_mode;
unsigned int spi_speed;
unsigned int spi_bitsPerWord;

/* spi_device 0=CS0, 1=CS1 */
/* 0 if successful */
int spi_open_port(int spi_device)
{
	int status;
	int *spi_cs_fd;
	char spi_mode;

	/* CPOL = 0, CPHA = 0, Clock idle low, data is clocked in on rising edge, output data (change) on falling edge */
	spi_mode = SPI_MODE_0;

	spi_bitsPerWord = 8;

	/* 8 MHz SPI bus */
	spi_speed = 8 * SPI_MHz;

	spi_cs_fd = spi_device ? &spi_cs1_fd : &spi_cs0_fd;
	*spi_cs_fd = open(spi_device ? "/dev/spidev0.1" : "/dev/spidev0.0", O_WRONLY);

	if (*spi_cs_fd < 0) {
		perror("couldn't open spi device");
		puts(
			"\nsome suggestions:"
			"\n- you need to have an spi device on your system (raspi has one built-in)"
			"\n- you need to run this as root or own the spi device"
			"\n- you need to enable spi in raspi-config or similar"
			"\n- you need a linux kernel build that includes spi support (raspian does)"
		);
		return 1;
	}

	status = ioctl(*spi_cs_fd, SPI_IOC_WR_MODE, &spi_mode);
	if (status < 0)
	{
		perror("couldn't set spi write mode");
		return 3;
	}

	status = ioctl(*spi_cs_fd, SPI_IOC_RD_MODE, &spi_mode);
	if(status < 0)
	{
		perror("couldn't set spi read mode");
		return 4;
	}

	status = ioctl(*spi_cs_fd, SPI_IOC_WR_BITS_PER_WORD, &spi_bitsPerWord);
	if(status < 0)
	{
		perror("couldn't set spi writing bits-per-word");
		return 5;
	}

	status = ioctl(*spi_cs_fd, SPI_IOC_RD_BITS_PER_WORD, &spi_bitsPerWord);
	if(status < 0)
	{
		perror("couldn't set spi reading bits-per-word");
		return 6;
	}

	status = ioctl(*spi_cs_fd, SPI_IOC_WR_MAX_SPEED_HZ, &spi_speed);
	if(status < 0)
	{
		perror("couldn't set spi write speed");
		return 7;
	}

	status = ioctl(*spi_cs_fd, SPI_IOC_RD_MAX_SPEED_HZ, &spi_speed);
	if(status < 0)
	{
		perror("couldn't set spi read speed");
		return 8;
	}

	return 0;
}

int spi_close_port(int spi_device)
{
	int status = -1;

	status = close(spi_device ? spi_cs1_fd : spi_cs0_fd);
	if (status < 0)
	{
		perror("couldn't close spi device");
		return 9;
	}

	return 0;
}

/* data		Bytes to write. Contents is overwritten with bytes read. */
int spi_write(int spi_device, uint8_t data[], size_t length)
{
	struct spi_ioc_transfer spi;
	int status;
	int *spi_cs_fd;

	spi_cs_fd = spi_device ? &spi_cs1_fd : &spi_cs0_fd;

	memset(&spi, 0, sizeof spi);
	spi.tx_buf		= (unsigned long) data;
	spi.len			= length;
	spi.speed_hz		= spi_speed;
	spi.bits_per_word	= spi_bitsPerWord;

	status = ioctl(*spi_cs_fd, SPI_IOC_MESSAGE(1), &spi);

	if (status < 0)
	{
		perror("couldn't transmit spi data");
		return 2;
	}

	return 0;
}
