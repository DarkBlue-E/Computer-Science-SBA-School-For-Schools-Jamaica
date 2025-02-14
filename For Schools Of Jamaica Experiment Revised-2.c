#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include "screenio.h"

// System memory resource properties
#define MEM_EXP_SZ 10  // growth increment to be applied to dynamically increasing lists

// Screen display properties (measured in characters)
#define MAX_SZ_ITEM_NO 4
#define MAX_SZ_FULLNAME 50
#define MAX_SZ_SUBJECT 30
#define MAX_SZ_GRADE 4

// User attribute max sizes
#define FNAME_SZ 14
#define LNAME_SZ 35
#define ADDR_SZ 1000
#define DOB_SZ 10
#define TOUT_SZ 10

// Password security settings
#define PASSCODE_SZ 4
#define PASSCODE_MN 1000
#define PASSCODE_MX 2000

// Logout type enumeration
#define LG_SESSION_TIMEOUT 1
#define LG_SESSION_INVALID 2

// Retry sub screen enumeration
#define RT_DEFAULT 0
#define RT_CONFIRM 1

// Irregular days month enumeration
#define FEB 2
#define APR 4
#define JUN 6
#define SEP 9
#define NOV 11

// Data entry file resources
#define PRINCIPAL_DATA_FILENAME "Principal.txt"
#define TEACHER_DATA_FILENAME "Teachers.txt"
#define STUDENT_DATA_FILENAME "Students.txt"
#define ENROLL_DATA_FILENAME "Enrollments.txt"
// Void data placeholders
#define UNSPEC_DATA "<NONE>"   // frontend view label shown in the place of values not yet assigned or set
#define BLANK_DATA "<BLANK>"   // backend data label used in place of string values that are empty
// Log messages
#define MSG_ACTION_CONTD "The current action may continue but render inconsistent results."
#define MSG_ACTION_ABORT "The current action will be aborted."
#define MSG_SAVE_ERROR   "The current changes may not be preserved."

// User type enumeration
#define USR_STUDENT 1
#define USR_TEACHER 2
#define USR_PRINCIPAL 3

// User registration status enumeration
#define REG_STAT_PROF 0
#define REG_STAT_SUBJ 1
#define REG_STAT_FULL 2

// User default timeout enumeration (in minutes)
#define T_DEF_STUDENT   10
#define T_DEF_TEACHER   15
#define T_DEF_PRINCIPAL 30


struct UserEntry {
int  loginID;
char Fname [FNAME_SZ + 1];
char Lname [LNAME_SZ + 1];
char Addr  [ADDR_SZ + 1];
char Dob   [DOB_SZ + 1];
int  timeout;   //measured in minutes
int  reg_stat;
};

struct EnrollEntry {
int studentID;
int subjectID;
int teacherID;
float grade;
};

typedef struct tm Date;
typedef struct UserEntry User;
typedef struct EnrollEntry Enrollment;

// Elementary size definition caching
const int USER_SZ = sizeof(User);
const int ENROLL_SZ = sizeof(Enrollment);

const char** SUBJECTS = {"Mathematics","English","Spanish","Computing","Biology,","Physics"};
const int SUBJECTS_SZ = 6;

User TEACHERS [6];
int TEACHERS_SZ = 6;

User Principal, *Students;
int principalCapacity = 1;
int studentCapacity = 0;

Enrollment *Enrollments;
int enrollCapacity = 0;

int CURRENT_USR_TYPE;
User* CURRENT_USR;

// Function prototype declarations for functions whose invocations occur BEFORE their declaration
// Note that this is only necessary for functions that return pointer types
const char* fsan(const char*);
void* setDataList(void *, int);


/********************************************************************/
/***************** Convenience Auxilliary Functions *****************/
/********************************************************************/

User *loadUserData(User *list, int *list_sz_ptr, FILE *fptr)
{
    int data_sz = *list_sz_ptr;

    if(!freadInt(&data_sz, fptr))
            return NULL;

    if(*list_sz_ptr != data_sz ||!list)
    {
        if (list = setDataList(list, data_sz))
            *list_sz_ptr  = data_sz;
        else
            data_sz = 0;
    }

    for (int i=0; i < data_sz; i++) {
        if (freadInt (&list[i].loginID, fptr)) {
            readChars (list[i].Fname, FNAME_SZ, fptr);
            readChars (list[i].Lname, LNAME_SZ, fptr);
            readChars (list[i].Addr , ADDR_SZ, fptr);
            readChars (list[i].Dob  , DOB_SZ,   fptr);
            freadInt  (&list[i].timeout, fptr);
            freadInt  (&list[i].reg_stat, fptr);
        }
        else return NULL;   // assert parity between expected and actually loaded data
    }

    return list;
}

Enrollment *loadEnrollData(Enrollment *list, int *list_sz_ptr, FILE *fptr)
{
    int data_sz = *list_sz_ptr;

    if(!freadInt(&data_sz, fptr))
        return NULL;

    if(*list_sz_ptr != data_sz ||!list)
    {
        if (list = setDataList(list, data_sz))
            *list_sz_ptr  = data_sz;
        else
            data_sz = 0;
    }

    for (int i=0; i < data_sz; i++) {
        if (freadInt  (&list[i].studentID, fptr)) {
            freadInt  (&list[i].teacherID, fptr);
            freadInt  (&list[i].subjectID, fptr);
            freadFloat (&list[i].grade, fptr);
        }
        else return NULL;   // assert parity between expected and actually loaded data
    }

    return list;
}

FILE *saveUserData(User *list, int list_sz, FILE *fptr)
{
    User* u;

    if (fprintf (fptr, "%d\n", getListEntryCnt(list, list_sz)) <= 0)
        return NULL;

    for (int i = 0; i < list_sz; i++)
    {
        u = list + i;
        if (u->loginID > 0) {
            if (fprintf (fptr, "%d\n%s\n%s\n%s\n%s\n%d\n%d\n",
                         u->loginID, fsan(u->Fname), fsan(u->Lname), fsan(u->Addr), fsan(u->Dob), u->timeout, u->reg_stat) <= 0)
                return NULL;
        }
    }
    return fptr;
}

FILE *saveEnrollData(Enrollment *list, int list_sz, FILE *fptr)
{
    Enrollment* e;

    if (fprintf (fptr, "%d\n", getListEntryCnt(list, list_sz)) <= 0)
        return NULL;

    for (int i = 0; i < list_sz; i++)
    {
        e = list + i;
        if (e->subjectID > 0) {
            if (fprintf (fptr, "%d\n%d\n%d\n%.2f\n", e->studentID, e->teacherID, e->subjectID, e->grade) <= 0)
                return NULL;
        }
    }
    return fptr;
}

FILE *stageData(void *list, int *list_sz_ptr, const char *DATA_FNAME, const int read_only_flg) { // used to initiate a transaction operation
    FILE* fptr = fopen (DATA_FNAME, "r");
    void* ptr;

    if (fptr)
    {
        if (list == Enrollments) {
            ptr = loadEnrollData ((Enrollment*)list, list_sz_ptr, fptr);
        } else {
            ptr = loadUserData ((User*)list, list_sz_ptr, fptr);
        }
        fclose(fptr);

        if (!ptr) fptr = NULL;

        if (!read_only_flg)
        {
            if (ptr) {
                fptr = fopen (DATA_FNAME, "w");
            }
        }
    }
    return fptr;
}

int persistData(void *list, int list_sz, FILE *fptr) { // used to conclude a transaction operation
    void *ptr;

    if (list == Enrollments) {
        ptr = saveEnrollData ((Enrollment*)list, list_sz, fptr);
    } else {
        ptr = saveUserData ((User*)list, list_sz, fptr);
    }
    fclose(fptr);

    return ptr != NULL;
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

char* getDataFileName(int usr_type) {

    if (usr_type <= 0) usr_type = CURRENT_USR_TYPE;

    switch (usr_type) {
        case USR_STUDENT:
            return STUDENT_DATA_FILENAME;
        case USR_TEACHER:
            return TEACHER_DATA_FILENAME;
        case USR_PRINCIPAL:
            return PRINCIPAL_DATA_FILENAME;
        default:
           return ENROLL_DATA_FILENAME;
    }
}

char* getUserTitle(const char* pre_ttl, int usr_type, const char* pst_ttl) {

    static char* title;

    if (!title)
        title = malloc(SCREEN_SZ);

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

int getListEntryCnt(void *list, int list_sz) {
    int cnt = 0;
    char *subj, trsubj[MAX_SZ_SUBJECT];
    Enrollment *enroll;
    User *usr;

    for (int i=0; i < list_sz; i++) {
        if (list == SUBJECTS)
        {
            subj = *(char**)(list + i);
            if (subj && strlen(trim(subj, -1, trsubj)) > 0)
                cnt++;
        }
        else if (list == Enrollments)
        {
            enroll = (Enrollment*)(list + i);
            if (enroll->subjectID > 0)
                cnt++;
        }
        else
        {
            usr = (User*)(list + i);
            if (usr->loginID > 0)
                cnt++;
        }
    }
    return cnt;
}

int* getDataListSz(int usr_type) {

    if (usr_type <= 0) usr_type = CURRENT_USR_TYPE;

    switch (usr_type) {
        case USR_STUDENT:
            return &studentCapacity;
        case USR_TEACHER:
            return &TEACHERS_SZ;
        case USR_PRINCIPAL:
            return &principalCapacity;
        default:
            return &enrollCapacity;
    }
}

void* getDataList(int usr_type) {

    if (usr_type <= 0) usr_type = CURRENT_USR_TYPE;

    switch (usr_type) {
        case USR_STUDENT:
            return Students;
        case USR_TEACHER:
            return TEACHERS;
        case USR_PRINCIPAL:
            return &Principal;
        default:
            return Enrollments;
    }
}

void* setDataList(void *list, int list_sz) {
    void* new_list;
    int data_sz;

    // determine the entry size based on the list identified
    if (list == Enrollments) {
        data_sz = ENROLL_SZ;
    } else {
        data_sz = USER_SZ;
    }

    new_list = realloc(list, list_sz * data_sz);

    if (new_list) {
        // determine the list to set based on the list identified
        if (list == Enrollments) {
            Enrollments = new_list;
        } else {
            Students = new_list;
        }
    }

    return new_list;
}

void* expandDataList(void *list, int *list_sz_ptr) {

    list = setDataList(list, *list_sz_ptr + MEM_EXP_SZ);

    if (list) {
        memset(list + *list_sz_ptr, 0, MEM_EXP_SZ);
        (*list_sz_ptr) += MEM_EXP_SZ;
    }
    return list;
}

User* getUser(int loginID, User* list, int list_sz) {
    for (int i=0; i < list_sz; i++) {
        if (list[i].loginID == loginID)
            return (User*)(list + i);
    }
    return NULL;
}

User* addUser(User *usr, User *list, int *list_sz_ptr) {
    // handle null reference
    if (!usr) {
        return usr;
    }

    // add user to available tombstoned slot (if any)
    int i;
    for (i = 0; i < *list_sz_ptr; i++) {
        if (list[i].loginID <= 0) {
            list[i] = *usr;
            return (User*)(list + i);
        }
    }

    // increase slot capacity and add user at the end
    if (list = expandDataList(list, list_sz_ptr)) {
        list[i] = *usr;
        return (User*)(list + i);
    }
    return list;
}

Enrollment* addEnrollment(Enrollment *enroll) {
    // handle null reference
    if (!enroll) {
        return enroll;
    }

    // add enrollment to available tombstoned slot (if any)
    int i;
    for (i = 0; i < enrollCapacity; i++) {
        if (Enrollments[i].subjectID <= 0) {
            Enrollments[i] = *enroll;
            return (Enrollment*)(Enrollments + i);
        }
    }

    // increase slot capacity and add enrollment at the end
    if (expandDataList(Enrollments, &enrollCapacity)) {
        Enrollments[i] = *enroll;
        return (Enrollment*)(Enrollments + i);
    }
    return NULL;
}

void deleteUser(User *usr) {
    if (usr) {
        usr->loginID = 0;
    }
}

void deleteEnrollment(Enrollment *enroll) {
    if (enroll) {
        enroll->subjectID = 0;
    }
}

User* registerUser(int loginID, int usr_type, User *list, int *list_sz_ptr) {

    User new_usr = {loginID};

    switch (usr_type) {
        case USR_STUDENT:
            new_usr.timeout = T_DEF_STUDENT;
            break;
        case USR_TEACHER:
            new_usr.timeout = T_DEF_TEACHER;
            break;
        case USR_PRINCIPAL:
            new_usr.timeout = T_DEF_PRINCIPAL;
    }

    return addUser(&new_usr, list, list_sz_ptr);
}

Enrollment* registerEnrollment(int studID, int subjID, int teachID) {

    Enrollment new_enroll = {studID, subjID, teachID, -1};

    return addEnrollment(&new_enroll);
}

int isLeapYear(int yr) {
    return yr > 0 && (yr % 100 == 0 && yr % 400 == 0 || yr % 100 > 0  && yr % 4 == 0);
}

int convertDate(const char* dstr, Date* dptr) {  // Gregarian-based validation date converter
    //check if date string is valid
    if (!dstr) return 0;

    int date_len = 3;
    int date_buf [date_len];

    //copy date to editable string
    int str_len = strlen(dstr);
    char* dt = malloc(str_len + 1);
    char c;

    for (int i=0; i < str_len; i++) {
        c = dstr[i];
        if (!(isdigit(c) || c == '/') )
            return 0;
        dt[i] = c;
    }
    dt[str_len] = '\0';

    //check date format syntax
    for (int i=0, j; i < date_len; i++)
    {
        j = strcspn(dt, "/");

        if (i == date_len - 1)
        {
            if (j == str_len && (date_buf[i] = atoi(dt)) > 0) {
                continue;
            }
        }
        else if (j < str_len)
        {
            dt[j] = '\0';
            if ((date_buf[i] = atoi(dt)) > 0) {
                dt += j + 1;
                str_len -= j + 1;
                continue;
            }
        }
        return 0;
    }

    //check date format semantics
    int dy = date_buf [0];
    int mn = date_buf [1];
    int yr = date_buf [2];

    if (dy > 31 || mn > 12)
        return 0;

    if (dy == 31 && (mn == APR || mn == JUN || mn == SEP || mn == NOV))
        return 0;

    if (dy > (isLeapYear(yr)? 29:28) && mn == FEB)
        return 0;


    if (dptr) {
        //use local time settings to properly initialize date
        time_t curr_tm = time(NULL);
        *dptr = *localtime(&curr_tm);

        dptr->tm_mday = dy;
        dptr->tm_mon  = mn - 1;
        dptr->tm_year = yr - 1900;
    }

    return 1;
}

int calculateAge(Date* tm_dob, const char* dob) {
    //parse date of birth
    if (!tm_dob) {
        Date tm_buf;
        if (!(dob && convertDate(dob, &tm_buf))) {
            return 0;
        }
        tm_dob = &tm_buf;
    }

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

int calculateTally(int loginID, int lgn_usr_type, int tally_usr_type) {
    int entry_ids [enrollCapacity];   // buffers the target IDs found for the given login ID
    int entry_cnt = 0;
    int entry_id, entry_found;

    memset(entry_ids, 0, enrollCapacity); // ensure buffer is properly initialized

    for (int i=0; i < enrollCapacity; i++, entry_found = 0)
    {
        if (loginID == getEnrollEntryID(lgn_usr_type, Enrollments+i))
        {
            entry_id = getEnrollEntryID(tally_usr_type, Enrollments+i);

            for (int j=0; j < entry_cnt; j++) {
                if (entry_id == entry_ids[j]) {
                    entry_found = 1;    // prevents the addition of a duplicate ID
                    break;
                }
            }
            if (!entry_found) {
                entry_ids [entry_cnt++] = entry_id;
            }
        }
    }
    return entry_cnt;
}

float calculateGradeAvg(int loginID) {
    float sum = 0;
    int count = 0;
    Enrollment* enroll;

    for (int i=0; i < enrollCapacity; i++)
    {
        enroll = Enrollments + i;

        if ((loginID <= 0  || loginID == enroll->studentID || loginID == enroll->teacherID || loginID == enroll->subjectID) && enroll->grade >= 0) {
            sum += enroll->grade;
            count++;
        }
    }

    if (count > 0)
        return sum / count;
    else
        return -1;
}

////////////// Application Convenience Functions Wrappers //////////////

int skpdtl(char *input) {
    return !input || strlen(input) == 0;
}

const char *fsan(const char* attr) { // sanitize blank user data attribute for file storage
    return !attr || !strlen(attr) ? BLANK_DATA : attr;
}

const char *vsan(const char* attr) { // sanitize blank user data attribute for on-screen viewing
    return !attr || !strlen(attr) || !strcmp(attr, BLANK_DATA)? UNSPEC_DATA : attr;
}

void printMargin() {
    printScrMargin (0);
}

void printTopic(char* caption) {
    printScrTopic (caption, "-", 0, 1);
}

void print3ColHdr(const char* col_txt1, int col_sz1,
                  const char* col_txt2, int col_sz2,
                  const char* col_txt3, int col_sz3)
{
    printScrHeader (col_txt1, col_sz1, col_txt2, col_sz2, col_txt3, col_sz3, NULL, 0, NULL, 0, "=", SCREEN_PADDING);
}

void print4ColHdr(const char* col_txt1, int col_sz1, const char* col_txt2, int col_sz2,
                  const char* col_txt3, int col_sz3, const char* col_txt4, int col_sz4)
{
    printScrHeader (col_txt1, col_sz1, col_txt2, col_sz2, col_txt3, col_sz3, col_txt4, col_sz4, NULL, 0, "=", SCREEN_PADDING);
}

void warn(int msg_type, char* msg_arg, char* msg_pst) {
    log (LVL_WARN, msg_type, msg_arg, msg_pst, stdout);
}

void sys_err(const char* lg_lvl, char* msg_pst) {
    log (lg_lvl? lg_lvl: LVL_ERROR, SYS_ERROR, NULL, msg_pst, stdout);
    pauseScr(NULL, 0);
}


/*******************************************************************/
/******************** Screen Navigation Functions ******************/
/*******************************************************************/

int main() {
    initSystem();
    initSysUsers();
    loadSchoolData();
    mainscreen();
    return 0;
}

void initSystem()       // Initialize system resources
{
    Students = calloc(MEM_EXP_SZ, USER_SZ);
    Enrollments = calloc(MEM_EXP_SZ, ENROLL_SZ);

    if (!Students || !Enrollments) {
        sys_err(LVL_FATAL, "One or more critical system components could not be initialized.\n\nThe application will now exit...");
        exit(1);
    }
}

void initSysUsers()     // Initialize non-student users
{
    Principal.loginID = 1000;
    strcpy(Principal.Fname, "Paul");
    strcpy(Principal.Lname, "Duncanson");
    strcpy(Principal.Addr , "Portsmouth, Lesser Portmore");
    strcpy(Principal.Dob  , "23/05/1975");
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
            ptr = loadEnrollData((Enrollment*) list, list_sz_ptr, fptr);
        } else {
            ptr = loadUserData((User*) list, list_sz_ptr, fptr);
        }
        fclose(fptr);

        if (!ptr) {
            warn(FILE_CORRUPT, data_fname, "\n");
        }
    }
    else
    {
        warn(FILE_UNREADABLE, data_fname, "(Resetting to default state...");

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

    ptr = loadEntryData(&Principal, &principalCapacity, PRINCIPAL_DATA_FILENAME) && ptr;
    ptr = loadEntryData(TEACHERS, &TEACHERS_SZ, TEACHER_DATA_FILENAME) && ptr;
    ptr = loadEntryData(Students, &studentCapacity, STUDENT_DATA_FILENAME) && ptr;
    ptr = loadEntryData(Enrollments, &enrollCapacity, ENROLL_DATA_FILENAME) && ptr;

    if (!ptr) {
        pauseScr (NULL, 1);
    }
}

FILE *reloadData(void *list, int *list_sz_ptr, const char *DATA_FNAME, const int read_only_flg) {    // used to refresh data in public domain context
    FILE* fptr = stageData(list, list_sz_ptr, DATA_FNAME, read_only_flg);
    if (!fptr) { // data refresh failure
        sys_err (read_only_flg? NULL: LVL_FATAL, read_only_flg? MSG_ACTION_CONTD: MSG_ACTION_ABORT);
    }
    return fptr;
}

FILE *refreshData(void *list, int *list_sz_ptr, const char *DATA_FNAME, const int read_only_flg, const int auth_mode_flg) {   // used to refresh data in authenticated user context
    if (!CURRENT_USR)  // current user is logged out
    {
        if (auth_mode_flg) {
            displayLogoutScreen(LG_SESSION_INVALID);
        } else {
            sys_err (LVL_FATAL, MSG_ACTION_ABORT);
        }
        return NULL;
    }

    int loginID = CURRENT_USR->loginID;
    FILE* fptr  = reloadData(list, list_sz_ptr, DATA_FNAME, read_only_flg);

    if (!(CURRENT_USR = getUser(loginID, (User*)getDataList(NULL), *getDataListSz(NULL))))  // current user refresh failure
    {
        if (fptr && !read_only_flg) {
            fclose(fptr);
        }
        if (auth_mode_flg) {
            displayLogoutScreen(LG_SESSION_TIMEOUT);
        } else {
            sys_err (LVL_FATAL, MSG_ACTION_ABORT);
        }
        return NULL;
    }
    return fptr;
}

void commitData(void *list, int list_sz, FILE *fptr) {
    if (!(list && persistData(list, list_sz, fptr))) {
        sys_err (NULL, MSG_SAVE_ERROR);
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

void displayRetrySubScreen(int rt_mode) {
    switch (rt_mode) {
    case RT_CONFIRM:
        printf ("\n\nAre you finished with the current action? (? means any value except 0)\n");
        printf ("[?] No, I want to continue\n");
        printf ("[0] Yes, I am finished\n");
        break;
    default:
        printf ("\n\nSelect an option below to proceed (? means any value except 0)\n");
        printf ("[?] Re-try current action\n");
        printf ("[0] Return to previous menu\n");
        break;
    }
}

int retry() {
    int choice = -1;

    displayRetrySubScreen(0);
    readOption(&choice);
    return choice;
}

int confirm() {
    int choice = -1;

    displayRetrySubScreen(RT_CONFIRM);
    readOption(&choice);
    return choice;
}

void displayLogoutScreen(int lg_type) {

    displayScreenHdr("WELCOME TO 'FOR SCHOOLS OF JAMAICA'", "YOU ARE SIGNED OUT!!!");

    printScrMargin(3);

    switch (lg_type) {
        case LG_SESSION_TIMEOUT:
            printf ("Your user session has expired. Please sign in again to renew your session.");
            printScrMargin(1);
            printf ("(You can adjust the session expiration time via the EDIT PROFILE option on your HOME menu.)");
            break;
        case LG_SESSION_INVALID:
            printf ("Your user session have been invalidated. Please sign in again to renew your session.");
            break;
        default:
            printf ("You have been signed out for some unknown reason.");
            printScrMargin(2);
            printf ("If you feel that this has occurred due a haphazard system error you may try signing in again.");
            printScrMargin(1);
            printf ("If this issue persists, please contact your administrator for further assistance.");
    }

    printScrMargin(15);

    pauseScr(NULL, 1);
}

void logout(int lg_type)
{
    CURRENT_USR = NULL;

    if (lg_type > 0)
    {
        displayLogoutScreen(lg_type);
    }
}

void displayMainScreen() {

    displayScreenHdr("WELCOME TO 'FOR SCHOOLS OF JAMAICA'", "MAIN MENU");

    printTopic ("Please select your account type");

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

    printTopic ("Select an option below");

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
            break;

            case 2:
            studentRegScreen();
            break;

            case 0:
            return; // to mainscreen

            default:
            pauseScr ("INVALID REQUEST. PLEASE TRY AGAIN", 1);
        }

    } while (1);
}

void studentRegScreen() {
    int reg_chkpnt = -1;

    displayScreenHdr("FOR SCHOOLS OF JAMAICA", "STUDENT REGISTRATION");

    if (CURRENT_USR) reg_chkpnt = CURRENT_USR->reg_stat;

    if (reg_chkpnt <= REG_STAT_PROF) {
        registerUserProfile(USR_STUDENT, reg_chkpnt, 0);
    }
    if (reg_chkpnt <= REG_STAT_SUBJ) {
        subjectRegistrationScreen(0);
    }
}

void registerUserProfile(int usr_type, int reg_chkpnt, const int edit_mode_flg) {

    printf ("Please provide your profile information below (press ENTER to skip optional details)\n\n");

    if (edit_mode_flg || reg_chkpnt < REG_STAT_PROF) {
        processUserAccount(usr_type, edit_mode_flg);
    }

    if (CURRENT_USR && (edit_mode_flg || reg_chkpnt <= REG_STAT_PROF))
    {
        User usr_cpy = *CURRENT_USR;

        processUserName(usr_cpy, edit_mode_flg);
        processUserAddr(usr_cpy, edit_mode_flg);
        processUserDob(usr_cpy, edit_mode_flg);
        processUserTimeout(usr_cpy, edit_mode_flg);

        int*  list_sz_ptr = getDataListSz(usr_type);
        void* list        = getDataList(usr_type);
        char* dat_fn      = getDataFileName(usr_type);

        FILE* fwptr  = refreshData(list, list_sz_ptr, dat_fn, 0, edit_mode_flg);  // begin txn operation
        if (fwptr)
        {
            *CURRENT_USR = usr_cpy;
            CURRENT_USR -> reg_stat = REG_STAT_SUBJ;
            commitData(list, *list_sz_ptr, fwptr);  // end txn operation
            pauseScr(edit_mode_flg? "\nProfile updated successfully.\n":"\nProfile created successfully.\n", 1);
        }
    }
}

void processUserAccount(int usr_type, const int edit_mode_flg)
{
    int*  list_sz_ptr = getDataListSz(usr_type);
    void* list        = getDataList(usr_type);
    char* dat_fn      = getDataFileName(usr_type);
    FILE* fwptr;
    User* usr;

    int loginID = 0;
    do {
        loginID = promptID(edit_mode_flg);

        if (loginID < 0) {
            return;  // skip login ID edit
        }

        if (loginID > 0) {

            if (edit_mode_flg) {  // begin txn operation
                fwptr = refreshData(list, list_sz_ptr, dat_fn, 0, edit_mode_flg);
            } else {
                fwptr = reloadData(list, list_sz_ptr, dat_fn, 0);
            }
            if (!fwptr) return;  // abort txn operation

            usr = getUser(loginID, list, *list_sz_ptr);

            if (!usr || (edit_mode_flg && usr == CURRENT_USR))
            {
                if (edit_mode_flg) {
                    CURRENT_USR->loginID = loginID;
                } else {
                    CURRENT_USR = registerUser(loginID, usr_type, list, list_sz_ptr);
                }

                commitData(list, *list_sz_ptr, fwptr);  // end txn operation
                return;
            }
        }

        printf("\nThe entered login ID is either already taken or invalid. Please provide a different ID between %d and %d.\n\n", PASSCODE_MN, PASSCODE_MX);

    } while (1);
}

void processUserName(User* usr, const int edit_mode_flg) {
    char fn[FNAME_SZ + 1], ln[LNAME_SZ + 1];

    do {
        promptLn("Enter First Name: ", fn, FNAME_SZ);

        if (edit_mode_flg && skpdtl(fn))
            break;

        trim(fn, -1, fn);

        if (strlen(fn) > 0) {
            strcpy(usr->Fname, fn);
            break;
        }

        printf("The name is invalid! Please specify a non-blank first name.\n\n");

    } while (1);

    promptLn("Enter Last Name: ", ln, LNAME_SZ);

    if (!(edit_mode_flg && skpdtl(ln)))
    {
        trim(ln, -1, usr->Lname);
    }
}

void processUserAddr(User* usr, const int edit_mode_flg) {
    char addr[ADDR_SZ + 1];

    promptLn("Enter Address: ", addr, ADDR_SZ);

    if (!(edit_mode_flg && skpdtl(addr)))
    {
        trim(addr, -1, usr->Addr);
    }
}

void processUserDob(User* usr, const int edit_mode_flg) {
    char dob[DOB_SZ + 1];
    Date tm;

    do {
        promptLn("Enter Date of Birth (DD/MM/YYYY): ", dob, DOB_SZ);

        if (edit_mode_flg && skpdtl(dob))
            break;

        if (convertDate(trim(dob, -1, NULL), &tm))
        {
            if (tm.tm_year < 0) {
                printf("Invalid date of birth! Please specify a date with year 1900 or later.\n\n");
            } else if (calculateAge(&tm, NULL) < 0) {
                printf("Date of birth cannot be in the future!\n\n");
            } else {
                strcpy(usr->Dob, dob);
                break;
            }
        } else {
            printf("The date is invalid! Please specify date in the correct format.\n\n");
        }

    } while (1);
}

void processUserTimeout(User* usr, const int edit_mode_flg) {
    char tout[TOUT_SZ + 1];
    Date tm;

    do {
        promptLn("Enter Session Timeout (mins): ", tout, TOUT_SZ);

        if (edit_mode_flg && skpdtl(tout))
            break;

        int tm_out = atoi(trim(tout, -1, NULL));
        if (tm_out > 0) {
            usr->timeout = tm_out;
            break;
        }

        printf("The timeout duration is invalid! Please enter a positive integer value.\n\n");

    } while (1);
}

void displaySubjRegScreen() {

    displayScreenHdr("FOR SCHOOLS OF JAMAICA", "SUBJECT REGISTRATION");

    printf ("Select any of the following options to continue\n");
    printf ("[1] Return to main menu\n");
    printf ("[2] Enter inmate data\n");
}

void subjectRegistrationScreen(const int edit_mode_flg) {

    displaySubjRegScreen();

}

int promptID(const int edit_mode_flg) {
    char pswd [PASSCODE_SZ + 1];

    promptLgn("Enter login ID: ", &pswd, PASSCODE_SZ);

    if (edit_mode_flg && skpdtl(pswd))
        return -1;
    if (strlen(pswd) != PASSCODE_SZ)
        return 0;

    int loginID = atoi(pswd);

    return (PASSCODE_MN <= loginID && loginID <= PASSCODE_MX)? loginID: 0;
}

void displayLoginScreen() {
    displayScreenHdr("FOR SCHOOLS OF JAMAICA", getUserTitle(NULL, NULL, " LOGIN"));
}

void loginScreen() {
    int   reg_err_flg;
    int   loginID     = 0;
    int*  list_sz_ptr = getDataListSz(NULL);
    void* list        = getDataList(NULL);
    char* dat_fn      = getDataFileName(NULL);

    do {
        displayLoginScreen();

        loginID = promptID(0);

        if (!reloadData(list, list_sz_ptr, dat_fn, 1)) {
            pauseScr("\nLogin failed! Please contact system administrator.", 1);
            return;
        }

        reg_err_flg = 0;

        if (CURRENT_USR = getUser(loginID, list, *list_sz_ptr)) {
            if (CURRENT_USR->reg_stat == REG_STAT_FULL) {
                userHomeScreen();
                return;
            }
            if (CURRENT_USR_TYPE == USR_STUDENT) {
                studentRegScreen();
                return;
            }
            reg_err_flg = -1;
        }

        if (reg_err_flg) {
            printf ("\nThe entered login ID is invalid. Please enter a different ID or report this incident to your administrator.");
        } else {
            printf ("\nThe entered login ID is either not found or incorrect.");
            if(CURRENT_USR_TYPE == USR_STUDENT)
            printf ("\n(If you do not have an account you can create one via the SIGN UP option on the previous screen.)");
        }

        if (!retry()) {
            return;
        }

    } while(1);
}

void displayUserHomeScreen() {

    displayScreenHdr("FOR SCHOOLS OF JAMAICA", getUserTitle(NULL, NULL, " HOME"));

    printTopic ("Select an option below");

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
            logout(0);
            return;

            case 1:
            userProfileScreen(CURRENT_USR, NULL, 0);
            return;

            case 2:
            userProfileScreen(CURRENT_USR, NULL, 1);
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
    const int H_MARGIN = 25;
    const int MAX_COL_SZ = 24 + SCREEN_PADDING;  //max size for the field names column
    float avg;

    if (usr_type <= 0) usr_type = CURRENT_USR_TYPE;

    displayScreenHdr("FOR SCHOOLS OF JAMAICA", getUserTitle(NULL, usr_type, " PROFILE"));

    printScrTitle(NULL, "----- PERSONAL INFORMATION -----", "\n\n");

    printScrHMargin(H_MARGIN);
    printScrColText("First Name:", MAX_COL_SZ, NULL);
    printScrColText(usr->Fname, 0, "\n");
    printScrHMargin(H_MARGIN);
    printScrColText("Last Name:", MAX_COL_SZ, NULL);
    printScrColText(vsan(usr->Lname), 0, "\n");
    printScrHMargin(H_MARGIN);
    printScrColText("Address:", MAX_COL_SZ, NULL);
    printScrColText(vsan(usr->Addr), 0, "\n");
    printScrHMargin(H_MARGIN);
    printScrColText("Date of Birth:", MAX_COL_SZ, NULL);
    printScrColText(usr->Dob, 0, "\n");
    printScrHMargin(H_MARGIN);
    printScrColText("Age:", MAX_COL_SZ, NULL);
    printScrColVal(calculateAge(NULL, usr->Dob), 0, 0, "\n\n");

    printScrTitle(NULL, "----- COURSE INFORMATION -----", "\n\n");

    avg = calculateGradeAvg(usr_type==USR_PRINCIPAL? NULL: usr->loginID);

    if (usr_type == USR_STUDENT)
    {
        printScrHMargin(H_MARGIN);
        printScrColText("Total Subjects:", MAX_COL_SZ, NULL);
        printScrColVal(calculateTally(usr->loginID, usr_type, NULL), 0, 0, "\n");
        printScrHMargin(H_MARGIN);
        printScrColText("Total Teachers:", MAX_COL_SZ, NULL);
        printScrColVal(calculateTally(usr->loginID, usr_type, USR_TEACHER), 0, 0, "\n");
        printScrHMargin(H_MARGIN);
        printScrColText("Average Grade:", MAX_COL_SZ, NULL);
        if(avg < 0)
        printScrColText(UNSPEC_DATA, 0, "\n\n");
        else
        printScrColVal(avg, 0, 0, "%%\n\n");
    }
    else if (usr_type == USR_TEACHER)
    {
        printScrHMargin(H_MARGIN);
        printScrColText("Total Students:", MAX_COL_SZ, NULL);
        printScrColVal(calculateTally(usr->loginID, usr_type, USR_STUDENT), 0, 0, "\n");
        printScrHMargin(H_MARGIN);
        printScrColText("Total Subjects:", MAX_COL_SZ, NULL);
        printScrColVal(calculateTally(usr->loginID, usr_type, NULL), 0, 0, "\n");
        printScrHMargin(H_MARGIN);
        printScrColText("Average Student's Grade:", MAX_COL_SZ, NULL);
        if(avg < 0)
        printScrColText(UNSPEC_DATA, 0, "\n\n");
        else
        printScrColVal(avg, 0, 0, "%%\n\n");
    }
    else if (usr_type == USR_PRINCIPAL)
    {
        printScrHMargin(H_MARGIN);
        printScrColText("Total Subjects:", MAX_COL_SZ, NULL);
        printScrColVal(SUBJECTS_SZ, 0, 0, "\n");
        printScrHMargin(H_MARGIN);
        printScrColText("Total Teachers:", MAX_COL_SZ, NULL);
        printScrColVal(TEACHERS_SZ, 0, 0, "\n");
        printScrHMargin(H_MARGIN);
        printScrColText("Total Students:", MAX_COL_SZ, NULL);
        printScrColVal(studentCapacity, 0, 0, "\n");
        printScrHMargin(H_MARGIN);
        printScrColText("Average Student's Grade:", MAX_COL_SZ, NULL);
        if(avg < 0)
        printScrColText(UNSPEC_DATA, 0, "\n\n");
        else
        printScrColVal(avg, 0, 0, "%%\n\n");
    }

    printScrTitle(NULL, "----- ACCOUNT INFORMATION -----", "\n\n");

    printScrHMargin(H_MARGIN);
    printScrColText("Session Duration:", MAX_COL_SZ, NULL);
    printScrColVal(usr->timeout, 0, 0, " minutes(s)\n\n");


    if (edit_mode_flg) {
        registerUserProfile(usr_type, -1, edit_mode_flg);
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

        if (edit_mode_flg) {

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

        if (edit_mode_flg) {

        } else {
            pauseScr (NULL, 1);
            break;
        }
    } while(1);

    if (edit_mode_flg) {

    } else {

    }
    userHomeScreen();
}


/*

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
}*/
