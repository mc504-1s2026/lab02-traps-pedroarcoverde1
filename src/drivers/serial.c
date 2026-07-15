#include <kernel/serial.h>
#include <kernel/panic.h>
#include <arch/csr.h>
#include <arch/plic.h>
#include <arch/spinlock.h>

#define SERIAL_BUF_SIZE 256
extern void hart_irq_disable(void);
extern void hart_irq_enable(void);
volatile int command_ready = 0; /* flag para indicar que um comando foi recebido */

struct serialdev {
    char buf[SERIAL_BUF_SIZE]; /* buffer guardando os dados recebidos */
    size_t len;                /* número de bytes armazenados atualmente */
    struct spinlock lock;      /* trava para proteger os dados */
} dev;

static void serial_write_reg(u64 offset, u8 value)
{
    volatile u8 *reg = (volatile u8 *)SERIAL_BASE + offset;
    *reg = value;
}

static u8 serial_read_reg(u64 offset)
{
    volatile u8 *reg = (volatile u8 *)SERIAL_BASE + offset;
    return *reg;
}

void serial_init()
{
	// habilitar escrita
	serial_write_reg(SERIAL_IER, SERIAL_IER_ERBFI); // ligar as interrupções de escrita
	serial_write_reg(SERIAL_FCR, SERIAL_FCR_FIFO_ENABLE); // ativar o FIFO
}

void serial_irq_enable()
{
	csr_set(CSR_SIE, CSR_SIE_SEIE); // habilitar interrupções de software
	u32 uart_irq = 10;
    u32 hart_id = 0;

    // 2. Configurar a Prioridade
    // Damos a prioridade 1 (poderia ser qualquer valor > 0)
    plic_irq_set_priority(uart_irq, 1);

    // 3. Abaixar a Guarda (Threshold)
    // Configuramos o limiar do hart 0 para 0, aceitando interrupções de prioridade 1 ou mais
    plic_hart_set_threshold(hart_id, 0);

    // 4. Habilitar no PLIC
    // Ligamos o fio do IRQ 10 especificamente para o hart 0
    plic_hart_enable_irq(hart_id, uart_irq);
}

void serial_irq_disable()
{
	csr_clear(CSR_SIE, CSR_SIE_SEIE); // desabilitar interrupções de software
    plic_irq_set_priority(10, 0);

}

void serial_irq(){
	u32 irq = plic_hart_claim_irq(0);
	if (irq == 0) {
		return; // Nenhuma interrupção pendente
	}

	spin_lock(&dev.lock);
	if (irq == IRQ_SERIAL) {
		// Lê o registrador de dados para limpar a interrupção
		u8 data = serial_read_reg(SERIAL_RBR);
		if(dev.len < SERIAL_BUF_SIZE) {
			// verificar se é \r ou \n
			if(data == '\r' || data == '\n') {
				data = '\n';
				command_ready = 1; // sinaliza que um comando foi recebido
			}
			dev.buf[dev.len++] = data;
			serial_putc(data);
		}
		// Se o buffer estiver cheio, o dado já foi lido e "descartado" pela leitura acima
	}
	spin_unlock(&dev.lock);

	plic_hart_complete_irq(0, irq);
}

size_t serial_read(char *buf)
{
	hart_irq_disable();     
    spin_lock(&dev.lock);
	size_t bytes_read = 0;
	while (bytes_read < dev.len) {
		buf[bytes_read] = dev.buf[bytes_read];
		bytes_read++;
	}
	dev.len = 0; // Reset the buffer length after reading
	spin_unlock(&dev.lock);
	hart_irq_enable();
	return bytes_read;
}

void serial_puts(char *str)
{
    /* Percorre a string até encontrar o terminador nulo '\0' */
    while (*str != '\0') {
        serial_putc(*str);
        str++;
    }
}

void serial_putc(char c)
{
	/* Espera o transmissor estar vazio (poll LSR_THRE) */
	while (!(serial_read_reg(SERIAL_LSR) & SERIAL_LSR_THRE));
	/* Escreve o caractere no registrador de transmissão */
    serial_write_reg(SERIAL_THR, c);
}
