/*
 * =====================================================================================
 *
 *       Filename:  at-parser.h
 *
 *    Description:  header for at command parser
 *
 *        Version:  1.0
 *        Created:  Thu Mar  8 12:27:07 CST 2018
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Aeluin Chen (aeluin.chen@moxa.com), 
 *   Organization:  Moxa Systems
 *
 * =====================================================================================
 */
#include <regex.h>

typedef struct {
    /* Regular expressions for successful replies */
    regex_t regex_ok;
    regex_t regex_connect;
    regex_t regex_sms;
    regex_t regex_custom_successful;
    /* Regular expressions for error replies */
    regex_t regex_cme_error;
    regex_t regex_cms_error;
    regex_t regex_cme_error_str;
    regex_t regex_cms_error_str;
    regex_t regex_ezx_error;
    regex_t regex_unknown_error;
    regex_t regex_connect_failed;
    regex_t regex_na;
} AT_PARSER;


void at_parser_free(AT_PARSER *parser);
AT_PARSER* at_parser_init();
int at_parser_parse(AT_PARSER* parser, char* response);
