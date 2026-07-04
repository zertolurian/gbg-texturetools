/* -----------------------------------------------------------------------------------------   
* AGTD2: Automatic GBG Texture Drawing 2
* v1.0
* AGTD2 by Zert
* AGTD originally for Arduino Leonardo by Borri
* AGTD original idea by Scrubz
*
* AGTD2 is an improved verion of AGTD with faster drawing speed, a more efficient texture
* encoding algorithm to allow more textures at a time, as well as the capability to include
* other settings. The old CSV format and SD Card functionality are currently not supported.
* 
* No external library dependencies are required.
* Disclaimer: AGTD2 was partially improved using AI
* Download the latest version and other tools here: https://github.com/zertolurian/gbg-texturetools
* ------------------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------------------
*                                          CONFIG
* ------------------------------------------------------------------------------------------
* NO_BUTTON
* If don't have a button, or a wire to simulate it, you can un-comment both lines below to make
* the script run automatically. NO_BUTTON_DELAY sets delay (in seconds) before program start.
* ------------------------------------------------------------------------------------------*/
//  #define NO_BUTTON
//  #define NO_BUTTON_DELAY   30

/*------------------------------------------------------------------------------------------
  If button/Wire is present, define the Arduino input board PIN
  By default, pins 8, 9, 10, 11, 12, and 13 are enabled.
  ------------------------------------------------------------------------------------------*/
static const byte buttonPins[] = {8, 9, 10, 11, 12, 13};

/*------------------------------------------------------------------------------------------
  SLOW MODE - For projects with a lot of nodon a delay has to be added to avoid lag-induced
  misclicks on the GBG Programming Screen. Recommended values are:

  Recommended option for new GBG projects (Programming screen with minimal lag):
     const int delay_mouse = 10;  (for Switch 1 users)
     const int delay_mouse = 0;   (for Switch 2 users)

  Worst-case option for the laggiest GBG projects (Programming screen with maximum lag):
     const int delay_mouse = 40;  (for Switch 1 users)
     const int delay_mouse = 10;  (for Switch 2 users)

  The optimal "delay_mouse" value is usually somewhere in between, depending on your project.
* ------------------------------------------------------------------------------------------*/
const int delay_mouse = 20;

/*------------------------------------------------------------------------------------------*/
const PROGMEM byte image[] = {
/* -----------------------------------------------------------------------------------------
*                             DATA IMAGE SPACE - START
*                  - PASTE THE GBG_Texture_Builder OUTPUT HERE -
*           (Remember to separate each individual texture with a comma!)
* ------------------------------------------------------------------------------------------*/
//PASTE YOUR AGTD2 CSV HERE
//PASTE YOUR AGTD2 CSV HERE
//PASTE YOUR AGTD2 CSV HERE
//PASTE YOUR AGTD2 CSV HERE
//PASTE YOUR AGTD2 CSV HERE
//PASTE YOUR AGTD2 CSV HERE
//PASTE YOUR AGTD2 CSV HERE
//PASTE YOUR AGTD2 CSV HERE
//PASTE YOUR AGTD2 CSV HERE
//PASTE YOUR AGTD2 CSV HERE
/* -----------------------------------------------------------------------------------------
*                              DATA IMAGE SPACE - END
* ------------------------------------------------------------------------------------------*/
}; 
/* -----------------------------------------------------------------------------------------
*                                   ENCODING INFO
* ------------------------------------------------------------------------------------------
// FORMAT of image[] in AGTD2:
//   Each texture is self-delimiting (its length follows from its own palSize/
//   flags), so the sketch walks them until sizeof(image) is reached.
//   Per texture:
//     [+0] bg       background color (most-used color; 47 = transparent)
//     [+1] palSize  number of non-bg distinct colors (0..117)
//     [+2..]        each non-bg distinct color in palette (1 byte per color)
//     [..]          packed pixels, (4096 * bpp / 8) bytes, MSB-first
//                    (omitted entirely when palSize == 0 -- see below)
//     [flags]       1 byte: bit0 faces, bit1 size, bit2 rotation,
//                            bit3 posX, bit4 posY, bit5 posZ
//     [settings..]  present fields only, in flag-bit order:
//                     faces : 2 bytes LE = faceX | faceY<<3 | faceZ<<6
//                     size  : sizeX(4B LE signed) sizeY(4B LE signed)   long(value*1e7)
//                     rot   : 4B LE signed                              long(value*1e7)
//                     posX/posY/posZ : 4B LE signed each                long(value*1e7)
//
//  bg does NOT get its own palette entry since the value is already known from
// the bg byte, and it is no longer processed during the drawing loop.
//
//  bpp is the number of bits per pixel used in the encoding. It can be derived
// from ceil(log2(palSize+1)).
//
//  The real GBG palette has only 118 valid colors (135 slots minus 17 unusable
// gaps), so palSize+1 never exceeds 118 and bpp never exceeds 7.
//
//  palSize == 0 means the texture is 100% background with no other colors at all;
// NO packed bytes follow in that case (the texture costs just 2 header bytes).
//
//  The order that the palette colors are listed will be the same order that they
// will be processed. Optimally, they should be arranged from most abundant to
// least abundant to maximize mask efficiency.
//
// Number of bytes per texture:
// | Colors | palSize | bpp | Total bytes | Textures per run |
// |   1    |    0    |  0  |     3-29    |       512        |
// |   2    |    1    |  1  |   516-542   |        39        |
// |  3-4   |   2-3   |  2  |  1029-1056  |        19        |
// |  5-8   |   4-7   |  3  |  1543-1572  |        13        |
// |  9-16  |   8-15  |  4  |  2059-2092  |         9        |
// | 17-32  |  16-31  |  5  |  2579-2620  |         7        |
// | 33-64  |  32-63  |  6  |  3107-3164  |         6        |
// | 65-118 |  64-117 |  7  |  3641-3720  |         5        |
* ------------------------------------------------------------------------------------------*/

//Masking values for flags
#define F_FACES 0x01
#define F_SIZE  0x02
#define F_ROT   0x04
#define F_POSX  0x08
#define F_POSY  0x10
#define F_POSZ  0x20

//For ArduiNodon functions
#define DEC_Q 10000000L
#define POS_OFF 2147483647L

// Decode state for the texture currently being drawn (cached once per texture).
unsigned int tex_offset;      // start of current texture inside image[]
byte cur_bg;                  // background color
byte cur_bpp;                 // 0 (solid bg, no packed data) or 1-7
byte cur_palSize;             // non-bg palette length (0 = solid background)
unsigned int cur_palBase;     // start of palette inside image[] (code -> real color index)
unsigned int cur_packedBase;  // start of packed pixel data inside image[]
unsigned int cur_texLen;      // full byte length of this texture (incl. settings)
byte cur_flags;               // which optional settings are present
unsigned int cur_faces;       // 9-bit XOR'd face word (0 = all GBG defaults, no clicks needed)
byte cur_bpmask;              // (1 << cur_bpp) - 1  -- bitmask for one pixel code
byte cur_shift;               // 16 - cur_bpp        -- base window-shift before bitOff
long cur_sizeX, cur_sizeY, cur_rot, cur_posX, cur_posY, cur_posZ;   // fixed-point *1e7

// Calculate number of bits required to represent each used color (including bg)
// Only ever called with palSize >= 1.
byte bppFromTotal(unsigned int total) {
  byte b = 1;
  while (((unsigned int)1 << b) < total) b++;
  return b;
}

// Read a signed 32-bit little-endian value from PROGMEM at byte offset off.
long readLong(unsigned int off) {
  return (long)pgm_read_dword(image + off);
}

// Parse a texture (header + palette + settings) at byte offset off into the cache.
__attribute__((noinline))
void loadTexture(unsigned int off) {
  cur_bg      = pgm_read_byte_near(image + off);
  cur_palSize = pgm_read_byte_near(image + off + 1);
  cur_palBase    = off + 2;                        // palette is read straight from PROGMEM
  cur_packedBase = off + 2 + cur_palSize;

  //calculate bpp and number of packed image bytes
  unsigned int packed;
  if (cur_palSize == 0) {
    cur_bpp = 0;     // solid background -- nothing else is ever drawn from this texture
    packed = 0;
  } else {
    cur_bpp = bppFromTotal((unsigned int)cur_palSize + 1);
    packed = ((unsigned long)4096 * cur_bpp + 7) >> 3;
    cur_shift  = 16 - cur_bpp;
    cur_bpmask = (1 << cur_bpp) - 1;
  }
  unsigned int o = cur_packedBase + packed;

  // defaults (GBG defaults: + and - faces on; size/rotation 0; position axes use
  // POS_OFF = the objectTriple skip sentinel, i.e. "leave this axis at its GBG value")
  cur_faces = 0;
  cur_sizeX = cur_sizeY = cur_rot = 0;
  cur_posX = cur_posY = cur_posZ = POS_OFF;

  //Extra settings
  cur_flags = pgm_read_byte_near(image + o); o += 1;
  if (cur_flags & F_FACES) {
    // XOR raw 9-bit word with default 0b011011011: result bit=1 means "click this toggle".
    // Layout: bits 0-2 = faceX, 3-5 = faceY, 6-8 = faceZ (3 bits per axis).
    cur_faces = (pgm_read_word(image + o) ^ 0b011011011U) & 0x1FFU;
    o += 2;
  }
  if (cur_flags & F_SIZE) { cur_sizeX = readLong(o); o += 4; cur_sizeY = readLong(o); o += 4; }
  if (cur_flags & F_ROT)  { cur_rot  = readLong(o); o += 4; }
  if (cur_flags & F_POSX) { cur_posX = readLong(o); o += 4; }
  if (cur_flags & F_POSY) { cur_posY = readLong(o); o += 4; }
  if (cur_flags & F_POSZ) { cur_posZ = readLong(o); o += 4; }

  cur_texLen = o - off;        // total length, used to advance to the next texture
}

// Real color index of pixel `index` (1-based, 1-4096) of the cached texture.
byte texPixel(unsigned int index) {
  if (cur_palSize == 0) return cur_bg;              // solid background, no packed data exists
  unsigned int p = index - 1;                       // 0-based position
  unsigned int bitpos = p * cur_bpp;
  unsigned int byteIdx = bitpos >> 3;
  byte bitOff = bitpos & 7;
  // a pixel can straddle two bytes (widths 3/5/6/7), so read a 2-byte window
  unsigned int w = ((unsigned int)pgm_read_byte_near(image + cur_packedBase + byteIdx) << 8)
                 |  (unsigned int)pgm_read_byte_near(image + cur_packedBase + byteIdx + 1);
  byte code = (w >> (cur_shift - bitOff)) & cur_bpmask;
  if (code == cur_palSize) return cur_bg;           // implicit bg code (one past the palette)
  return pgm_read_byte_near(image + cur_palBase + code);
}

// ArduiNodon functions
const int numpadCoords[13][2] PROGMEM = {
  {560,550},  // 0
  {480,470},  // 1
  {560,470},  // 2
  {640,470},  // 3
  {480,400},  // 4
  {560,400},  // 5
  {640,400},  // 6
  {480,320},  // 7
  {560,320},  // 8
  {640,320},  // 9
  {480,550},  // -
  {640,550},  // .
  {760,550}   // OK
};

void inputNumber(byte i) {
  int x = pgm_read_word(&numpadCoords[i][0]);
  int y = pgm_read_word(&numpadCoords[i][1]);
  clickMouseAt(x, y);
}

// Writes exactly `width` decimal digits of `v` into buf (forward order, most
// significant first), left-padded with zeros if v has fewer digits than width.
void digitsToBuf(unsigned long v, byte *buf, byte width) {
  for (byte i = width; i > 0; i--) { buf[i-1] = v % 10; v /= 10; }
}

void inputCalculator(int offsetX, int offsetY, long n) {
  bool neg = (n < 0);
  unsigned long mag = neg ? (unsigned long)(-n) : (unsigned long)n;
  unsigned long nInt = mag / DEC_Q;        // integer part
  unsigned long nDec = mag % DEC_Q;        // fractional part, 0-9999999

  clickMouseAt(offsetX, offsetY);           //Open menu
  if (neg) inputNumber(10);                //Negative sign

  //Integer part: zero-padded to 10 digits, then skip leading zeros (always
  //keeping at least the last one, so 0 still types as a single '0').
  byte ibuf[10];                           // up to 10 digits for an unsigned long
  digitsToBuf(nInt, ibuf, 10);
  byte i = 0;
  while (i < 9 && ibuf[i] == 0) i++;
  while (i < 10) inputNumber(ibuf[i++]);

  //Decimal part (strip trailing zeros, keep leading zeros)
  if (nDec) {
    inputNumber(11);                       // '.'
    byte dbuf[7];                          // 7 fractional digits (matches DEC_Q = 1e7)
    digitsToBuf(nDec, dbuf, 7);
    byte dlen = 7;
    while (dlen > 0 && dbuf[dlen-1] == 0) dlen--;
    for (byte j = 0; j < dlen; j++) inputNumber(dbuf[j]);
  }
  inputNumber(12);  //OK
}

__attribute__((noinline))
void closeNodon() {
  clickMouseAt(1240, 40);
}

__attribute__((noinline))
void zoomGrid() {
  clickMouseAt(950,695);
}

void objectList(int offsetX, int offsetY, unsigned int faces9) {
  clickMouseAt(offsetX, offsetY);  //Open menu
  int x = 170;
  for (byte col = 0; col < 3; col++, x += 208) {
    int y = 140;
    for (byte row = 0; row < 3; row++, y += 64, faces9 >>= 1)
      if (faces9 & 1) clickMouseAt(x, y);
  }
  closeNodon();    //Close menu
}

void objectTriple(int offsetX, int offsetY, long inTop, long inMid, long inBot, long inDefault=0) {
  if (inTop!=inDefault) inputCalculator(offsetX,offsetY,inTop);
  delay(40+delay_mouse);
  if (inMid!=inDefault) inputCalculator(offsetX,offsetY+60,inMid);
  delay(40+delay_mouse);
  if (inBot!=inDefault) inputCalculator(offsetX,offsetY+120,inBot);
  delay(40+delay_mouse);
}

#include <Mouse.h>
// local MouseTo recreation
int mt_posX, mt_posY, mt_targetX, mt_targetY;
bool mt_homed;       // false = currently homing to the corner; true = pursuing mt_target

void mt_home() { mt_homed = false; }              // call once in setup(), then loop mt_move() to completion
void mt_setTarget(int x, int y) { mt_targetX = x; mt_targetY = y; mt_homed = true; }
bool mt_move() {
  int toX, toY;
  if (mt_homed == false) { toX = -1280 - 200; toY = -720 - 200; }   // home to top-left corner
  else { toX = mt_targetX; toY = mt_targetY; }
  if (mt_posX != toX || mt_posY != toY) {
    int dx = toX - mt_posX;
    int dy = toY - mt_posY;
    int moveX = dx < -127 ? -127 : (dx > 127 ? 127 : dx);
    int moveY = dy < -127 ? -127 : (dy > 127 ? 127 : dy);
    Mouse.move(moveX, moveY, 0);
    mt_posX += moveX; mt_posY += moveY;
  } else {
    if (mt_homed == false) { mt_homed = true; mt_posX = 0; mt_posY = 0; return false; }
    mt_homed = false;
    delay(15+delay_mouse);
    return true;
  }
  delay(10);//
  return false;
}

bool anyPressed() {
  for (byte i = 0; i < sizeof(buttonPins); i++)
    if (digitalRead(buttonPins[i]) == LOW) return true;
  return false;
}

byte s_mask[513];
byte actual_pixel;

void setup(void) {

  Mouse.begin();
  
  pinMode(LED_BUILTIN, OUTPUT);
  for (byte i = 0; i < sizeof(buttonPins); i++) {
    pinMode(buttonPins[i], INPUT_PULLUP);
  }

  #if defined(NO_BUTTON)
  delay(NO_BUTTON_DELAY*1000);
  #else
  while (!anyPressed()) delay(100);
  #endif  

  // First move to get real mouse position
  mt_home();
  while (mt_move() == false) {}

  moveMouseTo(-1280 - 200, -720 - 200);
  mt_posX = 0;
  mt_posY = 0;

  for (byte i=0; i<28; i++) { //zoom at least 16 times
    zoomGrid();  //zoom button has big cooldown so double to be sure
  }

}


void loop() {

  tex_offset = 0;
  for (int n=0; tex_offset < sizeof(image); n++) {
  
    createTextureNodon();
  
    // Get first pixel data from image -> Represent the most used color on the image and it is used as Background color.
    loadTexture(tex_offset);
    actual_pixel = cur_bg;
    
    // --------------------------------------------------------------------------------------------------------------------
    
    if (actual_pixel!=47) {       //New textures have a transparent background already    
      selectColor(actual_pixel);
      clickMouseAt(960, 270);                                   //Mouse click on "Bucket Tool"
      clickMouseAt(810, 230);                                   //Just giving a close coord over the canvas to click-fill the background
      clickMouseAt(990, 195);                                   //Mouse click on "Pencil Tool"
    }
    if (n==0) {                                                 //Only click on pixel brush first time if multiple textures
      clickMouseAt(940, 375);                                   //Mouse click on "Pixel Brush"
    }

    // -------------------------------------------------------------------------------------------------------------------
    //initialize array for each texture iterarion
	  mask_reset();
    for (int index=1; index<4097; index++) {
      if (texPixel(index)==actual_pixel) {     
        mask_set(index);                                     // 'true' values if it's the background color
		  } 
    }
    
     // -------------------------------------------------------------------------------------------------------------------

    for (byte code = 0; code < cur_palSize; code++) {
      byte pixel_color = pgm_read_byte_near(image + cur_palBase + code);

      selectColor(pixel_color);

      for (int k=1; k<4097; k++) {
        if (texPixel(k) == pixel_color && mask_get(k) == false) {   // unpainted pixel of this color
          byte hcnt, vcnt;
          byte hreach = measureRun(k, pixel_color, &hcnt, true);     // horizontal extent
          byte vreach = measureRun(k, pixel_color, &vcnt, false);    // vertical extent
          byte step  = (vcnt > hcnt) ? 64 : 1;                       // busier direction wins (ties -> horizontal)
          byte reach = (vcnt > hcnt) ? vreach : hreach;
          drawRun(k, step, reach, pixel_color);
        }
      }
    }
    
    // ArduiNodon settings
    if (cur_flags & F_FACES && cur_faces) {  //Texture Face
      objectList(150, 150, cur_faces);
    }
    if (cur_flags & F_SIZE) {
      objectTriple(150, 250, cur_sizeX, cur_sizeY, 0);  //Size (X, Y)
    }
    if (cur_flags & F_ROT) {
      if (cur_rot) inputCalculator(150, 610, cur_rot);  //Rotation
    }
    if (cur_flags & (F_POSX | F_POSY | F_POSZ)) {     //Position (X, Y, Z)
      objectTriple(150, 400, cur_posX, cur_posY, cur_posZ, POS_OFF);  //POS_OFF = axis left at its GBG default
    }

    closeNodon();      //Mouse click on "X" to exit Nodon

    tex_offset += cur_texLen;     //Move to next variable-size texture
  }

  while(1) {
    //digitalWrite(LED_BUILTIN, HIGH);
    //delay(500);
    //digitalWrite(LED_BUILTIN, LOW);
    //delay(500);
  }
  
}


// Aux functions
// ----------------------------------------------------------------------------------------------------

void clickMouse() {
  Mouse.press(MOUSE_LEFT);
  delay(25+delay_mouse);//
  Mouse.release(MOUSE_LEFT);
}

void moveMouseTo(int x, int y) {
  mt_setTarget(x, y);
  while (mt_move() == false) {}
}

void clickMouseAt(int x, int y) {
  if (mt_targetX==x && mt_targetY==y) {
    delay(25+delay_mouse);//
  }else moveMouseTo(x, y);
  clickMouse();  
}

void createTextureNodon() {
  for (byte i=0; i<4; i++) { //zoom at least 2 times
    zoomGrid();  //zoom button has long cooldown so double zoom to be sure
  }
  clickMouseAt(360, 695);      //Objects
  clickMouseAt(360, 330);      //Special Objects
  clickMouseAt(490, 570);      //Texture
  delay(300+delay_mouse);      //A small delay to be sure the nodon is being created due possible lag on the code screen
  clickMouseAt(550, 570);      //Edit Texture Gear icon    
}

void selectColor(byte color) {
  clickMouseAt(1185, 545);        //Mouse click to open "Palette gear tool/Colour Palette"
  byte column = color % 15;
  byte row = color / 15;
  int pos_x = 77+(column*54);
  int pos_y = 169+(row*54);
  clickMouseAt(pos_x, pos_y);      //Move mouse to color coords. and click on it
  closeNodon();                  //Mouse click on "X" to exit palette
}

// Scrubz bitwise functions
void mask_reset() {
    for (int i = 0; i < 513; i++) s_mask[i] = 0;
}

void mask_set(unsigned int index) {
    s_mask[index / 8] |= 1 << index % 8;
}

__attribute__((noinline))
bool mask_get(unsigned int index) {
    return !!(1 << index % 8 & s_mask[index / 8]);
}

// 2D mask functions
__attribute__((noinline))
byte measureRun(int start, byte color, byte *cnt, bool horizontal) {
  byte reach = 0, c = 0;
  int k = start;
  while (true) {
    if (horizontal) { if ((k - 1) / 64 != k / 64) break; }   // next pixel would leave the row
    else            { if (k + 64 > 4096) break; }            // next pixel would leave the grid
    int nxt = horizontal ? k + 1 : k + 64;
    byte nd = texPixel(nxt);
    if (nd == color) {
      reach = horizontal ? (nxt - start) : ((nxt - start) >> 6);
      c++; k = nxt;
    }
    else if (mask_get(nxt) == false) { k = nxt; }            // bridge over a not-yet-drawn pixel
    else break;                                               // finished pixel of another color
  }
  *cnt = c;
  return reach;
}

// Move the mouse to the on-canvas screen position of pixel `idx` (1-based).
void moveToPixel(unsigned int idx) {
  byte pos_x = (idx - 1) % 64, pos_y = (idx - 1) / 64;  // locals -- see CHANGE 3a
  moveMouseTo(340 + (pos_x * 8), 158 + (pos_y * 8));
}

// Press at `start`, drag `reach` steps along `step` to the last same-color pixel,
// marking every same-color pixel on the path as done, then release. Bridged
// foreign pixels are left unmarked so they get repainted with their own color.
void drawRun(int start, byte step, byte reach, byte color) {
  moveToPixel(start);
  Mouse.press(MOUSE_LEFT);
  delay(40+delay_mouse);//
  mask_set(start);
  int end = start + (int)step * reach;
  for (int idx = start + step; idx <= end; idx += step) {
    if (texPixel(idx) == color) mask_set(idx);
  }
  if (reach > 0) { moveToPixel(end); }
  else { delay(delay_mouse*2/3); }
  Mouse.release(MOUSE_LEFT);
}

// ----------------------------------------------------------------------------------------------------
