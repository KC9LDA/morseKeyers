
// OledAscii.cpp

#include "OledAscii.h"

void OledAscii::clear(uint8_t c0, uint8_t c1, uint8_t r0, uint8_t r1) {
  m_skip = 0;
  // only rows on display will be cleared
  if (r1 >= displayRows()) r1 = displayRows() - 1;
  for (uint8_t r = r0; r <= r1; r++) {
    setCursor(c0, r);
    for (uint8_t c = c0; c <= c1; c++) {
      ssd1306WriteRamBuf(0);
    }
  }
  setCursor(c0, r0);
}

void OledAscii::clear() {
  clear(0, displayWidth() - 1, 0 , displayRows() - 1);
}

void OledAscii::clearToEOL() {
  clear(m_col, displayWidth() -1, m_row, m_row + fontRows() - 1);
}

void OledAscii::clearField(uint8_t col, uint8_t row, uint8_t n) {
  clear(col, col + fieldWidth(n) - 1, row, row + fontRows() - 1);
}

void OledAscii::displayRemap(bool mode) {
  ssd1306WriteCmd(mode ? SSD1306_SEGREMAP : SSD1306_SEGREMAP | 1);
  ssd1306WriteCmd(mode ? SSD1306_COMSCANINC : SSD1306_COMSCANDEC);
}

uint8_t OledAscii::displayHeight() const {
  return m_displayHeight;
}

uint8_t OledAscii::displayRows() const {
  return m_displayHeight/8;
}

uint8_t OledAscii::displayWidth() const {
  return m_displayWidth;
}

size_t OledAscii::fieldWidth(uint8_t n) {
  return n*(fontWidth() + 1);
}

uint8_t OledAscii::fontWidth() const {
  return m_font ? readFontByte(m_font + FONT_WIDTH) : 0;
}

uint8_t OledAscii::fontRows() const {
  return m_font ? ((readFontByte(m_font + FONT_HEIGHT) + 7)/8) : 0;
}

uint8_t OledAscii::row() const {
  return m_row;
}

uint8_t OledAscii::col() const {
  return m_col;
}

void OledAscii::home() {
  setCursor(0, 0);
}

void OledAscii::init(const DevType* dev) {
  m_col = 0;
  m_row = 0;
#ifdef __AVR__
  const uint8_t* table = (const uint8_t*)pgm_read_word(&dev->initcmds);
#else  // __AVR__
  const uint8_t* table = dev->initcmds;
#endif  // __AVR
  uint8_t size = readFontByte(&dev->initSize);
  m_displayWidth = readFontByte(&dev->lcdWidth);
  m_displayHeight = readFontByte(&dev->lcdHeight);
  m_colOffset = readFontByte(&dev->colOffset);
  for (uint8_t i = 0; i < size; i++) {
    ssd1306WriteCmd(readFontByte(table + i));
  }
  clear();
}

void OledAscii::setCol(uint8_t col) {
  if (col < m_displayWidth) {
    m_col = col;
    col += m_colOffset;
    ssd1306WriteCmd(SSD1306_SETLOWCOLUMN  | (col & 0xf));
    ssd1306WriteCmd(SSD1306_SETHIGHCOLUMN | (col >> 4));
  }
}

void OledAscii::setContrast(uint8_t value) {
  ssd1306WriteCmd(SSD1306_SETCONTRAST);
  ssd1306WriteCmd(value);
}

void OledAscii::setCursor(uint8_t col, uint8_t row) {
  setCol(col);
  setRow(row);
}

void OledAscii::setFont(const uint8_t* font) {
  m_font = font;
}

void OledAscii::setRow(uint8_t row) {
  if (row < displayRows()) {
    m_row = row;
    ssd1306WriteCmd(SSD1306_SETSTARTPAGE | m_row);
  }
}

void OledAscii::ssd1306WriteCmd(uint8_t c) {
  writeDisplay(c^m_invertMask, SSD1306_MODE_CMD);
}

void OledAscii::ssd1306WriteRam(uint8_t c) {
  if (m_col < m_displayWidth) {
    writeDisplay(c^m_invertMask, SSD1306_MODE_RAM);
    m_col++;
  }
}

void OledAscii::ssd1306WriteRamBuf(uint8_t c) {
  if (m_skip) {
    m_skip--;
  } else if (m_col < m_displayWidth) {
    writeDisplay(c^m_invertMask, SSD1306_MODE_RAM_BUF);
    m_col++;
  }
}

void OledAscii::skipColumns(uint8_t n) {
  m_skip = n;
}

size_t OledAscii::write(uint8_t ch) {
  if (!m_font) {
    return 0;
  }
  uint8_t w = readFontByte(m_font + FONT_WIDTH);
  uint8_t h = readFontByte(m_font + FONT_HEIGHT);
  uint8_t nr = (h + 7)/8;
  uint8_t first = readFontByte(m_font + FONT_FIRST_CHAR);
  uint8_t count = readFontByte(m_font + FONT_CHAR_COUNT);
  const uint8_t* base = m_font + FONT_WIDTH_TABLE;

  if (ch < first || ch >= (first + count)) {
    if (ch == '\r') {
      setCol(0);
      return 1;
    }
    if (ch == '\n') {
      setCol(0);
      uint8_t fr = nr;
      setRow(m_row + fr);
      return 1;
    }
    return 0;
  }
  ch -= first;
  uint8_t s = 1;
  uint8_t thieleShift = 0;
  base += nr*w*ch;  // fixed width font
  uint8_t scol = m_col;
  uint8_t srow = m_row;
  uint8_t skip = m_skip;
  for (uint8_t r = 0; r < nr; r++) {
    skipColumns(skip);
    if (r) setCursor(scol, m_row + 1);
    for (uint8_t c = 0; c < w; c++) {
      uint8_t b = readFontByte(base + c + r*w);
      if (thieleShift && (r + 1) == nr) {
        b >>= thieleShift;
      }
      ssd1306WriteRamBuf(b);
    }
    for (uint8_t i = 0; i < s; i++) {
      ssd1306WriteRamBuf(0);
    }
  }
  setRow(srow);
  return 1;
}

