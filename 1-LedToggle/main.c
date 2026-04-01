#include "main.h"

void clock_enable();
void led_pa5_config();
void button_pc13_config();

int main(void){

	clock_enable();

	led_pa5_config();

	button_pc13_config();

	//Non è altro che il tipo booleano nativo di C, un byte(in realtà dipende dall'architettura poi) in cui viene settato solo l'ultimo bit e su cui possiamo fare
	//tutte le operazioni che facciamo su bool
	_Bool update_led_status=0;

	/*
	 * Pseudocodice
	 * If il bottone è premuto{
	 * 		If il led non è stato ancora commutato per questo evento di pressione{
	 * 			If il valore di uscita sulla PA5 è 0{
	 * 				Impostiamo 1 per accendere il led
	 * 			}else{
	 * 				Impostiamo 0 per spegnere il led
	 * 			}
	 * 			Ricordiamoci di aver già aggiornato il led nel corso dell'evento attuale portando a 1 la variabile
	 * 		}
	 *  }else [se il bottone non è premuto]{
	 *  	Portiamo a 0 la variabile per ricordarci che alla prossima occorrenza di pressione il led và aggiornato
	 *  }
	 */

	while(1){

		if((GPIOC->IDR & GPIO_IDR_ID13_Msk)!=GPIO_IDR_ID13_Msk){
			if(!update_led_status){
				if((GPIOA->ODR & GPIO_ODR_OD5_Msk)!=GPIO_ODR_OD5_Msk){
					GPIOA->ODR |= GPIO_ODR_OD5_Msk;
				}else{
					GPIOA->ODR &= ~GPIO_ODR_OD5_Msk;
				}
				update_led_status=1;
			}
		}else{
			update_led_status=0;
		}
	}

	return 0;
}

void clock_enable(){
	//Abilitiamo il clock alla GPIOA (per il led) e alla GPIOC (per il pulsante)
	//RIF. Pagina 118 manuale "2 - STM32 F401xE Reference Manual"
	RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN_Msk | RCC_AHB1ENR_GPIOCEN_Msk;

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
		//RIF. Pagina 158 manuale "2 - STM32 F401xE Reference Manual"
	GPIOC->MODER &= ~GPIO_MODER_MODE13_Msk;

	GPIOC->PUPDR &= ~GPIO_MODER_MODE13_Msk;

	//I registri OTYPER e OSPEEDR controllano solo la parte di output della GPIO, qui non servono

	return;

}
