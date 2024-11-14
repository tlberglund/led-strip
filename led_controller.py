import spidev
import schedule
import time
import math
from datetime import datetime
from typing import List


class LEDStrip:
    def __init__(self, num_leds: int = 60):
        # Initialize SPI
        self.spi = spidev.SpiDev()
        self.spi.open(0, 0)
        self.spi.max_speed_hz = 1000000
        self.spi.mode = 0
        
        # Setup LED array
        self.num_leds = num_leds
        self.buffer = bytearray(num_leds * 4)  # 4 bytes per LED
        
    def set_led(self, index: int, red: int, green: int, blue: int, brightness: int) -> None:
        """
        Set a single LED's color and brightness
        
        Args:
            index: LED position (0 to num_leds-1)
            red: 8-bit value (0-255)
            green: 8-bit value (0-255)
            blue: 8-bit value (0-255)
            brightness: 5-bit value (0-31)
        """
        if not 0 <= index < self.num_leds:
            raise ValueError(f"LED index {index} out of range (0-{self.num_leds-1})")
            
        if not (0 <= brightness <= 31):
            raise ValueError("Brightness must be between 0 and 31")
            
        if not all(0 <= color <= 255 for color in (red, green, blue)):
            raise ValueError("Color values must be between 0 and 255")

        # Convert to 4 bytes and store in buffer
        base = index * 4
        self.buffer[base]     = brightness & 0x1f
        self.buffer[base + 1] = red & 0xff
        self.buffer[base + 2] = green & 0xff
        self.buffer[base + 3] = blue & 0xff
    
    def fill(self, brightness: int, green: int, blue: int, red: int) -> None:
        """Set all LEDs to the same color and brightness"""
        for i in range(self.num_leds):
            self.set_led(i, red, green, blue, brightness)
    
    def clear(self) -> None:
        """Turn off all LEDs"""
        self.fill(0, 0, 0, 0)
    
    def update(self) -> None:
        """Send the entire buffer to the LED strip"""
        self.spi.xfer3(self.buffer)
    
    def cleanup(self) -> None:
        """Clear the strip and close SPI connection"""
        self.clear()
        self.update()
        self.spi.close()


strip = LEDStrip(60)

while 1:
    strip.fill(red=255, green=0, blue=0, brightness=5)
    strip.update()
    time.sleep(0.5)
    strip.fill(red=0, green=255, blue=0, brightness=5)
    strip.update()
    time.sleep(0.5)
    strip.fill(red=0, green=0, blue=255, brightness=5)
    strip.update()
    time.sleep(0.5)

# schedule.every(1/24.0).seconds.do(controller.update_lights)

# try:
#     while controller.running:
#         schedule.run_pending()
#         time.sleep(0.1)  # Small sleep to prevent CPU hogging
# except KeyboardInterrupt:
#     controller.cleanup()

