#include "main.h"

#define PRESCALER_VALUE_1kHz (unsigned int) 15999
#define ARR_VALUE (unsigned int) 999

void clock_enable();
void led_pa5_config();
void button_pc13_config();

void TIM2_config(unsigned int,unsigned int);

int main(void){

	clock_enable();
	led_pa5_config();
	button_pc13_config();
	TIM2_config(PRESCALER_VALUE_1kHz,ARR_VALUE);

	_Bool update_led=0;

	/*
	 * PSEUDOCODICE
	 * if Il pulsante è premuto{
	 * 		if il led non è stato ancora acceso per questo evento di pressione{
	 * 			Accendiamo il led
	 * 			Riportiamo il contatore del timer a 0
	 * 			Azzeriamo il bit che ci segnala un evento sul timer (potrebbe essersi alzato tra quando abbiamo rilevato la pressione del timer e quando abbiamo azzerato il contatore)
	 * 			Avviamo il contatore (se stava già contanto e quindi era già alto questa azione è nulla sovrascrivendo un uno ad un uno)
	 * 			Aggiorniamo a 1 (True) la variabile che ci ricorda che per questo evento pressione il led è stato aggiornato a
	 * }else [se il pulsante è stato rilasciato]{
	 * 		Aggiorniamo a 0 (False) la variabile che ci ricorda dell'aggiornamento del led in modo da ricordarci che alla prossima pressione del bottone il led va aggiornato
	 * }
	 *
	 * if si è sollevato il bit di un evento sul TIM2 [nel nostro caso overflow del valore di CNT su ARR]{
	 * 		Spegnamo il led
	 * 		Resettiamo il bit dell'evento sul TIM2
	 * }
	 */

	while(1){
		if((GPIOC->IDR & GPIO_IDR_ID13_Msk)!=GPIO_IDR_ID13_Msk){
			if (!update_led){
				GPIOA->ODR |= GPIO_ODR_OD5_Msk;
				//Tecnicamente il timee è in OPM quindi teoricamente CNT dovrebbe azzerarsi da solo quando CNT supera ARR e si genera l'evento
				//Ma poiché vogliamo che se, durante un conteggio, i pulsante viene premuto il timer ricominci d'accapo, allora è necessario questa riga
				TIM2->CNT=0;
				TIM2->SR &= ~TIM_SR_UIF_Msk;
				TIM2->CR1 |= TIM_CR1_CEN_Msk;
				update_led=1;
			}
		}else{
			update_led=0;
		}

		if((TIM2->SR & TIM_SR_UIF_Msk)==TIM_SR_UIF_Msk){
			GPIOA->ODR &= ~GPIO_ODR_OD5_Msk;
			TIM2->SR &= ~TIM_SR_UIF_Msk;
			//Non è necessario fermare il contatore poichè in OPM il bit CEN si azzera da solo (e quindi si ferma di nuvo a 0) una volta superato il valore presente in ARR.
			//Inoltre, sempre in OPM, il CNT si ferma a 0
			//REF. Pagina 398 manuale "2 - STM32 F401xE Reference Manual"
		}
	}

	return 0;
}

void clock_enable(){
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

	return;
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

