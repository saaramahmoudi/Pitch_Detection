#include "stm32f4xx.h"
#include <math.h>
#include <string.h>
#include <limits.h>
#include <stdio.h>
#define l_max 1350
#define window_size 1350
#define index_step	14816.55
#define sample_rate 0x7



volatile int count = 0;
volatile uint64_t window[window_size];
volatile float freq_detected;
volatile int sum[l_max];
float l_note_max = INT_MIN;
float l_note_min = INT_MAX;

struct pitch{
  char* note;
  float freq;
  
};
struct pitch notes[12];
float diffrent_ls[108];

void init(void);
void search_and_print_result(float finded_l);
int c_in = 0;

int auto_correlation(volatile uint64_t* window_array, int l, int window_array_size)
{
  int sum_result = 0;
  for(int window_i = 0; window_i < window_array_size-l; window_i++)
  {
    sum_result += window_array[window_i]* window_array[window_i + l];
  }
  return sum_result;
  
}


void pda(){
 for (int i = 0 ; i < l_max;i++){
   sum[i] = 0;
   for(int j = i; j < window_size - i;j++){
     sum[i] += window[j - i] * window[j];
   }
 }

 int max1 = INT_MIN;
 int max2 = INT_MIN;
 int index1 = 0;
 int index2 = 0;
 
 for (int i = 0; i < l_max ; i++){
   if(sum[i] > max1){
     max1 = sum[i];
     index1 = i;
 }
   }
	
 
   
   for (int i = index1 + 1 ; i < l_max ; i++){
   if(sum[i] > max2){
     max2 = sum[i];
     index2 = i;
 }
   }

   search_and_print_result(abs(index2 - index1)*100);

 }  
  
  
  
  
void init_notes(){
  
  notes[0].note = "C";
  notes[0].freq = 16.35;
  
  notes[1].note = "C#";
  notes[1].freq = 17.32;
  
  notes[2].note = "D";
  notes[2].freq = 18.35;
  
  notes[3].note = "D#";
  notes[3].freq = 19.45;
  
  notes[4].note = "E";
  notes[4].freq = 20.60;
  
  notes[5].note = "F";
  notes[5].freq = 21.83;
  
  notes[6].note = "F#";
  notes[6].freq = 23.12;
  
  notes[7].note = "G";
  notes[7].freq = 24.50;
  
  notes[8].note = "G#";
  notes[8].freq = 25.96;
  
  notes[9].note = "A";
  notes[9].freq = 27.50;
  
  notes[10].note = "A#";
  notes[10].freq = 29.14;
  
  notes[11].note = "B";
  notes[11].freq = 30.87;
  
  
  
  for(int power = 0; power < 9; power++){

    for(int i = power * 12; i < (power + 1)*12; i++){
      float frequency = (notes[i % 12].freq) * pow(2, power);
      diffrent_ls[i] = index_step / frequency;
			diffrent_ls[i] *= 100;
    }
  }
}


int main(){
  init_notes();
  init();
  while(1);
}
void init(){
  
  RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN;
  
  //pin A1 for analog audio input
  RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
  RCC->AHB1ENR |= RCC_AHB1ENR_GPIOBEN;
  RCC->AHB1ENR |= RCC_AHB1ENR_GPIOCEN;
  
  
  //ADC init channel 1 continues
  RCC->APB2ENR |= RCC_APB2ENR_ADC1EN;
  
  ADC1->CR2  = 0;                /* Enable ADC1 */ 
  ADC1->SQR1 = 0;                /* conversion sequence length 1 */ 
  ADC1->SQR3 = 1;          			/*channel 1*/
	
  ADC1->SMPR2 = sample_rate << 3;
  ADC1->CR1 |= ADC_CR1_EOCIE;

	GPIOA->MODER |= 0x5555555C;
  GPIOB->MODER |= 0x55555555;
  GPIOC->MODER |= 0x55555555;
	
  __enable_irq();
  NVIC_ClearPendingIRQ(ADC_IRQn);
  NVIC_SetPriority(ADC_IRQn, 0);
  NVIC_EnableIRQ(ADC_IRQn);
  
  ADC1->CR2 |= ADC_CR2_ADON;
  ADC1->CR2 |= ADC_CR2_SWSTART;
  
  
}



void ADC_IRQHandler(){
	ADC1->SR = 0;
  window[count] = ADC1->DR;
  count++;
  if (count == window_size){
    count = 0;
    pda();
  }
  ADC1->CR2 |= ADC_CR2_SWSTART;
}

void search_and_print_result(float finded_l){
  
  float min_sub = INT_MAX;
  int min_index = 0;
  for(int i = 0; i < 108;i++){
    float sub = fabs(finded_l - diffrent_ls[i]);
    if (sub < min_sub){
      min_sub = sub;
      min_index = i;
    }
  }
  int freq_index_start = min_index / 12;
  int freq_offset = min_index % 12;
  int notechar = 0;
  switch (notes[freq_offset].note[0]){
    case 'A':notechar = 0xAAAAAAAA;  break;
    case 'B':notechar = 0xBBBBBBBB;  break;
    case 'C':notechar = 0xCCCCCCCC;  break;
    case 'D':notechar = 0xDDDDDDDD;  break;
    case 'E':notechar = 0xEEEEEEEE;  break;
    case 'F':notechar = 0xFFFFFFFF;  break;
    case 'G':notechar = 0x0;  break;
    default:notechar = 0x1;    break;
  }
  min_sub *= pow(2, 16);
  int sharp = 0;
  if (strlen(notes[freq_offset].note) == 2){
    sharp = 0xFFFFFFFF;
  }
  int freq_to_print = 0;
  
  for(int i = 0 ; i < 8; i++){
    freq_to_print |= freq_index_start << (i * 4);
  }
  
  
  GPIOA->ODR =  sharp<< 8;
  GPIOC->ODR = ((freq_to_print) >> 4) << 6;
  GPIOB->ODR = (notechar >> 8 ) << 12; 
  
}

