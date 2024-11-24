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
        self.spi.bits_per_word = 8
        self.spi.lsbfirst = False
        self.frame_time = 1.0/24.0
        animation_running = False

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
    
    def print_buffer(self):
        for i, byte in enumerate(self.buffer):
            print(f"{byte:02X} ", end ='')
            if (i+1) % 16 == 0:
                print()
        if len(self.buffer) % 16 != 0:
            print()

    def fill(self, brightness: int, green: int, blue: int, red: int) -> None:
        """Set all LEDs to the same color and brightness"""
        for i in range(self.num_leds):
            self.set_led(i, red, green, blue, brightness)
    
    def clear(self) -> None:
        """Turn off all LEDs"""
        self.fill(0, 0, 0, 0)
    
    def update(self) -> None:
        """Send the entire buffer to the LED strip"""
        self.print_buffer()
        self.spi.xfer3(self.buffer)
    
    def run_animation(self, pattern_func, duration=None):
        """
        Run an animation pattern for a specified duration
        pattern_func should be a method that updates LED states for one frame
        """
        self.running = True
        start_time = time.time()
        
        while self.running:
            frame_start = time.time()
            
            # Run the pattern function
            pattern_func()
            
            # Check if we should stop
            if duration and (time.time() - start_time) > duration:
                break
                
            # Calculate sleep time to maintain correct FPS
            elapsed = time.time() - frame_start
            sleep_time = max(0, self.frame_time - elapsed)
            time.sleep(sleep_time)
    
    def stop_animation(self):
        """Stop the current animation"""
        self.running = False
        self.clear_all()

    def cleanup(self) -> None:
        """Clear the strip and close SPI connection"""
        self.clear()
        self.update()
        self.spi.close()




def blue_blobs_on_orange():
    animator = LEDStrip(60)
    
    # Animation parameters
    ORANGE_COLOR = (237, 115, 21)
    BLUE_COLOR = (64, 88, 222)
    BLOB_WIDTH = 10
    TOTAL_TIME = 7
    
    # Calculate movement per frame
    pixels_per_second = animator.num_leds / TOTAL_TIME
    pixels_per_frame = pixels_per_second / 24  # Since we're running at 24 FPS
    
    blob_position = -BLOB_WIDTH  # Start blob just outside the strip
    
    def gaussian(x, mu, sig):
        """Calculate Gaussian distribution value at point x"""
        return math.exp(-((x - mu) ** 2) / (2 * sig ** 2))
    
    def blend_colors(color1, color2, ratio):
        """Blend two RGB colors based on ratio (0 to 1)"""
        return tuple(int(c1 * (1 - ratio) + c2 * ratio) 
                    for c1, c2 in zip(color1, color2))
    
    def update_frame():
        nonlocal blob_position
        
        # Update each LED
        for i in range(animator.num_leds):
            # Calculate blob influence at this position
            blob_influence = gaussian(i, blob_position, BLOB_WIDTH/3)
            blob_influence = min(1.0, max(0.0, blob_influence))  # Clamp between 0 and 1
            
            # Blend orange and blue based on blob influence
            r, g, b = blend_colors(ORANGE_COLOR, BLUE_COLOR, blob_influence)
            
            # Set the LED
            set_led(i, r, g, b, 31)  # Full brightness
        
        # Move blob position
        blob_position += pixels_per_frame
        
        # Reset blob position when it's completely off the strip
        if blob_position > animator.num_leds + BLOB_WIDTH:
            blob_position = -BLOB_WIDTH
    
    # Run the animation indefinitely (or until stopped)
    animator.run_animation(update_frame)

# Example usage:
if __name__ == "__main__":
    blue_blobs_on_orange()


# strip = LEDStrip(1)

# while 1:
#     input("Press enter for more SPI")
#     strip.fill(red=0, green=127, blue=0, brightness=15)
#     strip.update()
    # time.sleep(0.5)
    # strip.fill(red=0, green=255, blue=0, brightness=5)
    # strip.update()
    # time.sleep(0.5)
    # strip.fill(red=0, green=0, blue=255, brightness=5)
    # strip.update()
    # time.sleep(0.5)

# schedule.every(1/24.0).seconds.do(controller.update_lights)
