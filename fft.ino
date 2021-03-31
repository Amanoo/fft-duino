#include <arduinoFFT.h>


/*******************************************************************/
/*  Free Run ADC conversions of 1 analog input (A0)                */
/*******************************************************************/

/*
  20Hz-20kHz

  40kHz sample rate
  39.0625Hz per bin

  Number of bins= Max_Hz/bin_Hz
  Number of bins= 20000/39.0625=512 bins
  Number of sample_size= 512*2=1024 sample_size

  FFT resolution=1024/40000=1/39.0625s=25.6ms (39.0625Hz)

  https://electronics.stackexchange.com/questions/12407/what-is-the-relation-between-fft-length-and-frequency-resolution
*/

arduinoFFT FFT = arduinoFFT(); /* Create FFT object */

constexpr uint16_t sample_size = 1024;

const double samplingFrequency = 40000;
double adc_out[sample_size];
uint16_t adc_pointer = 0;

double re[sample_size], im[sample_size];
  boolean initiated = false;
void setup()
{
  //pinMode(LED_BUILTIN, OUTPUT);
  adc_setup();
  SerialUSB.begin(2000000);    // Initialize Native USB port
  while (!SerialUSB);          // Wait until connection is established
}

void loop()
{
  if(SerialUSB.available() >= 3) {
    String hoi = SerialUSB.readString();
    if(hoi.startsWith("hoi")) {
      send_init();
      initiated = true;
      delay(10);
    }
  }
  if(initiated){
  int pointer = adc_pointer;

  //populate array with samples
  for (int i = 0; i < sample_size; i++) {
    if (i + pointer == sample_size)pointer = 0 - i;
    re[i] = adc_out[i + pointer];
    im[i] = 0.0;
  }

  /* Send the results of the simulated sampling according to time */
  FFT.Windowing(re, sample_size, FFT_WIN_TYP_HAMMING, FFT_FORWARD);  /* Weigh data */
  FFT.Compute(re, im, sample_size, FFT_FORWARD); /* Compute FFT */
  FFT.ComplexToMagnitude(re, im, sample_size); /* Compute magnitudes */
  send_data(re, (sample_size >> 1));

  }
}


/**
 * Send FFT'ed data to the Arduino
 */
void send_data(double *vData, uint16_t bufferSize)
{
  //buffer containing the message sent to the PC
  byte values[8*bufferSize + 2 * sizeof(double)];

  //start word
  memset(&values[0], 0x00, 6);
  memset(&values[6], 0xFF, 2);

  //data
  memcpy(&values[8], (void*)vData, 8*bufferSize);
  
  //stop word
  int start = sizeof(values) - 8;
  memset(&values[start], 0x00, 4);
  memset(&values[start + 4], 0xFF, 4);

  SerialUSB.write(values, sizeof(values));

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

  adc_out[adc_pointer] = ((ADC->ADC_CDR[7]) >> 4) - 125.5;                // Reading ADC->ADC_CDR[i] clears EOCi bit
  adc_pointer = (adc_pointer + 1) % sample_size;

}

/**
 * Sends a message containing the sampling frequency and sample size to the computer.
 */
void send_init() {
  //buffer containing the message sent to the PC
  byte response[32];

  //start word
  memset(&response[0], 0xFF, 8);

  double ssize = (double)sample_size;

  //sampling frequency
  memcpy(&response[8], (void*)&samplingFrequency, sizeof(samplingFrequency));
  //sample size
  memcpy(&response[16], (void*)&ssize, sizeof(ssize));

  //stop word
  memset(&response[24], 0x00, 4);
  memset(&response[28], 0xFF, 4);

  SerialUSB.write(response, sizeof(response));
}
