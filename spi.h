#include <stdint.h>		/* uint8_t */
#include <stddef.h>		/* size_t */

#define SPI_MHz 1000000
#define SPI_kHz 1000

int spi_open_port(int spi_device);
int spi_close_port(int spi_device);
int spi_write(int spi_device, uint8_t *data, size_t length);
