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
        
    def set_led(self, index: int, brightness: int, green: int, blue: int, red: int) -> None:
        """
        Set a single LED's color and brightness
        
        Args:
            index: LED position (0 to num_leds-1)
            brightness: 5-bit value (0-31)
            green: 8-bit value (0-255)
            blue: 8-bit value (0-255)
            red: 8-bit value (0-255)
        """
        if not 0 <= index < self.num_leds:
            raise ValueError(f"LED index {index} out of range (0-{self.num_leds-1})")
            
        if not (0 <= brightness <= 31):
            raise ValueError("Brightness must be between 0 and 31")
            
        if not all(0 <= color <= 255 for color in (red, green, blue)):
            raise ValueError("Color values must be between 0 and 255")
        
        # Create the 32-bit word
        word = (0b111 << 29)           # Header bits
        word |= (brightness & 0x1F) << 24  # 5 bits of brightness
        word |= (green & 0xFF) << 16   # 8 bits of green
        word |= (blue & 0xFF) << 8     # 8 bits of blue
        word |= (red & 0xFF)           # 8 bits of red
        
        # Convert to 4 bytes and store in buffer
        base = index * 4
        self.buffer[base]     = (word >> 24) & 0xFF
        self.buffer[base + 1] = (word >> 16) & 0xFF
        self.buffer[base + 2] = (word >> 8) & 0xFF
        self.buffer[base + 3] = word & 0xFF
    
    def fill(self, brightness: int, green: int, blue: int, red: int) -> None:
        """Set all LEDs to the same color and brightness"""
        for i in range(self.num_leds):
            self.set_led(i, brightness, green, blue, red)
    
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

strip.fill(brightness=5, green=0, blue=0, red=255)
strip.update()
time.sleep(2)
strip.fill(brightness=5, green=0, blue=255, red=0)
strip.update()
time.sleep(2)
strip.fill(brightness=5, green=255, blue=0, red=0)
strip.update()
time.sleep(2)

# schedule.every(1/24.0).seconds.do(controller.update_lights)

# try:
#     while controller.running:
#         schedule.run_pending()
#         time.sleep(0.1)  # Small sleep to prevent CPU hogging
# except KeyboardInterrupt:
#     controller.cleanup()

