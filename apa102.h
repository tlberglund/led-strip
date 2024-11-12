


typedef struct {
    unsigned int red : 8;
    unsigned int green : 8;
    unsigned int blue : 8;
    unsigned int brightness : 5;
    unsigned int unused : 3;
} APA102_LED;


APA102_LED *apa102_init(uint16_t strip_len);
APA102_LED *apa102_get_strip();
uint16_t apa102_get_strip_count();
uint16_t apa102_get_buffer_size();
void apa102_strip_update();
void apa102_set_led(uint16_t led, uint8_t red, uint8_t green, uint8_t blue, uint8_t brightness);
char *apa102_led_to_s(APA102_LED *led, char *buffer);
void apa102_log_strip();

