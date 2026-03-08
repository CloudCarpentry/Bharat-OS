/* hal_serial.h — Minimal x86_64 serial (COM1) debug output
 *
 * Writes directly to the UART at 0x3F8 at the default baud rate BIOS
 * already configured. No initialisation needed for QEMU's emulated 16550.
 * This is purely a bring-up / debugging facility; production use would go
 * through the full driver stack.
 */
#ifndef BHARAT_HAL_SERIAL_H
#define BHARAT_HAL_SERIAL_H

#include <stdint.h>

#define SERIAL_COM1 0x3F8

#ifdef __x86_64__
/* Write one byte to the I/O port */
static inline void outb(uint16_t port, uint8_t val) {
  __asm__ volatile("outb %0, %1" : : "a"(val), "Nd"(port));
}

/* Read one byte from the I/O port */
static inline uint8_t inb(uint16_t port) {
  uint8_t ret;
  __asm__ volatile("inb %1, %0" : "=a"(ret) : "Nd"(port));
  return ret;
}
#else
/* Dummy implementations for non-x86 architectures to satisfy compilation of trace.c */
static inline void outb(uint16_t port, uint8_t val) {
  (void)port;
  (void)val;
}

static inline uint8_t inb(uint16_t port) {
  (void)port;
  return 0;
}
#endif

/* Block until the transmit holding register is empty */
static inline void serial_wait(void) {
  while (!(inb(SERIAL_COM1 + 5) & 0x20)) {
  }
}

static inline void serial_putc(char c) {
  serial_wait();
  outb(SERIAL_COM1, (uint8_t)c);
}

static inline void serial_puts(const char *s) {
  while (*s) {
    if (*s == '\n')
      serial_putc('\r');
    serial_putc(*s++);
  }
}

#endif /* BHARAT_HAL_SERIAL_H */
