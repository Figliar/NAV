#include "display_driver.h"

spi_device_handle_t spi_handle;
int16_t cursor_x = 0;     ///pozice x kurzoru pro psani textu
int16_t cursor_y = 0;     ///pozice y kurzoru pro psani textu

//inicializace sbernice SPI
void spi_init()
{
    //Nastaveni CS
    gpio_reset_pin(PIN_CS);
    gpio_set_direction(PIN_CS, GPIO_MODE_OUTPUT);
    gpio_set_level(PIN_CS, 0);
    //Nastaveni DC
    gpio_reset_pin(PIN_DC);
    gpio_set_direction(PIN_DC, GPIO_MODE_OUTPUT);
    gpio_set_level(PIN_DC, 0);
    //Nastaveni Reset
    gpio_reset_pin(PIN_RESET);
    gpio_set_direction(PIN_RESET, GPIO_MODE_OUTPUT);
    gpio_set_level(PIN_RESET, 0);
    vTaskDelay(pdMS_TO_TICKS(100));
    gpio_set_level(PIN_RESET, 1);
    //Nastaveni Backlight
    gpio_reset_pin(PIN_BL);
    gpio_set_direction(PIN_BL, GPIO_MODE_OUTPUT);
    gpio_set_level(PIN_BL, 0);

    //Nastaveni SCLK a MOSI
    spi_bus_config_t buscfg = {
        .sclk_io_num = PIN_SCLK,
        .mosi_io_num = PIN_MOSI,
        .miso_io_num = -1,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1};
    spi_bus_initialize(HSPI_HOST, &buscfg, SPI_DMA_CH_AUTO);
    
    //Nastaveni CS a frekvence
    spi_device_interface_config_t devcfg = {
        .clock_speed_hz = SPI_MASTER_FREQ_40M,
        .spics_io_num = PIN_CS,
        .queue_size = 7,
        .flags = SPI_DEVICE_NO_DUMMY};
    spi_bus_add_device(HSPI_HOST, &devcfg, &spi_handle);
}

//zapis dat
void spi_write(const uint8_t *data, size_t length)
{
    spi_transaction_t spi_transaction;

    if (length > 0) {
        memset(&spi_transaction, 0, sizeof(spi_transaction_t));
        spi_transaction.length = length * 8;
        spi_transaction.tx_buffer = data;
        spi_device_transmit(spi_handle, &spi_transaction);
    }
}

//zapis prikazu
void spi_write_cmd(uint8_t cmd)
{
    gpio_set_level(PIN_DC, 0);
    spi_write(&cmd, 1);
}

//zapis dat
void spi_write_data(uint8_t *data, size_t length)
{
    gpio_set_level(PIN_DC, 1);
    spi_write(data, length);
}

//zapis jednoho bytu dat
void spi_write_byte(uint8_t data)
{
    gpio_set_level(PIN_DC, 1);
    spi_write(&data, 1);
}

//zapis dvou bytu
void spi_write_word(uint16_t data)
{
    static uint8_t Byte[2];
    Byte[0] = (data >> 8) & 0xFF;
    Byte[1] = data & 0xFF;
    gpio_set_level(PIN_DC, 1);
    return spi_write(Byte, 2);
}

//zapis informace o barve pro vice pixelu
void spi_write_color(uint16_t color, uint16_t size)
{
    static uint8_t data[1024];
    int index = 0;
    for (int i = 0; i < size; i++) {
        data[index++] = (color >> 8) & 0xFF;
        data[index++] = color & 0xFF;
    }
    gpio_set_level(PIN_DC, 1);
    spi_write(data, size * 2);
}

//inicializace displeje
void lcd_init()
{
  	spi_init();

    spi_write_cmd(0x36);   //Memory Access Control
    spi_write_byte(0xC8);  

    spi_write_cmd(0x3A);   //Pixel Format
    spi_write_byte(0x55);  //16-bit/pixel

    spi_write_cmd(0x20);  //Inversion off

    spi_write_cmd(0x26);  //Gamma Set
    spi_write_byte(0x01);

    spi_write_cmd(0xE0);  //Positive Gamma Correction
    spi_write_byte(0x0F);
    spi_write_byte(0x31);
    spi_write_byte(0x2B);
    spi_write_byte(0x0C);
    spi_write_byte(0x0E);
    spi_write_byte(0x08);
    spi_write_byte(0x4E);
    spi_write_byte(0xF1);
    spi_write_byte(0x37);
    spi_write_byte(0x07);
    spi_write_byte(0x10);
    spi_write_byte(0x03);
    spi_write_byte(0x0E);
    spi_write_byte(0x09);
    spi_write_byte(0x00);

    spi_write_cmd(0xE1);  //Negative Gamma Correction
    spi_write_byte(0x00);
    spi_write_byte(0x0E);
    spi_write_byte(0x14);
    spi_write_byte(0x03);
    spi_write_byte(0x11);
    spi_write_byte(0x07);
    spi_write_byte(0x31);
    spi_write_byte(0xC1);
    spi_write_byte(0x48);
    spi_write_byte(0x08);
    spi_write_byte(0x0F);
    spi_write_byte(0x0C);
    spi_write_byte(0x31);
    spi_write_byte(0x36);
    spi_write_byte(0x0F);

    spi_write_cmd(0x11);  //Sleep Out
    vTaskDelay(pdMS_TO_TICKS(120));

    //zapnuti dispeje, podsvetleni a vymazani obrazovky
    lcd_display_on();
    lcd_backlight_on();
    lcd_clear();
}

// Draw pixel
// x:X coordinate
// y:Y coordinate
// color:color
void lcd_draw_pixel(uint16_t x, uint16_t y, uint16_t color)
{
    if (x >= DISPLAY_WIDTH) return;
    if (y >= DISPLAY_HEIGHT) return;

    uint16_t _x = x + DISPLAY_OFFSETX;
    uint16_t _y = y + DISPLAY_OFFSETY;

    spi_write_cmd(0x2A);  // set column(x) address
    spi_write_word(_x);
    spi_write_word(_x);
    spi_write_cmd(0x2B);  // set Page(y) address
    spi_write_word(_y);
    spi_write_word(_y);
    spi_write_cmd(0x2C);  // Memory Write
    spi_write_word(color);
}

// Draw rectangle of filling
// x1:Start X coordinate
// y1:Start Y coordinate
// x2:End X coordinate
// y2:End Y coordinate
// color:color
void lcd_draw_fill_rect(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color)
{
    if (x1 >= DISPLAY_WIDTH) return;
    if (x2 >= DISPLAY_WIDTH) x2 = DISPLAY_WIDTH - 1;
    if (y1 >= DISPLAY_HEIGHT) return;
    if (y2 >= DISPLAY_HEIGHT) y2 = DISPLAY_HEIGHT - 1;

    uint16_t _x1 = x1 + DISPLAY_OFFSETX;
    uint16_t _x2 = x2 + DISPLAY_OFFSETX;
    uint16_t _y1 = y1 + DISPLAY_OFFSETY;
    uint16_t _y2 = y2 + DISPLAY_OFFSETY;

    spi_write_cmd(0x2A);  // set column(x) address
    spi_write_word(_x1);
    spi_write_word(_x2);
    spi_write_cmd(0x2B);  // set Page(y) address
    spi_write_word(_y1);
    spi_write_word(_y2);
    spi_write_cmd(0x2C);  // Memory Write
    for (int i = _x1; i <= _x2; i++) {
        uint16_t size = _y2 - _y1 + 1;
        spi_write_color(color, size);
    }
}

// Display OFF
void lcd_display_off()
{
    spi_write_cmd(0x28);
}

// Display ON
void lcd_display_on()
{
    spi_write_cmd(0x29);
}

// Fill screen
// color:color
void lcd_fill_screen(uint16_t color)
{
    lcd_draw_fill_rect(0, 0, DISPLAY_WIDTH - 1, DISPLAY_HEIGHT - 1, color);
}

// Draw line
// x1:Start X coordinate
// y1:Start Y coordinate
// x2:End X coordinate
// y2:End Y coordinate
// color:color
void lcd_draw_line(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color)
{
    int i;
    int dx, dy;
    int sx, sy;
    int E;

    /* distance between two points */
    dx = (x2 > x1) ? x2 - x1 : x1 - x2;
    dy = (y2 > y1) ? y2 - y1 : y1 - y2;

    /* direction of two point */
    sx = (x2 > x1) ? 1 : -1;
    sy = (y2 > y1) ? 1 : -1;

    /* inclination < 1 */
    if (dx > dy) {
        E = -dx;
        for (i = 0; i <= dx; i++) {
            lcd_draw_pixel(x1, y1, color);
            x1 += sx;
            E += 2 * dy;
            if (E >= 0) {
                y1 += sy;
                E -= 2 * dx;
            }
        }

        /* inclination >= 1 */
    }
    else {
        E = -dy;
        for (i = 0; i <= dy; i++) {
            lcd_draw_pixel(x1, y1, color);
            y1 += sy;
            E += 2 * dx;
            if (E >= 0) {
                x1 += sx;
                E -= 2 * dy;
            }
        }
    }
}

// Draw rectangle
// x1:Start X coordinate
// y1:Start Y coordinate
// x2:End X coordinate
// y2:End Y coordinate
// color:color
void lcd_draw_rect(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color)
{
    lcd_draw_line(x1, y1, x2, y1, color);
    lcd_draw_line(x2, y1, x2, y2, color);
    lcd_draw_line(x2, y2, x1, y2, color);
    lcd_draw_line(x1, y2, x1, y1, color);
}

// Draw rectangle with angle
// xc:Center X coordinate
// yc:Center Y coordinate
// w:Width of rectangle
// h:Height of rectangle
// angle :Angle of rectangle
// color :color
//When the origin is (0, 0), the point (x1, y1) after rotating the point (x, y) by the angle is obtained by the following calculation.
// x1 = x * cos(angle) - y * sin(angle)
// y1 = x * sin(angle) + y * cos(angle)
void lcd_draw_rot_rect(uint16_t xc, uint16_t yc, uint16_t w, uint16_t h, uint16_t angle, uint16_t color)
{
    double xd, yd, rd;
    int x1, y1;
    int x2, y2;
    int x3, y3;
    int x4, y4;
    rd = -angle * M_PI / 180.0;
    xd = 0.0 - w / 2;
    yd = h / 2;
    x1 = (int)(xd * cos(rd) - yd * sin(rd) + xc);
    y1 = (int)(xd * sin(rd) + yd * cos(rd) + yc);

    yd = 0.0 - yd;
    x2 = (int)(xd * cos(rd) - yd * sin(rd) + xc);
    y2 = (int)(xd * sin(rd) + yd * cos(rd) + yc);

    xd = w / 2;
    yd = h / 2;
    x3 = (int)(xd * cos(rd) - yd * sin(rd) + xc);
    y3 = (int)(xd * sin(rd) + yd * cos(rd) + yc);

    yd = 0.0 - yd;
    x4 = (int)(xd * cos(rd) - yd * sin(rd) + xc);
    y4 = (int)(xd * sin(rd) + yd * cos(rd) + yc);

    lcd_draw_line(x1, y1, x2, y2, color);
    lcd_draw_line(x1, y1, x3, y3, color);
    lcd_draw_line(x2, y2, x4, y4, color);
    lcd_draw_line(x3, y3, x4, y4, color);
}

// Draw circle
// x0:Central X coordinate
// y0:Central Y coordinate
// r:radius
// color:color
void lcd_draw_circle(uint16_t x0, uint16_t y0, uint16_t r, uint16_t color)
{
    int x;
    int y;
    int err;
    int old_err;

    x = 0;
    y = -r;
    err = 2 - 2 * r;
    do {
        lcd_draw_pixel(x0 - x, y0 + y, color);
        lcd_draw_pixel(x0 - y, y0 - x, color);
        lcd_draw_pixel(x0 + x, y0 - y, color);
        lcd_draw_pixel(x0 + y, y0 + x, color);
        if ((old_err = err) <= x) err += ++x * 2 + 1;
        if (old_err > y || err > x) err += ++y * 2 + 1;
    } while (y < 0);
}

//vykresleni jednoho znaku na pozici x a y
void lcd_draw_char(int16_t x, int16_t y, unsigned char c,
                            uint16_t color) {

    c -= FIRST_CHAR;
    font_glyph glyph = font_chars[c];
    uint16_t offset = glyph.bitmap_offset;
    uint8_t xx, yy, bits = 0, bit = 0;

    for (yy = 0; yy < glyph.height; yy++) {
      for (xx = 0; xx < glyph.width; xx++) {
        if (!(bit & 7)) {
          bits = font_bitmaps[offset];
          offset++;
        }
        if (bits & 0x80) {
            lcd_draw_pixel(x + glyph.x_offset + xx, y + glyph.y_offset + yy, color);
        }
        bit++;
        bits <<= 1;
      }
    }
}

//vypsani znaku na obrazovku na pozici kurzoru
// doWrap - po dosazeni okraje displeje prejde na novy radek
void lcd_write_char(uint8_t c, bool doWrap, uint16_t textcolor) {
    if (cursor_y > DISPLAY_HEIGHT) {
        lcd_clear();    
    }
    if (c == '\n') {
      cursor_x = 0;
      cursor_y += LINE_HEIGHT;
    } else if (c != '\r') {
      if ((c >= FIRST_CHAR) && (c <= LAST_CHAR)) {
        font_glyph glyph = font_chars[c - FIRST_CHAR];
        if ((glyph.width > 0) && (glyph.height > 0)) { 
          if (doWrap && (cursor_x + glyph.x_offset + glyph.width) > DISPLAY_WIDTH) {
            cursor_x = 0;
            cursor_y += LINE_HEIGHT;
          }
          lcd_draw_char(cursor_x, cursor_y, c, textcolor);
        }
        cursor_x += glyph.x_advance;
      }
    }
}

// Backlight OFF
void lcd_backlight_off()
{
    gpio_set_level(PIN_BL, 0);
}

// Backlight ON
void lcd_backlight_on()
{
    gpio_set_level(PIN_BL, 1);
}

//smazani displeje
void lcd_clear() {
    cursor_x = 0;
    cursor_y = 18;
    lcd_fill_screen(COLOR_BLACK);
}