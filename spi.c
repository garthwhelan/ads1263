// to be run on the raspi
// see python file in folder for main machine
// gcc -o spi spi.c -l bcm2835
// sudo ./spi

#include <bcm2835.h>
#include <stdio.h>

#define PIN_READY RPI_BPLUS_GPIO_J8_03 //for ready
#define PIN_RESET RPI_BPLUS_GPIO_J8_05 //reset

int main(int argc, char **argv)
{
  if (!bcm2835_init()) {
    printf("bcm2835_init failed. Are you running as root??\n");
    return 1;
  }
    
  if (!bcm2835_spi_begin()) {
    printf("bcm2835_spi_begin failed. Are you running as root??\n");
    return 1;
  }

  //set the correct SPI mode and speed
  bcm2835_spi_setBitOrder(BCM2835_SPI_BIT_ORDER_MSBFIRST);
  bcm2835_spi_setDataMode(BCM2835_SPI_MODE2);
  bcm2835_spi_setClockDivider(BCM2835_SPI_CLOCK_DIVIDER_32);
  bcm2835_spi_chipSelect(BCM2835_SPI_CS0);
    
  //check to make sure ADC registers are the defaults... (only checking 10)
  //reading regs: 06h 20h 1Ah (27 bytes should be xx 11 05 00 80 04 01...
  int i;
  uint8_t rreg[10] =         {0x06,0x20,0x07,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
  uint8_t reg_vals[10] =     {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
  uint8_t correct_vals[10] = {0x00,0x00,0x00,0x23,0x11,0x05,0x00,0x80,0x04,0x01};

  bcm2835_spi_setChipSelectPolarity(BCM2835_SPI_CS0, LOW);
  bcm2835_spi_transfernb(rreg,reg_vals,10);
  bcm2835_spi_setChipSelectPolarity(BCM2835_SPI_CS0, HIGH);
    
  for(i=0;i<10;i++){
    if(reg_vals[i]!=correct_vals[i]) {
      printf("%.2x %.2x", reg_vals[i],correct_vals[i]);
      printf("check connections\n");
      return 1;
    }
  }

  //load appropiate settings into the registers
  //see ads1263 datasheet for settings
  //(samples per second, gain, etc)
  uint8_t adc_setup[6]= {0x43,0x03,0x00,0x66,0x09,0x01};
  uint8_t reg_check_in[6]= {0x23,0x03,0x00,0x66,0x09,0x01};    
  uint8_t reg_check_out[6]= {0x00,0x00,0x00,0x00,0x00,0x00};
    
  bcm2835_spi_setChipSelectPolarity(BCM2835_SPI_CS0, LOW);    
  bcm2835_spi_writenb(adc_setup,6);
  bcm2835_spi_transfernb(reg_check_in,reg_check_out,6);	
  bcm2835_spi_setChipSelectPolarity(BCM2835_SPI_CS0, HIGH);

  for(i=2;i<6;i++){
    if(adc_setup[i]!=reg_check_out[i]) {
      printf("error setting registers\n");
      return 1;
    }
  }
    
  uint8_t ADC_data[7];
  uint8_t get_data[7] = {0x12,0x00,0x00,0x00,0x00,0x00,0x00};

  //trigger when data ready pin on ADC is on
  bcm2835_gpio_fsel(PIN_READY, BCM2835_GPIO_FSEL_INPT);
  bcm2835_gpio_set_pud(PIN_READY, BCM2835_GPIO_PUD_UP);//pullup
  bcm2835_gpio_len(PIN_READY);//low event detect
  i = 0;
  while (1)
    {
      if (bcm2835_gpio_eds(PIN_READY))
        {
          i+=1;
          fflush(stdout);
          bcm2835_spi_setChipSelectPolarity(BCM2835_SPI_CS0, LOW);
          bcm2835_spi_transfernb(get_data,ADC_data,7);
          bcm2835_spi_setChipSelectPolarity(BCM2835_SPI_CS0, HIGH);
          if(ADC_data[1]&0x40){
            //40h is new data, 10h low reference, 08h low output 04h high output 02h differential 01h device reset
            printf("%.2x%.2x%.2x%.2x%.2x%.2x\n",ADC_data[1],ADC_data[2],ADC_data[3],ADC_data[4],ADC_data[5],ADC_data[6]);
            
          } else {
            //shouldn't happen unless SPI is slower than data transfer
            printf("weird\n");
          }
          bcm2835_gpio_set_eds(PIN_READY);//reset it
        }
    }     
  bcm2835_spi_end();
  bcm2835_close();
  return 0;
}

