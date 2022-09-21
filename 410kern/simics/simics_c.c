#include <simics.h>
#include <stdarg.h>
#include <stdio/stdio.h>

void SIM_printf(char* fmt, ...)
{
  char str[256];
  va_list ap;

  va_start(ap, fmt);
  vsnprintf (str, 255, fmt, ap);
  va_end(ap);

  SIM_puts(str);
}

#include <cr.h>

void set_cr3_debug(unsigned int cr3)
{
  set_cr3(cr3);
  SIM_switch(cr3);
}
