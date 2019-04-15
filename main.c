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

#define DEF_TIME_OUT    5

static volatile int timeout_flag = 0;

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

// handler - catch alarm signal and set flag.
void handler(int sig)
{
    printf("At command timeout.\n");
    timeout_flag = 1;
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
    printf(" -t : set timeout (second)\n");
    printf(" ******************************************************\n");
}

int main(int argc, char **argv)
{
    char optstring[]    = "c:d:t:h";
    char at_result      [4096];
    char at_dev         [128];
    char at_cmd         [512];
    char tmp            [128];
    char c;
    int  time_out_sec   = DEF_TIME_OUT;
    
    memset (at_result,  0, 4096);
    memset (at_cmd,     0, 512);
    memset (at_dev,     0, 128);
    
    siginterrupt(SIGALRM, 1);
    signal(SIGALRM, handler);
    
    while ((c = getopt(argc, argv, optstring)) != -1)
    {
        int exit = 0;
        switch (c) 
        {
            case 'c':
                memcpy (at_cmd, optarg, strlen(optarg));
                //printf (" - at_cmd = %s.\n", at_cmd);
                break;
            case 'd':
                memcpy (at_dev, optarg, strlen(optarg));
                //printf (" - at_dev = %s.\n", at_dev);
                break;
            case 't':
                time_out_sec = atoi(optarg);
                break;
            case 'h':
                usage();
                return 0;
            default:
                exit = 1; 
                break;
        }
        
        if(exit)break;
    }
    
    if (strlen(at_cmd) != 0 && strlen(at_dev) != 0)
    {
        sprintf(tmp, "stty -F %s -echo raw", at_dev);
        pcommend(tmp);
        
        at_cmd_run(at_dev, at_cmd, at_result, time_out_sec);
        if (!timeout_flag)
            printf("%s", at_result);
    }
    else
    {
        printf(" [!] no at device path or at command input.\n");
        usage();
    }
    
    return 0;
}