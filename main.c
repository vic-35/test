

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <signal.h>
  #include <sys/wait.h>

int listener=0;
int sock=0;

void  INThandler(int sig)
{

     signal(sig, SIG_IGN);

     if( listener > 0)
     {
         pid_t wpid;
         int status = 0;
         while ((wpid = wait(&status)) > 0)
         {
             printf("Exit status of %d was %d (%s)\n", (int)wpid, status,
                    (status > 0) ? "accept" : "reject");
         }
         sleep(1);
         shutdown(listener,SHUT_RDWR);
         printf("\nClose listener......\n");
     }

     if( sock > 0)
     {
         shutdown(sock,SHUT_RDWR);
         sleep(1);
         printf("\nClose socket......\n");
     }

}

int main()
{

    struct sockaddr_in addr;
    char buf[1024];
    int bytes_read;


    listener = socket(AF_INET, SOCK_STREAM, 0);
    if(listener < 0)
    {
        perror("socket");
        exit(1);
    }

    addr.sin_family = AF_INET;
    addr.sin_port = htons(3425);
    addr.sin_addr.s_addr = INADDR_ANY;

    if(bind(listener, (struct sockaddr *)&addr, sizeof(addr)) < 0)
    {
        perror("bind");
        exit(2);
    }

    listen(listener, 1);


    signal(SIGINT, INThandler);


    while(1)
    {

        int iResult;
        struct timeval tv;
        fd_set rfds;
        FD_ZERO(&rfds);
        FD_SET(listener, &rfds);

        tv.tv_sec = (long)5;
        tv.tv_usec = 0;

        iResult = select(listener+1, &rfds, (fd_set *) 0, (fd_set *) 0, &tv);
        if( iResult > 0)
        {
            sock = accept(listener, NULL, NULL);
        }
        else
        {
            printf("wait...\n");
            continue;
        }


        if(sock <= 0)
        {
            close(listener); listener =0;
            perror("Stop accept");
            _exit(0);
        }

        switch(fork())
        {
        case -1:
            perror("fork");
            break;

        case 0:
            close(listener);listener=0;
            while(1)
            {
                bytes_read = recv(sock, buf, 1023, 0);
                if(bytes_read <= 0) {
                    break;
                }
                buf[bytes_read]=0x0;
                printf("%s\n",buf);
                send(sock, buf, bytes_read, 0);
            }

            printf("End conn...\n");
            close(sock);
            _exit(0);

        default:
        {
            perror("default");
            close(sock);
            sock=0;
        }

        }
    }
    close(listener);

    return 0;
}
