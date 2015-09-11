/*
 * Written by Victor Clément <victor.clement@openwide.fr>
 * Parts of this code are from: https://balau82.wordpress.com
 */

#include <stdint.h>

/* UART registers */
#define UART0_BASE_ADDR 0x101f1000
#define UART0_DR (*((volatile uint32_t *)(UART0_BASE_ADDR + 0x000)))
#define UART0_IMSC (*((volatile uint32_t *)(UART0_BASE_ADDR + 0x038)))

/* Timer registers */
#define TIMER0_BASE_ADDR 0x101e2000
#define TIMER0_DR (*((volatile uint32_t *)(TIMER0_BASE_ADDR + 0x000)))
#define TIMER0_CTL (*((volatile uint32_t *)(TIMER0_BASE_ADDR + 0x008)))
#define TIMER0_CLR (*((volatile uint32_t *)(TIMER0_BASE_ADDR + 0x00C)))
/* Masked interrupt status register */
#define TIMER0_MIS (*((volatile uint32_t *)(TIMER0_BASE_ADDR + 0x014)))

/* GPIOs registers */
#define GPIO_BASE_ADDR 0x101e4000
#define GPIO0_DATA (*((volatile uint32_t *)(GPIO_BASE_ADDR + 0x004)))
#define GPIO2_DATA (*((volatile uint32_t *)(GPIO_BASE_ADDR + 0x0010)))
#define GPIO_DIR (*((volatile uint32_t *)(GPIO_BASE_ADDR + 0x400)))
/* Interrupt mask register (enable or not interrupt on pin) */
#define GPIO_IE (*((volatile uint32_t *)(GPIO_BASE_ADDR + 0x410)))
/* Interrupt event register (rising or falling edge interrupt) */
#define GPIO_IEV (*((volatile uint32_t *)(GPIO_BASE_ADDR + 0x40C)))
/* Masked interrupt status register */
#define GPIO_MIS (*((volatile uint32_t *)(GPIO_BASE_ADDR + 0x418)))
/* Interrupt clear register */
#define GPIO_IC (*((volatile uint32_t *)(GPIO_BASE_ADDR + 0x41C)))

/* Vectorized Interuption Controller registers */
#define VIC_BASE_ADDR 0x10140000
#define VIC_INTENABLE (*((volatile uint32_t *)(VIC_BASE_ADDR + 0x010)))

void print_uart0(const char *s) {
	while(*s != '\0') { /* Loop until end of string */
		UART0_DR = (unsigned int)(*s); /* Transmit char */
		s++; /* Next char */
	}
}

int toogle() {

	if ((GPIO0_DATA&(1<<0)) == (1<<0)) {
		GPIO0_DATA &= ~(1<<0);
	}
	else {
		GPIO0_DATA |= 1<<0;
	}

}

void __attribute__((interrupt)) irq_handler() {
	if ((GPIO_MIS&(1<<1)) == (1<<1)) {
		TIMER0_CTL |= 1<<7;
		GPIO_IC |= 1<<1;
		//print_uart0("Interrupt from GPIO 1\n");
	}
	else if (TIMER0_MIS == 0x01) {
		TIMER0_CTL &= ~(1<<7);
		TIMER0_CLR = 1;
		toogle();
		// print_uart0("Interrupt from timer 0\n");
	}
	else {
		print_uart0("Unknow interrupt source\n");
	}
}

void unhandled_irq() {
	print_uart0("Unhandled IRQ handler reached. This is the end...\n");
	for(;;);
}

/* all other handlers are infinite loops */
void __attribute__((interrupt)) undef_handler(void) { unhandled_irq(); }
void __attribute__((interrupt)) swi_handler(void) { unhandled_irq(); }
void __attribute__((interrupt)) prefetch_abort_handler(void) { unhandled_irq(); }
void __attribute__((interrupt)) data_abort_handler(void) { unhandled_irq(); }
void __attribute__((interrupt)) fiq_handler(void) { unhandled_irq(); }

void copy_vectors(void) {
	extern uint32_t vectors_start;
	extern uint32_t vectors_end;
	uint32_t *vectors_src = &vectors_start;
	uint32_t *vectors_dst = (uint32_t *)0;

	while(vectors_src < &vectors_end)
		*vectors_dst++ = *vectors_src++;
}

void main(void) {
	print_uart0("Starting\n");

	/* Setting GPIO direction */
	// Pin 0: output signal
	GPIO_DIR = 1<<0;
	// Pin 1: input trigger
	GPIO_DIR |= 0<<1;
	// Pin 2: start signal
	GPIO_DIR |= 1<<2;

	/* Enable Timer0 IRQ */
	// Enable Timer0 IRQ in interrupt controller
	VIC_INTENABLE |= 1<<4;
	// Enable GPIO0 IRQ in interrupt controller
	VIC_INTENABLE |= 1<<6;
	// Set timer load
	TIMER0_DR = 0x2710;
	// Set periodic mode (needed by oneshot mode)
	TIMER0_CTL |= 1<<6;
	// Set oneshot mode
	TIMER0_CTL |= 1<<0;
	// Set GPIO IRQ in on pin 1, rising
	GPIO_IEV |= 1<<1;
	GPIO_IE |= 1<<1;

	print_uart0("Initialized\n");

	// Send start signal on GPIO pin 2
	GPIO2_DATA |= 1<<2;

	for(;;)
		// Go to doze (sleep) mode, wait for interrupts
		asm("MCR p15, 0, r1, c7, c0, 4 \n");
}
