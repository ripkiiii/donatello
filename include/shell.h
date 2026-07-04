/* shell.h — Donatello's command line. The keyboard driver feeds us one
 * character at a time; we buffer a line and run it on Enter. */
#ifndef SHELL_H
#define SHELL_H

void shell_init(void);          /* print banner + first prompt   */
void shell_input(char c);       /* called by the keyboard driver */

#endif
