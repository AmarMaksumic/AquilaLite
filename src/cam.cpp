// ArduCAM demo (C)2017 Lee
// Web: http://www.ArduCAM.com
// This program is a demo of how to use most of the functions
// of the library with a supported camera modules, and can run on any Arduino platform.
//
// This demo was made for ARDUCAM_SHIELD_REVC.
// It will run the ArduCAM ESP8266 2MP/3MP/5MP as a real 2MP/3MP/5MP digital camera, provide both JPEG capture.
// The demo sketch will do the following tasks:
// 1. Set the sensor to JPEG mode.
// 2. Capture and buffer the image to FIFO every 5 seconds 
// 3. Store the image to Micro SD/TF card with JPEG format in sequential.
// 4. Resolution can be changed by myCAM.set_JPEG_size() function.
// This program requires the ArduCAM V4.0.0 (or later) library and ARDUCAM_SHIELD_REVC
// and use Arduino IDE 1.6.8 compiler or above
#include <ArduCAM.h>
#include <Wire.h>
#include <SD.h>
#include <SPI.h>
#include "memorysaver.h"

#define BMPIMAGEOFFSET 66
const char bmp_header[BMPIMAGEOFFSET] PROGMEM =
{
  0x42, 0x4D, 0x36, 0x58, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x42, 0x00, 0x00, 0x00, 0x28, 0x00,
  0x00, 0x00, 0x40, 0x01, 0x00, 0x00, 0xF0, 0x00, 0x00, 0x00, 0x01, 0x00, 0x10, 0x00, 0x03, 0x00,
  0x00, 0x00, 0x00, 0x58, 0x02, 0x00, 0xC4, 0x0E, 0x00, 0x00, 0xC4, 0x0E, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xF8, 0x00, 0x00, 0xE0, 0x07, 0x00, 0x00, 0x1F, 0x00,
  0x00, 0x00
};

int cam_pin = 10;
int resolution = 2;

ArduCAM myCAM(OV2640, cam_pin);

void myCAMSaveToSDFile() {
  char str[8];
  byte buf[256];
  static int i = 0;
  static int k = 0;
  File outFile;
  uint8_t temp = 0,temp_last=0;
  uint32_t length = 0;
  bool is_header = false;
  //Flush the FIFO
  myCAM.flush_fifo();
  //Clear the capture done flag
  myCAM.clear_fifo_flag();
  //Start capture
  myCAM.start_capture();
  Serial.println(F("start Capture"));
  while(!myCAM.get_bit(ARDUCHIP_TRIG , CAP_DONE_MASK));
  Serial.println(F("Capture Done."));  
  length = myCAM.read_fifo_length();
  Serial.print(F("The fifo length is :"));
  Serial.println(length, DEC);
  if (length >= MAX_FIFO_SIZE) //384K
  {
    Serial.println(F("Over size."));
    return ;
  }
  if (length == 0 ) //0 kb
  {
    Serial.println(F("Size is 0."));
    return ;
  }
  //Construct a file name
  k = k + 1;
  itoa(k, str, 10);
  strcat(str, ".jpg");
  //Open the new file
  outFile = SD.open(str, O_WRITE | O_CREAT | O_TRUNC);
  if(!outFile){
    Serial.println(F("File open faild"));
    return;
  }
  myCAM.CS_LOW();
  myCAM.set_fifo_burst();
  while ( length-- )
  {
    temp_last = temp;
    temp =  SPI.transfer(0x00);
    //Read JPEG data from FIFO
    if ( (temp == 0xD9) && (temp_last == 0xFF) ) //If find the end ,break while,
    {
      buf[i++] = temp;  //save the last  0XD9     
      //Write the remain bytes in the buffer
      myCAM.CS_HIGH();
      outFile.write(buf, i);    
      //Close the file
      outFile.close();
      Serial.println(F("Image save OK."));
      is_header = false;
      i = 0;
    }  
    if (is_header == true)
    { 
      //Write image data to buffer if not full
      if (i < 256)
      buf[i++] = temp;
      else
      {
        //Write 256 bytes image data to file
        myCAM.CS_HIGH();
        outFile.write(buf, 256);
        i = 0;
        buf[i++] = temp;
        myCAM.CS_LOW();
        myCAM.set_fifo_burst();
      }        
    }
    else if ((temp == 0xD8) & (temp_last == 0xFF))
    {
      is_header = true;
      buf[i++] = temp_last;
      buf[i++] = temp;   
    } 
  } 
}

int initialize_cam(int cam_pin, int resolution) {
  uint8_t vid, pid;
  pinMode(cam_pin, OUTPUT);
  digitalWrite(cam_pin, HIGH);
  // initialize SPI:
  SPI.begin();
  //Reset the CPLD
  myCAM.write_reg(0x07, 0x80);
  myCAM.write_reg(0x07, 0x00);
  //Check if the ArduCAM SPI bus is OK
  myCAM.write_reg(ARDUCHIP_TEST1, 0x55);
  if (myCAM.read_reg(ARDUCHIP_TEST1) != 0x55) {
    return -1;
  }
      
  //Check if the camera module type is OV2640
  myCAM.wrSensorReg8_8(0xff, 0x01);
  myCAM.rdSensorReg8_8(OV2640_CHIPID_HIGH, &vid);
  myCAM.rdSensorReg8_8(OV2640_CHIPID_LOW, &pid);
  if ((vid != 0x26 ) && (( pid != 0x41 ) || ( pid != 0x42 ))) {
    Serial.println(F("ACK CMD Can't find OV2640 module! END"));
    return -2;
  }
  
  //Change to JPEG capture mode and initialize the OV5642 module
  myCAM.set_format(JPEG);
  myCAM.InitCAM();
  myCAM.OV2640_set_JPEG_size(OV2640_320x240);
  myCAM.clear_fifo_flag();
  myCAM.write_reg(ARDUCHIP_FRAMES,0x00);

  switch (resolution) {
    case 0:
      myCAM.OV2640_set_JPEG_size(OV2640_160x120);
      break;
    case 1:
      myCAM.OV2640_set_JPEG_size(OV2640_176x144);
      break;
    case 2: 
      myCAM.OV2640_set_JPEG_size(OV2640_320x240);
      break;
    case 3:
      myCAM.OV2640_set_JPEG_size(OV2640_352x288);
      break;
    case 4:
      myCAM.OV2640_set_JPEG_size(OV2640_640x480);
      break;
    case 5:
      myCAM.OV2640_set_JPEG_size(OV2640_800x600);
      break;
    case 6:
      myCAM.OV2640_set_JPEG_size(OV2640_1024x768);
      break;
    case 7:
      myCAM.OV2640_set_JPEG_size(OV2640_1280x1024);
      break;
    case 8:
      myCAM.OV2640_set_JPEG_size(OV2640_1600x1200);
      break;
  }
  return 1;
}
