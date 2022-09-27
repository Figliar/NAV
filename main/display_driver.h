#include <driver/gpio.h>
#include <driver/spi_master.h>
#include <math.h>
#include <string.h>

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define DISPLAY_WIDTH 	128       //sirka displeje
#define DISPLAY_HEIGHT 	128       //vyska displeje
#define DISPLAY_OFFSETX 2         //offset souradnice x
#define DISPLAY_OFFSETY 3         //offset souradnice y
//cisla pinu, na ktere je pripojen displej
#define PIN_MOSI		23   
#define PIN_SCLK		18
#define PIN_CS			14
#define PIN_DC			27
#define PIN_RESET		4
#define PIN_BL			16

//kody zakladnich barev
#define COLOR_BLACK		0x0000
#define COLOR_WHITE		0xffff
#define COLOR_RED     0xf800
#define COLOR_GREEN		0x07e0
#define COLOR_BLUE		0x001f
#define COLOR_GRAY		0x8c51
#define COLOR_YELLOW	0xFFE0
#define COLOR_CYAN		0x07FF
#define COLOR_PURPLE	0xF81F

//font
#define FIRST_CHAR      0x20
#define LAST_CHAR       0x7E
#define LINE_HEIGHT     20

//struktura pro praci s fontem
typedef struct {
  uint16_t bitmap_offset; //index definice znaku
  uint8_t width;          //sirka znaku
  uint8_t height;         //vyska znaku 
  uint8_t x_advance;      //posun na ose x
  int8_t x_offset;        //vzdalenost x k levemu hornimu rohu
  int8_t y_offset;        //vzdalenost y k levemu hornimu rohu
} font_glyph;

//definice fontu
extern const uint8_t font_bitmaps[];
extern const font_glyph font_chars[]; 

//funkce pro praci se sbernici SPI
void spi_init();
void spi_write(const uint8_t *data, size_t length);
void spi_write_cmd(uint8_t cmd);
void spi_write_data(uint8_t *data, size_t length);
void spi_write_byte(uint8_t data);
void spi_write_word(uint16_t data);

//kresleni 
void lcd_init();
void lcd_draw_pixel(uint16_t x, uint16_t y, uint16_t color);
void lcd_draw_fill_rect(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color);
void lcd_fill_screen(uint16_t color);
void lcd_draw_line(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color);
void lcd_draw_rect(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color);
void lcd_draw_rot_rect(uint16_t xc, uint16_t yc, uint16_t w, uint16_t h, uint16_t angle, uint16_t color);
void lcd_draw_circle(uint16_t x0, uint16_t y0, uint16_t r, uint16_t color);
void lcd_draw_char(int16_t x, int16_t y, unsigned char c, uint16_t color);
void lcd_write_char(uint8_t c, bool doWrap, uint16_t textcolor);
void lcd_backlight_off();
void lcd_backlight_on();
void lcd_display_off();
void lcd_display_on();
void lcd_clear();
