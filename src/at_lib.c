#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <errno.h>

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

int at_cmd_run(char *at_dev, char *cmd_str, char *result_str, int time_out_sec)
{
    FILE        *modem;
    char        *line;
    char        *cmd;
    int         res;
    int         success;

    cmd = (char *)malloc(1024);
    sprintf(cmd, "%s\r\n", cmd_str);

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
        strcat(result_str, buf2);
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