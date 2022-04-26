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

void porownaj_zrodlowy(char *zrodlowa, char *docelowa);
void powiadamiam(int sig);
int ourDemon(char *plik_zr, char *plik_doc, int czas, char rekurencja);
char* plik_na_sciezke(char* sciezka_zrodlowa, char* plik_tymczasowy_nazwa);
time_t data_modyfikacji(char * pliczek);
bool sprawdz_plik_zrodlowy(char* sciezka_pliku_tymczasowego, char* sciezka_docelowa);
int kopiuj_plik(char * plik_zrodlowy, char* plik_docelowy);

void porownaj_zrodlowy(char *zrodlowa, char *docelowa)
{
    syslog(LOG_NOTICE, "Poczatek porownania");
    printf("porownaj_zrodlowy");
    syslog(LOG_NOTICE, "zrodlowa: %s", zrodlowa);
    char * pointer;

    char* sciezka_zrodla = malloc(sizeof(char) * PATH_MAX);
    char* sciezka_docelu = malloc(sizeof(char) * PATH_MAX);
    pointer=realpath(zrodlowa, sciezka_zrodla);
    syslog(LOG_NOTICE,"sciezka_zrodla: %s", pointer);
    DIR* sciezka_zrodlowa = opendir(pointer);

    pointer=realpath(docelowa, sciezka_docelu);
    syslog(LOG_NOTICE,"sciezka_docelu: %s", pointer);
    DIR* sciezka_docelowa = opendir(pointer);

    struct dirent* pliktymczasowy;
    char* sciezka_pliku;
    // przejscie po wszystkich plikach i folderach w folderze wejsciowym
    while(pliktymczasowy=readdir(sciezka_zrodlowa))
    {
        if((pliktymczasowy->d_type) == DT_REG)
        {
            syslog(LOG_NOTICE, "znaleziono plik");

            sciezka_pliku = plik_na_sciezke(zrodlowa, (pliktymczasowy->d_name));
            if(sprawdz_plik_zrodlowy(pliktymczasowy->d_name,docelowa)==false)
            {
                syslog(LOG_NOTICE, "Należy skopiować plik");
                //brakuje pliku i trzeba go skopiowac
                kopiuj_plik(sciezka_pliku,plik_na_sciezke(docelowa, pliktymczasowy->d_name));
            }
        
        }
        else
        {
            syslog(LOG_NOTICE, "ni znaleziono pliku");
        //rekurencja
        //jesli plik jest folderem lub dowiazaniem nie robimy nic
        }
    }
    syslog(LOG_NOTICE, "Koniec porownania");
}

void powiadamiam(int sig)
{
    syslog(LOG_NOTICE, "Żądanie natychmiastowgo wybudzenia demona (SIGUSR1)");
}

int ourDemon(char *plik_zr, char *plik_doc, int czas, char rekurencja){
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
        porownaj_zrodlowy(plik_zr, plik_doc);
        syslog(LOG_NOTICE, "Demon idzie spać! (Buka tu wróci)");
        sleep(czas);

    }

}

char* plik_na_sciezke(char* sciezka_zrodlowa, char* plik_tymczasowy_nazwa)
{
    syslog(LOG_NOTICE, "zamiana pliku na sciezke");
    char* sciezka_pliku = malloc(strlen(sciezka_zrodlowa) + strlen(plik_tymczasowy_nazwa) + 2 );
    strcpy(sciezka_pliku,sciezka_zrodlowa);
    strcat(sciezka_pliku,"/");
    strcat(sciezka_pliku, plik_tymczasowy_nazwa);
    sciezka_pliku[strlen(sciezka_zrodlowa)+1+strlen(plik_tymczasowy_nazwa)]='\0';
    return sciezka_pliku;
    
}

time_t data_modyfikacji(char * pliczek)
{
    struct stat czas;
    syslog(LOG_NOTICE,"data modyfikacji");
    if(stat(pliczek, &czas) == -1)
    {
        syslog(LOG_ERR, "Blad z pobraniem daty modyfikacji dla pliku");
        exit(EXIT_FAILURE);
    }
    return czas.st_mtime;
}

bool sprawdz_plik_zrodlowy(char* sciezka_pliku_tymczasowego, char* sciezka_docelowa)
{
    syslog(LOG_NOTICE, "sprawdzanie pliku zrodlowego");
    bool czy_istnieje = false;
    DIR* sciezka_pliku_docelowego=opendir(sciezka_docelowa);
    struct dirent* plik_tymczasowy_docelowy;

    int roznica_czasu=0;
    // przejscie po wszystkich plikach i folderach w folderze wyjsciowym
    while(plik_tymczasowy_docelowy=readdir(sciezka_pliku_docelowego))
    {
        if(strcmp(plik_tymczasowy_docelowy->d_name,sciezka_pliku_tymczasowego)==0)
        {
            free(sciezka_pliku_tymczasowego);
            if((plik_tymczasowy_docelowy->d_type)==DT_REG)
            {

                roznica_czasu=(int)data_modyfikacji(sciezka_pliku_tymczasowego)-
                (int)data_modyfikacji(plik_na_sciezke(sciezka_docelowa, 
                plik_tymczasowy_docelowy->d_name));
                //jesli data modyfikacji plikow rozni sie to nalezy plik docelowy zamienic
                if(roznica_czasu==0){
                    return true;
                }
                else
                {
                    return false;
                }
            }
            else
            {
                //jesli plik jest dowiazaniem lub folderem nie robimy nic
                return true;
            }
        }
    }

    return false;
}

int kopiuj_plik(char * plik_zrodlowy, char* plik_docelowy)
{
    //kopiowanie pliku jesli plik docelowy lub zamiana jesli plik juz istnieje
    syslog(LOG_NOTICE, "kopiowanie pliku");
    unsigned int rozmiar_bufora=32;
    FILE *plik_wejsciowy=fopen(plik_zrodlowy,"rb");
	if(plik_wejsciowy==NULL)
	{
		syslog(LOG_ERR,"blad otwierania pliku wejsciowego");
		return -1;
	}
	FILE *plik_wyjsciowy=fopen(plik_docelowy,"wb");
	if(plik_wyjsciowy==NULL)
	{
		fclose(plik_wejsciowy);
		syslog(LOG_ERR,"blad otwierania pliku wyjsciowego");
		return -1;
	}
	unsigned char *bufor=malloc(rozmiar_bufora);
	while(1==1)
	{
	unsigned int czytaj_bity=fread(bufor,1,rozmiar_bufora,plik_wejsciowy);
	fwrite(bufor,1,czytaj_bity,plik_wyjsciowy);
	if(czytaj_bity<rozmiar_bufora)
		break;
	}
	free(bufor);
	fclose(plik_wejsciowy);
	fclose(plik_wyjsciowy);

    syslog(LOG_NOTICE, "Zmodyfikowano/utworzono plik");
    //dodac zmiane czasu i date modyfikacji zeby byla taka sama ale nie terazniejsza
    
}


int main(int argc, char **argv)
{
    //deklaracja zmiennych zaleznych od parametrow
    char *plik_zrodlowy;
    char *plik_docelowy;
    int czas=3;
    bool rekurencja=NULL;
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

    //utworzenie logu o nazwie Daemon
    openlog("Daemon", LOG_PID, LOG_DAEMON);

    syslog(LOG_NOTICE,"DAEMON started work");
    //przywolanie demona
    ourDemon(plik_zrodlowy, plik_docelowy, czas, rekurencja);

    syslog(LOG_NOTICE, "Daemon ended work");
    printf("Demon cos tam zrobil");

    

    return 0;
}
