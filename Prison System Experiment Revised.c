#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

const char INMATE_DATA_FILENAME[] = "Inmates Database.txt";
const char UNSPECIFIED_CONTENT [] = "<BLANK>";

struct InmateEntry {
char IFname [10000];
char Indre[10000];
char INdob[10000];
char CriSLO [10][10000];
char SenS [10][10000];
int INage;
};


struct InmateEntry Data;
const int DATA_SZ = sizeof(Data);


struct InmateEntry* Inmates;
int entryCount = 0;

char* readChars(char*, int, FILE*);  // function prototype declaration to prevent compiler type conflict error


void calculateAge() {
    time_t t = time(NULL);
    struct tm tm = *localtime(&t);

    int currentYear = tm.tm_year + 1900;
    int birthYear;

    sscanf(Data.INdob, "%*d/%*d/%d", &birthYear);
    Data.INage = currentYear - birthYear;
}

int main() {
    loadInmateData();
    mainscreen();
    return 0;
}


void loadInmateData() {
    FILE *fptr;

    fptr = fopen (INMATE_DATA_FILENAME, "r");

    if (fptr != NULL)
    {
        while(readChars (Data.IFname, sizeof(Data.IFname), fptr)) {
            freadInt (&Data.INage,  fptr);
            readChars (Data.Indre, sizeof(Data.Indre), fptr);
            readChars (Data.INdob,  sizeof(Data.INdob), fptr);
            readChars (Data.CriSLO, sizeof(Data.CriSLO), fptr);
            readChars (Data.SenS,   sizeof(Data.SenS), fptr);

            registerInmateData();

            fscanf(fptr, "\n");  // consume additional new line token
        }

        fclose(fptr);
    }
    else
    {
        clearScr();
        printf("\nWarning: %s cannot be opened because it does not exist.\n", INMATE_DATA_FILENAME);
        pause (NULL, 1);
    }
}

void displayMainScreen() {
    clearScr();  //refreshes the screen
    printf (" - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -\n\n");
    printf ("                                      WELCOME TO THE PAUL'S WARDEN HOUSE                                  \n\n");
    printf ("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -\n\n");
    printf ("                                                  MAIN MENU                                             \n\n");
    printf ("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -\n\n");
    printf ("Please enter the number that you see at the side in order to select the specific task that you want to complete.\n");
    printf ("[1] Screen 1 to enter administrator information\n");
    printf ("[2] Screen 2 to enter inmate information\n");
    printf ("[3] Screen 3 to access inmate information\n");
    printf ("[4] Exit the program\n");
}

void mainscreen() {
    int choice = 0;

    do {

        displayMainScreen();

        readOption(&choice);

        switch (choice)
        {
            case 1:
            screen1();
            break;

            case 2:
            screen2();
            break;

            case 3:
            screen3();
            break;

            case 4:
            printf ("Exiting the program....\n");
            exit(0);

            default:
            pause ("Invalid input. Please enter a valid option.", 1);
        }

    } while (1);
}

void displayScreen1() {
    clearScr();  //refreshes the screen
    printf ("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -\n\n");
    printf ("                                              SCREEN 1                                         \n\n");
    printf ("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -\n\n");
    printf ("                                       PAUL'S WARDEN HOUSE                                     \n\n");
    printf ("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - \n\n");
    printf ("[1] Enter Administrator name\n");
    printf ("[2] Enter Inmate Data\n");
    printf ("[3] Back to Main Menu\n");
}

void displayAdminHdr() {
    clearScr();  //refreshes the screen
    printf ("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -\n\n");
    printf ("                                           ADMIN SCREEN                                         \n\n");
    printf ("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -\n\n");
    printf ("                                       PAUL'S WARDEN HOUSE                                     \n\n");
    printf ("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - \n\n");
}

void displayAdminDtl(char * Fullname) {
    printf("Thank you for using this program. We appreciate your service, officer %s\n\n", Fullname);
    pause ("Thank you!\n", 1);
}

void screen1() {
    int choice = 0;
    char Fullname [100];

    do {

        displayScreen1();

        readOption(&choice);

        switch (choice)
        {
            case 1:
            displayAdminHdr();

            promptLn("Enter administrator's name: ", Fullname, sizeof(Fullname));

            displayAdminDtl(Fullname);

            adminscreen();
            break;

            case 2:
            screen2();
            break;

            case 3:
            mainscreen();
            break;

            default:
            pause ("INVALID REQUEST. PLEASE TRY AGAIN", 1);
        }

    } while (1);
}

void displayAdminMenu() {
    clearScr();  //refreshes the screen
    printf ("Select any of the following options to continue\n");
    printf ("[1] Return to main menu\n");
    printf ("[2] Enter inmate data\n");
}

void adminscreen() {
    int choice = 0;

    do {

        displayAdminMenu();

        readOption(&choice);

        switch (choice)
        {
            case 1:
            mainscreen();
            break;

            case 2:
            screen2();
            break;

            default:
            pause ("Invalid input. Please enter a valid option.", 1);
        }

    } while (1);
}

void displayScreen2() {
    clearScr();  //refreshes the screen
    printf ("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -\n\n");
    printf ("                                              SCREEN 2                                         \n\n");
    printf ("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -\n\n");
    printf ("                                       PAUL'S WARDEN HOUSE                                     \n\n");
    printf ("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - \n\n");
}

void displayInmateSenSHint() {
    printf ("\nThe option selected is a crime punishable by law. The number of years are displayed below.");
}

void displayInmateSummary(float ProbP, float TotSE) {
    printf ("\nAn inmate is eligible for probation after having served half of their total sentence.");
    printf ("\nThis will be done through an assessment that will test to see if the inmates' behaviour have improved since they have been inside jail.");
    printf ("\nThis assessment is given on a date which will be decided by the judge. Thus, this inmate is eligible for probation in: %.1f years time.", ProbP);
    printf("\n\n");
    printf("The total sentence is %.1f years\n", TotSE);
    printf("The number of persons in the program is %d\n", entryCount);
}

void InmateCrimes() {

int choice;
int randomYears;

    strcpy(Data.CriSLO [0],"Arson");
    strcpy(Data.CriSLO [1],"Illegal Firearm, Deadly Weapon and Possession of Drugs");
    strcpy(Data.CriSLO [2],"Kidnapping");
    strcpy(Data.CriSLO [3],"Murder");
    strcpy(Data.CriSLO [4],"Rape and abusive sexual contact");
    strcpy(Data.CriSLO [5],"Robbery and theft");
    strcpy(Data.CriSLO [6],"Stalking, Assault and Vandalism");
    strcpy(Data.CriSLO [7],"Terrorism");
    strcpy(Data.CriSLO [8],"White Collar Crime, Embezzlement and Tax Evasion");

    
    srand(time(NULL));


    printf("\nChoose a crime from the following list:\n");

    for (int i = 0; i < 9; i++) {
        printf("[%d] %s\n", i + 1, Data.CriSLO[i]);
    }

    // Allow the user to choose only 1 crime
    for (int i = 0; i < 1; i++) {
        printf("\nEnter the number corresponding to the crime: ");
        scanf("%d", &choice);

        if (choice >= 1 && choice <= 9) {
             strcpy(Data.CriSLO[0], Data.CriSLO[choice - 1]);
            int randomYears = rand() % 30 + 1;
            sprintf(Data.SenS[0], "%d", randomYears);

        
        }
    }







}
void screen2() {
    char *token;
    float TotSE = 0.0;
    float ProbP = 0.0;
    Data.INage = 0;  // need to pre-initialize this variable


    displayScreen2();

    promptLn("Enter Inmate full Name. Please follow this format and enter the name (Last name, Middle Name, First Name)\n",
              Data.IFname, sizeof(Data.IFname));

   promptLn("\nEnter Inmate Date of Birth. The format for this is DD/MM/YYYY:\n", Data.INdob, sizeof(Data.INdob));

   calculateAge();

   printf("\nYour current age is: %d\n", Data.INage);

    promptLn("\nEnter Inmate Full Address. The format for the inmates address is (lot number, street, post office, parish)\n",
              Data.Indre, sizeof(Data.Indre));

    InmateCrimes();
    
    displayInmateSenSHint();
    

    sanitizeInmateDtl (Data.IFname);
    sanitizeInmateDtl (Data.Indre);
    sanitizeInmateDtl (Data.INdob);
    sanitizeInmateDtl (Data.CriSLO);
    sanitizeInmateDtl (Data.SenS);

    registerInmateData();

    token = strtok(Data.SenS, ",");

    while (token != NULL) {
        TotSE += (float) atof(token);
        token = strtok (NULL, ",");
    }

    ProbP = (float) TotSE / 2;

    displayInmateSummary(ProbP, TotSE);

    saveInmateData();

    while (getchar() != '\n');
    
    pause(NULL, 1);

    adminscreen();
}

void sanitizeInmateDtl(char* Detail) {
    if (strlen(Detail) == 0) {
        strcpy(Detail, UNSPECIFIED_CONTENT);
    }
}

void registerInmateData() {
    Inmates = realloc(Inmates, ++entryCount * DATA_SZ);
    Inmates[entryCount-1] = Data;
}

void saveInmateData() {
    FILE *fptr;
    fptr = fopen(INMATE_DATA_FILENAME, "a"); // Open file in append mode

    if (fptr != NULL) {
        Data = Inmates[entryCount-1];

        fprintf(fptr, "%s\n%d\n%s\n%s\n%s\n%s\n%s\n\n", Data.IFname, Data.INage, Data.Indre, Data.INdob, Data.CriSLO, Data.SenS);
        fclose(fptr);
    } else {
        printf("Error: Unable to open file for appending.\n");
    }
}


void displayScreen3() {
    clearScr();  //refreshes the screen
    printf ("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -\n\n");
    printf ("                                              SCREEN 3                                         \n\n");
    printf ("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -\n\n");
    printf ("                                       PAUL'S WARDEN HOUSE                                     \n\n");
    printf ("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - \n\n");
}

void screen3(){

    displayScreen3();

    displayInmateData();

    pause ("[ENTER] Return to main menu\n", 0);

    mainscreen();
}

void displayInmateData() {

    if (entryCount == 0) {
        printf ("\n\nNo Data.\n\n\n\n");
        return;
    }

    for (int i = 0; i < entryCount; i++) {
        Data = Inmates[i];

        printf ("Name: %s\n", Data.IFname);
        printf ("Age: %d\n", Data.INage);
        printf ("Address: %s\n", Data.Indre);
        printf ("Date of Birth: %s\n", Data.INdob);
        printf("Crime: %s\n", Data.CriSLO);
        printf("Sentence: %s year(s)\n\n", Data.SenS);
        }
    }

            

/*******************************************************************/
/****************** Utility/Convenience Functions ******************/
/*******************************************************************/

void clearScr() {
#if defined(_WIN32) || defined(__CYGWIN__)
    system ("cls");   // Windows OS
#else
    system ("clear"); // other OS
#endif
}

void flush() {
    while (getchar() != '\n');  // clear console input stream (remove unconsumed input characters)
}

char *readChars(char* input, int input_sz, FILE* stream) {
    char *result; int spn_len;

    if (input_sz > 0) {

        result = fgets(input, input_sz + 1, stream);  // fgets actually reads input_sz - 1 characters from the input stream

        if (result != NULL) {
            spn_len = strcspn(input, "\n");

            if (spn_len < input_sz) {
                input[spn_len] = '\0';
            }
            else if (stream == stdin) {
                flush();
            }
        }
    }

    return result;
}

int *readInt(int* input, int input_sz, FILE* stream) {
    char *result; char str_input [input_sz];

    if (input_sz > 0) {

        result = readChars(str_input, input_sz, stream);

        if (result != NULL) {
           *input = atoi(str_input);
            return input;
        }
    }

    return NULL;
}

void freadInt(int* input, FILE* fptr) {
    fscanf(fptr, "%d\n", input);
}

void readOption(int* input) {
    readInt(input, 2, stdin);
}

void promptLn(char * message, char* input, int input_sz) {  // reads an entire line of characters on the console input stream
    printf(message);
    readChars(input, input_sz, stdin);
}

void promptInt(char * message, int* input, int input_sz) {  // reads an integer value on the console input stream
    printf (message);
    readInt(input, input_sz, stdin);
}

void pause(char * message, const int alt_msg_flg) {
    printf (message);
    if (alt_msg_flg > 0) {
        printf ("\nPress ENTER key to continue");
    }
    flush();
}
