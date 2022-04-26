#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <syslog.h>
#include <sys/types.h>
#include <limits.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <linux/fs.h>
#include <signal.h>
#include <sys/mman.h>
#include <dirent.h>
#include <stdlib.h>
#include <stdbool.h>

void powiadamiam(int sig)
{
    syslog(LOG_NOTICE, "Żądanie natychmiastowgo wybudzenia demona (SIGUSR1)");
}

void ourDemon(char *plik_zr, char *plik_doc, int czas, char rekurencja){
        pid_t pid;
        pid = fork();
        if(pid==-1){
            return -1;
        }
        else if (pid !=0){
            exit(EXIT_SUCCESS);
        }

        /* stwórz nową sesję i grupę procesów */
        if (setsid ( ) == -1){
        return -1;
        }

        /* ustaw katalog roboczy na katalog główny */
        if (chdir ("/") == -1){
        return -1;
        }

        /* zamknij wszystkie pliki otwarte - użycie opcji NR_OPEN to przesada, lecz działa */
        close(STDIN_FILENO);
        close(STDOUT_FILENO);
        close(STDERR_FILENO);

        /* przeadresuj deskryptory plików 0, 1, 2 na /dev/null */
        open ("/dev/null", O_RDWR); /* stdin */
        dup (0); /* stdout */
        dup (0); /* stderror */

        /* tu należy wykonać czynności demona… */
        
        //sprawdzenie czy program powinien natychmiastowo obudzić demona
        
       if(signal(SIGINT, powiadamiam)==SIG_ERR){
           syslog(LOG_ERR,"Błąd związany z wysyłaniem sygnału");
           exit(EXIT_FAILURE);
       }


        //wykonywanie operacji na katalogach i czekanie
       while(1){
           syslog(LOG_NOTICE, "Demon się budzi! (Buka tu jest)");

           syslog(LOG_NOTICE, "Demon idzie spać! (Buka tu wróci)");
           sleep(czas);

       }
        

    }

int main(int argc, char **argv)
{
    //deklaracja zmiennych zaleznych od parametrow
    char *plik_zrodlowy;
    char *plik_docelowy;
    int czas=300000;
    bool rekurencja=NULL;
    printf("%d\n", argc);
    //
    //kod na sprawdzenie parametrow
    bool czy_parametry_poprawne=true;

    if(argc<=2 || argc>6) {
        czy_parametry_poprawne = false;
        printf("\nzla liczba parametrow");
    }
    else {
        //analiza i przypisywanie parametrow
        for(int i = 1; i <  argc; i++)
        {
            printf("\nparametr %d-ty to: %s",i, argv[i]);
            if(argv[i][0] == '-')
            {
                if(argv[i][1] == 'R')
                {
                    rekurencja = true;
                }
                else if(argv[i][1] == 'i')
                {
                    if(i+1 < argc )
                    {
                        czas=atoi(argv[i+1]);
                        i++;
                        printf("\nparametr %d-ty to: %s",i, argv[i]);
                    }
                    else
                    {
                        printf("\nnie podano czasu");
                    }
                }
                else
                {
                    printf("\nzly parametr z myslnikiem na poczatku");
                    czy_parametry_poprawne=false;
                    break;
                }
            }
            else if(i+1<argc)
            {
                plik_zrodlowy=argv[i];
                i++;
                plik_docelowy=argv[i];
                printf("\nparametr %d-ty to: %s",i, argv[i]);
            }
            else
            {
                printf("\nzle parametry wejsciowe");
                czy_parametry_poprawne=false;
                break;
            }



        }
        //
    }
    if(!czy_parametry_poprawne)
    {
        printf("\n Parametry niepoprawne");
        printf("\nPoprawne parametry to: ");
        printf("\nsynchronizacja_katalogow plik_zrodlowy plik_docelowy [-i czas(liczba calkowita)] [-R] ");


        return -1;
    }
    else
    {
        printf("\nParametry sa poprawne.");
    }
    //
    //analiza plikow przekazanych do programu
    bool czy_katalogi_poprawne=true;

    DIR *katalog_wejsciowy=opendir(plik_zrodlowy);
    DIR *katalog_wyjsciowy=opendir(plik_docelowy);
    if(katalog_wejsciowy==NULL)
    {
        czy_katalogi_poprawne=false;
    }
    else if(katalog_wyjsciowy==NULL)
    {
        czy_katalogi_poprawne=false;
    }
    else if(closedir(katalog_wejsciowy)==-1)
    {
        czy_katalogi_poprawne=false;
    }
    else if(closedir(katalog_wyjsciowy)==-1)
    {
        czy_katalogi_poprawne=false;
    }
    if(!czy_katalogi_poprawne)
    {
        printf("\nConajmniej jeden plik wejsciowy jest niepoprawny");

        return -2;
    }
    else
    {
        printf("\nKatalogi sa poprawne. ");
    }

    //

    //jesli wszystkie parametry i pliki sie zgadzaja mozna wlaczyc demona

    //przywolanie demona
    openlog("Daemon", LOG_PID, LOG_DAEMON);


    // if(daemon(0,0))
    // {
    //     printf("Demon dziala i to jak.");
    // }
    

    //sleep(czas);


    ourDemon(plik_zrodlowy, plik_docelowy, czas, rekurencja);

   
    syslog(LOG_NOTICE, "Daemon ended work");
    printf("Demon cos tam zrobil");

    

    return 0;
}



