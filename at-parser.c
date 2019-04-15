/*
 * =====================================================================================
 *
 *       Filename:  at_&(parser.c
 *
 *    Description:  AT command response &(parser.
 *
 *        Version:  1.0
 *        Created:  03/08/2018 10:23:23 AM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Aeluin Chen (aeluin.chen@moxa.com), 
 *   Organization:  Moxa Systems
 *
 * =====================================================================================
 */
#include <stdlib.h>
#include <stdio.h>
#include <regex.h>
#include "at-parser.h"


void at_parser_free(AT_PARSER *parser) {
    if (NULL == parser)
        return;

    regfree(&(parser->regex_ok));
    regfree(&(parser->regex_connect));
    regfree(&(parser->regex_sms));
    regfree(&(parser->regex_cme_error));
    regfree(&(parser->regex_cms_error));
    regfree(&(parser->regex_cme_error_str));
    regfree(&(parser->regex_cms_error_str));
    regfree(&(parser->regex_ezx_error));
    regfree(&(parser->regex_unknown_error));
    regfree(&(parser->regex_connect_failed));
    regfree(&(parser->regex_na));
    free(parser);
}

AT_PARSER* at_parser_init() {
    AT_PARSER *parser = NULL;
    int flag = REG_EXTENDED;

    parser = calloc(1, sizeof(AT_PARSER));
    /* Samsung Z810 may reply "NA" to report a not-available error */
    if (NULL == parser ||
            regcomp(&(parser->regex_ok),
                "\r\nOK(\r\n)+$", flag) ||
            regcomp(&(parser->regex_connect),
                "\r\nCONNECT.*\r\n", flag) ||
            regcomp(&(parser->regex_sms),
                "\r\n>\\s*$", flag) ||
            regcomp(&(parser->regex_cme_error),
                "\r\n\\+CME ERROR:\\s*([[:digit:]]+)\r\n$", flag) ||
            regcomp(&(parser->regex_cms_error),
                "\r\n\\+CMS ERROR:\\s*([[:digit:]]+)\r\n$", flag) ||
            regcomp(&(parser->regex_cme_error_str),
                "\r\n\\+CME ERROR:\\s*([^\n\r]+)\r\n$", flag) ||
            regcomp(&(parser->regex_cms_error_str),
                "\r\n\\+CMS ERROR:\\s*([^\n\r]+)\r\n$", flag) ||
            regcomp(&(parser->regex_ezx_error),
                "\r\n\\MODEM ERROR:\\s*([[:digit:]]+)\r\n$", flag) ||
            regcomp(&(parser->regex_unknown_error),
                "\r\n(ERROR)|(COMMAND NOT SUPPORT)\r\n$", flag) ||
            regcomp(&(parser->regex_connect_failed),
                "\r\n(NO CARRIER)|(BUSY)|(NO ANSWER)|(NO DIALTONE)\r\n$", flag) ||
            regcomp(&(parser->regex_na), "\r\nNA\r\n", flag)) {
        fprintf(stderr, "Could not compile regex\n");
        at_parser_free(parser);
        return NULL;
    }

    return parser;
}

int at_parser_parse(AT_PARSER* parser, char* response) {
    if (!regexec(&(parser->regex_ok), response, 0, NULL, 0) || 
            !regexec(&(parser->regex_connect), response, 0, NULL, 0) || 
            !regexec(&(parser->regex_sms), response, 0, NULL, 0) || 
            !regexec(&(parser->regex_cme_error), response, 0, NULL, 0) || 
            !regexec(&(parser->regex_cms_error), response, 0, NULL, 0) || 
            !regexec(&(parser->regex_cme_error_str), response, 0, NULL, 0) || 
            !regexec(&(parser->regex_cms_error_str), response, 0, NULL, 0) || 
            !regexec(&(parser->regex_ezx_error), response, 0, NULL, 0) || 
            !regexec(&(parser->regex_unknown_error), response, 0, NULL, 0) || 
            !regexec(&(parser->regex_connect_failed), response, 0, NULL, 0) || 
            !regexec(&(parser->regex_na), response, 0, NULL, 0))
        return 0;
    return 1;
}

int _main(int argc, char **argv) {
    AT_PARSER* parser = at_parser_init();
    regex_t regex;

    /* 
    char test[] = "\r\nOK\r\n";
    char test[] = "\r\nERROR\r\n";
    char test[] = "\r\nCONNECT\r\n";
    char test[] = "\r\n> ";
    char test[] = "\r\n+CME ERROR: 1234\r\n";
    char test[] = "\r\n+CMS ERROR: 5678\r\n";
    char test[] = "\r\n+CME ERROR: abcd 1234\r\n";
    char test[] = "\r\n+CMS ERROR: abcd 5678\r\n";
    char test[] = "\r\nMODEM ERROR: 1234\r\n";
    char test[] = "\r\nCOMMAND NOT SUPPORT\r\n";
    char test[] = "\r\nNO CARRIER\r\n";
    char test[] = "\r\nBUSY\r\n";
    char test[] = "\r\nNO ANSWER\r\n";
    char test[] = "\r\nNO DIALTONE\r\n";
    char test[] = "\r\nNA\r\n";
     **/
    char test[] = "\r\n+CME ERROR: 1234\r\n";
    regmatch_t match[2];

    printf("match? %d\n", at_parser_parse(parser, test));

    regcomp (&regex, "\r\n\\+CME ERROR:\\s*([[:digit:]]+)\r\n$", REG_EXTENDED);
    //regcomp (&regex, "\r\n\\+CME ERROR:\\s*([^\n\r]+)\r\n$", REG_EXTENDED);
    printf("%d\n", regexec(&regex, test, 2, match, 0));
    printf("%d, %d\n", match[0].rm_so, match[0].rm_eo);
    printf("%d, %d\n", match[1].rm_so, match[1].rm_eo);

    at_parser_free(parser);
    return 0;
}
