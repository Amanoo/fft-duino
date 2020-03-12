/*******************************************************************/
/*  Free Run ADC conversions of 1 analog input (A0)                */
/*******************************************************************/

/*
20Hz-20kHz

40kHz sample rate
20Hz per bin

Number of bins= 20000/20=1000 bins
Number of samples= 1000*2=2000 samples

FFT resolution=2000/40000=1/20s=50ms

https://electronics.stackexchange.com/questions/12407/what-is-the-relation-between-fft-length-and-frequency-resolution
 */
#define sample_size 2000

uint16_t adc_out[sample_size];
uint16_t adc_pointer=0;

void setup()
{
  //pinMode(LED_BUILTIN, OUTPUT);
  adc_setup();
  Serial.begin(9600);
}

void loop()
{
  //Serial.println(adc_out);
    delay(2);

}

/*************  Configure adc_setup function  *******************/
void adc_setup() {

  PMC->PMC_PCER1 |= PMC_PCER1_PID37;                    // ADC power ON

  ADC->ADC_CR = ADC_CR_SWRST;                           // Reset ADC
  ADC->ADC_MR |=  ADC_MR_TRGEN_DIS                      // Hardware trigger disable
                  | ADC_MR_FREERUN
                  | ADC_MR_PRESCAL(49);                  // PRESCAL (49) to limit to 40kHz !   || ADCClock = MCK / ( (PRESCAL+1) * 2 )   || MCK=4MHz

  //ADC->ADC_ACR = ADC_ACR_IBCTL(0b01);                   // For frequencies > 500 KHz

  ADC->ADC_IER = ADC_IER_EOC7;                          // End Of Conversion interrupt enable for channel 7
  NVIC_EnableIRQ(ADC_IRQn);                             // Enable ADC interrupt
  ADC->ADC_CHER = ADC_CHER_CH7;                         // Enable Channel 7 = A0
}

void ADC_Handler() {
  static uint32_t Count;
 
  adc_out[adc_pointer] = ADC->ADC_CDR[7];                    // Reading ADC->ADC_CDR[i] clears EOCi bit
  adc_pointer=(adc_pointer+1)%sample_size;
 
 /*// For debugging only
  if (Count++ == 40000) {
    Count = 0;
    PIOB->PIO_ODSR ^= PIO_ODSR_P27;
    Serial.println(diff);
  }*/

}
