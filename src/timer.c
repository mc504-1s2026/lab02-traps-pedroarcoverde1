#include <arch/csr.h>
#include <arch/timer.h>
#include <kernel/panic.h>
#include <kernel/serial.h>

#define TIMER_FREQ 10000000 // frequência do timer em Hz (10 MHz)

u64 timer_read() {
  // fuinção para ler o tempo atual do timer, usando o CSR time
  return csr_read(CSR_TIME);
}

void timer_irq_enable() {
  // verficar se a interrupção do timer está habilitada, usando o CSR sie
  csr_set(CSR_SIE,
          (CSR_SIE_STIE)); // habilita a interrupção do timer (bit 5 do CSR sie)
}

void timer_irq_disable() {
  // função para desabilitar a interrupção do timer, usando o CSR sie
  csr_clear(
      CSR_SIE,
      (CSR_SIE_STIE)); // desabilita a interrupção do timer (bit 5 do CSR sie)
}

void timer_set_alarm(u64 secs) {
  // função para configurar o alarme do timer
  csr_write(CSR_STIMECMP, secs);
}

void timer_irq() {
  serial_puts("alarm\n>");
  timer_irq_disable();
}
