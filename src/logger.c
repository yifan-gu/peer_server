#include <stdio.h>
#include <stdarg.h>
#include <logger.h>


/*
@brief
  This log function simulates normal logging system.
  LIMITATION:
    For varaibles passed behind, currently it supports only "%d" and "%s"
 */
void log_print(int level, char* filename, int line, char *fmt,...)
{
    va_list         list;
    FILE *log_fp = stdout;

    switch (level) {
    case LOG_DEBUG:
        fprintf(log_fp,"-DEBUG: ");
        break;
    case LOG_INFO:
        fprintf(log_fp,"-INFO: ");
        break;
    case LOG_WARN:
        fprintf(log_fp,"-WARN: ");
        break;
    case LOG_ERROR:
        fprintf(log_fp,"-ERROR: ");
        break;
    default:
        break;
    }

    fprintf(log_fp,"[%s][line: %d] ",filename,line);
    va_start( list, fmt );

    vfprintf(log_fp, fmt, list);

    va_end( list );
    fputc( '\n', log_fp );
}
