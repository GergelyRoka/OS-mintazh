//használt eszközök az első feladathoz
//fork
//signal
//nevtelen pipe

#include <stdio.h>
#include <stdlib.h>
#include <string.h> //String kezeleshez
#include <time.h>   //Rendszeridohoz
#include <errno.h>  //perror, errno
#include <unistd.h> //access, pipe
#include <signal.h>

// Deklarációk

void Parent(int, int *);   //Az elnok
void Child1(pid_t, int *); //Az ellenor
void Child2(pid_t);        //A pecsetelo

// 1. feladat - START
void Random_Number_Init();                     // randomszam generator inicializalasa (local time)
void Argumentum_check(int argc, char *argv[]); // parancssori argumentumok szamanak vizsgalata
int *UnnamedPipe();                            // egy nevtelen pipe letrehozasa
void ClosePipe(int *);                         // egy pipe vegeinek lezarasa

void PARENT_SIGUSR1_handler(int);                     // parenthez tartozo signal handler
void PARENT_SIGUSR2_handler(int);                     // parenthez tartozo signal handler
void PARENT_waiting_for_signals();                    // Parent a Child processek szignáljaira vár
void PARENT_Identification_Number_Sender(int, int *); // A szulo kuldi az azonositokat az Ellenor gyereknek csovon keresztul
void PARENT_Waiting_for_Death_of_Children();          // gyerekfolyamatok befejeződésére váró eljárás.

void CHILD1_RdySignalToParent(pid_t);              //Az Ellenor gyerek szignalt kuld a szulonek
void CHILD1_Identification_Number_receiver(int *); //Az Ellenor gyerek fogadja Pipe-on az azonositokat
void CHILD1_SayGoodbye();                          // Elkoszones

void CHILD2_RdySignalToParent(pid_t); // A pecsetelo gyerek szignalt kuld a szulonek.
void CHILD2_SayGoodbye();             // Elkoszones
// 1. feladat - END

// Definíciók

void Parent(int Voters_Number, int *Pipe) // Az elnok
{
    PARENT_waiting_for_signals();                             // "Az elnök megvárja, amíg mindkét tag nem jelzi, hogy kész a munkára (jelzéssel)"
    PARENT_Identification_Number_Sender(Voters_Number, Pipe); // "a parancssori argumentumokon keresztül "fogadja" a szavazókat (hány darab érkezik). A szülő generáljon minden szavazóhoz egy véletlen azonosítószámot! A választók azonosítószámait egy csövön keresztül továbbítja az adatellenőrző tagnak"
    PARENT_Waiting_for_Death_of_Children();                   // "A szülő bevárja a gyerekek befejezését. "
}

void Child1(pid_t Parent_ID, int *Pipe) // Az ellenor
{
    CHILD1_RdySignalToParent(Parent_ID);         // Szignált küld a szülőnek, hogy rdy to work.
    CHILD1_Identification_Number_receiver(Pipe); // A szülőtől a csövön érkező azonosítókat kiírja a konzolra.
    CHILD1_SayGoodbye();                         // Elkoszon, mert illedelmes.
}

void Child2(pid_t Parent_ID) // A pecsetelo
{
    CHILD2_RdySignalToParent(Parent_ID); // Szignált küld a szülőnek, h rdy to work.
    CHILD2_SayGoodbye();                 // Elkoszon, mert eltanulta a testvérétől.
}

//MAIN - START
int main(int argc, char *argv[])
{
    Argumentum_check(argc, argv);
    Random_Number_Init();

    int *Pipe = UnnamedPipe();

    pid_t child1 = fork();
    if (child1 > 0)
    {
        pid_t child2 = fork();
        if (child2 == 0)
        {
            ClosePipe(Pipe);   //nem hasznalt cso zarasa
            Child2(getppid()); // paraméter: <szülő pid-je>
        }
        else
        {
            Parent(atoi(argv[1]), Pipe); // paraméterek: <parancssori argumentumban megadott érték integerre konvertálva>, <csovezetek>
        }
    }
    else
    {
        Child1(getppid(), Pipe); // paraméterek: <szülő pid-je>, <csov>
    }

    return 0; //Ha minden oké, akkor itt ér véget mind a 3 folyamat (igy nem hasznalok exit(0)-t feleslegesen, pedig szeretem)
}
//MAIN - END

void Random_Number_Init()
{
    time_t t;
    srand(time(&t)); // local time-ot használja fel a random szám generálásához.
}

void Argumentum_check(int argc, char *argv[])
{
    if (argc != 2)
    {
        printf(" The Boss: Egy szam argumentumra van szukseg a folytatashoz.\n"); // valójában azt nézi, hogy pontosan egy parancssori argumetuma legyen
        exit(0);                                                                  // Lelovi a programot
    }
}

int *UnnamedPipe()
{
    static int pipefd[2];   // static, mert igy működik (hehe). Talán azért, mert így a csövünk fájlleírója nem a függvény stack memóriájába kerül, hanem a "Data Segment"-ben marad meg mindannyiunk örömére.
    if (pipe(pipefd) == -1) // ha sikertelen az integer tömb "pájposítása", akkor -1 a visszatérési értéke a pipe() fv-nek.
    {
        perror("Hiba uzenet: Hiba a pipe nyitaskor!");
        exit(EXIT_FAILURE);
    }
    return pipefd;
}

void ClosePipe(int *Pipe)
{
    close(Pipe[0]); // A cső olvasási oldalának lezárása.
    close(Pipe[1]); // A cső írási oldalának lezárása.
}

void PARENT_SIGUSR1_handler(int sig) //parent SIGUSR1 szignal handlere
{
    printf(" (signal) El Presidente: Master of Laws, a jegyzokonyvvezeto megerkezett! (signum: %d)\n", sig);
}

void PARENT_SIGUSR2_handler(int sig) //parent proccess SIGUSR2 szignal handlere
{
    printf(" (signal) El Presidente: Foka ur, a pecsetelo megerkezett! (signum: %d)\n", sig);
}

void PARENT_waiting_for_signals() //Parent a Child processek szignáljaira vár
{
    signal(SIGUSR1, PARENT_SIGUSR1_handler); // SIGUSR1 signal hozzákötése a saját handler-hez.
    signal(SIGUSR2, PARENT_SIGUSR2_handler); // SIGUSR2 signal hozzákötése a saját handler-hez.
    printf("\n El Presidente: En vagyok az Alfa es az Omega! (pid: %d)\n", getpid());
    pause(); // signal hatására tovább lép. (Child1-től kapja majd)
    pause(); // egy másik signal hatására itt is tovább lép. (Child2-től kapja majd)
}

void PARENT_Identification_Number_Sender(int Voters_Number, int *Pipe)
{
    int i = 0;
    int End = -1;                  // Utolsó utáni üzenet, végjel.
    int Identification_Number = 0; // azonosito, nem lesz majd egyedi
    printf("\n El Presidente: A szavazok szama %d.\n\n", Voters_Number);
    for (i = 0; i < Voters_Number; ++i)
    {
        Identification_Number = rand(); // Egy random szám generálás 0 és RAND_MAX között, ami a szerveren 2147483647 = 2^31-1, ráadásul még prím is (wiki szerint)
        write(Pipe[1], &Identification_Number, sizeof(Identification_Number));
    }
    write(Pipe[1], &End, sizeof(int));
    ClosePipe(Pipe);
}

void PARENT_Waiting_for_Death_of_Children()
{
    int status;
    //wait(&status); //ez nem varja meg az osszes kolkot
    while (wait(&status) > 0)
        ; // Megvárja az összes gyerekfolyamat végét/halálát/bezáródását.
    printf("\n El Presidente: Igy volt, nem igy volt, egbol pottyant mese volt.\n\n");
}

void CHILD1_RdySignalToParent(pid_t Parent_ID)
{
    sleep(1);                 // Altatás, hogy biztos, hogy érje a szülőnek a pause()-át.
    kill(Parent_ID, SIGUSR1); // SIGUSR1 szignal kuldese a szulonek
    printf(" Master of Laws: En vagyok a Torveny! (pid: %d)\n", getpid());
}

void CHILD1_Identification_Number_receiver(int *Pipe)
{
    int Identification_Number = 0;
    int i = 0;
    int End = -1; // Ez az utolsó azonosito utáni záróüzenet/végjel. -1
    sleep(3);
    close(Pipe[1]); // Cső írási végének lezárása.
    while (read(Pipe[0], &Identification_Number, sizeof(Identification_Number)))
    {
        sleep(1); // sec-es altatások, hogy ne legyen olyan heves a kiiras.
        ++i;
        if (Identification_Number != End)
            printf(" Master of Laws: %d. szavazo azonositoja %d.\n", i, Identification_Number);
    }
    close(Pipe[0]); //olvasasi veg zarasa
}

void CHILD1_SayGoodbye()
{
    printf("\n Master of Laws: Vegeztem. Lemondok.\n");
}

void CHILD2_RdySignalToParent(pid_t Parent_ID)
{
    sleep(3);                 // 3 sec-es altatás, hogy a szülőnek a második pause()-át érje.
    kill(Parent_ID, SIGUSR2); // SIGUSR2 signal küldése a szülőnek.
    printf(" Foka ur: En vagyok a pecsetmester! (pid: %d)\n", getpid());
}

void CHILD2_SayGoodbye()
{
    usleep(500); // Vár fél másodpercet, mert hezitál, hogy jó lesz-e még a buli, vagy inkább Fradi meccset nézzen. (De inkább meccset néz.)
    printf(" Foka ur: Vegeztem. Kikuszom.\n\n");
}