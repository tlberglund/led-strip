#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/spi/spidev.h>

// Usually /dev/spidev0.0 for CE0 and /dev/spidev0.1 for CE1
#define SPI_DEVICE "/dev/spidev0.0"
#define SPI_SPEED_HZ 1000000  // 1 MHz

int main(int argc, char **argv) {
    int fd;
    uint8_t buffer[] = {0x01, 0x02, 0x03, 0x04};
    uint8_t read_buffer[sizeof(buffer)];
    
    // Open SPI device
    fd = open(SPI_DEVICE, O_RDWR);
    if (fd < 0) {
        perror("Can't open SPI device");
        return 1;
    }
    
    // Configure SPI mode
    uint8_t mode = SPI_MODE_0;
    if (ioctl(fd, SPI_IOC_WR_MODE, &mode) < 0) {
        perror("Can't set SPI mode");
        close(fd);
        return 1;
    }
    
    // Configure bits per word
    uint8_t bits = 8;
    if (ioctl(fd, SPI_IOC_WR_BITS_PER_WORD, &bits) < 0) {
        perror("Can't set bits per word");
        close(fd);
        return 1;
    }
    
    // Configure max speed
    uint32_t speed = SPI_SPEED_HZ;
    if (ioctl(fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed) < 0) {
        perror("Can't set max speed");
        close(fd);
        return 1;
    }
    
    // Prepare the transfer
    struct spi_ioc_transfer transfer = {
        .tx_buf = (unsigned long)buffer,
        .rx_buf = (unsigned long)read_buffer,
        .len = sizeof(buffer),
        .speed_hz = SPI_SPEED_HZ,
        .bits_per_word = bits,
        .delay_usecs = 0,
    };
    
    // Perform the transfer
    if (ioctl(fd, SPI_IOC_MESSAGE(1), &transfer) < 0) {
        perror("SPI transfer failed");
        close(fd);
        return 1;
    }
    
    // Print what we sent and received
    printf("Sent: ");
    for (size_t i = 0; i < sizeof(buffer); i++) {
        printf("0x%02X ", buffer[i]);
    }
    printf("\n");
    
    printf("Received: ");
    for (size_t i = 0; i < sizeof(buffer); i++) {
        printf("0x%02X ", read_buffer[i]);
    }
    printf("\n");
    
    close(fd);
    return 0;
}