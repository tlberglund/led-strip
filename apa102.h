


typedef struct {
    unsigned int unused : 3;
    unsigned int brightness : 5;
    unsigned int blue : 8;
    unsigned int green : 8;
    unsigned int red : 8;
} APA102_LED;


void apa102_init();
void apa102_strip_init(APA102_LED *strip, uint16_t led_count);
void apa102_strip_update(APA102_LED *strip, uint16_t led_count);
