//
// Created by Romir Kulshrestha on 04/06/2025.
//

#ifndef EXCEPTIONS_H
#define EXCEPTIONS_H

void handle_boot_exception(void);

void handle_page_fault(void);

void handle_irq(void);

void handle_fiq(void);

void handle_svc(void);

void setup_exceptions(void);

void enable_interrupts(void);
void disable_interrupts(void);

#endif //EXCEPTIONS_H
