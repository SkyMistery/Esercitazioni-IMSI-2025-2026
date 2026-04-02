#include "main.h"

//Questo valoro vanno  definiti a mano poiché non è presente nei file forniti
#define NVIC_ISER_EXTI15_10_IR_EN (unsigned int) (1<<8)

#define PRESCALER_VALUE_16MHz_to_1kHz (unsigned int) 15999
#define ARR_VALUE (unsigned int) 999

void clock_config(void);
void led_pa5_config(void);
void button_pc13_config(void);
void TIM2_config(unsigned int,unsigned int);
void interruption_config(void);
int main(void){

	clock_config();
	led_pa5_config();
	button_pc13_config();
	TIM2_config(PRESCALER_VALUE_16MHz_to_1kHz,ARR_VALUE);
	interruption_config();

	while(1);

	return 0;
}

void clock_config(void){

	//Abilitiamo il clock alla GPIOA (per il led) e alla GPIOC (per il pulsante) sulla linea AHB1
	//REF. Pagina 118 manuale "2 - STM32 F401xE Reference Manual"
	RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN_Msk | RCC_AHB1ENR_GPIOCEN_Msk;

	//Abilitiamo il clock al TIM2 sulla linea APB1
	//REF. Pagina 119 manuale "2 - STM32 F401xE Reference Manual"
	RCC->APB1ENR |= RCC_APB1ENR_TIM2EN_Msk;

	//Resettiamo tutti i regisstri del TIM2 dato che siamo gli unici utilizzatori e vogliamo essere certi dei valori presenti (i valori presenti sono quelli scritti nella documentazione)
	//REF. Pagina 114 manuale "2 - STM32 F401xE Reference Manual"
	RCC->APB1RSTR |= RCC_APB1RSTR_TIM2RST_Msk;
	RCC->APB1RSTR &= ~RCC_APB1RSTR_TIM2RST_Msk;

	//Abilitiamo il clock alla periferica SYSCFG che ci permette di configurare il microcontrollore a livello hardware
	//REF. Pagina 122 manuale "2 - STM32 F401xE Reference Manual"
	RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN_Msk;

}


void led_pa5_config(){

	//Settiamo la porta pa5 (il led) in modalità alternate (10) in modo che possa essere pilota da un altro hardware del microcontrollore
	//RIF. Pagina 158 manuale "2 - STM32 F401xE Reference Manual"
	GPIOA->MODER &= ~GPIO_MODER_MODE5_0; //Bit 0 del MODER5
	GPIOA->MODER |= GPIO_MODER_MODE5_1; //Bit 1 del MODER5

	//Impostiamo sul PA5 il modo AFRL per collegarlo al canale 1 del TIM2
	//REF. 162 manuale "2 - STM32 F401xE Reference Manual"
	GPIOA->AFR[0] &= ~GPIO_AFRL_AFRL5;
	GPIOA->AFR[0] |= GPIO_AFRL_AFRL5_0;


	//Settiamo la porta in modalità push-pull (0) in modo da poter forzare livello logico basso e alto da software
	//Se la impostassimo in open-drain e mettessimo in uscita 1 avremmo che il pin viene settato in alta impedenza, (come se venisse staccato dal circuito) è utile per pilotare il valore del pin dall'esterno
	//REF. Pagina 158 manuale "2 - STM32 F401xE Reference Manual"
	GPIOA->OTYPER &= ~GPIO_OTYPER_OT5_Msk;

	//Impostiamo la velocità della porta su Low-Speed (00) (fino a 8 Mhz a seconda delle condizioni) (Impostarla a low aiuta a risparimare energia e a gagrantire una certa resistenza al rumore)
	//REF. Pagina 159 manuale "2 - STM32 F401xE Reference Manual"
	GPIOA->OSPEEDR &= ~GPIO_OSPEEDER_OSPEEDR5;

	//Scolleghiamo sia la rete di pull-up che la rete di pull-down (00) in uscita così da pilotare liberamente il segnale
	//In alternativa si può attivare una delle due reti per tenere il segnale alto o basso
	//REF. Pagina 159 manuale "2 - STM32 F401xE Reference Manual"
	GPIOA->PUPDR &= ~GPIO_PUPDR_PUPD5_Msk;

	return;
}

void button_pc13_config(){

	//Settiamo la porta pa5 (il pulsante) in modalità input (00)
	//REF. Pagina 158 manuale "2 - STM32 F401xE Reference Manual"
	GPIOC->MODER &= ~GPIO_MODER_MODE13_Msk;

	GPIOC->PUPDR &= ~GPIO_MODER_MODE13_Msk;

	//I registri OTYPER e OSPEEDR controllano solo la parte di output della GPIO, qui non servono

	return;

}

void TIM2_config(unsigned int prescaler_value,unsigned int arr_value){

	//Impostiamo il timer in one-pulse mode (1 sul bit 3 del control register) in modo che faccia un ciclo di conteggio e poi si fermi
	//REF. Pagina 353 manuale "2 - STM32 F401xE Reference Manual"
	TIM2->CR1 |= TIM_CR1_OPM_Msk;

	//Poichè nel microcontrollore il canale 1 del TIM2 e la PA5 sono connesse a livello hardware posiamo utilizzare questo canale per pilotare proprio l'uscita su quel pin
	//REF pagina 44 manuale "3 - STM32 F401xE Datasheet" per la tabella delle alternate function
	//REF pagina 153 manuale "2 - STM32 F401xE Reference Manual" per la descrizione della alternate function nelle GPIO
	//REF pagina 331 manuale "2 - STM32 F401xE Reference Manual" per la descrizione dei capture/compare channel usati

	//Impostiamo il canale in modo che sia in output mode (00) e al momento della comparazione tra l'arr register e il ccr1 register si attivi
	//REF Pagina 362 manuale "2 - STM32 F401xE Reference Manual"
	TIM2->CCMR1 &= ~TIM_CCMR1_CC1S_Msk;

	//Impostiamo la modalità inactive on match (010) in modo che il canale sia forzato a low quando arr==cccr1
	TIM2->CCMR1 &= ~TIM_CCMR1_OC1M_Msk;
	TIM2->CCMR1 |= TIM_CCMR1_OC1M_1;

	//Impostiamo il valore con cui si deve coparare arr e ragiunto il quale si attiva il canale
	//REF Pagina 369 manuale "2 - STM32 F401xE Reference Manual"
	TIM2->CCR1 = arr_value;

	//Impostiamo il canale in modo che l'uscita segua l'andamento del segnale del canale ovvero active on high(0) quindi non inverte la polarità del segnale
	//REF Pagina 367 manuale "2 - STM32 F401xE Reference Manual"
	TIM2->CCER &= ~TIM_CCER_CC1P_Msk;

	//Abilitiamo il canale 1 affinchè sia visto all'esterno
	TIM2->CCER |= TIM_CCER_CC1E_Msk;

	//Impostiamo il timer in one-pulse mode (1 sul bit 3 del control register) in modo che faccia un ciclo di conteggio e poi si fermi
	//REF. Pagina 353 manuale "2 - STM32 F401xE Reference Manual"
	TIM2->CR1 |= TIM_CR1_OPM_Msk;

	//Impostiamo il precaler in modo da portare la frequenza da 16 Mhz (di default sul APB1) a 1 kHz (in questo caso)
	//La frequenza di conteggio ottenuta sarà frequenza_di_clock/(prescaler_value+1)
	//REF. Pag 368 manuale "2 - STM32 F401xE Reference Manual"
	TIM2->PSC=prescaler_value;

	//Impostiamo il valore superato il quale si genera l'overflow (se il timer è settato in upcounting mode come in questo caso)
	//O il valore dal quale il timer deve partire se settato in downcounting mode(1)
	//Pagina 368 manuale "2 - STM32 F401xE Reference Manual"
	TIM2->ARR=arr_value;

	//Azzeriamo il valore di conteggio (ricorda, questo registro non è mai bufferizzato)
	//REF. Pagina 368 manuale "2 - STM32 F401xE Reference Manual"
	TIM2->CNT=0;

	//Generiamo un evento software in modo da aggioranre i registri bufferizzati
	//In questo caso il PSC è sempre bufferizzaro
	//L'ARR è bufferizzato solo se si attiva (2)
	//REF. Pagina 361 manuale "2 - STM32 F401xE Reference Manual"
	TIM2->EGR |= TIM_EGR_UG_Msk;

	//Azzeriamo il bit che ci segnala l'evento (è stato sollevato mediante l'istruzione precedente)
	//REF Pagina 359 manuale "2 - STM32 F401xE Reference Manual"
	TIM2->SR &= ~TIM_SR_UIF_Msk;

	//(1)La moalità di conteggio si setta tramite il bit 4 del control register (CR1)
	//REF. Pagina 353 manuale "2 - STM32 F401xE Reference Manual"

	//(2) Per far si che l'ARR sia bufferizzato, ovvero il valore passi dal registro di appoggio(buffer) a quello effettivo(shadow)
	//quando arriva un evento e non immediatamente si deve settare a 1 il bit 7 (ARPE) del Control register CR1
	//REF. Pagina 353 manuale "2 - STM32 F401xE Reference Manual"

	return;

}

void interruption_config(void){

	//Abilitiamo la linea 13 delle interruzioni a ricevere il segnale dalla linea corrispondente della GPIOC (0010)
	//REF Pagina 143 manuale "2 - STM32 F401xE Reference Manual"
	//REF Pagina 208 manuale "2 - STM32 F401xE Reference Manual" (esempi di config possibili)
	SYSCFG->EXTICR[3] &= ~SYSCFG_EXTICR4_EXTI13_Msk; //è una maschera  di 1111 nei quattro bit che configurano la EXTI 13 nel registro
	SYSCFG->EXTICR[3] |= SYSCFG_EXTICR4_EXTI13_PC; //è la maschera 0010 nei quattro bit che configurano la EXTI 13 nel registro (e corrispondono al codice per configurare il mux a far passare la linea realtiva alla GPIOC)

	//Disabilitiamo la maschera all'interruzione EXTI13 in modo che il processore la possa vedere
	//REF Pagina 209 manuale "2 - STM32 F401xE Reference Manual"
	EXTI->IMR |= EXTI_IMR_MR13_Msk;

	//Impostiamo il riconoscimento dell'evento sul fronte di discesa del segnale impostando 1 al bit 13 (1)
	//REF Pagina 210 manuale "2 - STM32 F401xE Reference Manual"
	EXTI->FTSR |= EXTI_FTSR_TR13_Msk;

	//Ripuliamo ogni eventuale richeista di interruzione pendente sulla linea 13
	//REF Pagina 211 manuale "2 - STM32 F401xE Reference Manual"
	EXTI->PR |= EXTI_PR_PR13_Msk;

	//Abilitiamo l'interruzione realtiva alla linea di EXTI da 10 a 15 (occhio a se si usa più di una, vanno verificate una a una le periferiche a cui ogni linea è collegata)
	// REF Pagina 202 manuale "2 - STM32 F401xE Reference Manual"
	NVIC->ISER[1] |= NVIC_ISER_EXTI15_10_IR_EN; //Si attiva mettendo a 1 il bit 8 sel secondo resitro ISER (2)

	//(1) Per riconoscere il fronte di salita va impostato 1 nel registro RTSR). Si può generare un evento sia sul fronte di salita che su quello di discesa
	//REF Pagina 210 manuale "2 - STM32 F401xE Reference Manual"

	//(2) Il vettore ISER si divide fisicamente in due registri, quindi tutte le interruzioni che hanno una posizione >=32
	//Hanno il bit da attivare nel registro NVIC->ISER[1] la cui posizione si calcola come posizione_nella_tabella-32
	//Esempio, la NVIC_ISER_EXTI15_10_IR_EN è alla posizione 40 nella tabella, 40-32=8, si trova al bit 8 nel registro fisico
	// REF Pagina 202 manuale "2 - STM32 F401xE Reference Manual"

	return;
}

void EXTI15_10_IRQHandler (void){

	/*
	 * PSEUDOCODICE
	 * 		Riportiamo il timer a 0 in modo che non generi eventi mentre lo configuriamo
	 * 		Forziamo il canale 1 del TIM2 ad avere un livello alto in uscita così che il led si accenda
	 * 		Reimpostiamo la modalità del canale su inactive on match in modo che abbassi il livello logico sulla linea quando carr1==arr
	 * 		Puliamo il bit che ci segnala le interruzioni relative al timer
	 * 		Facciamo ripartire il timer
	 * 		Resettiamo lo status dell'interruzione per ricordare che è stata servita
	 */

	TIM2->CNT = 0;

	// impostiamo la modalità di uscita su force active (101)
	TIM2->CCMR1 &= ~TIM_CCMR1_OC1M_Msk;
	TIM2->CCMR1 |= (TIM_CCMR1_OC1M_0 | TIM_CCMR1_OC1M_2);


	// rimettiamo incative on match (010) per azzerarsi quando arriva arr
	TIM2->CCMR1 &= ~TIM_CCMR1_OC1M_Msk;
	TIM2->CCMR1 |= TIM_CCMR1_OC1M_1;


	TIM2->SR &= ~TIM_SR_UIF_Msk;
	TIM2->CR1 |= TIM_CR1_CEN_Msk;

	EXTI->PR |= EXTI_PR_PR13_Msk;

}
