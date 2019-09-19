#include "chip.h"
#define TIC_SISTEMA (100)
#define DHT22_PORT	(2)
#define DHT22_PIN	(3)
#define LED_PORT	(0)
#define LED_PIN		(22)

/****************************************************************************************
 * Function Name : void DemorauS(uint32_t);
 * Description : Blocking delay
 * Input : Time in Microseconds
 * Output :
 * Void Note :
****************************************************************************************/
void DemorauS(uint32_t micros){
	uint32_t time = 0;
	uint32_t i,j;

	DWT->CYCCNT=0;
	for(i=0 ; i<micros ; i++){
		for (j=0 ; j<8 ; j++);
	}
	time = DWT->CYCCNT;
}

void esperaUno(void)
{
	int pin = 0;
	do{
		pin = Chip_GPIO_ReadPortBit(LPC_GPIO, DHT22_PORT, DHT22_PIN);
		DemorauS(1);
	}while((!pin) && (Chip_GPIO_ReadPortBit(LPC_GPIO, DHT22_PORT, DHT22_PIN)));
	pin = Chip_GPIO_ReadPortBit(LPC_GPIO, DHT22_PORT, DHT22_PIN);
}

void esperaCero(void)
{
	int pin = 0;
	do{
		pin = Chip_GPIO_ReadPortBit(LPC_GPIO, DHT22_PORT, DHT22_PIN);
		DemorauS(1);
	}while(pin);
	pin = Chip_GPIO_ReadPortBit(LPC_GPIO, DHT22_PORT, DHT22_PIN);
}

void TareaSensor(void)
{
	static int espera = 100;
	static int estado = 0;

	int i,j,aux;
	uint8_t val=0;
	uint8_t trama[5];
	uint32_t checksum = 0;
	uint32_t humedad = 0;
	uint32_t temperatura = 0;
	bool muestra[450];
	uint32_t uno =0;
	uint32_t cero =0;
	uint32_t time = 0;

	if(!--espera)
	{
		espera = 100;
		estado = 1;
	}

	if(estado)
	{
		estado--;
		Chip_GPIO_SetPinDIROutput(LPC_GPIO, DHT22_PORT, DHT22_PIN);
	}
	else
	{
		for(i=0;i<5;i++){
			trama[i]=0;
		}
		uno = 0;
		cero = 0;

		Chip_GPIO_SetPinDIRInput(LPC_GPIO, DHT22_PORT, DHT22_PIN);
		esperaCero();

		DWT->CYCCNT=0;
		esperaUno();
		time = DWT->CYCCNT;
		esperaCero();

		for(j=0; j<5 ; j++){
			for(i=0;i<8;i++)
			{

				esperaUno();
				DemorauS(40);

				if(Chip_GPIO_ReadPortBit(LPC_GPIO, DHT22_PORT, DHT22_PIN))
				{
					val = 1;
					uno++;
				}else{
					val = 0;
					cero++;
				}
				trama[j] = (trama[j]<<1) & 0xFF;
				trama[j] = (trama[j]+val) & 0xFF;
			}
			aux = 1;
		}

		aux = 1;
		Chip_GPIO_SetPinToggle(LPC_GPIO,LED_PORT, LED_PIN);
		humedad = (((uint32_t)trama[0])<<8) + ((uint32_t)trama[1]);
		temperatura = (((uint32_t)trama[2])<<8) + ((uint32_t)trama[3]);

		for(j=0 ; j<4 ; j++){
			checksum += ((trama[j]) & 0xFF);
			checksum &= 0xFF;
		}
		if(checksum != trama[4]){
			for(i = 0 ; i<5 ; i++){
				trama[i] = 0xFF;
			}
		}else{
			aux = 1;
		}
	}

}

int main(void) {

	Chip_SetupXtalClocking();
	Chip_SYSCTL_SetFLASHAccess(FLASHTIM_100MHZ_CPU);
	SystemCoreClockUpdate();

	/* Initialize GPIO */
	Chip_GPIO_Init(LPC_GPIO);
	Chip_IOCON_Init(LPC_IOCON);

	Chip_IOCON_PinMuxSet(LPC_IOCON, DHT22_PORT, DHT22_PIN, IOCON_FUNC0);
	Chip_GPIO_SetPinOutLow(LPC_GPIO, DHT22_PORT, DHT22_PIN);
	Chip_GPIO_SetPinDIRInput(LPC_GPIO, DHT22_PORT, DHT22_PIN);

	Chip_GPIO_SetPinDIROutput(LPC_GPIO, LED_PORT, LED_PIN);
	Chip_GPIO_SetPinOutHigh(LPC_GPIO, LED_PORT, LED_PIN);

	/*Inicializa Systick*/
	SysTick_Config(SystemCoreClock/TIC_SISTEMA); //100Hz Corre a 100MHz y cuenta hasta SystemanCr/TIC TIC es la frec que quiero.

	while(1)
	{
		TareaSensor();
		__WFI();
    }
    return 0 ;
}

void SysTick_Handler(void)
{

}
