#include "ch32v003fun.h"
#include <stdio.h>

// I2C Address of this slave (7-bit)
#define SLAVE_ADDRESS 0x3C

// --- Dummy Data Definitions ---
#define dummy1 0x03
#define dummy2 0x04
#define dummy3 0x05

uint8_t dummy1_value[] = {0x12, 0x34, 0x56, 0x78};
uint8_t dummy2_value[] = {0x01, 0x00, 0xFF, 0xAA, 0xAA, 0xAA, 0xAA, 0xAb};
uint8_t dummy3_value[] = {0xAA, 0xAA, 0xCC};

// --- Globals ---
volatile int I2C_slave_read_byte_counter = 0;
uint8_t *volatile I2C_variable_ptr = NULL;
volatile uint32_t I2C_variable_size = 0;

// --- Helper Functions ---

uint8_t *get_register_address(int I2C_transaction_register_id)
{
	switch (I2C_transaction_register_id)
	{
	case dummy1:
		I2C_variable_size = sizeof(dummy1_value);
		return dummy1_value;
	case dummy2:
		I2C_variable_size = sizeof(dummy2_value);
		return dummy2_value;
	case dummy3:
		I2C_variable_size = sizeof(dummy3_value);
		return dummy3_value;
	default:
		I2C_variable_size = 0;
		return NULL;
	}
}

// --- Logic Handlers ---

void I2C_ADDR_handler(int is_master_reading)
{
	// If Master is requesting to Read (DIR=1)
	if (is_master_reading)
	{
		I2C_slave_read_byte_counter = 0;
	}
}

void I2C_slave_read_byte_handler(void)
{
	// Read data from register
	uint8_t data = I2C1->DATAR;

	if (I2C_slave_read_byte_counter == 0)
	{
		// First byte received is the Register ID
		I2C_variable_ptr = get_register_address(data);
	}
	else
	{
		// Subsequent bytes are data to be written to that register
		if (I2C_variable_ptr != NULL && (I2C_slave_read_byte_counter - 1) < I2C_variable_size)
		{
			I2C_variable_ptr[I2C_slave_read_byte_counter - 1] = data;
		}
	}
	I2C_slave_read_byte_counter++;
}

void I2C_slave_write_byte_handler(void)
{
	if (I2C_variable_ptr != NULL && I2C_slave_read_byte_counter < I2C_variable_size)
	{
		I2C1->DATAR = I2C_variable_ptr[I2C_slave_read_byte_counter];
	}
	else
	{
		I2C1->DATAR = 0xFF; // Send dummy data if out of bounds
	}
	I2C_slave_read_byte_counter++;
}

void I2C_STOPF_handler(void)
{
	I2C_slave_read_byte_counter = 0;

	// Clear STOPF (Hardware sequence: Read STAR1 (done in ISR), Write CTLR1)
	// We rewrite the current value of CTLR1 to clear the flag
	I2C1->CTLR1 = I2C1->CTLR1;
}

// --- Interrupt Service Routines ---

// Forward declaration of ISRs
void I2C1_EV_IRQHandler(void) __attribute__((interrupt));
void I2C1_ER_IRQHandler(void) __attribute__((interrupt));

void I2C1_EV_IRQHandler(void)
{
	uint16_t star1 = I2C1->STAR1;
	uint16_t star2;

	// 1. ADDR: Address Matched
	if (star1 & I2C_STAR1_ADDR)
	{
		// Must read STAR1 (already done) then STAR2 to clear ADDR flag
		star2 = I2C1->STAR2;

		// Check TRA bit in STAR2 (1 = Transmitter/Slave-TX, 0 = Receiver/Slave-RX)
		int is_master_reading = (star2 & I2C_STAR2_TRA) ? 1 : 0;

		I2C_ADDR_handler(is_master_reading);
	}

	// 2. RXNE: Receive Not Empty (Master Write)
	if (star1 & I2C_STAR1_RXNE)
	{
		I2C_slave_read_byte_handler();
	}

	// 3. TXE: Transmit Buffer Empty (Master Read)
	if ((star1 & I2C_STAR1_TXE) && (I2C1->STAR2 & I2C_STAR2_TRA))
	{
		I2C_slave_write_byte_handler();
	}

	// 4. STOPF: Stop Detection
	if (star1 & I2C_STAR1_STOPF)
	{
		I2C_STOPF_handler();
	}
}

void I2C1_ER_IRQHandler(void)
{
	// Reading STAR1 clears some error flags
	volatile uint16_t star1 = I2C1->STAR1;
	(void)star1; // Prevent unused variable warning

	// If Bus Error or Arbitration Loss, reset flags
	I2C1->STAR1 &= ~(I2C_STAR1_BERR | I2C_STAR1_ARLO | I2C_STAR1_AF);
}

// --- Initialization Code ---

void I2C_Slave_Init(void)
{
	// 1. Enable Clocks (I2C1 and GPIOC)
	RCC->APB2PCENR |= RCC_APB2Periph_GPIOC;
	RCC->APB1PCENR |= RCC_APB1Periph_I2C1;

	// 2. GPIO Setup (PC2=SDA, PC1=SCL)
	// CNF=10 (Alt Open-Drain), MODE=11 (50MHz Output)
	// Clear PC1, PC2 settings
	GPIOC->CFGLR &= ~(0xFF << 4);
	// Set 0xA (Multiplexed Open Drain, 50MHz)
	GPIOC->CFGLR |= (0xA << 8) | (0xA << 4);

	// 3. Reset I2C
	I2C1->CTLR1 |= I2C_CTLR1_SWRST;
	I2C1->CTLR1 &= ~I2C_CTLR1_SWRST;

	// 4. I2C Config
	// Assuming 48MHz System Clock (Standard ch32fun default)
	// Set Clock Freq in MHz (used for timings)
	I2C1->CTLR2 = 48;

	// Set Own Address (Shifted left by 1 for 7-bit address)
	// ch32fun uses OADDR1 instead of OAR1
	I2C1->OADDR1 = (SLAVE_ADDRESS << 1);
	I2C1->OADDR2 = 0;

	// Enable I2C, Enable ACK
	I2C1->CTLR1 = I2C_CTLR1_PE | I2C_CTLR1_ACK;

	// 5. Interrupt Enable
	I2C1->CTLR2 |= I2C_CTLR2_ITBUFEN | I2C_CTLR2_ITEVTEN | I2C_CTLR2_ITERREN;

	// Enable NVIC for I2C1 Event and Error
	NVIC_EnableIRQ(I2C1_EV_IRQn);
	NVIC_EnableIRQ(I2C1_ER_IRQn);
}

int main()
{
	SystemInit();

	// Optional: Setup printf for debugging (PD1)
	// SetupDebugPrintf();
	// printf("I2C Slave Starting...\n");

	I2C_Slave_Init();

	while (1)
	{
		// Main loop
	}
}