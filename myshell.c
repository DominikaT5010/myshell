#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <sys/stat.h>
#include <errno.h>
#include <dirent.h>
#include <fcntl.h>
#include <sys/utsname.h>
#include <sys/wait.h>

#define stU 41 // število vgrajenih ukazov

int tokens[25];
int token_count = 0;

char zgodovina[256][2048];
int zgInx = 0; // kje smo v bufferju zgodovine
int nazaj = 0; // koliko smo zavrnili ukazov ob premikanju v zgodovino

char vhod[1024] = "";
char izhod[1024] = "";
int background = 0; // če true je nastavljeno izvajanje v ozadju

typedef void (*klicFunkcije)(char*);
typedef struct ukaz {
    char* ime;
    klicFunkcije funkcija;
    char* opis;
} ukaz;

int cmp(const void *a, const void *b) {
    return (*(int*)a - *(int*)b);
}

typedef struct info{
    int pid, ppid;
    char stanje;
    char ime[256];
} info;
int cmpPid(const void *a, const void *b) {
    return ((struct info*)a)->pid - ((struct info*)b)->pid;
}

int tolkenize (char* vrstica){
    int dolzina = strlen(vrstica);

    token_count = 0;
    int narekovaj = 0; // če true se nahajamo med narekovaji
    int presledek = 1; // če true je bil prejšnji znak presledek in gre za nov argument
    int prazno = 1; // če false je vsaj en znak, ki ni witespace v vrstici

    for (int i = 0; i < 2048; i++) {
        if (vrstica[i] == '\0') break;
        if (vrstica[i] == '\n') {
            vrstica[i] = '\0';
            break;
        }
        if (!isspace(vrstica[i])) prazno = 0; // preverjanje če gre za neprazno vrstico
            
        if (vrstica[i] == '"' && !narekovaj) { // preverjanje, če smo prišli do narekovaja za začetek
            narekovaj = 1;
            continue;
        }
        else if (vrstica[i] == '"' && narekovaj) { // ali narekovaja za konec
            vrstica[i] = '\0';
            narekovaj = 0;
            continue;
        }

        if (presledek && (vrstica[i] != ' ' || narekovaj)) {
            presledek = 0;
            if (vrstica[i] == '#') { // pregledamo če je znak za komentar in ustavimo prebiranje naprej, ker nas ne zanima
                vrstica[i] = '\0';
                token_count--;
                break;
            }
            tokens[token_count] = i; // dodamo indeks zacetka novega argumenta            
        }
        if (vrstica[i] == ' ' && !narekovaj && !presledek){ // če nismo v narekovajih in je znak presledek povečamo števec argumentov
            token_count++;
            presledek = 1;
            vrstica[i] = '\0';
        }
    }

    if (prazno) token_count = -1;
    if (presledek) token_count--;
    return dolzina;
}

void printToken (char* vrstica, int dolzina){
    int j = 0;
    
    printf("Input line: '");
    for (int i = 0; i < dolzina-1; i++){
        if (vrstica[i] == '\0') printf(" ");
        else printf("%c", vrstica[i]);
    }
    printf("'\n");
    
    if (token_count == -1) return;

    for (int i = 0; i <= token_count; i++) {
        j = tokens[i];
        if (vrstica[j] == '\0') break;

        printf("Token %d: '", i);
        
        while (vrstica[j] != '\0') {
            if (vrstica[j] == '"') {
                j++;
                continue;
            }
            printf("%c", vrstica[j]);
            j++;
        }
        printf("'\n");        
    }
    fflush(stdout);

    return;
}

void preusmeritve (char* vrstica){
    for (int i = 0; i < 3; i++) {  
        int j = tokens[token_count];  
        if (vrstica[j] == '&'){
            background = 1;
            token_count--;
        }
        if (vrstica[j] == '>'){
            j++;
            strcpy(izhod, &vrstica[j]);
            token_count--;
        }
        if (vrstica[j] == '<'){
            j++;
            strcpy(vhod, &vrstica[j]);
            token_count--;
        }
    }

    return;
}

void printPreusmeritve (char* vrstica){
    if (strlen(vhod) > 0){
        printf("Input redirect: '%s'\n", vhod);
    }
    if (strlen(izhod) > 0){
        printf("Output redirect: '%s'\n", izhod);
    }
    if (background){ // izpis nastavitve za izvajanje v ozadju
        printf("Background: 1\n");
    }
    fflush(stdout);

    return;
}


int nivoRazhroscevanja = 0;
int returnStatus = 0;
char pozivnik[8] = "mysh";
char procfs[25] = "/proc";

// funkcije za izvajanje vgrajenih ukazov
void dbg (char* vrstica){
    if (token_count < 1){
        printf("%d\n", nivoRazhroscevanja);
        fflush(stdout);

    } else {
        char temp[100] = ""; // preberem level
        int j = tokens[1];
        while (vrstica[j] != '\0'){
            temp[strlen(temp)] = vrstica[j];
            temp[strlen(temp) + 1] = '\0';
            j++;
        }

        // preverim kaj je vrednost in če ni veljavna privzamem 0
        nivoRazhroscevanja = atoi(temp);
    }
    returnStatus = 0;
    return;
}

void prmt (char* vrstica){
    returnStatus = 0;
    if (token_count < 1){
        printf("%s\n", pozivnik);
        fflush(stdout);
    } else {
        char temp[100] = ""; // preberem pozivnik
        int j = tokens[1];
        while (vrstica[j] != '\0'){
            temp[strlen(temp)] = vrstica[j];
            temp[strlen(temp) + 1] = '\0';
            j++;
        }

        if (strlen(temp) <= 8){ // nastavi se nov pozivnik, če je pravilne oblike
            strcpy(pozivnik, temp);
        } else {
            returnStatus = 1;
        }
    }
    return;
}

void stst (char* vrstica){
    printf("%d\n", returnStatus);
    fflush(stdout);
    return;
}

void ext (char* vrstica){
    if (token_count < 1){
        exit(returnStatus);
    } else {
        char temp[100] = ""; // preberem izhodni status
        int j = tokens[1];
        while (vrstica[j] != '\0'){
            temp[strlen(temp)] = vrstica[j];
            temp[strlen(temp) + 1] = '\0';
            j++;
        }

        exit(atoi(temp));
    }
}

void hlp (char* vrstica);

void prt (char* vrstica){
    for (int i = 1; i <= token_count; i++){
        int j = tokens[i];
        if (vrstica[j] == '\0') break;
        while(vrstica[j] != '\0'){
            printf("%c", vrstica[j]);
            j++;
        }
        if (i < token_count) printf(" ");
        fflush(stdout);
    }
    return;
}

void ech (char* vrstica){
    for (int i = 1; i <= token_count; i++) {
        printf("%s\n", &vrstica[tokens[i]]);
    }
    fflush(stdout);
    return;
}

void ln (char* vrstica){
    int dolzina = 0;
    for (int i = 1; i <= token_count; i++){
        int j = tokens[i];
        while(vrstica[j] != '\0'){
            dolzina++;
            j++;
        }
    }
    printf("%d\n", dolzina);
    fflush(stdout);
    return;
}

void sm (char* vrstica){
    int vsota = 0;
    int argument = 0;
    for (int i = 1; i <= token_count; i++){
        int j = tokens[i];
        
        argument = atoi(&vrstica[j]);
        vsota += argument;
    }
    printf("%d\n", vsota);
    fflush(stdout);
    return;
}

void clc (char* vrstica){
    int arg1 = atoi(&vrstica[tokens[1]]);
    int arg2 = atoi(&vrstica[tokens[3]]);

    char op = vrstica[tokens[2]];
    switch (op)
    {
    case '+':
        printf("%d\n", arg1 + arg2);
        break;
    
    case '-':
        printf("%d\n", arg1 - arg2);
        break;

    case '*':
        printf("%d\n", arg1 * arg2);
        break;

    case '/':
        printf("%d\n", arg1 / arg2);
        break;

    case '%':
        printf("%d\n", arg1 % arg2);
        break;
    
    default:
        break;
    }
    fflush(stdout);
    return;
}

void bsnm (char* vrstica){
    if (token_count < 1){
        returnStatus = 1;
        return;
    }
    int j = tokens[1];
    int slash = j-1;
    while (vrstica[j] != '\0'){
        if (vrstica[j] == '/') slash = j;
        j++;
    }

    j = slash + 1;
    while (vrstica[j] != '\0'){
        printf("%c", vrstica[j]);
        j++;
    }
    printf("\n");
    fflush(stdout);
    return;
}

void drnm (char* vrstica){
    if (token_count < 1){
        returnStatus = 1;
        return;
    }

    int j = tokens[1];
    int slash = j-1;
    while (vrstica[j] != '\0'){
        if (vrstica[j] == '/') slash = j;
        j++;
    }

    j = tokens[1];
    if (slash == j-1){
        printf(".\n");
        return;
    }

    while (j < slash){
        printf("%c", vrstica[j]);
        j++;
    }
    printf("\n");
    fflush(stdout);

    return;
}

// delo z imeniki
void drch (char* vrstica){
    char temp[100];
    if (token_count < 1){
        strcpy(temp, "/");
    } else {
        strcpy(temp, &vrstica[tokens[1]]);
    }
    if (chdir(temp) == -1 && token_count > 0){
        returnStatus = errno;
        perror("dirch");
        fflush(stdout);
    } else returnStatus = 0;
    return;
}

void drwd (char* vrstica){
    char trImenik[1024];
    if (getcwd(trImenik, sizeof(trImenik)) != NULL){
        if (token_count > 0 && strcmp(&vrstica[tokens[1]], "full") == 0){
            printf("%s\n", trImenik);
            fflush(stdout);
        } else {
            int j = 0;
            int slash = -1;
            while (trImenik[j] != '\0'){
                if (trImenik[j] == '/') slash = j;
                j++;
            }

            j = slash + 1;
            if (trImenik[j] == '\0') printf("%c", trImenik[j-1]);
            while (trImenik[j] != '\0'){
                printf("%c", trImenik[j]);
                j++;
            }
            printf("\n");
            fflush(stdout);
        }
    }
    return;
}

void drmk (char* vrstica){
    char temp[100];
    int j = tokens[1];
    strcpy(temp, &vrstica[j]);

    if (mkdir(temp, 0755) == -1){
        returnStatus = errno;
        perror("dirmk");
        fflush(stdout);
    } else returnStatus = 0;

    return;
}

void drrm (char* vrstica){
    char temp[1024];
    strcpy(temp, &vrstica[tokens[1]]);

    if (rmdir(temp) == -1){
        returnStatus = errno;
        perror("dirrm");
        fflush(stdout);
    } else returnStatus = 0;

    return;
}

void drls (char* vrstica){
    char temp[1024] = ".";
    if (token_count > 0) strcpy(temp, &vrstica[tokens[1]]);
    
    struct dirent *entry;
    DIR *dir = opendir(temp);

    if (dir == NULL){
        returnStatus = 1;
        return;
    }
    entry = readdir(dir);
    printf("%s", entry->d_name);

    while ((entry = readdir(dir)) != NULL){
        printf("  %s", entry->d_name);
    }
    printf("\n");
    fflush(stdout);
    
    closedir(dir);
    return;
}

// delo z datotekami
void rnm (char* vrstica){
    char* izvor = &vrstica[tokens[1]];
    char* ponor = &vrstica[tokens[2]];

    if (rename(izvor, ponor) != 0){
        returnStatus = errno;
        perror("rename");
    } else returnStatus = 0;
    return;
}

void unlk (char* vrstica){
    char* temp = &vrstica[tokens[1]];
    returnStatus = 0;
    unlink(temp);
    return;
}

void rmv (char* vrstica){
    char* temp = &vrstica[tokens[1]];
    
    if (remove(temp) != 0){
        returnStatus = errno;
        perror("remove");
    } else returnStatus = 0;
    return;
}

void lnkhrd (char* vrstica){
    char* cilj = &vrstica[tokens[1]];
    char* ime = &vrstica[tokens[2]];

    if (link(cilj, ime) != 0){
        returnStatus = errno;
        perror("linkhard");
    } else returnStatus = 0;
    return;
}

void lnksft (char* vrstica){
    char* cilj = &vrstica[tokens[1]];
    char* ime = &vrstica[tokens[2]];

    if (symlink(cilj, ime) != 0){
        returnStatus = errno;
        perror("linksoft");
    } else returnStatus = 0;
    return;
}

void lnkrd (char* vrstica){
    char* ime = &vrstica[tokens[1]];
    char cilj[100];

    int dolzina = readlink(ime, cilj, sizeof(cilj)-1);
    if (dolzina == -1){
        returnStatus = errno;
        perror("linkread");
        return;
    } else returnStatus = 0;

    cilj[dolzina] = '\0';
    printf("%s\n", cilj);
    return;
}

void lnklst (char* vrstica){
    char* ime = &vrstica[tokens[1]];
    struct stat cilj;

    if (stat(ime, &cilj) != 0){
        returnStatus = errno;
        perror("stat");
    }

    DIR* dir = opendir(".");
    if (!dir){
        perror("opendir");
        returnStatus = errno;
        return;
    }

    struct dirent* entry;
    int prvo = 1;

    while ((entry = readdir(dir)) != NULL){
        struct stat trenutni_stat;
        if (stat(entry->d_name, &trenutni_stat) != 0) continue;

        if (trenutni_stat.st_ino == cilj.st_ino){
            if (!prvo) printf("  "); // dva presledka
            printf("%s", entry->d_name);
            prvo = 0;
        }
    }
    printf("\n");
    fflush(stdout);
    closedir(dir);
    returnStatus = 0;

    return;
}

void cpct (char* vrstica){
    int in;
    int out;
    if (token_count < 1 || strcmp(&vrstica[tokens[1]], "-") == 0){
        in = STDIN_FILENO;
    } else {
        in = open(&vrstica[tokens[1]], O_RDONLY);
        if (in == -1){
            returnStatus = errno;
            perror("cpcat");
            return;
        }
    }
    if (token_count < 2){
        out = STDOUT_FILENO;
    } else {
        out = open(&vrstica[tokens[2]], O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (out == -1){
            returnStatus = errno;
            perror("cpcat");
            return;
        }
    }

    unsigned char trenutni;
    int r, w;
    while ((r = read(in, &trenutni, 1)) > 0){
        w = write(out, &trenutni, 1);
        if (w == -1){
            returnStatus = errno;
            perror("cpcat");
            return;
        }
    }
    if (r == -1){
        returnStatus = errno;
        perror("cpcat");
        return;
    }

    if (in != STDIN_FILENO) close(in);
    if (out != STDOUT_FILENO) close(out);

    return;
}


void pd (char* vrstica){
    returnStatus = 0;
    printf("%d\n", getpid());
    fflush(stdout);
    return;
}

void ppd (char* vrstica){
    returnStatus = 0;
    printf("%d\n", getppid());
    fflush(stdout);
    return;
}

void ud (char* vrstica){
	returnStatus = 0;
    printf("%d\n", getuid());
    fflush(stdout);
    return;
}

void eud (char* vrstica){
    returnStatus = 0;
    printf("%d\n", geteuid());
    fflush(stdout);
    return;
}

void gd (char* vrstica){
    returnStatus = 0;
    printf("%d\n", getgid());
    fflush(stdout);
    return;
}

void egd (char* vrstica){
    returnStatus = 0;
    printf("%d\n", getegid());
    fflush(stdout);
    return;
}

void sysinf (char* vrstica){
    returnStatus = 0;
    struct utsname temp;
    if (uname(&temp) != 0){
        returnStatus = errno;
        perror("systeminfo");
        return;
    }
    printf("Sysname: %s\nNodename: %s\nRelease: %s\nVersion: %s\nMachine: %s\n", 
            temp.sysname, temp.nodename, temp.release, temp.version, temp.machine);
    return;
}


void prc (char* vrstica){
    returnStatus = 0;
    if (token_count > 0){
        char* temp = &vrstica[tokens[1]];
        if (access(temp, F_OK | R_OK) != 0){
            returnStatus = 1;
            return;
        }
        strcpy(procfs, temp);
    } else {
        printf("%s\n", procfs);
    }
    return;
}

void pds (char* vrstica){
    int pids[1024];
    int count = 0;

    struct dirent *entry;
    DIR *dir = opendir(procfs);

    if (dir == NULL){
        returnStatus = 1;
        return;
    }

    while ((entry = readdir(dir)) != NULL){
        if (atoi(entry->d_name) != 0){
            int pid = atoi(entry->d_name);
            if (pid > 0 && count < 1024) {
                pids[count] = pid;
                count++;
            }
        }
    }

    qsort(pids, count, sizeof(int), cmp);

    for (int i = 0; i < count; i++){
        printf("%d\n", pids[i]);
        fflush(stdout);
    }
    
    closedir(dir);
    return;
}

void pnfo (char* vrstica){
    struct dirent *entry;
    DIR *dir = opendir(procfs);

    if (dir == NULL){
        returnStatus = 1;
        return;
    }

    printf("%5s %5s %6s %s\n", "PID", "PPID", "STANJE", "IME");
    fflush(stdout);

    info informacija[1024];
    int count = 0;    

    while ((entry = readdir(dir)) != NULL){
        if (atoi(entry->d_name) != 0){
            char path[1024];
            snprintf(path, sizeof(path), "%s/%s/stat", procfs, entry->d_name);

            FILE* fp = fopen(path, "r");

            int pid, ppid;
            char stanje;
            char ime[256];
            if (fscanf(fp, "%d (%[^)]) %c %d", &pid, ime, &stanje, &ppid) == 4) {
                informacija[count].pid = pid;
                informacija[count].ppid = ppid;
                informacija[count].stanje = stanje;
                strcpy(informacija[count].ime, ime);
                count++;
            }

            fclose(fp);
        }
    }

    closedir(dir);

    qsort(informacija, count, sizeof(info), cmp);

    for (int i = 0; i < count; i++) {
        printf("%5d %5d %6c %s\n", informacija[i].pid, informacija[i].ppid, informacija[i].stanje, informacija[i].ime);
    }
    fflush(stdout);
    
    return;
}


void wtone (char* vrstica){
    int status;
    if (token_count == 0){ // če PID ni podan čaka na enega otroka
        if (wait(&status) == -1){ // noben otrok ne obstaja več
            returnStatus = 0;
            return;
        }
        if (WIFEXITED(status)) {
            returnStatus = WEXITSTATUS(status);
        }
    } else {
        int pid = atoi(&vrstica[tokens[1]]); // podani PID je uporabljen za čakanje
        if (waitpid(pid, &status, 0) == -1){ // napaka pri čakanju
            returnStatus = 0;
            return;
        }
        if (WIFEXITED(status)) {
            returnStatus = WEXITSTATUS(status);
        }
    }
    return;
}

void wtall (char* vrstica){
    int status;
    while (wait(&status) != -1){ // čaka na otroke dokler ne ulovi vseh
        if (WIFEXITED(status)) {
            returnStatus = WEXITSTATUS(status);
        }
    }
    returnStatus = 0;
    return;
}


void pip (char* vrstica);


void clr (char* vrstica){
    if (token_count == 0) { // če ni argumentov se ponastavijo barve
        printf("\33[0m");
        fflush(stdout);
        return;
    }

    int tok = 1;
    // preveri nastavitev barve besedila
    char* barve[] = {"black", "red", "green", "orange", "blue", "purple", "cyan", "white", "rgb", "-"};
    for (int i = 0; i < 10; i++){
        if (strcmp(&vrstica[tokens[1]], barve[i]) == 0){
            if (i == 9) { // če ponastavimo samo to barvo
                printf("\33[3%dm", i);
                fflush(stdout);
                break;
            } else if (i == 8){ // v primeru barve po meri se upoševa še dodatni argument
                tok++;
                printf("\33[3%d;2;%sm", i, &vrstica[tokens[tok]]);
                fflush(stdout);
                break;
            }
            printf("\33[9%dm", i);
            fflush(stdout);
            break;
        }
    }
    tok++;

    // če je dovolj argumentov se preveri še nastavitev barve ozadja
    if (token_count >= tok){
        char* barveOz[] = {"black", "red", "green", "orange", "blue", "purple", "cyan", "white", "rgb", "-"};
        for (int i = 0; i < 10; i++){
            if (strcmp(&vrstica[tokens[tok]], barveOz[i]) == 0){
                if (i == 9) { // če ponastavimo samo to barvo
                    printf("\33[4%dm", i);
                    fflush(stdout);
                    break;
                } else if (i == 8){ // v primeru barve po meri se upoševa še dodatni argument
                    tok++;
                    printf("\33[4%d;2;%sm", i, &vrstica[tokens[tok]]);
                    fflush(stdout);
                    break;
                }
                printf("\33[10%dm", i);
                fflush(stdout);
                break;
            }
        }
    }
    return;
}

void hist (char* vrstica) {
    for (int i = 0; i < zgInx; i++){ // izpiše dosedanje ukaze
        printf("%s\n", zgodovina[i]);
    }
    fflush(stdout);
    return;
}

void bfr (char* vrstica);

// vgrajeni ukazi
ukaz ukazi[] = {{"debug", dbg, "pove nivo razhroščevanja in če ni argumenta ga izpiše, sicer je nivo podani nivo"}, 
                {"prompt", prmt, "nastavimo pozivnik, ki ima lahko do 8 znakov"}, 
                {"status", stst, "izpiše se izhodnji status prejšnjega ukaza in se status ne spremeni"}, 
                {"exit", ext, "konča program z podanim izhodnim statusom ali statusom zadnjega ukaza"}, 
                {"help", hlp, "ipiše podprte ukaze"},
                {"print", prt, "izpiše argumente brez izpisa nove vrstice"},
                {"echo", ech, "izpiše argumente z novo vrstico"},
                {"len", ln, "izpiše dolžino argumentov"},
                {"sum", sm, "sešteje svoje argumente in izpiše vsoto"},
                {"calc", clc, "nad argumentoma naredi eno od operacij +, -, *, /, %"},
                {"basename", bsnm, "izpiše osnovno ime podane poti argumenta, če ga ni vrne status 1"},
                {"dirname", drnm, "izpiše imenik podane poti argumenta, če ga ni vrne status 1"},
                // delo z imeniki
                {"dirch", drch, "zamenja trenutni delavni imenik, brez imenika skoči v korenski"},
                {"dirwd", drwd, "izpiše trenutni delavni imenik, če mode FULL celotnega, sicer samo osnovo imena"},
                {"dirmk", drmk, "ustvari nov imenik"},
                {"dirrm", drrm, "izbriše podani imenik"},
                {"dirls", drls, "izpiše imena datotek tega imenika ločena z dvema presledkoma"},
                // delo z datotekami
                {"rename", rnm, "preimenovanje datoteke arg1 v arg2"},
                {"unlink", unlk, "odstrani datoteko s podanim imenom"},
                {"remove", rmv, "odstrani datoteko ali imenik s podanim imenom"},
                {"linkhard", lnkhrd, "ustvari trdo povezavo iz arg2 na arg1"},
                {"linksoft", lnksft, "ustvari simbolično povezavo iz arg2 na arg1"},
                {"linkread", lnkrd, "izpiše cilj podane simbolične povezave"},
                {"linklist", lnklst, "v delavnem imeniku poišče vse trde povezave na podano datoteko ločene z dvema presledkoma"},
                {"cpcat", cpct, "združi cp in cat - kopira iz arg1 v arg2 oz iz stdin in stdout, če nista podana"},
                
                {"pid", pd, "izpiše PID te lupine"},
                {"ppid", ppd, "ižpiše PID starševskega procesa te lupine"},
                {"uid", ud, "izpiše UID uporabnika, ki je lastnik procesa lupine"},
                {"euid", eud, "izpiše UID uporabnika, ki je aktualen lastnik lupine"},
                {"gid", gd, "izpiše GID skupine, ki ji pripada lupina"},
                {"egid", egd, "izpiše GID skupine, ki ji aktualno pripada lupina"},
                {"sysinfo", sysinf, "izpiše osnovne informacije o sistemu"},
                
                {"proc", prc, "nastavi pot do procf sistema, če argument ni podan je /proc"},
                {"pids", pds, "izpiše vse PIDe trenutnih procesov iz procfs"},
                {"pinfo", pnfo, "izpiše informacije o trenutnih procesih iz stat v procfs"},
                
                {"waitone", wtone, "počaka na otroka s podanim PID, če ta ni podan čaka na poljubnega otroka"},
                {"waitall", wtall, "počaka na vse otroke"},
                
                {"pipes", pip, "ustvari cevovod z vsaj 2 stopnjama"},
                
                {"color", clr, "spremeni barvo izpisa na black, red, green, orange, blue, purple, cyan, če želiš barvo ponastaviti - na enem argumentu ali brez argumentov za ponastavitev obeh, za barvo po meri je argument rgb r;g;b"},
                {"history", hist, "izpiše zgodovino ukazov lupine"},
                {"!", bfr, "izvaja prejšnji ukaz"}};


// kliče funkcijo ukaza, ki je bil razpoznan
void execute_builtin(int inx, char* vrstica){
    if (nivoRazhroscevanja > 0){ // izpisi se naredijo le kadar je nivo razhroščevanja dovolj visok
        if (!background) { // preveri, kje se izvaja ukaz
            printf("Executing builtin '%s' in foreground\n", ukazi[inx].ime);
        } else {
            printf("Executing builtin '%s' in background\n", ukazi[inx].ime);
        }
        fflush(stdout);
    }
    
    // če se izvaja ukaz v ozadju mora nastati nov proces
    if (background || strlen(vhod) > 0 || strlen(izhod) > 0) {
        sleep(0.2);
        fflush(stdin);
        int pid = fork();
        if (pid < 0){
            returnStatus = errno;
            perror(ukazi[inx].ime);
            return;
        } else if (pid == 0){ // pri izvajanju v ozadju nastane otrok
            // preusmeri vhod in izhod, če je to potrebno
            if (strlen(vhod) > 0){
                int vh = open(vhod, O_RDONLY);
                dup2(vh, 0);
                close(vh);
            }
            
            if (strlen(izhod) > 0){
                int izh = open(izhod, O_WRONLY | O_CREAT | O_TRUNC, 0644);
                dup2(izh, 1);
                close(izh);
            }
            
            ukazi[inx].funkcija(vrstica);
            exit(returnStatus);
        }

        // če ne gre za izvajanje v ozadju počaka na otroka preden gre naprej
        if (!background){
            int status;
            waitpid(pid, &status, 0); // čaka točno določenega otroka
            if (WIFEXITED(status)) {
                returnStatus = WEXITSTATUS(status);
            }
        }
    } else ukazi[inx].funkcija(vrstica);
    return;
}

void execute_external(char *imeUkaza, char* vrstica){
    if (token_count < 0) return;
    char* arg[26]; // ustvari seznam argumentov ukaza, ki ga bomo zaganjali
    arg[token_count + 1] = NULL; // zadnje mesto seznama mora povedati, da je argumentov konec
    
    for (int i = 0; i <= token_count; i++){
        arg[i] = &vrstica[tokens[i]];
    }

    fflush(stdin);
    int pid = fork(); // zunanji ukazi se izvajajo v svojem procesu
    returnStatus = 0;

    if (pid < 0){
        returnStatus = errno;
        perror(imeUkaza);
        return;
    } else if (pid == 0){ // otrok
        // preusmeri vhod in izhod, če je to potrebno
        if (strlen(vhod) > 0){
            int vh = open(vhod, O_RDONLY);
            dup2(vh, 0);
            close(vh);
        }

        if (strlen(izhod) > 0){
            int izh = open(izhod, O_WRONLY | O_CREAT | O_TRUNC, 0644);
            dup2(izh, 1);
            close(izh);
        }

        int ex = execvp(imeUkaza, arg);
        perror("exec");
        returnStatus = 127;
        return;
    }
    if (!background) { // preveri, kje se izvaja ukaz, če ni v ozadju čaka na konec izvajanja
        int status;
        waitpid(pid, &status, 0); // čaka točno določenega otroka
        if (WIFEXITED(status)) {
            returnStatus = WEXITSTATUS(status);
        }
    }
    return;
}

// poišče kateri ukaz se bo izvajal
void find_builtin(char *vrstica){
    char temp[100] = ""; // ukaz shranjen v temp, da ga lahko primerjam z poznanimi ukazi
    int j = 0;
    while (vrstica[j] != '\0'){
        temp[strlen(temp)] = vrstica[j];
        temp[strlen(temp) + 1] = '\0';
        j++;
    }

    for (int i = 0; i < stU; i++){ // primerja ime ukaza z vsemi vgrajenimi ukazi
        if (strcmp(temp, ukazi[i].ime) == 0){
            execute_builtin(i, vrstica);
            return;
        }
    }
    execute_external(temp, vrstica); // če ne najde vgrajenega kliče zunanje izvajanje
    return;
}

void hlp (char* vrstica){
    if (token_count == 0) { // brez argumentov se izpišejo vsi vgrajeni ukazi
        for (int i = 0; i < stU; i++){
            printf("\33[1m%s\33[22m - %s\n", ukazi[i].ime, ukazi[i].opis);
        }
    } else { // če je en specificiran se izpiše pomoč le za tistega
        for (int i = 0; i < stU; i++){
            if (strcmp(&vrstica[tokens[1]], ukazi[i].ime) == 0) printf("\33[1m%s\33[22m - %s\n", ukazi[i].ime, ukazi[i].opis);
        }
    }
    return;
}

void pip (char* vrstica) {
    int fd[2];      // trenutno pipe
    int prev_fd = -1; // za vhod prejšnjega procesa

    for (int i = 1; i <= token_count; i++) {
        if (i < token_count) {
            if (pipe(fd) == -1) {
                perror("pipe");
                exit(EXIT_FAILURE);
            }
        }

        fflush(stdin);
        pid_t pid = fork();
        if (pid == -1) {
            perror("fork");
            exit(EXIT_FAILURE);
        }

        if (pid == 0) {
            if (i == 1 && strlen(vhod) > 0){
                int vh = open(vhod, O_RDONLY);
                dup2(vh, 0);
                close(vh);
            }
            if (i == token_count && strlen(izhod) > 0){
                int iz = open(izhod, O_WRONLY);
                dup2(iz, 1);
                close(iz);
            }

            // če ni prvi ukaz, nastavi vhod iz prejšnjega pipe-a
            if (prev_fd != -1) {
                dup2(prev_fd, 0);
                close(prev_fd);
            }

            // če ni zadnji ukaz, nastavi izhod na trenutno cev
            if (i < token_count) {
                close(fd[0]);         // zapri bralni konec pipe
                dup2(fd[1], 1);
                close(fd[1]);
            }

            // naredi še enkrat razčlenjevanje ukaza
            strcpy(vhod, "");
            strcpy(izhod, "");
            background = 0;
            char pomozna[1024];
            strcpy(pomozna, &vrstica[tokens[i]]);
            tolkenize(pomozna);
            find_builtin(pomozna);

            exit(0);
        } else {
            // starševski proces
            if (prev_fd != -1)
                close(prev_fd);

            if (i < token_count) {
                close(fd[1]);    // zapri pisalni konec
                prev_fd = fd[0]; // shrani bralni konec za naslednjega
            }
        }
    }

    for (int i = 0; i <= token_count; i++) {
        wait(NULL);
    }

    return;
}

void shraniZgo (char* vrstica) {
    if (zgInx < 256) { // če buffer še ni poln shranimo vanj in samo povečamo števec bufferja
        strcpy(zgodovina[zgInx], vrstica);
        for (int i = 1; i <= token_count; i++){
            strcat(zgodovina[zgInx], " ");
            strcat(zgodovina[zgInx], &vrstica[tokens[i]]);
        }
        zgInx++;
    } else { // sicer premaknemo vse ukaze in dodamo trenutnega na konec
        for (int i = 1; i < zgInx; i++){
            *zgodovina[i-1] = *zgodovina[i];
        }
        strcpy(zgodovina[zgInx], vrstica);
        for (int i = 1; i <= token_count; i++){
            strcat(zgodovina[zgInx], " ");
            strcat(zgodovina[zgInx], &vrstica[tokens[i]]);
        }
    }
    return;
}

void bfr (char* vrstica) {
    zgInx--;
    strcpy(vrstica, zgodovina[zgInx-1]);

    printf("%s: [Y/N]", vrstica); // vpraša če je ta ukaz tisti, ki ga želimo ponoviti
    char odgovor[3];

    fgets(odgovor, 3, stdin);
    if (strcmp(odgovor, "Y\n") == 0){
        int dolzina = tolkenize(vrstica);
        if (nivoRazhroscevanja > 0) printToken(vrstica, dolzina); // izpisi se naredijo le kadar je nivo razhroščevanja dovolj visok
        
        preusmeritve(vrstica);
        if (nivoRazhroscevanja > 0) printPreusmeritve(vrstica); // izpisi se naredijo le kadar je nivo razhroščevanja dovolj visok

        zgInx += nazaj; // preden si shranimo vrstico moramo popraviti števec
        shraniZgo(vrstica);
        find_builtin(vrstica);
        return;
    } else if (strcmp(odgovor, "N\n") == 0 && zgInx > 1){
        nazaj++;
        bfr(vrstica);
    } else zgInx += nazaj;
    return;
}

int main(){
    int interaktivno = isatty(STDIN_FILENO);

    if (interaktivno) {
        printf("\33[93;5;1m\n  **     ****  ******  *    ***      **     **   **\n");
        printf("  **     **      **    *  **       **     **  *  **\n");
        printf("  **     ****    **        **      ** **  **  *  **\n");
        printf("  **     **      **         **     **  *  **  *    \n");
        printf("  *****  ****    **      *****      ***     **   **\n\33[0m\n");
    }

    while (1){
        char vrstica[2048];
        strcpy(vhod, "");
        strcpy(izhod, "");
        background = 0;
        
        if (interaktivno) {
            printf("\33[1m%s>\33[22m ", pozivnik);
        }
        int m = 0; // število argumentov

        // branje in parse operacija
        if (fgets(vrstica, 2048, stdin) == NULL) break;

        int dolzina = tolkenize(vrstica);
        if (nivoRazhroscevanja > 0) printToken(vrstica, dolzina); // izpisi se naredijo le kadar je nivo razhroščevanja dovolj visok
        
        preusmeritve(vrstica);
        if (nivoRazhroscevanja > 0) printPreusmeritve(vrstica); // izpisi se naredijo le kadar je nivo razhroščevanja dovolj visok

        shraniZgo(vrstica);
        find_builtin(vrstica);
    }

    return returnStatus;
}