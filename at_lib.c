#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include "at-parser.h"

#ifndef FALLTHROUGH
# if __GNUC__ < 7
#  define FALLTHROUGH ((void) 0)
# else
#  define FALLTHROUGH __attribute__ ((__fallthrough__))
# endif
#endif

#define TRUE    1
#define FALSE   0
#define MAX_BUF_LENGTH (4 * 1024)
#define MAX_RESP_LENGTH (16 * 1024)

/* Replace '\n' with '\r', aka `tr '\012' '\015'` */
static int tr_lf_cr(const char *s) {
    char *p;

    p = strchr(s, '\n');
    if (p == NULL || p[1] != '\0') {
        return FALSE;
    }
    *p = '\r';
    return TRUE;
}

static void strip_cr(char *s) {
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

static void format_str(char *s) {
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

int max(int a, int b) {
    if(a > b)
        return a ;
    else
        return b ;
}

void mdelay(unsigned long msec) {
    struct timeval  time;

    time.tv_sec = msec / 1000;
    time.tv_usec = (msec % 1000) * 1000;
    select(1, NULL, NULL, NULL, &time);
}

int at_resp_read (
        unsigned int fd,
        char **resp,
        int timeout,
        AT_PARSER* parser) {
    int maxfd;
    static int status;
    fd_set readfds;
    struct timeval authTime; /*  Timer for release FD */

    int flag = 1;            /*  set priority as flush all data */
    int loop_times = 0;

    int read_len = 0;
    int total_len = 0;
    int remain_size = MAX_RESP_LENGTH;
    char buf[MAX_BUF_LENGTH];


    maxfd = max(fd, 2) + 1;

    /*  define minimal timeout */
    if (timeout < 200)
        timeout = 200 ;

    loop_times = timeout / 10 ;
    while (loop_times-- > 0 || flag) {
        flag = 0;

        /*  retry until timeout */
        authTime.tv_usec = 10;
        authTime.tv_sec = 0;
        FD_ZERO(&readfds);
        FD_SET(fd, &readfds);

        status = select(maxfd, &readfds, NULL, NULL, &authTime);
        if (status > 0) {
            if (FD_ISSET(fd, &readfds)) {
                memset(buf,'\0',MAX_BUF_LENGTH);
                read_len = read(fd, buf, sizeof(buf)-1);
                if (read_len < 0) {
                    fprintf(stderr, "[ERROR] Fail to read response.\n");
                    return total_len;
                } else if (read_len == 0) {
                    continue;
                } else {
                    flag = 1;
                    total_len += read_len;
                    remain_size -= read_len;

                    if (*resp)
                        *resp = realloc(*resp, sizeof(char)*(total_len + 1));
                    else
                        *resp = calloc(total_len + 1, sizeof(char));
                    strcat(*resp, buf);
                }

                /*  reach max read size */
                if(remain_size <= 0) {
                    fprintf(stderr, "[ERROR] Reach max read size: %d\n",
                            MAX_RESP_LENGTH);
                    return total_len;
                }

                if(parser) {
                    if(!at_parser_parse(parser, *resp))
                        return total_len;
                }
            }
        }
        else if (status < 0) {
            fprintf(stderr, "[ERROR] Serial select error\n");
        }
        mdelay(10);
    }

    if (total_len == 0)
        return -1 ;

    return total_len;
}

int at_cmd_run (
        char *at_dev,
        char *cmd_str,
        char **result_str,
        int timeout_msec) {
    unsigned int modem;
    char        *cmd;
    int         res;
    int         success;
    int         length = 0;
    AT_PARSER* parser;


    cmd = (char *)calloc(strlen(cmd_str)+3, sizeof(char));
    sprintf(cmd, "%s\r\n", cmd_str);

    format_str(cmd);
    success = tr_lf_cr(cmd);
    if (!success) {
        fprintf(stderr, "[ERROR] at_cmd_run, invalid string: '%s'.", cmd);
        free(cmd);
        return -1;
    }

    modem = open(at_dev, O_RDWR|O_NONBLOCK);
    if (modem == -1) {
        fprintf(stderr, "[ERROR] at_cmd_run, fopen(%s) failed.", at_dev);
        goto at_fail;
    }

    res = write(modem, cmd, strlen(cmd));
    if(res < strlen(cmd)) {
        fprintf(stderr,
                "[ERROR] at_cmd_run, failed to send '%s' to modem (res = %d)",
                cmd,
                res);
        goto at_fail;
    }

    *result_str = NULL;
    parser = at_parser_init();
    if (-1 == at_resp_read(modem, result_str, timeout_msec, parser)) {
        fprintf(stderr, "[ERROR] at_cmd_run, failed to get from modem\n");
        goto at_fail;
    }

    free(cmd);
    at_parser_free(parser);
    res = close(modem);
    if (res != 0) {
        fprintf(stderr, "[ERROR] closing modem failed.");
        return -1;
    }
    return 0;

at_fail:
    free(cmd);
    at_parser_free(parser);
    res = close(modem);
    if (res != 0)
        fprintf(stderr, "[ERROR] closing modem failed.");

    return -1;
}
