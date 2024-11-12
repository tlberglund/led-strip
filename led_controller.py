import spidev
import schedule
import time
import math
from datetime import datetime


class LEDController:
    def __init__(self):
        self.spi = spidev.SpiDev()
        self.spi.open(0, 0)
        self.spi.max_speed_hz = 1000000
        self.spi.mode = 0
        self.running = True
        self.frame_count = 0
        
    def format_led_word(self, red, green, blue, brightness):
        word = (0b111 << 29)
        word |= (brightness & 0x1F) << 24
        word |= (blue & 0xFF) << 16
        word |= (green & 0xFF) << 8
        word |= (red & 0xFF)
        
        return [(word >> 24) & 0xFF, 
                (word >> 16) & 0xFF, 
                (word >> 8) & 0xFF, 
                word & 0xFF]

    def update_lights(self):
        """Update the LED states - called every frame"""
        # Example: Make a pulsing white light based on frame count
        brightness = abs(int(15 * math.sin(self.frame_count * 0.2)) + 16)
        buffer = self.format_led_word(255, 255, 255, brightness)
        
        self.spi.xfer2(buffer)
        self.frame_count += 1

    def cleanup(self):
        self.running = False
        self.spi.close()
        print("\nCleaning up...")


controller = LEDController()

schedule.every(1/24.0).seconds.do(controller.update_lights)

try:
    while controller.running:
        schedule.run_pending()
        time.sleep(0.1)  # Small sleep to prevent CPU hogging
except KeyboardInterrupt:
    controller.cleanup()

