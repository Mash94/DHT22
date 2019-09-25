#include "chip.h"
#define TIC_SISTEMA (100)
#define DHT22_PORT	(2)
#define TIME_PORT	(2)
#define DHT22_PIN	(3)
#define TIME_PIN	(4)
#define LED_PORT	(0)
#define LED_PIN		(22)

#define MIN_INTERVAL 2000 /**< min interval value */
#define TIMEOUT -1        /**< timeout on */
#define TIMELIMIT 10000

uint8_t data[5];

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
/****************************************************************************************
 * Function Name : uint32_t expectPulse(uint32_t);
 * Description : Blocking delay
 * Input : Time in Microseconds
 * Output :
 * Void Note :
****************************************************************************************/
uint32_t expectPulseOne(uint32_t estado){
	uint32_t time;
	DWT->CYCCNT=0;
	if(estado){
		while( (!Chip_GPIO_GetPinState(LPC_GPIO, DHT22_PORT, DHT22_PIN)) && ((int)((DWT->CYCCNT)/100) < TIMELIMIT));
	}else{
		while( (Chip_GPIO_GetPinState(LPC_GPIO, DHT22_PORT, DHT22_PIN)) && ((int)((DWT->CYCCNT)/100) < TIMELIMIT));
	}
	time = (int)((DWT->CYCCNT)/100);
	if(time >= TIMELIMIT){
		return TIMEOUT;
	}else{
		return time;
	}
}
uint32_t expectPulseZero(uint32_t estado){
	uint32_t time;
	DWT->CYCCNT=0;
	if(estado){
		while( (!Chip_GPIO_GetPinState(LPC_GPIO, DHT22_PORT, DHT22_PIN)) && ((int)((DWT->CYCCNT)/100) < TIMELIMIT));
	}else{
		while( (Chip_GPIO_GetPinState(LPC_GPIO, DHT22_PORT, DHT22_PIN)) && ((int)((DWT->CYCCNT)/100) < TIMELIMIT));
	}
	time = (int)((DWT->CYCCNT)/100);
	if(time >= TIMELIMIT){
		return TIMEOUT;
	}else{
		return time;
	}
}
/****************************************************************************************
 * Function Name : void DHT22_update(void);
 * Description : Blocking read
 * Input :
 * Output :
 * Void Note :
****************************************************************************************/
void DHT22_update(void)
{
	static int espera = 100;
	static int estado = 0;;

    // Check if sensor was read less than two seconds ago and return early to use last reading.
    //Esta función no puede llamarse más de una vez cada 2 segundos, colocar dentro del TDS y listo
    // Reset 40 bits of received data to zero.
    uint32_t cycles[80];
    bool lastresult = 0;
    uint32_t lowCycles = 0;
    uint32_t highCycles = 0;
    uint8_t i;

    //Chip_GPIO_SetPinDIRInput(LPC_GPIO, DHT22_PORT, DHT22_PIN);

	if(!--espera)
	{
		espera = 100;
		estado = 1;
	}

	if(estado)
	{
		estado--;
		Chip_GPIO_SetPinDIROutput(LPC_GPIO, DHT22_PORT, DHT22_PIN);

		for (i = 0; i < 5; i++) {
			data[i] = 0;
		}
		// Send start signal.  See DHT datasheet for full signal diagram:
		// http://www.adafruit.com/datasheets/Digital%20humidity%20and%20temperature%20sensor%20AM2302.pdf

		// Go into high impedence state to let pull-up raise data line level and
		// start the reading process.
		//Chip_GPIO_SetPinDIRInput(LPC_GPIO, DHT22_PORT, DHT22_PIN);
		//DemorauS(1000); //demora de 1 milisegundo

		//Chip_GPIO_SetPinToggle(LPC_GPIO, TIME_PORT, TIME_PIN);

		// First set data line low for a period according to sensor type
		Chip_GPIO_SetPinDIROutput(LPC_GPIO, DHT22_PORT, DHT22_PIN);
		Chip_GPIO_SetPinOutLow(LPC_GPIO, DHT22_PORT, DHT22_PIN);
		DemorauS(1100); // data sheet says "at least 1ms"

		// End the start signal by setting data line high for 40 microseconds.
		Chip_GPIO_SetPinDIRInput(LPC_GPIO, DHT22_PORT, DHT22_PIN);

		// Delay a moment to let sensor pull data line low.
		DemorauS(50);

		// Now start reading the data line to get the value from the DHT sensor.

		// Turn off interrupts temporarily because the next sections
		// are timing critical and we don't want any interruptions.

		// First expect a low signal for ~80 microseconds followed by a high signal
		// for ~80 microseconds again.
		if (expectPulseZero(0) == TIMEOUT) {
			//DEBUG_PRINTLN(F("DHT timeout waiting for start signal low pulse."));
			lastresult = false;
		}
		if (expectPulseOne(1) == TIMEOUT) {
			//DEBUG_PRINTLN(F("DHT timeout waiting for start signal high pulse."));
			lastresult = false;
		}

		// Now read the 40 bits sent by the sensor.  Each bit is sent as a 50
		// microsecond low pulse followed by a variable length high pulse.  If the
		// high pulse is ~28 microseconds then it's a 0 and if it's ~70 microseconds
		// then it's a 1.  We measure the cycle count of the initial 50us low pulse
		// and use that to compare to the cycle count of the high pulse to determine
		// if the bit is a 0 (high state cycle count < low state cycle count), or a
		// 1 (high state cycle count > low state cycle count). Note that for speed
		// all the pulses are read into a array and then examined in a later step.

		 for (i = 0; i < 81; i += 2) {
			cycles[i] = expectPulseZero(0);
			cycles[i + 1] = expectPulseOne(1);
		}
		// Timing critical code is now complete.

		// Inspect pulses and determine which ones are 0 (high state cycle count < low
		// state cycle count), or 1 (high state cycle count > low state cycle count).
		for (int i = 0; i < 41; ++i) {
			lowCycles = cycles[(2 * i)+1];
			highCycles = cycles[(2 * i) + 2];
			if ((lowCycles == TIMEOUT) || (highCycles == TIMEOUT)) { //DHT timeout waiting for pulse
				lastresult = false;
			}
			data[i / 8] <<= 1;
			// Now compare the low and high cycle times to see if the bit is a 0 or 1.
			if (highCycles > lowCycles) {
				// High cycles are greater than 50us low cycle count, must be a 1.
				data[i / 8] |= 1;
			}
			// Else high cycles are less than (or equal to, a weird case) the 50us low
			// cycle count so this must be a zero.  Nothing needs to be changed in the
			// stored data.
		}

		//   DEBUG_PRINTLN(F("Received from DHT:"));
		//   DEBUG_PRINT(data[0], HEX);
		//   DEBUG_PRINT(F(", "));
		//   DEBUG_PRINT(data[1], HEX);
		//   DEBUG_PRINT(F(", "));
		//   DEBUG_PRINT(data[2], HEX);
		//   DEBUG_PRINT(F(", "));
		//   DEBUG_PRINT(data[3], HEX);
		//   DEBUG_PRINT(F(", "));
		//   DEBUG_PRINT(data[4], HEX);
		//   DEBUG_PRINT(F(" =? "));
		//   DEBUG_PRINTLN((data[0] + data[1] + data[2] + data[3]) & 0xFF, HEX);

		// Check we read 40 bits and that the checksum matches.
		if (data[4] == ((data[0] + data[1] + data[2] + data[3]) & 0xFF)) {
			lastresult = true;
		} else {
			//DEBUG_PRINTLN(F("DHT checksum failure!"));
			lastresult = false;
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
	Chip_GPIO_SetPinDIRInput(LPC_GPIO, DHT22_PORT, DHT22_PIN);

	Chip_GPIO_SetPinDIROutput(LPC_GPIO, LED_PORT, LED_PIN);
	Chip_GPIO_SetPinOutHigh(LPC_GPIO, LED_PORT, LED_PIN);

	Chip_GPIO_SetPinDIROutput(LPC_GPIO, TIME_PORT, TIME_PIN);
	Chip_GPIO_SetPinOutHigh(LPC_GPIO, TIME_PORT, TIME_PIN);


	/*Inicializa Systick*/

	SysTick_Config(SystemCoreClock/200); //200Hz  = 5mS Corre a 100MHz y cuenta hasta SystemanCr/TIC TIC es la frec que quiero.

	while(1)
	{
		DHT22_update();
		__WFI();
    }
    return 0 ;
}

void SysTick_Handler(void)
{

}
