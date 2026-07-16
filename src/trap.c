#include <kernel/trap.h>
#include <kernel/panic.h>
#include <arch/csr.h>
#include <kernel/printf.h>
#include <arch/timer.h>
#include <kernel/serial.h>

/* defined in src/trap_entry.S */
extern void trap_entry();

#define CSR_SCAUSE_INTERRUPT_MASK	(1UL << 63)

void handle_irq()
{
	u64 cause = csr_read(CSR_SCAUSE) & 0xff; 

    if (cause == 5) {
        timer_irq();
    } else if (cause == 9) {
        serial_irq();
    }
}

void handle_exception()
{
	u64 cause = csr_read(CSR_SCAUSE);
    u64 tval = csr_read(CSR_STVAL);
    u64 epc = csr_read(CSR_SEPC);

    error("Excecao capturada! scause: 0x%lx, stval: 0x%lx, sepc: 0x%lx\n", cause, tval, epc);
    
}

void trap_setup()
{
	csr_write(CSR_STVEC, (u64)trap_entry);
}

void handle_trap()
{
	// ler o valor de scause para determinar se é uma exceção ou interrupção
	u64 scause = csr_read(CSR_SCAUSE);
	if (scause & CSR_SCAUSE_INTERRUPT_MASK) { //se fior igual a 1, é uma interrupção
		handle_irq();
	} else { // se for igual a 0, é uma exceção
		handle_exception();
	}
}

void hart_irq_enable()
{
	csr_set(CSR_SSTATUS, CSR_SSTATUS_SIE); // habilita interrupções globais (bit 1 do CSR sstatus)
}

u64 hart_irq_save()
{
	return 0;
}

void hart_irq_restore(u64 flags)
{
	flags = flags; // evita warning de variável não utilizada
}

void hart_irq_disable()
{
	csr_clear(CSR_SSTATUS, CSR_SSTATUS_SIE); // desabilita interrupções globais (bit 1 do CSR sstatus)
}
