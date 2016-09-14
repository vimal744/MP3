#include <stdint.h>
#include "print.h"

/*
 *
 * Part of a fault exception handler. Prints the given register values.
 * pc: the value of the program counter when the fault occurred.
 * lr: the value of the link register when the fault occurred.
 *
 */
void FaultPrint(uint32_t pc, uint32_t lr)
{
    // Print an error message specifying the PC and LR values when the fault occurred
    PrintString("Error!!\n");
    PrintString("PC = 0x"); PrintHex(pc); PrintString("\n");
    PrintString("LR = 0x"); PrintHex(lr); PrintString("\n");
}
