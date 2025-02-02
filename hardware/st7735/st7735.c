/* vim: set ai et ts=4 sw=4: */
#include "st7735.h"
#include "time.h"
#include <stdio.h>
#include <string.h>
#include <sys/sysinfo.h>
#include <sys/vfs.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <net/if.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <fcntl.h>
#include "rpiInfo.h"

int i2cd;

/*
* Set display coordinates
*/
void lcd_set_address_window(uint8_t x0, uint8_t y0, uint8_t x1,uint8_t y1) 
{
  // col address set
  i2c_write_command(X_COORDINATE_REG,x0 + ST7735_XSTART,x1 + ST7735_XSTART);
  // row address set
  i2c_write_command(Y_COORDINATE_REG,y0 + ST7735_YSTART,y1 + ST7735_YSTART);
  // write to RAM
  i2c_write_command(CHAR_DATA_REG,0x00,0x00);

  i2c_write_command(SYNC_REG,0x00,0x01);

}


/*
* Display a single character
*/
void lcd_write_char(uint16_t x, uint16_t y, char ch, FontDef font,uint16_t color, uint16_t bgcolor) 
{
  uint32_t i, b, j;

  lcd_set_address_window(x, y, x + font.width - 1, y + font.height - 1);

  for (i = 0; i < font.height; i++) 
  {
    b = font.data[(ch - 32) * font.height + i];
    for (j = 0; j < font.width; j++) 
    {
      if ((b << j) & 0x8000) 
      {
        i2c_write_data(color >> 8, color & 0xFF);
      } 
      else 
      {
        i2c_write_data(bgcolor >> 8, bgcolor & 0xFF);
      }
    }
  }
}


/*
* display string
*/
void lcd_write_string(uint16_t x, uint16_t y,  char *str, FontDef font, uint16_t color, uint16_t bgcolor) 
{

  while (*str) 
  {
    if (x + font.width >= ST7735_WIDTH) 
    {
      x = 0;
      y += font.height;
      if (y + font.height >= ST7735_HEIGHT) 
      {
        break;
      }

      if (*str == ' ') 
      {
        // skip spaces in the beginning of the new line
        str++;
        continue;
      }
    }

    lcd_write_char(x, y, *str, font, color, bgcolor);
    i2c_write_command(SYNC_REG,0x00,0x01);
    x += font.width;
    str++;
  }

}

/*
* fill rectangle
*/
void lcd_fill_rectangle(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color) 
{
  uint8_t buff[320] = { 0 };
  uint16_t count = 0;
  // clipping
  if ((x >= ST7735_WIDTH) || (y >= ST7735_HEIGHT))
    return;
  if ((x + w - 1) >= ST7735_WIDTH)
    w = ST7735_WIDTH - x;
  if ((y + h - 1) >= ST7735_HEIGHT)
    h = ST7735_HEIGHT - y;
  lcd_set_address_window(x, y, x + w - 1, y + h - 1);
  
  for (count = 0; count < w; count++)
  {
    buff[count*2]   = color >> 8;
    buff[count*2+1] = color & 0xFF;
  }
  for (y = h; y > 0; y--) 
  {
      i2c_burst_transfer(buff, sizeof(uint16_t) * w);
  }
}

/*
* fill screen
*/

void lcd_fill_screen(uint16_t color) 
{
  lcd_fill_rectangle(0, 0, ST7735_WIDTH, ST7735_HEIGHT, color);
  i2c_write_command(SYNC_REG,0x00,0x01);
}

void lcd_draw_image(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint8_t *data) 
{
  uint16_t col = h - y;
  uint16_t row =  w - x;
  lcd_set_address_window(x, y, x + w - 1, y + h - 1);
  i2c_burst_transfer(data, sizeof(uint16_t) * col * row);
}




uint8_t lcd_begin(void)
{
  uint8_t count=0;
  uint8_t buffer[20]={0};
  uint8_t i2c[20]="/dev/i2c-1";
	// I2C Init
  i2cd = open(i2c,O_RDWR);//"/dev/i2c-1"
  if (i2cd < 0) 
  {
	  fprintf(stderr, "Device I2C-1 failed to initialize\n");
	  return 1;
  }
  if (ioctl(i2cd, I2C_SLAVE_FORCE, I2C_ADDRESS) < 0)
  {
    return 1;
  }
  return 0;
}

void i2c_write_data(uint8_t high, uint8_t low)
{
  uint8_t msg[3]={WRITE_DATA_REG,high,low};
  write(i2cd, msg, 3);
  usleep(10);
}

void i2c_write_command(uint8_t command,uint8_t high, uint8_t low)
{
  uint8_t msg[3]={command,high,low};
  write(i2cd, msg, 3);  
  usleep(10);
}

void i2c_burst_transfer(uint8_t* buff, uint32_t length)
{
	uint32_t count = 0;
	i2c_write_command(BURST_WRITE_REG,0x00,0x01);
	while(length > count)
	{
		if((length  - count) > BURST_MAX_LENGTH)
		{
			write(i2cd, buff+count, BURST_MAX_LENGTH);
			count += BURST_MAX_LENGTH;
		}
		else
		{
			write(i2cd, buff+count, length - count);
			count += (length - count);
		}
		usleep(700);
	}
	i2c_write_command(BURST_WRITE_REG,0x00,0x00);
  i2c_write_command(SYNC_REG,0x00,0x01);
}

void lcd_display(uint8_t symbol)
{
  switch(symbol)
  {
    case 0:
      lcd_display_cpuLoad();
    break;
    case 1:
      lcd_display_ram();
    break;    
    case 2:
      lcd_display_temp();
    break;
    case 3:
      lcd_display_disk();
    break;
    default:
    break;
  }

}

void lcd_display_percentage(uint8_t val, const uint8_t warning, const uint8_t critical){
  uint16_t color = ST7735_GREEN;
  uint8_t count = 0;
  uint8_t xCoordinate = 30;
  val += 10;
  if(val >= 100)
  {
    val = 100;
  }
  val /= 10;
  for(count = 0; count < val; count++)
  {
    if((critical-1) / 10 < count) color = ST7735_RED;
    else if((warning-1) / 10 < count) color = ST7735_YELLOW;
    lcd_fill_rectangle(xCoordinate,60,6,10,color);
    xCoordinate+=10;
  }
  for(count = 0; count < 10-val; count++)
  {
    lcd_fill_rectangle(xCoordinate,60,6,10,ST7735_GRAY);
    xCoordinate+=10;
  }
   
}

void lcd_blank_display(void)
{
  lcd_fill_screen(ST7735_BLACK);
}

void lcd_display_hostname_and_ip(void)
{
  // Font is 7px wide and 10px high
  // so 7*15 = 
  char hostname[11]={0};
  char iPSource[20]={0};
  gethostname(hostname, 11);
  lcd_write_string(0,5,hostname,Font_7x10,ST7735_WHITE,ST7735_BLACK); // Print hostname on left side
  strcpy(iPSource,get_ip_address());   //Get the IP address of the device's wireless network card 
  lcd_write_string(80,5,iPSource,Font_7x10,ST7735_WHITE,ST7735_BLACK);   //Send the IP address to the lower machine
  lcd_fill_rectangle(0,15,ST7735_WIDTH,5,ST7735_GRAY);
}

void lcd_setup_grid(void)
{
  // Create Divider
  lcd_fill_rectangle(75,20,5,80,ST7735_GRAY);
  // Setup CPU Top Left
  lcd_write_string(0,25,"CPU:",Font_8x16,ST7735_WHITE,ST7735_BLACK);
  // Setup Temp Middle Left
  lcd_write_string(0,45,"TMP:",Font_8x16,ST7735_WHITE,ST7735_BLACK);
  // Setup Memory Bottom Left
  lcd_write_string(0,65,"MEM:",Font_8x16,ST7735_WHITE,ST7735_BLACK);
  // Setup SD Card Top Right
  lcd_write_string(80,25,"SDC:",Font_8x16,ST7735_WHITE,ST7735_BLACK);
  // Setup SSD Card Top Right
  lcd_write_string(80,45,"SSD:",Font_8x16,ST7735_WHITE,ST7735_BLACK);
  // Setup Uptime Card Top Right
  lcd_write_string(80,65,"UPT:",Font_8x16,ST7735_WHITE,ST7735_BLACK);
}

void lcd_display_cpuLoad(void)
{
  uint8_t  cpuLoad = 0;
  uint8_t cpuStr[10] = {0};
  char* formattedCpuLoad;
  cpuLoad = get_cpu_message();
  sprintf(cpuStr, "%3d", cpuLoad);
  if (cpuLoad < CPU_LOAD_WARNING ) {
    lcd_write_string(40,25,cpuStr,Font_8x16,ST7735_NO_ISSUE,ST7735_BLACK);
    lcd_write_string(65,25,"%",Font_8x16,ST7735_NO_ISSUE,ST7735_BLACK);
    lcd_write_string(0,25,"CPU:",Font_8x16,ST7735_WHITE,ST7735_BLACK);
  } else if ( cpuLoad >= CPU_LOAD_WARNING && cpuLoad < CPU_LOAD_CRITICAL ){
    lcd_write_string(40,25,cpuStr,Font_8x16,ST7735_WARNING,ST7735_BLACK);
    lcd_write_string(65,25,"%",Font_8x16,ST7735_WARNING,ST7735_BLACK);
    lcd_write_string(0,25,"CPU:",Font_8x16,ST7735_WARNING,ST7735_BLACK);
  } else if ( cpuLoad >= CPU_LOAD_CRITICAL){
    lcd_write_string(40,25,cpuStr,Font_8x16,ST7735_CRITICAL,ST7735_BLACK);
    lcd_write_string(65,25,"%",Font_8x16,ST7735_CRITICAL,ST7735_BLACK);
    lcd_write_string(0,25,"CPU:",Font_8x16,ST7735_CRITICAL,ST7735_BLACK);
  } else {
    //idk how we got here, but we did...
  }
}



void lcd_display_ram(void)
{
  float Totalram = 0.0;
  float freeram = 0.0;
  uint8_t residue = 0;
  uint8_t Total[10]={0};
  uint8_t free[10]={0};
  uint8_t residueStr[10] = {0};
  get_cpu_memory(&Totalram,&freeram);
  residue = (Totalram - freeram)/Totalram*100;
  sprintf(residueStr, "%3d", residue);
  if (residue < MEMORY_WARNING ) {
    lcd_write_string(40,65,residueStr,Font_8x16,ST7735_NO_ISSUE,ST7735_BLACK);
    lcd_write_string(65,65,"%",Font_8x16,ST7735_NO_ISSUE,ST7735_BLACK);
    lcd_write_string(0,65,"MEM:",Font_8x16,ST7735_WHITE,ST7735_BLACK);
  } else if ( residue >= MEMORY_WARNING && residue < MEMORY_CRITICAL ){
    lcd_write_string(40,65,residueStr,Font_8x16,ST7735_WARNING,ST7735_BLACK);
    lcd_write_string(65,65,"%",Font_8x16,ST7735_WARNING,ST7735_BLACK);
    lcd_write_string(0,65,"MEM:",Font_8x16,ST7735_WARNING,ST7735_BLACK);
  } else if ( residue >= MEMORY_CRITICAL){
    lcd_write_string(40,65,residueStr,Font_8x16,ST7735_CRITICAL,ST7735_BLACK);
    lcd_write_string(65,65,"%",Font_8x16,ST7735_CRITICAL,ST7735_BLACK);
    lcd_write_string(0,65,"MEM:",Font_8x16,ST7735_CRITICAL,ST7735_BLACK);
  } else {
    //idk how we got here, but we did...
  }
}

void lcd_display_temp(void)
{
  uint16_t temp = 0;
  uint8_t tempStr[10] = {0};
  char* unit="C";
  temp=get_temperature(); 
  sprintf(tempStr, "%2d", temp);
  if(TEMPERATURE_TYPE == FAHRENHEIT) {
    char* unit="F";
  }
  if (temp < TEMP_WARNING ) {
    lcd_write_string(47,45,tempStr,Font_8x16,ST7735_NO_ISSUE,ST7735_BLACK);
    lcd_write_string(65,45,unit,Font_8x16,ST7735_NO_ISSUE,ST7735_BLACK);
    lcd_write_string(0,45,"TMP:",Font_8x16,ST7735_WHITE,ST7735_BLACK);
  } else if ( temp >= TEMP_WARNING && temp < TEMP_CRITICAL ){
    lcd_write_string(47,45,tempStr,Font_8x16,ST7735_WARNING,ST7735_BLACK);
    lcd_write_string(65,45,unit,Font_8x16,ST7735_WARNING,ST7735_BLACK);
    lcd_write_string(0,45,"TMP:",Font_8x16,ST7735_WARNING,ST7735_BLACK);
  } else if ( temp >= TEMP_CRITICAL){
    lcd_write_string(47,45,tempStr,Font_8x16,ST7735_CRITICAL,ST7735_BLACK);
    lcd_write_string(65,45,unit,Font_8x16,ST7735_CRITICAL,ST7735_BLACK);
    lcd_write_string(0,45,"TMP:",Font_8x16,ST7735_CRITICAL,ST7735_BLACK);
  } else {
    //idk how we got here, but we did...
  }
}


void lcd_display_disk(void)
{
  
  uint16_t diskMemSize = 0;
  uint16_t diskUseMemSize = 0;
  uint32_t sdMemSize = 0; 
  uint32_t sdUseMemSize = 0;

  uint16_t SDUsed = 0;
  uint8_t SDUsedStr[10] = {0};
  uint16_t SSDUsed = 0;
  uint8_t SSDUsedStr[10] = {0};
  
  get_sd_memory(&sdMemSize,&sdUseMemSize);
  get_hard_disk_memory(&diskMemSize,&diskUseMemSize);

  SDUsed = sdUseMemSize * 1.0 / sdMemSize * 100;
  SSDUsed = diskUseMemSize * 1.0 / diskMemSize * 100;

  sprintf(SDUsedStr, "%3d", SDUsed);
  sprintf(SSDUsedStr, "%3d", SSDUsed);

  if (SDUsed < SD_WARNING ) {
    lcd_write_string(120,25,SDUsedStr,Font_8x16,ST7735_NO_ISSUE,ST7735_BLACK);
    lcd_write_string(145,25,"%",Font_8x16,ST7735_NO_ISSUE,ST7735_BLACK);
    lcd_write_string(80,25,"SDC:",Font_8x16,ST7735_WHITE,ST7735_BLACK);
  } else if ( SDUsed >= SD_WARNING && SDUsed < SD_CRITICAL ){
    lcd_write_string(120,25,SDUsedStr,Font_8x16,ST7735_WARNING,ST7735_BLACK);
    lcd_write_string(145,25,"%",Font_8x16,ST7735_WARNING,ST7735_BLACK);
    lcd_write_string(80,25,"SDC:",Font_8x16,ST7735_WARNING,ST7735_BLACK);
  } else if ( SDUsed >= SD_CRITICAL){
    lcd_write_string(120,25,SDUsedStr,Font_8x16,ST7735_CRITICAL,ST7735_BLACK);
    lcd_write_string(145,25,"%",Font_8x16,ST7735_CRITICAL,ST7735_BLACK);
    lcd_write_string(80,25,"SDC:",Font_8x16,ST7735_CRITICAL,ST7735_BLACK);
  } else {
    //idk how we got here, but we did...
  }

  if (SSDUsed < SSD_WARNING ) {
    lcd_write_string(120,45,SSDUsedStr,Font_8x16,ST7735_NO_ISSUE,ST7735_BLACK);
    lcd_write_string(145,45,"%",Font_8x16,ST7735_NO_ISSUE,ST7735_BLACK);
    lcd_write_string(80,45,"SSD:",Font_8x16,ST7735_WHITE,ST7735_BLACK);
  } else if ( SSDUsed >= SSD_WARNING && SSDUsed < SSD_CRITICAL ){
    lcd_write_string(120,45,SSDUsedStr,Font_8x16,ST7735_WARNING,ST7735_BLACK);
    lcd_write_string(145,45,"%",Font_8x16,ST7735_WARNING,ST7735_BLACK);
    lcd_write_string(80,45,"SSD:",Font_8x16,ST7735_WARNING,ST7735_BLACK);
  } else if ( SSDUsed >= SSD_CRITICAL){
    lcd_write_string(120,45,SSDUsedStr,Font_8x16,ST7735_CRITICAL,ST7735_BLACK);
    lcd_write_string(145,45,"%",Font_8x16,ST7735_CRITICAL,ST7735_BLACK);
    lcd_write_string(80,45,"SSD:",Font_8x16,ST7735_CRITICAL,ST7735_BLACK);
  } else {
    //idk how we got here, but we did...
  }
}



