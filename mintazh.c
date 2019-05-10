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

// Macrok a párhuzamos csövek címzéséhez. Pipe[4]
// A masodik csovet tobb feladatra is hasznalok, nem feltetlen szerencses. Idozitessel oldom meg, hogy az egyik folyamat ne olvasson ki olyat, ami nem neki van szanva. (vajon a msgqueue osztalyozasa ebben segitene?)
#define rPIPE1 0 // Olvas az 1. csobol Pipe[0]
#define wPIPE1 1 // Ir az 1. csobe Pipe[1]
#define rPIPE2 2 // Olvas a 2. csobol Pipe[2]
#define wPIPE2 3 // Ir a 2. csobe Pipe[3]
#define PIPE1 0
#define PIPE2 1

// Deklarációk

void Parent(int, int *);   //Az elnok
void Child1(pid_t, int *); //Az ellenor
void Child2(pid_t, int *); //A pecsetelo

// 1. feladat - START
void Random_Number_Init();                     // randomszam generator inicializalasa (local time)
void Argumentum_check(int argc, char *argv[]); // parancssori argumentumok szamanak vizsgalata
int *UnnamedPipe();                            // egy nevtelen pipe letrehozasa
void ClosePipe(int *, int);                    // egy pipe vegeinek lezarasa

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

// 2. feladat - START
struct ID_and_Right; // struktúra, ID + jog a szavazáshoz

void PARENT_SIGUSR1_handler2(int); // Uj signal handler. SIGUSR1 és SIGUSR2 hasznalata/ujradefinialasa elengendo ezekhez a feladatokhoz, nem kell a tobbi signalt "piszkalni"
void PARENT_Result_Halfway(int *); // Szülő fogadja Pipe-on a szavazhatók és nem szavazhatók számát, majd százalékosan kiirja ide-oda.

void CHILD1_Verdict(int *, int, pid_t); // Az ellenor ID_and_Right struktúrát ír a csőbe, majd külcs egy signalt

void CHILD2_PID_Share(int *);               //Masodik pipe-on elkuldi a sajat pidjét.
void CHILD2_SIGUSR2_handler(int);           // A pecsetelo SIGUSR2 signal handlere.
int *CHILD2_Sealing(int *);                 // A Pipe-ról olvasott ID_and_Right struktúrákat dolgozza fel egy int Result[2] array-be.
void CHILD2_Result_to_PARENT(int *, int *); // Pipe-ra irja az eredményeket, majd külcs egy signalt a szülőnek.

// Definíciók

void Parent(int Voters_Number, int *Pipe) // Az elnok
{
    PARENT_waiting_for_signals();                             // "Az elnök megvárja, amíg mindkét tag nem jelzi, hogy kész a munkára (jelzéssel)"
    PARENT_Identification_Number_Sender(Voters_Number, Pipe); // "a parancssori argumentumokon keresztül "fogadja" a szavazókat (hány darab érkezik). A szülő generáljon minden szavazóhoz egy véletlen azonosítószámot! A választók azonosítószámait egy csövön keresztül továbbítja az adatellenőrző tagnak"
    PARENT_Result_Halfway(Pipe);                              // Szülő fogadja Pipe-on a szavazhatók és nem szavazhatók számát, majd százalékosan kiirja ide-oda.
    PARENT_Waiting_for_Death_of_Children();                   // "A szülő bevárja a gyerekek befejezését. "
}

void Child1(pid_t Parent_PID, int *Pipe) // Az ellenor
{
    CHILD1_RdySignalToParent(Parent_PID);        // Szignált küld a szülőnek, hogy rdy to work.
    CHILD1_Identification_Number_receiver(Pipe); // A szülőtől a csövön érkező azonosítókat kiírja a konzolra.
    CHILD1_SayGoodbye();                         // Elkoszon, mert illedelmes.
}

void Child2(pid_t Parent_PID, int *Pipe) // A pecsetelo
{
    CHILD2_PID_Share(Pipe);                // "Masodik" Pipe-on elkuldi a pid-jét.
    CHILD2_RdySignalToParent(Parent_PID);  // Szignált küld a szülőnek, h rdy to work.
    int *Result = CHILD2_Sealing(Pipe);    // A Pipe-ról olvasott ID_and_Right struktúrákat dolgozza fel egy int Result[2] array-be.
    CHILD2_Result_to_PARENT(Pipe, Result); // Pipe-ra irja az eredményeket, majd külcs egy signalt a szülőnek.
    CHILD2_SayGoodbye();                   // Elkoszon, mert eltanulta a testvérétől.
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
            Child2(getppid(), Pipe); // paraméter: <szülő pid-je>, <cso>
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

int *UnnamedPipe() // "párhuzamos" csövek, hogy megmutassuk, ilyet is tudunk. pipe[0]+pipe[1] pipe[2]+pipe[3]
{
    static int pipefd[4]; // static, mert igy működik (hehe). Talán azért, mert így a csövünk fájlleírója nem a függvény stack memóriájába kerül, hanem a "Data Segment"-ben marad meg mindannyiunk örömére.
    int i;
    for (i = 0; i < 2; ++i) // Megcsinálja az 1. csövet Pipe[0]-Pipe[1], majd a 2. csövet Pipe[2]-Pipe[3] ("a csövek fájlleírójá")
    {
        if (pipe(pipefd + i * 2) == -1) // ha sikertelen az integer tömb "pájposítása", akkor -1 a visszatérési értéke a pipe() fv-nek.
        {
            perror("Hiba uzenet: Hiba a pipe nyitaskor!");
            exit(EXIT_FAILURE);
        }
    }
    return pipefd;
}

void ClosePipe(int *Pipe, int i)
{
    close(Pipe[i]);     // A cső olvasási oldalának lezárása.
    close(Pipe[i + 1]); // A cső írási oldalának lezárása.
}

void PARENT_SIGUSR1_handler(int sig) //parent SIGUSR1 szignal handlere
{
    printf(" (SIGNAL) El Presidente: Master of Laws, a jegyzokonyvvezeto megerkezett! (signum: %d)\n", sig);
}

void PARENT_SIGUSR2_handler(int sig) //parent proccess SIGUSR2 szignal handlere
{
    printf(" (SIGNAL) El Presidente: Foka ur, a pecsetelo megerkezett! (signum: %d)\n", sig);
}

void PARENT_waiting_for_signals() //Parent a Child processek szignáljaira vár
{
    signal(SIGUSR1, PARENT_SIGUSR1_handler); // SIGUSR1 signal hozzákötése a saját handler-hez.
    signal(SIGUSR2, PARENT_SIGUSR2_handler); // SIGUSR2 signal hozzákötése a saját handler-hez.
    printf("\n (START) El Presidente: En vagyok az Alfa es az Omega! (pid: %d)\n", getpid());
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
        write(Pipe[wPIPE1], &Identification_Number, sizeof(Identification_Number));
    }
    write(Pipe[wPIPE1], &End, sizeof(int));
    ClosePipe(Pipe, PIPE1);
}

void PARENT_Waiting_for_Death_of_Children()
{
    int status;
    //wait(&status); //ez nem varja meg az osszes kolkot
    while (wait(&status) > 0)
        ; // Megvárja az összes gyerekfolyamat végét/halálát/bezáródását.
    printf("\n (END) El Presidente: Igy volt, nem igy volt, egbol pottyant mese volt.\n\n");
}

void CHILD1_RdySignalToParent(pid_t Parent_PID)
{
    sleep(1);                  // Altatás, hogy biztos, hogy érje a szülőnek a pause()-át.
    kill(Parent_PID, SIGUSR1); // SIGUSR1 szignal kuldese a szulonek
    printf(" (START) Master of Laws: En vagyok a Torveny! (pid: %d)\n", getpid());
}

void CHILD1_Identification_Number_receiver(int *Pipe)
{
    pid_t BroPID; // Tesvért, azaz a pecsételő PID-je.
    int Identification_Number = 0;
    int i = 0;
    int End = -1; // Ez az utolsó azonosito utáni záróüzenet/végjel. -1
    sleep(3);
    close(Pipe[wPIPE1]); // Cső írási végének lezárása.
    BroPID = CHILD1_Get_Bro_PID(Pipe);
    while (read(Pipe[rPIPE1], &Identification_Number, sizeof(Identification_Number)))
    {
        sleep(1); // sec-es altatások, hogy ne legyen olyan heves a kiiras.
        ++i;
        if (Identification_Number != End)
            printf("\n Master of Laws: %d. szavazo azonositoja %d.\n", i, Identification_Number);
        CHILD1_Verdict(Pipe, Identification_Number, BroPID);
    }
    close(Pipe[rPIPE1]); //olvasasi veg zarasa
}

void CHILD1_SayGoodbye()
{
    printf("\n (END) Master of Laws: Vegeztem. Lemondok.\n");
}

void CHILD2_RdySignalToParent(pid_t Parent_PID)
{
    sleep(3);                  // 3 sec-es altatás, hogy a szülőnek a második pause()-át érje.
    kill(Parent_PID, SIGUSR2); // SIGUSR2 signal küldése a szülőnek.
    printf(" (START) Foka ur: En vagyok a pecsetmester! (pid: %d)\n", getpid());
}

void CHILD2_SayGoodbye()
{
    usleep(500); // Vár fél másodpercet, mert hezitál, hogy jó lesz-e még a buli, vagy inkább Fradi meccset nézzen. (De inkább meccset néz.)
    printf(" (END) Foka ur: Vegeztem. Kikuszom.\n\n");
}
// 1. feladat - END

// 2. feladat - START
struct ID_and_Right // Az ellenor az ellenorzes után ilyen strukturat kuld a pecsetelonek.
{
    int ID;    // Azonositoszam
    int Right; // Savazhat/Nem Savazhat {0,1,2,3,4} 0 jelenti a nem szavazhatot kihasznalva azt, hogy C-ben 0 == Hamis
};

void PARENT_SIGUSR1_handler2(int sig) // Egy ujabb signal handler
{
    printf(" (SIGNAL) El Presidente: Keszen allok az eredmenyek feldolgozasara.\n");
}

void PARENT_Result_Halfway(int *Pipe)
{
    signal(SIGUSR1, PARENT_SIGUSR1_handler2); // SIGUSR1 signal "hozzakotese" az uj handlerhez.
    pause();                                  // Var, amig nem kap a pecsetelotol szignalt, hogy olvashat a csorol (a masodik csovet Pipe[2]-Pipe[3] tobb feladatra is hasznaljuk, ezert kell az idozites. Nem mondom, hogy szerencses)
    int Yes = 0, No = 0;
    read(Pipe[rPIPE2], &Yes, sizeof(int)); // Kiolvassa a csőről a szavazó emberek számát
    read(Pipe[rPIPE2], &No, sizeof(int));  // Kiolvasssa a csőről a nem szavazó emberek számát
    FILE *ResultTXT;
    ResultTXT = fopen("Result.txt", "w+"); // (nem tudom leírni szépen magyarul, se) Result.txt-t létrehozza/megnyítja (w+)-szal, azaz egy blank file-t kapunk, amit írhatunk.
    if ((Yes + No) != 0)
    {
        printf(" El Presidente: %.2f%% szavazhat\n                %.2f%% nem szavazhat\n", (float)(Yes) / (Yes + No) * 100, (float)(No) / (Yes + No) * 100);
        fprintf(ResultTXT, "  %.2f%% szavazhat\n  %.2f%% nem szavazhat\n", (float)Yes / (Yes + No) * 100, (float)No / (Yes + No) * 100);
    }
    else
    {
        printf(" El Presidente: A 0 db szavazo 100%% hogy szavazhat, es 100%% hogy nem szavazhat.\n");
        fprintf(ResultTXT, "A 0 db szavazo 100%% hogy szavazhat, es 100%% hogy nem szavazhat.\n");
    }
    fclose(ResultTXT); // a fajl bezárása lényeges a megfelelő mentés érdekében.
}

pid_t CHILD1_Get_Bro_PID(int *Pipe) // Ha máshogy fork-olok, akkor erre nem lenne szükség, mert ez a folyamat ismerné a testvérének a PID-jét, akinek majd signal-t kell küldenie.
                                    // De igy a 2. Pipe-on keresztül olvassa ki a tesvére által küldött PID-et.
{
    pid_t BroPID;                                        // testvérének pid-jéhez létrehozott pid_t változó
    if (read(Pipe[rPIPE2], &BroPID, sizeof(BroPID)) < 0) // ha valami gond adódik, akkor -1 a visszatérési érték, amúgy vagy az van, hogy vár, amíg lehet, hogy tud olvasni, vagy ha sikerült, akkor nem -1 a visszatérési érték.
        printf(" (HIBA) Master of Laws: Nem sikerult kiolvasni Foka ur ID-jat.\n");
    return BroPID;
}

void CHILD1_Verdict(int *Pipe, int ID, pid_t BroPID) // Az Ellenor itelete
{
    struct ID_and_Right IDR = {ID, rand() % 5}; // ID_and_Right->ID lesz a szavazo azonositoja, ID_and_Right->Right 0-4 random szam lesz. 0 hamisnak lesz kiertekelve
    write(Pipe[wPIPE2], &IDR, sizeof(IDR));     // A 2. csobe irja a struktúrát.
    kill(BroPID, SIGUSR2);                      // Testvérnek küldött SIGUSR2 szignál.
}

void CHILD2_PID_Share(int *Pipe) // A pecsetelo megosztja egy csovon a sajat pid-jet.
{
    pid_t MyPid = getpid();                             // saját pid
    ClosePipe(Pipe, PIPE1);                             //nem hasznalt cso zarasa
    if (write(Pipe[wPIPE2], &MyPid, sizeof(pid_t)) < 0) // -1 a visszatérési érték, ha nem sikerült írni, amőgy a 2. csőbe kiírja a pid_t-jét.
        printf("(HIBA) Foka ur: Masodik CSobe nem sikerult irni.\n");
}

void CHILD2_SIGUSR2_handler(int sig) // A pecsetelonek szant SIGUSR2 handlere. SIGCONT is lehet, teljesen jo lehet alapbol, nem kene vacakolni mas signallal, handlerrel. Kov verzioban meglesem.
                                     //SIGCONT	18	Continue (POSIX) Signal sent to process to make it continue.
{
    ;
}

int *CHILD2_Sealing(int *Pipe) // A csovon erkezo ID_and_Right osszesítése Result[2] tömbbe szavazhat/nem szavazhat száma
{
    static int Result[2]; // Alapból 0-ra inicializálja. Static, hogy visszatteresi ertek lehessen.
    //Result[0]=0;
    //Result[1]=0;
    struct ID_and_Right IDR = {0, 0};
    signal(SIGUSR2, CHILD2_SIGUSR2_handler);

    while (IDR.ID != -1) // az Ellenor a -1 vegjelet is elkuldi, innen tudjuk, mikor kell abbahagyni az olvasast. dowhile is jó lenne itt.
    {
        pause();                               // A feladat szerint minden olvasas elott kapnia kell egy signalt.
        read(Pipe[rPIPE2], &IDR, sizeof(IDR)); // Olvas a 2. csorol
        if (IDR.ID != -1)                      // Ha az azonosito nem vegjel, akkor ellenorzi a szavazati jogot.
        {
            if (IDR.Right)
            {
                ++Result[0]; // szavazo polgarok szama
                printf(" Foka ur: %d azonositoju szavazo, szavazhat! :-)\n", IDR.ID);
            }
            else
            {
                ++Result[1]; // nem szavazhato polgarok szama
                printf(" Foka ur: %d azonositoju szavazo, nem szavazhat! :'(\n", IDR.ID);
            }
        }
    }
    return Result;
}

void CHILD2_Result_to_PARENT(int *Pipe, int *Result) // szavazo es nem szavazo emberek szamanak kuldese pipe-on + signal
{
    write(Pipe[wPIPE2], &Result[0], sizeof(int)); // tombot nem tudok egyszerre atkudeni, struktura itt is menobb lenne, mint igy egyenkent.
    write(Pipe[wPIPE2], &Result[1], sizeof(int));
    kill(getppid(), SIGUSR1); // SIGUSR1 signal kuldese
}
