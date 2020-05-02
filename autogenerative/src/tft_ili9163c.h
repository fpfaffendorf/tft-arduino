#ifndef _TFT_ILI9163CLIB_H_
#define _TFT_ILI9163CLIB_H_

#include "Arduino.h"
#include "Print.h"
#include <Adafruit_GFX.h>

#define _TFTWIDTH       128     //the REAL W resolution of the TFT
#define _TFTHEIGHT      128     //the REAL H resolution of the TFT
#define _GRAMWIDTH      128
#define _GRAMHEIGH      160
#define _GRAMSIZE       _GRAMWIDTH * _GRAMHEIGH //*see note 1
#define __OFFSET        0   // 32 *see note 2

const uint8_t pGammaSet[15]= {0x36,0x29,0x12,0x22,0x1C,0x15,0x42,0xB7,0x2F,0x13,0x12,0x0A,0x11,0x0B,0x06};
const uint8_t nGammaSet[15]= {0x09,0x16,0x2D,0x0D,0x13,0x15,0x40,0x48,0x53,0x0C,0x1D,0x25,0x2E,0x34,0x39};

#define CMD_NOP     	0x00  //Non operation
#define CMD_SWRESET 	0x01  //Soft Reset
#define CMD_SLPIN   	0x10  //Sleep ON
#define CMD_SLPOUT  	0x11  //Sleep OFF
#define CMD_PTLON   	0x12  //Partial Mode ON
#define CMD_NORML   	0x13  //Normal Display ON
#define CMD_DINVOF  	0x20  //Display Inversion OFF
#define CMD_DINVON   	0x21  //Display Inversion ON
#define CMD_GAMMASET 	0x26  //Gamma Set (0x01[1],0x02[2],0x04[3],0x08[4])
#define CMD_DISPOFF 	0x28  //Display OFF
#define CMD_DISPON  	0x29  //Display ON
#define CMD_IDLEON  	0x39  //Idle Mode ON
#define CMD_IDLEOF  	0x38  //Idle Mode OFF
#define CMD_CLMADRS   	0x2A  //Column Address Set
#define CMD_PGEADRS   	0x2B  //Page Address Set

#define CMD_RAMWR   	0x2C  //Memory Write
#define CMD_RAMRD   	0x2E  //Memory Read
#define CMD_CLRSPACE   	0x2D  //Color Space : 4K/65K/262K
#define CMD_PARTAREA	0x30  //Partial Area
#define CMD_VSCLLDEF	0x33  //Vertical Scroll Definition
#define CMD_TEFXLON		0x35  //Tearing Effect Line ON
#define CMD_TEFXLOF		0x34  //Tearing Effect Line OFF
#define CMD_MADCTL  	0x36  //Memory Access Control
#define CMD_VSSTADRS	0x37  //Vertical Scrolling Start address
#define CMD_PIXFMT  	0x3A  //Interface Pixel Format
#define CMD_FRMCTR1 	0xB1  //Frame Rate Control (In normal mode/Full colors)
#define CMD_FRMCTR2 	0xB2  //Frame Rate Control(In Idle mode/8-colors)
#define CMD_FRMCTR3 	0xB3  //Frame Rate Control(In Partial mode/full colors)
#define CMD_DINVCTR		0xB4  //Display Inversion Control
#define CMD_RGBBLK		0xB5  //RGB Interface Blanking Porch setting
#define CMD_DFUNCTR 	0xB6  //Display Fuction set 5
#define CMD_SDRVDIR 	0xB7  //Source Driver Direction Control
#define CMD_GDRVDIR 	0xB8  //Gate Driver Direction Control 

#define CMD_PWCTR1  	0xC0  //Power_Control1
#define CMD_PWCTR2  	0xC1  //Power_Control2
#define CMD_PWCTR3  	0xC2  //Power_Control3
#define CMD_PWCTR4  	0xC3  //Power_Control4
#define CMD_PWCTR5  	0xC4  //Power_Control5
#define CMD_VCOMCTR1  	0xC5  //VCOM_Control 1
#define CMD_VCOMCTR2  	0xC6  //VCOM_Control 2
#define CMD_VCOMOFFS  	0xC7  //VCOM Offset Control
#define CMD_PGAMMAC		0xE0  //Positive Gamma Correction Setting
#define CMD_NGAMMAC		0xE1  //Negative Gamma Correction Setting
#define CMD_GAMRSEL		0xF2  //GAM_R_SEL

class TFT_ILI9163C : public Adafruit_GFX {

	public:

		TFT_ILI9163C(uint8_t cspin,uint8_t dcpin,uint8_t rstpin=255);
		
		void     	begin(void),
					setAddrWindow(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1),//graphic Addressing
					setCursor(int16_t x,int16_t y),//char addressing
					pushColor(uint16_t color),
					fillScreen(uint16_t color=0x0000),
					clearScreen(uint16_t color=0x0000),//same as fillScreen
					drawPixel(int16_t x, int16_t y, uint16_t color),
					drawFastVLine(int16_t x, int16_t y, int16_t h, uint16_t color),
					drawFastHLine(int16_t x, int16_t y, int16_t w, uint16_t color),
					fillRect(int16_t x, int16_t y, int16_t w, int16_t h,uint16_t color),
					setRotation(uint8_t r),
					invertDisplay(boolean i);
		uint8_t 	errorCode(void);			
		void		idleMode(boolean onOff);
		void		display(boolean onOff);	
		void		sleepMode(boolean mode);
		void 		defineScrollArea(uint16_t tfa, uint16_t bfa);
		void		scroll(uint16_t adrs);
		void 		startPushData(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1);
		void 		pushData(uint16_t color);
		void 		endPushData();
		void		writeScreen24(const uint32_t *bitmap,uint16_t size=_TFTWIDTH*_TFTHEIGHT);
		inline uint16_t Color565(uint8_t r, uint8_t g, uint8_t b) {return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);};
		inline uint16_t Color24To565(int32_t color_) { return ((((color_ >> 16) & 0xFF) / 8) << 11) | ((((color_ >> 8) & 0xFF) / 4) << 5) | (((color_) &  0xFF) / 8);}

 	protected:

		volatile uint8_t		_Mactrl_Data;//container for the memory access control data
		uint8_t					_colorspaceData;
		void				spiwrite(uint8_t);
		volatile uint8_t 	*dataport, *clkport, *csport, *rsport;
		uint8_t 			_cs,_rs,_rst;
		uint8_t  			datapinmask, clkpinmask, cspinmask, rspinmask;
		void		writecommand(uint8_t c);
		void		writedata(uint8_t d);
		void		writedata16(uint16_t d);

	private:

		void 		colorSpace(uint8_t cspace);
		void 		setAddr(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1);
		uint8_t		sleep;
		void 		chipInit();
		bool 		boundaryCheck(int16_t x,int16_t y);
		void 		homeAddress();
		uint8_t		_initError;

};

#endif