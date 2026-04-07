#ifndef EXCEPTIONS_H
#define EXCEPTIONS_H

void handle_boot_exception(void);

void handle_data_abort(void);

void handle_irq(void);

void handle_fiq(void);

void handle_svc(int);

void enable_interrupts(void);

void disable_interrupts(void);

#endif // EXCEPTIONS_H
