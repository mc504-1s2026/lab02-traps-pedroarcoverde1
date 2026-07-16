#include <arch/timer.h>
#include <kernel/mm.h>
#include <kernel/printf.h>
#include <kernel/serial.h>
#include <kernel/string.h>
#include <kernel/trap.h>
#include <stdint.h>

extern volatile int command_ready;
extern int _hartid[];
extern uint64_t boot_time = 0;

void print_uptime(uint64_t segs) {
  if (segs == 0) {
    serial_puts("0s\n");
    return;
  }
  char buf[32];
  int pos = 0;
  while (segs > 0) {
    buf[pos++] = (segs % 10) + '0';
    segs /= 10;
  }
  while (pos > 0) {
    pos--;
    serial_putc(buf[pos]);
  }
  serial_puts("s\n");
}

void kmain() {
  printk_set_level(LOG_DEBUG);
  info("entered S-mode\n");
  info("booting on hart %d\n", _hartid[0]);
  info("setting up virtual memory...\n");
  vm_init();

  info("enabling traps...\n");
  trap_setup();
  info("enabling timer...\n");
  timer_irq_enable();
  boot_time = timer_read();
  info("enabling serial...\n");
  serial_init();
  serial_irq_enable();

  info("enabling global interrupts...\n");
  hart_irq_enable();

  char cmd[256];
  serial_puts("> ");

  while (1) {
    // O kmain fica girando aqui. Assim que a interrupção mudar a variável para
    // 1, ele entra no IF.
    if (command_ready) {
      // Lemos o buffer preenchido pela interrupção
      size_t len = serial_read(cmd);

      // Garantimos que a string lida termine com o caractere nulo (para as
      // funções de string funcionarem)
      if (len > 0 && cmd[len - 1] == '\n') {
        cmd[len - 1] = '\0'; // Troca o Enter final por um \0
      } else {
        cmd[len] = '\0';
      }

      if (cmd[0] == '\0') {
        command_ready = 0;
        continue;
      }

      // O seu processador de comandos!
      if (strncmp(cmd, "uptime", 6) == 0) {
        uint64_t current_time = timer_read();
        uint64_t uptime_segundos = (current_time - boot_time) / 10000000;
        print_uptime(uptime_segundos);
      } else if (strncmp(cmd, "echo ", 5) == 0) {
        serial_puts(cmd + 5);
        serial_putc('\n');

      } else if (strncmp(cmd, "alarm ", 6) == 0) {
        uint64_t alarm_time_secs = 0;
        char *p = cmd + 6;

        while (*p >= '0' && *p <= '9') {
          alarm_time_secs = alarm_time_secs * 10 + (*p - '0');
          p++;
        }

        if (alarm_time_secs > 0) {
          uint64_t current_time = timer_read();
          // Converter segundos para ticks multiplicando por 10.000.000
          uint64_t ticks = alarm_time_secs * 10000000;
          timer_set_alarm(current_time + ticks);

          // Habilitar a interrupção do timer para ouvirmos o disparo
          timer_irq_enable();

          command_ready = 0;
          continue;
        }
      } // Abaixamos a flag e imprimimos o prompt para o próximo comando
      command_ready = 0;
      serial_puts("> ");
    }
  }
}
