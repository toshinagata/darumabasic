#include "kernel.h"
#include "wiring.h"
#include <stdio.h>   /*  For snprintf()  */
#include <string.h>  /*  For strerror()  */
#include <errno.h>   /*  For errno  */
#include <sys/time.h>  /*  For gettimeofday()  */
#include <sys/times.h>   /*  For implementation of _times()  */

__attribute__ ((interrupt ("IRQ"))) void interrupt_irq() {
#ifdef HAVE_USPI
    USPiInterruptHandler ();
#endif
}

typedef volatile unsigned int vu32_t;

// UART peripheral
#define PHY_PERI_ADDR(x) (x)

//#define UART0_BASE		0x00201000
#define UART0_DR		((vu32_t *)PHY_PERI_ADDR(UART0_BASE + 0x00))
#define UART0_FR		((vu32_t *)PHY_PERI_ADDR(UART0_BASE + 0x18))
#define UART0_IBRD		((vu32_t *)PHY_PERI_ADDR(UART0_BASE + 0x24))
#define UART0_FBRD		((vu32_t *)PHY_PERI_ADDR(UART0_BASE + 0x28))
#define UART0_LCRH		((vu32_t *)PHY_PERI_ADDR(UART0_BASE + 0x2c))
#define UART0_CR		((vu32_t *)PHY_PERI_ADDR(UART0_BASE + 0x30))

//#define GPIO_BASE	(0x00200000)
#define GPFSEL0		((vu32_t *)PHY_PERI_ADDR(PERIPHERAL_BASE + GPIO_BASE + 0x00))
#define GPFSEL1		((vu32_t *)PHY_PERI_ADDR(PERIPHERAL_BASE + GPIO_BASE + 0x04))
#define GPFSEL2		((vu32_t *)PHY_PERI_ADDR(PERIPHERAL_BASE + GPIO_BASE + 0x08))
#define GPFSEL3		((vu32_t *)PHY_PERI_ADDR(PERIPHERAL_BASE + GPIO_BASE + 0x0c))
#define GPFSEL4		((vu32_t *)PHY_PERI_ADDR(PERIPHERAL_BASE + GPIO_BASE + 0x10))
#define GPFSEL5		((vu32_t *)PHY_PERI_ADDR(PERIPHERAL_BASE + GPIO_BASE + 0x14))
#define GPSET0		((vu32_t *)PHY_PERI_ADDR(PERIPHERAL_BASE + GPIO_BASE + 0x1c))
#define GPSET1		((vu32_t *)PHY_PERI_ADDR(PERIPHERAL_BASE + GPIO_BASE + 0x20))
#define GPCLR0		((vu32_t *)PHY_PERI_ADDR(PERIPHERAL_BASE + GPIO_BASE + 0x28))
#define GPCLR1		((vu32_t *)PHY_PERI_ADDR(PERIPHERAL_BASE + GPIO_BASE + 0x2c))
#define GPLEV0		((vu32_t *)PHY_PERI_ADDR(PERIPHERAL_BASE + GPIO_BASE + 0x34))
#define GPLEV1		((vu32_t *)PHY_PERI_ADDR(PERIPHERAL_BASE + GPIO_BASE + 0x38))
#define GPFSEL_MASK_ALT0(n)	(0x04 << ((n % 10) * 3))

static void setALT0(int pin)
{
  volatile unsigned int *res;
  
  // GPFSEL select
	if (0 <= pin && pin <= 9) {
		res = GPFSEL0;
	} else if (pin <= 19) {
		res = GPFSEL1;
	} else if (pin <= 29) {
		res = GPFSEL2;
	} else if (pin <= 39) {
		res = GPFSEL3;
	} else if (pin <= 49) {
		res = GPFSEL4;
	} else if (pin <= 53) {
		res = GPFSEL5;
	} else {
		// pin mismatch
		return;
	}
  
  // Set ALT0
  *res |= GPFSEL_MASK_ALT0(pin);

	return;
}

int uart_init(void)
{
	/****	初期設定開始	***/
	// UART無効化
	*UART0_CR 	= 0;
	
	//ポートの設定
	pinMode(14, INPUT_PULLDOWN);
  pinMode(15, INPUT_PULLDOWN);
  
  //  set pinMode ALT0 for gpio 14 and 15
  setALT0(14);
  setALT0(15);

	// ボーレートの設定
	//(3000000 / (16 * 115200) = 1.627
	//(0.627*64)+0.5 = 40
	// IBRD 1 FBRD 40
	*UART0_IBRD = 1;
	*UART0_FBRD = 40;

	// LCRH
	// stick parity dis, 8bit, FIFO en, two stop bit no, odd parity, parity dis, break no
	*UART0_LCRH = 0x70;
  
	// CTS dis, RTS dis, OUT1-2=0, RTS dis, DTR dis, RXE en, TXE en, loop back dis, SIRLP=0, SIREN=0, UARTEN en
	*UART0_CR 	= 0x0301;
	/****	初期設定終了	***/
	return 0;
}

int uart_getch(void)
{
  if ((*UART0_FR & 0x10) != 0)
    return -1;
  return (*UART0_DR) & 0xff;
}

int uart_putch(int c)
{
  while ((*UART0_FR & 0x20) != 0);
  *UART0_DR = (c & 0xff);
  return c;
}

int tty_puts(const char *s)
{
  while (*s != 0) {
    uart_putch(*s & 0xff);
    s++;
	  usleep(1000);
  }
  return 0;
}

int tty_getch(void)
{
	int n = getch();
	if (n == -1)
		n = uart_getch();
	return n;
}

int
lm_getch_raw(void)
{
	return tty_getch();
}

/*  This implementation is missing from the Maccasoft kernel  */
clock_t
_times (struct tms *tp)
{
	clock_t timeval;
	struct timeval tv;
	gettimeofday(&tv, NULL);
	timeval = tv.tv_sec;
	if (tp != NULL) {
		tp->tms_utime = timeval;
		tp->tms_stime = 0;
		tp->tms_cutime = 0;
		tp->tms_cstime = 0;
	}
	return timeval;
}

int baremetal_init(void)
{
	int n;
	static char msg[80];

	n = uart_init();
	if (n != 0)
		return n;
	
	if (mount("sd:") != 0) {
		snprintf(msg, sizeof msg, "\r\nSD CARD NOT MOUNTED (%s)\r\n", strerror(errno));
		tty_puts(msg);
	} else {
		tty_puts("\r\nSD Card mounted\r\n");
	}

    usb_init();
    if (keyboard_init() != 0) {
        tty_puts("\r\nNO KEYBOARD DETECTED\r\n");
    }
	
	return 0;
}
