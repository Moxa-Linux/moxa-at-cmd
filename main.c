#if HAVE_CONFIG_H
# include <config.h>
#else
# define VERSION "0.9.0"
#endif

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>

#define DEF_TIME_OUT    5000


int pcommend(char *cmd)
{
    FILE *fp;
    char buff[1024];
    
    // Open the command for reading. 
    fp = popen(cmd, "r");
    if (fp == NULL) {
        printf("[ERROR]Failed to run command.");
        return -1;
    }

    // Read the output a line at a time - output it. 
    while (fgets(buff, sizeof(buff)-1, fp) != NULL);

    // close 
    pclose(fp);
    return 0;
}

void usage() 
{
    printf("\n");
    printf(" ******************************************************\n");
    printf(" moxa at inout tool. ver: %s\n", VERSION);
    printf(" build date   : %s - %s.\n", __DATE__, __TIME__);
    printf("\n");
    printf(" support command : \n");
    printf(" -d : at device path. ex: /dev/ttyUSB2.\n");
    printf(" -c : at command, it is best to add \" like \"AT^SYSINFO\".\n");
    printf(" -t : set timeout (milliseconds)\n");
    printf(" ******************************************************\n");
}

int main(int argc, char **argv)
{
    char optstring[]    = "c:d:t:h";
    char *at_result     = NULL;
    char at_dev         [128];
    char at_cmd         [1024];
    char tmp            [128];
    char c;
    int  timeout_msec   = DEF_TIME_OUT;
    int  ret = 0;


    memset (at_cmd, 0, 1024);
    memset (at_dev, 0, 128);

    while ((c = getopt(argc, argv, optstring)) != -1)
    {
        int exit = 0;
        switch (c) 
        {
            case 'c':
                if (strlen(optarg) >= sizeof(at_cmd) - 1) {
                    fprintf(stderr, "[ERROR] AT command too long (< 1024).");
                    return -1;
                }
                memcpy (at_cmd, optarg, strlen(optarg));
                break;
            case 'd':
                if (strlen(optarg) >= sizeof(at_dev) - 1) {
                    fprintf(stderr, "[ERROR] Device path too long (< 128).");
                    return -1;
                }
                memcpy (at_dev, optarg, strlen(optarg));
                break;
            case 't':
                timeout_msec = atoi(optarg);
                break;
            case 'h':
                usage();
                return 0;
            default:
                exit = 1; 
                break;
        }
        
        if (exit)
            break;
    }

    if (strlen(at_cmd) != 0 && strlen(at_dev) != 0) {
        sprintf(tmp, "stty -F %s -echo raw", at_dev);
        pcommend(tmp);
        
        ret = at_cmd_run(at_dev, at_cmd, &at_result, timeout_msec);
        if (at_result) {
            printf("%s", at_result);
            free(at_result);
        }
        return ret;
    } else {
        usage();
    }

    return 0;
}
