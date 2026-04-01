#include "main.h"

//Questi due valori vannod efiniti a mano poiché non sono presenti nei file forniti
#define NVIC_ISER_EXTI15_10_IR_EN (unsigned int) (1<<8)
#define NVIC_ISER_TIM2_IR_EN (unsigned int) (1<<28)

#define PRESCALER_VALUE_16MHz_to_1kHz (unsigned int) 15999
#define ARR_VALUE (unsigned int) 999

void clock_config(void);
void led_pa5_config(void);
void button_pc13_config(void);
void TIM2_config(unsigned int,unsigned int);
void interruption_config(void);

int main (void){

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

	//Settiamo la porta pa5 (il led) in modalità output (01)
	//RIF. Pagina 158 manuale "2 - STM32 F401xE Reference Manual"
	GPIOA->MODER |= GPIO_MODER_MODE5_0; //Bit 0 del MODER5
	GPIOA->MODER &= ~GPIO_MODER_MODE5_1; //Bit 1 del MODER5

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

	//Inizializziamo il led come spento attraverso il valore che pilota il segnale sul PA5 nell' output register della GPIOA
	//REF. Pagina 160 manuale "2 - STM32 F401xE Reference Manual"
	GPIOA->ODR &= ~GPIO_ODR_OD5_Msk;
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

	//Abilitiamo il bit che permette al TIM2 di generare interruzioni interne
	//REF Pagina 358 manuale "2 - STM32 F401xE Reference Manual"
	TIM2->DIER |= TIM_DIER_UIE_Msk;

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

	//Abilitiamo l'interruzione realtiva al TIM2 in modo da permettere al processore di attivare la realtiva subroutine. Se non attiviamo l'interruzione viene vista come in attesa ma non sarà mai attivata la corrispondente procedura dall'NVIC
	// REF Pagina 202 manuale "2 - STM32 F401xE Reference Manual"
	NVIC->ISER[0] |= NVIC_ISER_TIM2_IR_EN; //Si attiva mettendo a 1 bit 28 del primo regisro ISER (2)

	//Abilitiamo l'interruzione realtiva alla linea di EXTI da 10 a 15 (occhio a se si usa più di una, vanno verificate una a una le periferiche a cui ogni linea è collegata)
	// REF Pagina 202 manuale "2 - STM32 F401xE Reference Manual"
	NVIC->ISER[1] |= NVIC_ISER_EXTI15_10_IR_EN; //Si attiva mettendo a 1 il bit 8 sel secondo resitro ISER (2)

	//(1) Per riconoscere il fronte di salita va impostato 1 nel registro RTSR). Si può generare un evento sia sul fronte di salita che su quello di discesa
	//REF Pagina 210 manuale "2 - STM32 F401xE Reference Manual"

	//(2) Il vettore ISER si divide fisicamente in due registri, quindi tutte le interruzioni che hanno una posizione >=32
	//Hanno il bit da attivare nel registro NVIC->ISER[1] la cui posizione si calcola come posizione_nella_tabella-32
	//Esempio, la NVIC_ISER_EXTI15_10_IR_EN è alla posizione 40 nella tabella, 40-32=8, si trova al bit 8 nel registro fisico
	// REF Pagina 202 manuale "2 - STM32 F401xE Reference Manual"

}


void TIM2_IRQHandler(void){

	/*
	 * PSEUDOCODICE
	 *
	 * All'arrivo dell'interruzione dal TIM2{
	 * 		Si spegne il led
	 * 		Si azzera il bit del TIM che segnala l'interruzione
	 * }
	 */

	GPIOA->ODR &= ~GPIO_ODR_OD5_Msk;
	TIM2->SR &= ~TIM_SR_UIF_Msk;

	//Non è necessario fermare il contatore poichè in OPM il bit CEN si azzera da solo (e quindi si ferma di nuvo a 0) una volta superato il valore presente in ARR.
	//Inoltre, sempre in OPM, il CNT si ferma a 0, quindi non si deve azzerare a mano
	//REF. Pagina 398 manuale "2 - STM32 F401xE Reference Manual"

}

void EXTI15_10_IRQHandler (void){

	/*
	 * PSEUDOCODICE
	 * All'arrivo sell'interruzione sulla linea EXTI13 (il pulsante premuto){
	 * 		Accendiamo il led
	 * 		Riportiamo il contatore del timer a 0
     *		Azzeriamo il bit che ci segnala un evento sul timer (potrebbe essersi alzato tra quando abbiamo rilevato la pressione del timer e quando abbiamo azzerato il contatore)
     *		Avviamo il contatore (se stava già contanto e quindi era già alto questa azione è nulla sovrascrivendo un uno ad un uno)
     *		Resettiamo lo status dell'interruzione per ricordare che è stata servita
	 * }
	 */

	GPIOA->ODR |= GPIO_ODR_OD5_Msk;
	TIM2->CNT = 0;
	TIM2->SR &= ~TIM_SR_UIF_Msk;
	TIM2->CR1 |= TIM_CR1_CEN_Msk;

	EXTI->PR |= EXTI_PR_PR13_Msk;

}

