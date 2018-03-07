#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <dirent.h>
#include <errno.h>

#ifndef FALLTHROUGH
# if __GNUC__ < 7
#  define FALLTHROUGH ((void) 0)
# else
#  define FALLTHROUGH __attribute__ ((__fallthrough__))
# endif
#endif

#define TRUE    1
#define FALSE   0
#define MAX_LINE_LENGTH (4 * 1024)
static char buf[MAX_LINE_LENGTH];
static char buf2[MAX_LINE_LENGTH];

/* Replace '\n' with '\r', aka `tr '\012' '\015'` */
static int tr_lf_cr(const char *s)
{
    char *p;
    p = strchr(s, '\n');
    if (p == NULL || p[1] != '\0') {
        return FALSE;
    }
    *p = '\r';
    return TRUE;
}

static void strip_cr(char *s)
{
    char *from, *to;
    from = to = s;
    while (*from != '\0') {
        if (*from == '\r') {
            from++;
            continue;
        }
        *to++ = *from++;
    }
    *to = '\0';
}

/*  Convert C from hexadecimal character to integer.  */
static int hextobin (unsigned char c) {
    switch (c) {
        default: return c - '0';
        case 'a': case 'A': return 10;
        case 'b': case 'B': return 11;
        case 'c': case 'C': return 12;
        case 'd': case 'D': return 13;
        case 'e': case 'E': return 14;
        case 'f': case 'F': return 15;
    }
}

static void format_str(char *s)
{
    char c;
    char *to;

    to = s;
    while ((c = *s++)) {
        if (c == '\\' && *s) {
            switch (c = *s++) {
                case 'n': c = '\n'; break;
                case 'r': c = '\r'; break;
                case 't': c = '\t'; break;
                case 'x': {
                              unsigned char ch = *s;
                              if (! isxdigit (ch))
                                  goto not_an_escape;
                              s++;
                                  c = hextobin (ch);
                              ch = *s;
                              if (isxdigit (ch))
                              {
                                  s++;
                                  c = c * 16 + hextobin (ch);
                              }
                          }
                          break;
                case '0':
                          c = 0;
                          if (! ('0' <= *s && *s <= '7'))
                              break;
                          c = *s++;
                          FALLTHROUGH;
                case '1': case '2': case '3':
                case '4': case '5': case '6': case '7':
                          c -= '0';
                          if ('0' <= *s && *s <= '7')
                              c = c * 8 + (*s++ - '0');
                          if ('0' <= *s && *s <= '7')
                              c = c * 8 + (*s++ - '0');
                          break;
                case '\\': break;

not_an_escape:
                default:  *to++ = '\\'; break;
            }
        }
        *to++ = c;
    }
    *to++ = 0;
}

static int is_final_result(const char * const response)
{
#define STARTS_WITH(a, b) ( strncmp((a), (b), strlen(b)) == 0)
    switch (response[0]) {
    case '+':
        if (STARTS_WITH(&response[1], "CME ERROR:")) {
            return TRUE;
        }
        if (STARTS_WITH(&response[1], "CMS ERROR:")) {
            return TRUE;
        }
        return FALSE;
    case 'B':
        if (strcmp(&response[1], "USY\r\n") == 0) {
            return TRUE;
        }
        return FALSE;

    case 'E':
        if (strcmp(&response[1], "RROR\r\n") == 0) {
            return TRUE;
        }
        return FALSE;
    case 'N':
        if (strcmp(&response[1], "O ANSWER\r\n") == 0) {
            return TRUE;
        }
        if (strcmp(&response[1], "O CARRIER\r\n") == 0) {
            return TRUE;
        }
        if (strcmp(&response[1], "O DIALTONE\r\n") == 0) {
            return TRUE;
        }
        return FALSE;
    case 'O':
        if (strcmp(&response[1], "K\r\n") == 0) return TRUE;
        /* no break */
    default:
        return FALSE;
    }
}

int at_cmd_run(char *at_dev, char *cmd_str, char **result_str, int time_out_sec)
{
    FILE        *modem;
    char        *line;
    char        *cmd;
    int         res;
    int         success;
    int         length = 0;

    cmd = (char *)malloc(1024);
    sprintf(cmd, "%s\r\n", cmd_str);

    format_str(cmd);
    success = tr_lf_cr(cmd);
    if (!success) 
    {
        printf("[ERROR] at_cmd_run, invalid string: '%s'.", cmd);
        free(cmd);
        return -1;
    }

    modem = fopen(at_dev, "r+b");
    if (modem == NULL) 
    {
        printf("[ERROR] at_cmd_run, fopen(%s) failed.", at_dev);
        goto at_fail;
    }

    res = fputs(cmd, modem);
    if (res < 0) 
    {
        printf("[ERROR] at_cmd_run, failed to send '%s' to modem (res = %d)", cmd, res);
        goto at_fail;
    }

    *result_str = NULL;
    do 
    {
        alarm(time_out_sec);
        line = fgets(buf, (int)sizeof(buf), modem);
        if (line == NULL) 
        {
            printf("[ERROR] EOF from modem.\n");
            goto at_fail;
        }

        strcpy(buf2, line);
        strip_cr(buf2);
        length += strlen(buf2);
        if (*result_str == NULL)
            *result_str = calloc(length + 1, sizeof(char));
	else
            *result_str = realloc(*result_str, sizeof(char) * (length + 1));
        if (*result_str == NULL)
        {
            printf("[ERROR] at_cmd_run, failed to allocate memory (%d)", length);
            goto at_fail;
        }
        strcat(*result_str, buf2);
        alarm(0);
    } while (!is_final_result(line));
    
    free(cmd);
    res = fclose(modem);
    if (res != 0) 
    {
        printf("[ERROR] closing modem failed.");
        return -1;
    }
    return 0;

at_fail:
    free(cmd);
    res = fclose(modem);
    if (res != 0) printf("[ERROR] closing modem failed.");

    return -1;
}
