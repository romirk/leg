//
// Created by Romir Kulshrestha on 04/06/2025.
//

#ifndef EXCEPTIONS_H
#define EXCEPTIONS_H

void handle_boot_exception(void);

void handle_page_fault_boot(void);

void handle_irq_boot(void);

void handle_fiq_boot(void);
#endif //EXCEPTIONS_H
