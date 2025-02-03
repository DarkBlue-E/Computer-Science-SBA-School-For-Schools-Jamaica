#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>      // For isprint() & isdigit() functions
#if defined(_WIN32) || defined(__CYGWIN__)
#include <windows.h>    // For Windows Sleep() function
#else
#include <unistd.h>     // For Linux sleep() function
#endif

// Screen display properties (measured in characters)
#define SCREEN_SZ 104  
#define SCREEN_PADDING 5
#define MAX_SZ_ITEM_NO 4  
#define MAX_SZ_FULLNAME 50  
#define MAX_SZ_SUBJECT 30
#define MAX_SZ_GRADE 4

// Data entry file resources
#define PINCIPAL_DATA_FILENAME "Principal.txt"
#define TEACHER_DATA_FILENAME "Teachers.txt"
#define STUDENT_DATA_FILENAME "Students.txt"
#define ENROLL_DATA_FILENAME "Enrollments.txt"
// Void data placeholders
#define UNSPECIFIED_DATA "<NONE>"   // frontend view label shown in the place of values not yet assigned or set
#define BLANK_DATA "<BLANK>"        // backend data label used in place of string values that are empty 

// Synchronized read settings
#define FILE_READ_FRQ 20        // no. of retries upon read failure
#define FILE_READ_LAT 50        // wait interval between each retry (in milliseconds)

// Warn message type enumeration
#define FILE_UNREADABLE 0
#define FILE_CORRUPT 1

// User type enumeration
#define USR_STUDENT 1
#define USR_TEACHER 2
#define USR_PRINCIPAL 3

// User default timeout enumeration (in minutes)
#define T_DEF_STUDENT   10
#define T_DEF_TEACHER   15
#define T_DEF_PRINCIPAL 30

// User registration status enumeration
#define REG_STAT_PROF -1
#define REG_STAT_SUBJ  0
#define REG_STAT_FULL  1


struct UserEntry {
int  loginID;    
char Fname [14];
char Lname [35];
char Addr  [1000];
char Dob   [10];
int  timeout;   //measured in minutes
int  reg_stat;
};

struct EnrollEntry {
int studentID;
int subjectID;
int teacherID;   
float grade;
};

typedef struct UserEntry User;
typedef struct EnrollEntry Enrollment;

// Elementary size definition caching
const int USER_SZ = sizeof(User);
const int ENROLL_SZ = sizeof(Enrollment);

const char* SUBJECTS[] = {"Mathematics","English","Spanish","Computing","Biology,","Physics"};
const int SUBJECTS_SZ = sizeof(SUBJECTS) / sizeof(char*);

User TEACHERS [6];
const int TEACHERS_SZ = sizeof(TEACHERS) / USER_SZ;

User Principal, *Students;
int principalCount = 1;
int studentCount = 0;

Enrollment *Enrollments;
int enrollCount = 0;

int CURRENT_USR_TYPE;
User* CURRENT_USR;

// Function prototype declarations for functions whose invocations occur BEFORE their declaration
// Note that this is only necessary for functions that return pointer types 
char* readChars(char*, int, FILE*);
int* freadInt(int*, FILE*);
float* freadFloat(float*, FILE*);


/********************************************************************/
/***************** Convenience Auxilliary Functions *****************/
/********************************************************************/

User *loadUserData(User *list, int *list_sz_ptr, FILE *fptr, const int stud_data_flg) 
{
    char buffer[1];   // needed to consume unused '\n' character
    int data_sz = *list_sz_ptr;

    if (stud_data_flg > 0) 
    {
        if(!freadInt(&data_sz, fptr)) 
            return NULL;

        if(*list_sz_ptr != data_sz ||!list) {
           *list_sz_ptr  = data_sz;
            list = realloc(list, data_sz * USER_SZ);
        }
    }

    for (int i=0; i < data_sz; i++) {
        if (readChars (list[i].Fname, NULL, fptr)) {
            readChars (list[i].Lname, NULL, fptr);
            readChars (list[i].Addr , NULL, fptr);
            readChars (list[i].Dob  , NULL,   fptr);
            freadInt  (&list[i].timeout, fptr);

            if (stud_data_flg > 0)
                freadInt (&list[i].loginID, fptr);

            readChars (buffer, 1, fptr);   // consume additional new line token
        } 
        else return NULL;   // assert parity between expected and actually loaded data
    }

    return list;
}

Enrollment *loadEnrollData(Enrollment *list, int *list_sz_ptr, FILE *fptr) 
{
    char buffer[1];    // needed to consume unused '\n' character
    int data_sz = *list_sz_ptr;

    if(!freadInt(&data_sz, fptr)) 
        return NULL;

    if(*list_sz_ptr != data_sz ||!list) {
       *list_sz_ptr  = data_sz;
        list = realloc(list, data_sz * ENROLL_SZ);   
    }

    for (int i=0; i < data_sz; i++) {
        if (freadInt  (&list[i].studentID, fptr)) {
            freadInt  (&list[i].teacherID, fptr);
            freadInt  (&list[i].subjectID, fptr);
            freadFloat (&list[i].grade, fptr);

            readChars (buffer, 1, fptr);   // consume additional new line token
        }
        else return NULL;   // assert parity between expected and actually loaded data
    }

    return list;
}

FILE *saveUserData(User *list, int list_sz, FILE *fptr, const int stud_data_flg) 
{
    User* u;

    if (fprintf (fptr, "%d\n", list_sz) <= 0)
        return NULL;

    for (int i = 0; i < list_sz; i++) 
    {
        u = list + i;
        if (fprintf (fptr, "%s\n%s\n%s\n%s\n%d\n", u->Fname, u->Lname, u->Addr, u->Dob, u->timeout) <= 0)
            return NULL;

        if (stud_data_flg > 0)
            if (fprintf (fptr, "%d\n", u->loginID) <= 0)
                return NULL;

        if (fprintf (fptr, "\n") <= 0)
            return NULL;
    }

    return fptr;
}

FILE *saveEnrollData(Enrollment *list, int list_sz, FILE *fptr) 
{
    Enrollment* e;

    if (fprintf (fptr, "%d\n", list_sz) <= 0)
        return NULL;

    for (int i = 0; i < list_sz; i++) 
    {
        e = list + i;
        if (fprintf (fptr, "%d\n%d\n%d\n%.2f\n\n", e->studentID, e->teacherID, e->subjectID, e->grade) <= 0)
            return NULL;
    }

    return fptr;
}

FILE *refreshData(void *list, int *list_sz_ptr, const char *DATA_FNAME) {
    FILE* fptr = fopen (DATA_FNAME, "r");
    void* ptr;

    if (fptr) 
    {
        if (DATA_FNAME == ENROLL_DATA_FILENAME) {
            ptr = loadEnrollData (list, list_sz_ptr, fptr);
        } else {
            ptr = loadUserData (list, list_sz_ptr, fptr, abs(DATA_FNAME == STUDENT_DATA_FILENAME));
        }
        fclose(fptr);

        if (ptr) {
            fptr = fopen (DATA_FNAME, "w");
            return fptr;
        }
    }

    return NULL;
}

int persistData(void *list, int list_sz, FILE *fptr) {
    void *ptr;
    
    if (list == Enrollments) {
        ptr = saveEnrollData (list, list_sz, fptr);
    } else {
        ptr = saveUserData (list, list_sz, fptr, abs(list == Students));
    }
    fclose(fptr);

    return ptr;
}


char* getFullName(User* usr) {
    if (usr) {
        static char full_name [MAX_SZ_FULLNAME];

        strcpy(full_name, usr->Fname);
        strcat(full_name, " ");
        strcat(full_name, usr->Lname);  

        return full_name;         
    }
    return NULL;
}

char* getUserTitle(const char* pre_ttl, int usr_type, const char* pst_ttl) {

    static char title [SCREEN_SZ];

    strcpy(title, "");  // clears any previously set title
    
    if (pre_ttl)
        strcat(title, pre_ttl);

    if (usr_type <= 0) usr_type = CURRENT_USR_TYPE;

    switch (usr_type) {
        case USR_STUDENT:
            strcat(title, "STUDENT");
            break;
        case USR_TEACHER:
            strcat(title, "TEACHER");
            break;
        case USR_PRINCIPAL:
            strcat(title, "PRINCIPAL"); 
            break;  
        default:
           strcat(title, "SUBJECT");         
    }

    if (pst_ttl)
        strcat(title, pst_ttl);

    return title;
}

int getEnrollEntryID(int usr_type, Enrollment* enroll) {
    switch (usr_type) {
        case USR_STUDENT:
            return enroll->studentID;
        case USR_TEACHER:
            return enroll->teacherID;
        default:
           return enroll->subjectID;     
    }
}

int getUserListSz(int usr_type) {
    
    if (usr_type <= 0) usr_type = CURRENT_USR_TYPE;

    switch (usr_type) {
        case USR_STUDENT:
            return studentCount;
        case USR_TEACHER:
            return TEACHERS_SZ;
        case USR_PRINCIPAL:
            return principalCount;
        default:
            return -1;            
    }
}

User* getUserList(int usr_type) {

    if (usr_type <= 0) usr_type = CURRENT_USR_TYPE;

    switch (usr_type) {
        case USR_STUDENT:
            return Students;
        case USR_TEACHER:
            return TEACHERS;
        case USR_PRINCIPAL:
            return &Principal;
        default:
            return NULL;            
    }
}

User* getUser(int userID, User* list, int list_sz) {
    for (int i=0; i < list_sz; i++) {
        if (list[i].loginID = userID)
            return (User*)(list + i);
    }
    return NULL;
}

int calculateTally(int entryID, int entry_usr_type, int tally_usr_type) {
    int res_ids [enrollCount];   // holds the target IDs found for the given entry ID
    int res_cnt = 0;
    int res_id;

    for (int i=0; i < enrollCount; i++)
    {
        if (entryID == getEnrollEntryID(entry_usr_type, Enrollments+i)) 
        {
            res_id = getEnrollEntryID(tally_usr_type, Enrollments+i);

            for (int j=0; j < res_cnt; j++) {
                if (res_id == res_ids[j]) 
                    goto e;     // prevents the addition of a duplicate ID
            }

            res_ids [res_cnt++] = res_id;
        }
e:  }

    return res_cnt;
}

float calculateGradeAvg(int entryID) {
    float sum = 0;
    int count = 0;
    Enrollment* enroll;

    for (int i=0; i < enrollCount; i++) 
    {
        enroll = Enrollments + i;
        
        if ((entryID <= 0  || entryID == enroll->studentID || entryID == enroll->teacherID || entryID == enroll->subjectID) && enroll->grade >= 0) {
            sum += enroll->grade;
            count++;
        }
    }

    if (count > 0)
        return sum / count;
    else
        return -1;
}

void warn(int msg_type, const char* msg_arg) {
    char msg[255] = "Warning: ";

    switch(msg_type) {
        case FILE_UNREADABLE:
            strcat(msg, "%s does not exist or cannot be opened. (Reverting to default state...");
            break;
        case FILE_CORRUPT:
            strcat(msg, "%s is empty, corrupted or cannot be read.\n");
            break;
    }

    printf(msg, msg_arg);
}

void printScrPat(const char* pre_pat, const char* pattern, int repeat, const char* pst_pat) { 
    if (pre_pat)
        printf (pre_pat);
    if (pattern)
    for (int i=0; i <= repeat; i++)
        printf (pattern);
    if (pst_pat)
        printf (pst_pat);
}

void printScrTitle(const char* pre_pat, const char* title, const char* pst_pat) {

    const int TTL_SZ = title ? strlen(title) : 0;
    const int DEF_PADDING = (SCREEN_SZ - TTL_SZ) / 2;

    int padding = DEF_PADDING - getPrintableCharCnt(pre_pat);

    if (pre_pat) 
        printf (pre_pat);
    if (title) 
        printf ("%*s", (padding < 0 ? 0 : padding) + TTL_SZ, title);
    if (pst_pat) 
        printf ("%*s", (padding < 0 ? padding : 0) + DEF_PADDING, pst_pat);    
}

void printScrMargin() {
    printf ("%*s", SCREEN_PADDING, "");
}

void printScrColText(const char* col_txt, int col_mx_sz, const char* pst_txt) {
    if (col_txt) 
        printf ("%-*s", col_mx_sz, col_txt); 
    if (pst_txt)
        printf (pst_txt);
}

void printScrColVal(float col_val, int col_mx_sz, int col_val_prec, const char* pst_txt) {
    
    printf ("%-*.*f", col_mx_sz, col_val_prec < 0 ? 0 : col_val_prec, col_val); 
    
    if (pst_txt)
        printf (pst_txt);
}


/*******************************************************************/
/******************** Screen Navigation Functions ******************/
/*******************************************************************/

int main() {
    initSysUsers();
    loadSchoolData();
    mainscreen();
    return 0;
}

void initSysUsers()     // Initialize non-student users 
{
    Principal.loginID = 1000;
    strcpy(Principal.Fname, "Paul");
    strcpy(Principal.Lname, "Duncanson");
    strcpy(Principal.Addr , "Portsmouth, Lesser Portmore");
    strcpy(Principal.Dob  , "23/05/2005");
    Principal.timeout = T_DEF_PRINCIPAL;
    Principal.reg_stat = REG_STAT_FULL;

    TEACHERS[0].loginID = 1100;
    strcpy(TEACHERS[0].Fname, "Grace");
    strcpy(TEACHERS[0].Lname, "Peters");
    strcpy(TEACHERS[0].Addr , "A1");
    strcpy(TEACHERS[0].Dob  , "23/05/1995");
    TEACHERS[0].timeout = T_DEF_TEACHER;
    TEACHERS[0].reg_stat = REG_STAT_FULL;

    TEACHERS[1].loginID = 1200;
    strcpy(TEACHERS[1].Fname, "Kim");
    strcpy(TEACHERS[1].Lname, "Long");
    strcpy(TEACHERS[1].Addr , "A2");
    strcpy(TEACHERS[1].Dob  , "23/07/1995");
    TEACHERS[1].timeout = T_DEF_TEACHER;
    TEACHERS[1].reg_stat = REG_STAT_FULL;

    TEACHERS[2].loginID = 1300;
    strcpy(TEACHERS[2].Fname, "Mercy");
    strcpy(TEACHERS[2].Lname, "James");
    strcpy(TEACHERS[2].Addr , "A3");
    strcpy(TEACHERS[2].Dob  , "17/04/1986");
    TEACHERS[2].timeout = T_DEF_TEACHER;
    TEACHERS[2].reg_stat = REG_STAT_FULL;

    TEACHERS[3].loginID = 1400;
    strcpy(TEACHERS[3].Fname, "John");
    strcpy(TEACHERS[3].Lname, "Jackson");
    strcpy(TEACHERS[3].Addr , "A4");
    strcpy(TEACHERS[3].Dob  , "17/04/1987");
    TEACHERS[3].timeout = T_DEF_TEACHER;
    TEACHERS[3].reg_stat = REG_STAT_FULL;

    TEACHERS[4].loginID = 1500;
    strcpy(TEACHERS[4].Fname, "Paul");
    strcpy(TEACHERS[4].Lname, "Wright");
    strcpy(TEACHERS[4].Addr , "A5");
    strcpy(TEACHERS[4].Dob  , "17/01/2001");
    TEACHERS[4].timeout = T_DEF_TEACHER;
    TEACHERS[4].reg_stat = REG_STAT_FULL;

    TEACHERS[5].loginID = 1600;
    strcpy(TEACHERS[5].Fname, "Kerry");
    strcpy(TEACHERS[5].Lname, "Truman");
    strcpy(TEACHERS[5].Addr , "A6");
    strcpy(TEACHERS[5].Dob  , "05/12/1991");
    TEACHERS[5].timeout = T_DEF_TEACHER;
    TEACHERS[5].reg_stat = REG_STAT_FULL;
}

void *loadEntryData(void *list, int *list_sz_ptr, const char *data_fname) 
{    
    void *ptr  = NULL;
    FILE *fptr = fopen (data_fname, "r");

    if (fptr) 
    {
        if (data_fname == ENROLL_DATA_FILENAME) {
            ptr = loadEnrollData(list, list_sz_ptr, fptr);
        } else {
            ptr = loadUserData(list, list_sz_ptr, fptr, abs(data_fname == STUDENT_DATA_FILENAME));
        }  
        fclose(fptr);

        if (!ptr) {
            warn(FILE_CORRUPT, data_fname);
        }
    } 
    else 
    {
        warn(FILE_UNREADABLE, data_fname);  

        if ((fptr = fopen(data_fname, "w")) && persistData(list, *list_sz_ptr, fptr)) {
            printf("Success)\n");
        } else {
            printf("Fail)\n");
        }
    }

    return ptr;
}

void loadSchoolData() {
    void *ptr;

    clearScr();

    ptr = loadEntryData(&Principal, &principalCount, PINCIPAL_DATA_FILENAME) && ptr;
    ptr = loadEntryData(TEACHERS, &TEACHERS_SZ, TEACHER_DATA_FILENAME) && ptr;
    ptr = loadEntryData(Students, &studentCount, STUDENT_DATA_FILENAME) && ptr;
    ptr = loadEntryData(Enrollments, &enrollCount, ENROLL_DATA_FILENAME) && ptr;

    if (!ptr) {
        pauseScr (NULL, 1);
    }
}

void displayScreenHdr(const char* main_title, const char* sub_title) {

    const int repeat = SCREEN_SZ / 2 - 1;

    clearScr();  //refreshes the screen
    printScrPat   (NULL, "- ", repeat, "\n\n");
    printScrTitle (NULL, main_title, "\n\n");
    printScrPat   (NULL, "- ", repeat, "\n");
    printScrTitle (NULL, sub_title, "\n");
    printScrPat   (NULL, "- ", repeat, "\n\n");
}

void displayErrorSubScreen() {
    printf ("\n\nSelect an option below to proceed (? means any value except 0)\n");
    printf ("[?] Re-try current action\n");
    printf ("[0] Return to previous menu\n");
}

void displayMainScreen() {

    displayScreenHdr("WELCOME TO 'FOR SCHOOLS OF JAMAICA'", "MAIN MENU");

    printf ("Please select your account type\n");
    printf ("[1] Student\n");
    printf ("[2] Teacher\n");
    printf ("[3] Principal\n");
    printf ("[0] Exit the program\n");
}

void mainscreen() {
    int choice = -1;

    do {
        displayMainScreen();

        readOption(&choice);

        switch (choice)
        {
            case 1:
            CURRENT_USR_TYPE = USR_STUDENT;
            studentMenuScreen();
            break;

            case 2:
            CURRENT_USR_TYPE = USR_TEACHER;
            loginScreen();
            break;

            case 3:
            CURRENT_USR_TYPE = USR_PRINCIPAL;
            loginScreen();
            break;

            case 0:
            printf ("Exiting the program....\n");
            exit(0);

            default:
            pauseScr ("Invalid input. Please enter a valid option.", 1);
        }

    } while (1);
}

void displayStudMenuScreen() {
    
    displayScreenHdr("FOR SCHOOLS OF JAMAICA", "STUDENT MENU");
    
    printf ("Select an option below\n");
    printf ("[1] Sign In\n");
    printf ("[2] Sign Up\n");
    printf ("[0] Back to Main Menu\n");
}

void studentMenuScreen() {
    int choice = -1;

    do {
        displayStudMenuScreen();

        readOption(&choice);

        switch (choice)
        {
            case 1:
            loginScreen();
            return;

            case 2:
            studentRegistrationScreen();
            break;

            case 0:
            mainscreen();
            return;

            default:
            pauseScr ("INVALID REQUEST. PLEASE TRY AGAIN", 1);
        }

    } while (1);
}

void displayStudRegScreen() {

    displayScreenHdr("FOR SCHOOLS OF JAMAICA", "STUDENT REGISTRATION");
    
    printf ("Please provide your profile information below (press ENTER to skip optional details)\n");
}

void studentRegistrationScreen(const int edit_mode_flg) {


    int choice = -1;

    do {
        displayScreenHdr("FOR SCHOOLS OF JAMAICA", "STUDENT REGISTRATION");

        readOption(&choice);

        switch (choice)
        {
            case 1:
            mainscreen();
            break;

            case 2:
            loginScreen();
            break;

            default:
            pauseScr ("Invalid input. Please enter a valid option.", 1);
        }

    } while (1);
}

void displaySubjRegScreen() {

    displayScreenHdr("FOR SCHOOLS OF JAMAICA", "SUBJECT REGISTRATION");
    
    printf ("Select any of the following options to continue\n");
    printf ("[1] Return to main menu\n");
    printf ("[2] Enter inmate data\n");
}

void subjectRegistrationScreen(const int edit_mode_flg) {
    int choice = -1;

    do {
        displaySubjRegScreen();

        readOption(&choice);

        switch (choice)
        {
            case 1:
            mainscreen();
            break;

            case 2:
            loginScreen();
            break;

            default:
            pauseScr ("Invalid input. Please enter a valid option.", 1);
        }

    } while (1);
}

void displayLoginScreen() {
    displayScreenHdr("FOR SCHOOLS OF JAMAICA", getUserTitle(NULL, NULL, " LOGIN"));
}

void loginScreen() {
    int loginID = 0;
    int choice = -1;

    do {
        displayLoginScreen();

        promptLgn("Enter login ID: ", &loginID);

        if (CURRENT_USR = getUser(loginID, getUserList(NULL), getUserListSz(NULL))) {
            break;
        }

        printf ("\nThe entered login ID is either not found or incorrect.");
        if(CURRENT_USR_TYPE == USR_STUDENT)
        printf ("\n(If you do not have an account you can create one via the SIGN UP option on the previous screen.)");
       
        displayErrorSubScreen();

        readOption(&choice);

        if (choice == 0) {
            if (CURRENT_USR_TYPE == USR_STUDENT)
                studentMenuScreen();
            else
                mainscreen();
            return;
        }

    } while(1);

    userHomeScreen();
}

void displayUserHomeScreen() {

    displayScreenHdr("FOR SCHOOLS OF JAMAICA", getUserTitle(NULL, NULL, " HOME"));
    
    printf ("Select an option below\n");
    printf ("[1] View Profile\n");
    printf ("[2] Edit Profile\n");

    switch (CURRENT_USR_TYPE) {
    case USR_STUDENT:
        printf ("[3] View Subjects\n");
        printf ("[4] Add Subjects\n");
        printf ("[5] Drop Subjects\n");
        break;
    case USR_TEACHER:
        printf ("[3] View Students\n");
        printf ("[4] Edit Grades\n");
        break;
    case USR_PRINCIPAL:
        printf ("[3] View Subjects\n");    
        printf ("[4] View Teachers\n"); 
        printf ("[5] View Students\n");    
        printf ("[6] Reassign Teachers\n");    
        printf ("[7] Deregister Student\n");     
    }
    printf ("[0] Sign Out\n");
}

void userHomeScreen(){
    int choice = -1;

    do {
        displayUserHomeScreen();

        readOption(&choice);

        switch (choice)
        {
            case 0:
            mainscreen();
            return;

            case 1:
            userProfileScreen(&CURRENT_USR, NULL, 0);
            return;

            case 2:
            userProfileScreen(&CURRENT_USR, NULL, 1);
            return;
        }

        if (CURRENT_USR_TYPE == USR_STUDENT) {
            switch (choice) {
                case 3:
                subjectListScreen();
                return;

                case 4:
                
                return;

                case 5:
                
                return;
            }
        } else if (CURRENT_USR_TYPE == USR_TEACHER) {
            switch (choice) {
                case 3:
                studentListScreen(0);
                return;

                case 4:
                studentListScreen(1);
                return;
            }
        } else if (CURRENT_USR_TYPE == USR_PRINCIPAL) {
            switch (choice) {
                case 3:
                enrollmentListScreen(0);
                return;

                case 4:
                enrollmentListScreen(1);
                return;

                case 5:
                enrollmentListScreen(0);
                return;

                case 6:
                enrollmentListScreen(1);
                return;

                case 7:
                enrollmentListScreen(1);
                return;
            }
        }

        pauseScr ("Invalid input. Please enter a valid option.", 1);

    } while (1);
}

void userProfileScreen(User* usr, int usr_type, const int edit_mode_flg) {

    const int MAX_COL_SZ = 24 + SCREEN_PADDING;  //max size for the field names column
    float avg;

    if (usr_type <= 0) usr_type = CURRENT_USR_TYPE;

    displayScreenHdr("FOR SCHOOLS OF JAMAICA", getUserTitle(NULL, usr_type, " PROFILE"));

    printScrTitle(NULL, "----- PERSONAL INFORMATION -----", "\n\n");

    printScrMargin();
    printScrColText("First Name:", MAX_COL_SZ, NULL);
    printScrColText(usr->Fname, 0, "\n");
    printScrMargin();
    printScrColText("Last Name:", MAX_COL_SZ, NULL);
    printScrColText(usr->Lname, 0, "\n");
    printScrMargin();
    printScrColText("Address:", MAX_COL_SZ, NULL);
    printScrColText(usr->Addr, 0, "\n");
    printScrMargin();
    printScrColText("Date of Birth:", MAX_COL_SZ, NULL);
    printScrColText(usr->Dob, 0, "\n\n");

    printScrTitle(NULL, "----- COURSE INFORMATION -----", "\n\n");

    avg = calculateGradeAvg(usr_type==USR_PRINCIPAL? NULL: usr->loginID);

    if (usr_type == USR_STUDENT) 
    {
        printScrMargin();
        printScrColText("Total Subjects:", MAX_COL_SZ, NULL);
        printScrColVal(calculateTally(usr->loginID, usr_type, NULL), 0, 0, "\n");
        printScrMargin();
        printScrColText("Total Teachers:", MAX_COL_SZ, NULL);
        printScrColVal(calculateTally(usr->loginID, usr_type, USR_TEACHER), 0, 0, "\n");  
        printScrMargin();
        printScrColText("Average Grade:", MAX_COL_SZ, NULL);
        if(avg < 0)
        printScrColText(UNSPECIFIED_DATA, 0, "\n\n");   
        else 
        printScrColVal(avg, 0, 0, "%%\n\n"); 
    }
    else if (usr_type == USR_TEACHER) 
    {
        printScrMargin();
        printScrColText("Total Students:", MAX_COL_SZ, NULL);
        printScrColVal(calculateTally(usr->loginID, usr_type, USR_STUDENT), 0, 0, "\n"); 
        printScrMargin();
        printScrColText("Total Subjects:", MAX_COL_SZ, NULL);
        printScrColVal(calculateTally(usr->loginID, usr_type, NULL), 0, 0, "\n");
        printScrMargin();
        printScrColText("Average Student's Grade:", MAX_COL_SZ, NULL);
        if(avg < 0)
        printScrColText(UNSPECIFIED_DATA, 0, "\n\n");   
        else 
        printScrColVal(avg, 0, 0, "%%\n\n"); 
    }
    else if (usr_type == USR_PRINCIPAL) 
    {
        printScrMargin();
        printScrColText("Total Subjects:", MAX_COL_SZ, NULL);
        printScrColVal(SUBJECTS_SZ, 0, 0, "\n");
        printScrMargin();
        printScrColText("Total Teachers:", MAX_COL_SZ, NULL);
        printScrColVal(TEACHERS_SZ, 0, 0, "\n");
        printScrMargin();
        printScrColText("Total Students:", MAX_COL_SZ, NULL);
        printScrColVal(studentCount, 0, 0, "\n");
        printScrMargin();
        printScrColText("Average Student's Grade:", MAX_COL_SZ, NULL);
        if(avg < 0)
        printScrColText(UNSPECIFIED_DATA, 0, "\n\n");   
        else 
        printScrColVal(avg, 0, 0, "%%\n\n"); 
    }

    printScrTitle(NULL, "----- ACCOUNT INFORMATION -----", "\n\n");

    printScrMargin();
    printScrColText("Session Duration:", MAX_COL_SZ, NULL);
    printScrColVal(usr->timeout, 0, 0, " minutes(s)\n\n");


    if (edit_mode_flg > 0) {

    } else {
        pauseScr (NULL, 1);
    }

    if (CURRENT_USR_TYPE == USR_PRINCIPAL)
        switch (usr_type) {
            case USR_STUDENT:
            enrollmentListScreen();
            break;
            
            case USR_TEACHER:
            break;
            
            default:
            userHomeScreen();
        }
    else
        userHomeScreen();
}

void displayUserListHdr() {
    char col_hdr1[50], col_hdr3[50];

    switch (CURRENT_USR_TYPE) {
        case USR_STUDENT:
            strcpy(col_hdr1, "Teacher");
            strcpy(col_hdr3, "Grade");
            break;
        case USR_TEACHER:
            strcpy(col_hdr1, "Student");
            strcpy(col_hdr3, "Grade");
            break;
        case USR_PRINCIPAL:
            strcpy(col_hdr1, "Student");
            strcpy(col_hdr3, "Teacher");
            break;       
    }

    printf ("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -\n\n");
    printf ("     %-50s Subject %30s\n\n",                                                     col_hdr1, col_hdr3);
    printf ("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -\n\n");
}

void subjectListScreen() {

    displayUserListHdr();

    pauseScr (NULL, 1);

    userHomeScreen();
}

void studentListScreen(const int edit_mode_flg) {
    do {
        displayUserListHdr();

        if (edit_mode_flg > 0) {

        } else {
            pauseScr (NULL, 1);
            break;
        }
    } while(1);

    userHomeScreen();
}

void enrollmentListScreen(const int edit_mode_flg) {
    do {
        displayUserListHdr();

        if (edit_mode_flg > 0) {

        } else {
            pauseScr (NULL, 1);
            break;
        }
    } while(1);

    if (edit_mode_flg > 0) {

    } else {
    
    }
    userHomeScreen();
}

/*
int calculateAge(struct tm* tm_dob) {
    //get current time
    time_t t = time(NULL);
    struct tm *tm_cur = localtime(&t);
    
    //calculate Age
    int age = tm_cur->tm_year - tm_dob->tm_year;
    
    //make fine adjustment to age based on further date details
    if (tm_dob->tm_mon > tm_cur->tm_mon || tm_dob->tm_mon == tm_cur->tm_mon && tm_dob->tm_mday > tm_cur->tm_mday) {
        age -= 1;
    }
    
    return age;    
}

void processInmateCrime() {
    int randomYears1, randomYears2;
    int choice = 0;
    
    srand(time(NULL));  //initialize and seed random number generator using the current time
    
    do {
        
        clearScr();
        printf("\nChoose a crime from the following list:\n");
    
        for (int i = 0; i < CRIME_LIST_SZ; i++) {
            printf("[%d] %s\n", i + 1, CRIME_LIST[i]);
        }
    
        // Allow the user to choose only 1 crime
        promptInt("\nEnter the number corresponding to the crime: ", &choice, 2);
        
        if (1 <= choice && choice <= CRIME_LIST_SZ) {
            break;
        }
        
        pauseScr("\nInvalid input. Please enter a valid option.", 1);
        
    } while(1);
    
    strcpy(Data.CriSLO, CRIME_LIST [choice - 1]);
        
    randomYears1 = rand() % 100 + 1;
    randomYears2 = rand() % 100 + 1;
    
    Data.SenS = (float) randomYears2 / randomYears1;
}

void sanitizeEntryDtl(char* Detail) {
    if (strlen(Detail) == 0) {
        strcpy(Detail, BLANK_DATA);
    }
}

void registerInmateData() {
    Inmates = realloc(Inmates, ++entryCount * DATA_SZ);
    Inmates[entryCount-1] = Data;
}

void saveInmateData() {
    FILE *fptr;
    fptr = fopen (INMATE_DATA_FILENAME, "a");

    Data = Inmates[entryCount-1];

    fprintf (fptr, "%s\n%d\n%s\n%s\n%s\n%.2f\n\n", Data.IFname, Data.INage, Data.Indre, Data.INdob, Data.CriSLO, Data.SenS);
    fclose (fptr);
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
        printf("Sentence: %.1f year(s)\n\n", Data.SenS);
    }
}*/
      

/*******************************************************************/
/****************** Rudimentary Utility Functions ******************/
/*******************************************************************/

void msleep(int delay) {   // delay in milliseconds
#if defined(_WIN32) || defined(__CYGWIN__)  // Windows OS
    Sleep (delay);          
#else  // Linux OS
    int delay_sec = delay / 1000;  // convert milliseconds to seconds
    while ((delay_sec = sleep(delay_sec)) > 0); 
    usleep (delay % 1000 * 1000);
#endif
}

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
    char *result; int spn_len, retry = 0;

    if (input_sz <= 0 && input != NULL)
        input_sz = sizeof(input);

    if (input_sz > 0) {

        result = fgets(input, input_sz + 1, stream);  // fgets actually reads input_sz - 1 characters from the input stream

        while (!result && retry++ < FILE_READ_FRQ) {
            msleep (FILE_READ_LAT);
            result = fgets(input, input_sz + 1, stream);
        }

        if (result) {
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

        if (result && isDigitStr(str_input)) {
           *input = atoi(str_input);
            return input;
        }
    }

    return NULL;
}

void* freadVal(void* input, const char* fmt, FILE* fptr) {
    int retry = 0, result = fscanf(fptr, fmt, input); 

    while (result < 0 && retry++ < FILE_READ_FRQ) {
        msleep (FILE_READ_LAT);
        result = fscanf(fptr, fmt, input);
    }
    return result > 0 ? input : NULL;
}

int* freadInt(int* input, FILE* fptr) {
    return freadVal(input, "%d\n", fptr);
}

float* freadFloat(float* input, FILE* fptr) {
    return freadVal(input, "%f\n", fptr);
}

void readOption(int* input) {
    readInt(input, 2, stdin);
}

void promptInt(const char * message, int* input, int max_digits) {  // reads an integer value on the console input stream
    printf (message);
    readInt(input, max_digits + 1, stdin);
}

void promptLn(const char * message, char* input, int input_sz) {  // reads an entire line of characters on the console input stream
    printf (message);
    readChars(input, input_sz, stdin);
}

void promptLgn(const char * message, int* input) {  // captures line of characters on the console input stream without displaying it
    char* pass = getpass(message);
    if (!pass || strlen(pass) != 4)
        *input = 0;
    else
        *input = atoi(pass);
}

void pauseScr(const char * message, const int alt_msg_flg) {
    printf (message);
    if (alt_msg_flg > 0) {
        printf ("\nPress ENTER key to continue");
    }
    flush();
}

const char* trim (const char* str) 
{
    if (str == NULL || str == "")  
        return str; 

    int end = strlen(str) - 1, j = 0;
    while (j++ < end && isspace(*str)) {
        str++;
    }
    end = strlen(str) - 1;
    while (end >= 0 && isspace(str[end])) {
        end--;
    }

    static char* trstr;
    static int trsz;

    int sz = end + (str[end] ? 2 : 1); 
    if (!trstr || trsz  < sz) {
        trstr = realloc(trstr, sz);
        trsz = sz;
    }
    memset(trstr, 0, strlen(trstr));
    for (j = 0; j <= end; j++, str++) {
        trstr[j] = *str;
    }

    return trstr;
}

int getPrintableCharCnt(const char* str) {
    int count = 0;
    if (str) {
        int len = strlen(str);
        for (int i=0; i < len; i++) {
            if (isprint(str[i])) {
                count++;
            }
        }
    }
    return count;
}

int isDigitStr(const char* str) {
    if (!str) { return 0; }
    int len = strlen(str);
    if (!len) { return 0; }
    for (int i=0; i < len; i++) {
        if (!isdigit(str[i])) {
            return 0;
        }
    }
    return 1;
}