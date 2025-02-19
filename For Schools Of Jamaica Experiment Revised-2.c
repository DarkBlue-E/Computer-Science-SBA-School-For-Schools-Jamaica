#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include "screenio.h"

// Application resource properties
#define TTL_MAIN "FOR SCHOOLS OF JAMAICA"
#define PTR_SZ 8
#define MEM_EXT_SZ 10  // fixed capacity increment to be applied when extending a dynamic list

// Screen display column max sizes (measured in characters)
#define ITEM_NO_SZ 4  
#define FULL_NAME_SZ 50  
#define SUBJ_NAME_SZ 30
#define GRADE_SZ 4

// User attribute max sizes
#define FNAME_SZ 14
#define LNAME_SZ 35
#define ADDR_SZ 105
#define DOB_SZ 10
#define TOUT_SZ 10

// Password security settings
#define PASSCODE_SZ 4
#define PASSCODE_MN 1000
#define PASSCODE_MX 2000

// Logout type enumeration
#define LGO_SESS_EXPIRED 1
#define LGO_SESS_INVALID 2

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
// Other data list type enumeration
#define LST_ENROLL 4
#define LST_SUBJECT 5
#define LST_DEFAULT 6

// User registration status enumeration
#define REG_STAT_PROF 0
#define REG_STAT_SUBJ 1
#define REG_STAT_FULL 2

// User default timeout (in minutes)
#define TM_DEF_STUDENT   10
#define TM_DEF_TEACHER   15
#define TM_DEF_PRINCIPAL 30


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
const int SUBJECT_SZ = sizeof(char*);

const char* SUBJECTS[] = {"Mathematics","English","Spanish","Computing","Biology","Physics"};
const int SUBJECTS_SZ = 6;

User TEACHERS [6];
int TEACHERS_SZ = 6;

User Principal, *Students;
int principalCapacity = 1;
int studentCapacity = 0;

Enrollment *Enrollments;
int enrollCapacity = 0;
User DEF_USER;
Enrollment DEF_ENROLL;

int CURRENT_USR_TYPE;
User* CURRENT_USR;

// Function prototype declarations for functions whose invocations occur BEFORE their declaration
// Note that this is only necessary for functions that return pointer types 
const char* fsan(const char*); 
void* setDataListSz(void*, int);


/********************************************************************/
/***************** Convenience Auxilliary Functions *****************/
/********************************************************************/

User *loadUserData(User *list, int *list_sz_ptr, FILE *fptr)  // NOTE: can produce partial loads upon failure
{
    int data_sz = *list_sz_ptr;

    if(!freadInt(&data_sz, fptr)) 
            return NULL;

    if(*list_sz_ptr != data_sz ||!list) 
    {
        if (list = setDataListSz(list, data_sz))
            *list_sz_ptr = data_sz;
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

Enrollment *loadEnrollData(Enrollment *list, int *list_sz_ptr, FILE *fptr)  // NOTE: can produce partial loads upon failure 
{
    int data_sz = *list_sz_ptr;

    if(!freadInt(&data_sz, fptr)) 
        return NULL;

    if(*list_sz_ptr != data_sz ||!list) 
    {
        if (list = setDataListSz(list, data_sz))
            *list_sz_ptr = data_sz;
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
        static char full_name [FULL_NAME_SZ + 1];

        strcpy(full_name, usr->Fname);
        strcat(full_name, " ");
        strcat(full_name, usr->Lname);  

        return full_name;         
    }
    return NULL;
}

char* getTitle(const char* pre_ttl, int lst_type, const char* pst_ttl) {

    static char* title;

    if (!title)
        title = malloc(SCR_SIZE);

    strcpy(title, "");  // clears any previously set title
    
    if (pre_ttl)
        strcat(title, pre_ttl);

    if (lst_type <= 0) lst_type = CURRENT_USR_TYPE;

    switch (lst_type) {
        case USR_STUDENT:
            strcat(title, "STUDENT");
            break;
        case USR_TEACHER:
            strcat(title, "TEACHER");
            break;
        case USR_PRINCIPAL:
            strcat(title, "PRINCIPAL"); 
            break; 
        case LST_ENROLL:
            strcat(title, "ENROLLMENT"); 
            break;   
        case LST_SUBJECT:
           strcat(title, "SUBJECT");         
    }

    if (pst_ttl)
        strcat(title, pst_ttl);

    return title;
}

char* getDataFileName(int lst_type) {

    if (lst_type <= 0) lst_type = CURRENT_USR_TYPE;

    switch (lst_type) {
        case USR_STUDENT:
            return STUDENT_DATA_FILENAME;
        case USR_TEACHER:
            return TEACHER_DATA_FILENAME;
        case USR_PRINCIPAL:
            return PRINCIPAL_DATA_FILENAME; 
        case LST_ENROLL:
           return ENROLL_DATA_FILENAME;
        default:
           return NULL;        
    }
}

void* getDataList(int lst_type) {

    if (lst_type <= 0) lst_type = CURRENT_USR_TYPE;

    switch (lst_type) {
        case USR_STUDENT:
            return Students;
        case USR_TEACHER:
            return TEACHERS;
        case USR_PRINCIPAL:
            return &Principal;
        case LST_ENROLL:
            return Enrollments;  
        case LST_SUBJECT:
            return SUBJECTS;    
        default:
            return NULL;        
    }
}

int* getDataListSz(int lst_type) {
    
    if (lst_type <= 0) lst_type = CURRENT_USR_TYPE;

    switch (lst_type) {
        case USR_STUDENT:
            return &studentCapacity;
        case USR_TEACHER:
            return &TEACHERS_SZ;
        case USR_PRINCIPAL:
            return &principalCapacity;
        case LST_ENROLL:
            return &enrollCapacity;
        case LST_SUBJECT:
            return &SUBJECTS_SZ; 
        default:
            return NULL;            
    }
}

int getListEntryCnt(void *list, int list_sz) {
    int cnt = 0;
    char *subj, trsubj[SUBJ_NAME_SZ + 1];
    Enrollment *enroll;
    User *usr;

    for (int i=0; i < list_sz; i++) {
        if (list == SUBJECTS) 
        {
            subj = *((char**)list + i);
            if (subj && strlen(trim(subj, -1, trsubj, 0)) > 0)
                cnt++;
        }
        else if (list == Enrollments) 
        {
            enroll = (Enrollment*)list + i;
            if (enroll->subjectID > 0)
                cnt++;
        }
        else 
        {
            usr = (User*)list + i;
            if (usr->loginID > 0)
                cnt++;
        }
    }
    return cnt;
}

void init(void *list, int lst_type, int offset, int count) 
{
    if (lst_type == LST_DEFAULT) {
        memset(list+offset, 0, count);
    }
    else
    for (int i=offset; i < offset + count; i++) {
        if (lst_type == LST_ENROLL)
            ((Enrollment*)list)[i] = DEF_ENROLL;
        else
            ((User*)list)[i] = DEF_USER;
    }
}

void* setDataListSz(void *list, int list_sz) {
    int data_sz;
    static void* new_list;   

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
        } else if (list == Students) {
            Students = new_list;
        }
    }

    return new_list;
}

void* extDataListSz(void *list, int *list_sz_ptr, int lst_type) 
{
    list = setDataListSz(list, *list_sz_ptr + MEM_EXT_SZ);

    if (list) {
        init (list, lst_type, *list_sz_ptr, MEM_EXT_SZ); 
        (*list_sz_ptr) += MEM_EXT_SZ;
    }
    return list;
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
    if (list = extDataListSz(list, list_sz_ptr, NULL)) {
        list[i] = *usr; 
        return (User*)(list + i);
    }
    return NULL;
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
    if (extDataListSz(Enrollments, &enrollCapacity, LST_ENROLL)) {
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

User* registerUser(int loginID, int usr_type) 
{
    int tm_out;
    switch (usr_type) {
        case USR_STUDENT:
            tm_out = TM_DEF_STUDENT;
            break;
        case USR_TEACHER:
            tm_out = TM_DEF_TEACHER;
            break;
        case USR_PRINCIPAL:
            tm_out = TM_DEF_PRINCIPAL;
        default:
            return NULL;
    }

    User new_usr = {loginID}; new_usr.timeout = tm_out;

    return addUser(&new_usr, getDataList(usr_type), getDataListSz(usr_type));
}

Enrollment* registerEnrollment(int studID, int subjID, int teachID) 
{    
    Enrollment new_enroll = {studID, subjID, teachID, -1};

    return addEnrollment(&new_enroll);
}

User* getUser(int loginID, User* list, int list_sz) {
    if (list)
    for (int i=0; i < list_sz; i++) {
        if (list[i].loginID == loginID)
            return (User*)(list + i);
    }
    return NULL;
}

User* userSearch(int loginID, int usr_type) 
{
    return getUser(loginID, (User*)getDataList(usr_type), *getDataListSz(usr_type));
}

int getEnrollEntryID(int usr_type, Enrollment* enroll) 
{   
    if (!enroll) return 0;

    switch (usr_type) 
    {
        case USR_STUDENT:
            return enroll->studentID;
        case USR_TEACHER:
            return enroll->teacherID;
        default:
           return enroll->subjectID;     
    }
}

int enrollSearch(int entryID, int usr_type, int tgt_usr_type, Enrollment* *res_list, int res_limit) 
{   
    //Warning: if res_list is provided, res_limit should not exceed the actual capacity of res_list  
    int entry_cnt = 0;
    int entry_id, j;
    Enrollment* enroll;

    if (res_limit < 0) {
        res_limit = enrollCapacity;
    }
    if (res_list) { 
        init (res_list, LST_DEFAULT, 0, res_limit);  //ensure list is properly initialized
    } else if (enrollCapacity > 0) {
        res_list = calloc(res_limit, PTR_SZ);  //provide local buffer list 
    }

    for (int i=0; i < enrollCapacity; i++)
    {
        enroll = Enrollments+i;

        if (entryID == getEnrollEntryID(usr_type, enroll)) 
        {
            entry_id = getEnrollEntryID(tgt_usr_type, enroll);

            for (j=0; j < entry_cnt; j++) 
            {  //prevents duplicate addition of enrollment entries
                if (entry_id == getEnrollEntryID(tgt_usr_type, res_list[j])) {
                    break;
                }
            }
            if (j == entry_cnt) {
                if (entry_cnt < res_limit)
                    res_list[entry_cnt++] = enroll;
                else
                    return -1;  //search results exceed limit
            }
        }
    }
    return entry_cnt;
}

Enrollment* getEnrollment(int entryID, int usr_type, Enrollment* list, int list_sz) {
    Enrollment* enroll;

    for (int i=0; i < list_sz; i++) {
        enroll = list + i;
        if (entryID == getEnrollEntryID(usr_type, enroll)) {
            return enroll;
        }
    }
    return NULL;
}

Enrollment* getEnrollPtr(int entryID, int usr_type, Enrollment** list, int list_sz) {
    Enrollment* enroll;

    for (int i=0; i < list_sz; i++) {
        enroll = list[i];
        if (entryID == getEnrollEntryID(usr_type, enroll)) {
            return enroll;
        }
    }
    return NULL;
}

int getAvlSubjects(int entryID, int usr_type, int* subj_buf)  // get available subjects for user
{  
    Enrollment* enroll_buf[enrollCapacity];
    int total, avl_total;

    total = enrollSearch(entryID, usr_type, NULL, enroll_buf, -1); 
    avl_total = SUBJECTS_SZ - total;

    if (avl_total > 0 && subj_buf) {  // populate subj_buf with indexes of available subjects
        for (int i=0, j=0; i < SUBJECTS_SZ; i++) 
        {
            if (!getEnrollPtr(i+1, NULL, enroll_buf, total)) {
                subj_buf [j++] = i;
            }
        }
    }
    return avl_total;
}

int calculateTally(int entryID, int usr_type, int tgt_usr_type) {
    return enrollSearch(entryID, usr_type, tgt_usr_type, NULL, -1);
}

float calculateGradeAvg(int entryID) {
    float sum = 0;
    int count = 0;
    Enrollment* enroll;

    for (int i=0; i < enrollCapacity; i++) 
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

int calculateAge(Date* tm_dob, const char* str_dob) {
    //parse date of birth
    if (!tm_dob) {
        Date tm_buf;
        if (!(str_dob && convertDate(str_dob, &tm_buf))) {
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
            if (j == str_len && (date_buf[i] = atoi(dt)) > 0)
                continue;
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


////////////// Application Convenience Functions Wrappers //////////////

int hash(int loginID) {
    const int SEED  = -579;
    const int SALT  = 10000;
    const int PRIME = 7;

    return (SEED + loginID) * PRIME + SALT;
}

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
    printScrTopic (caption, -1, "-", 0, 1);
}

int print3ColTblHdr(const char* col_txt1, int col_sz1, 
                     const char* col_txt2, int col_sz2, 
                     const char* col_txt3, int col_sz3) 
{
    return print4ColTblHdr (col_txt1, col_sz1, col_txt2, col_sz2, col_txt3, col_sz3, NULL, 0);
}

int print4ColTblHdr(const char* col_txt1, int col_sz1, const char* col_txt2, int col_sz2, 
                     const char* col_txt3, int col_sz3, const char* col_txt4, int col_sz4) 
{
    return printScrHeader (col_txt1, col_sz1, col_txt2, col_sz2, col_txt3, col_sz3, col_txt4, col_sz4, NULL, 0, "=", -1);
}

void warn(int msg_type, char* msg_arg, char* msg_pst) {
    log (LVL_WARN, msg_type, msg_arg, msg_pst, stdout);
}

void sys_err(const char* lg_lvl, char* msg_pst) 
{
    if (!lg_lvl) lg_lvl = LVL_ERROR;
 
    char lvl [strlen(lg_lvl) + 2]; 
    sprintf(lvl, "\n%s", lg_lvl);

    log (lvl, SYS_ERROR, NULL, msg_pst, stdout);
    pauseScr (NULL, 0);
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
    Students = calloc(0, USER_SZ);
    Enrollments = calloc(0, ENROLL_SZ);

    if (!Students || !Enrollments) {
        sys_err(LVL_FATAL, "One or more critical system components could not be initialized.\n\nThe application will now exit...");
        exit(1);
    }
}

void initSysUsers()     // Initialize non-student users 
{
    Principal.loginID = hash(1000);
    strcpy(Principal.Fname, "Paul");
    strcpy(Principal.Lname, "Duncanson");
    strcpy(Principal.Addr , "Portsmouth, Lesser Portmore");
    strcpy(Principal.Dob  , "23/05/1975");
    Principal.timeout =  TM_DEF_PRINCIPAL;
    Principal.reg_stat = REG_STAT_FULL;

    TEACHERS[0].loginID = hash(1100);
    strcpy(TEACHERS[0].Fname, "Grace");
    strcpy(TEACHERS[0].Lname, "Peters");
    strcpy(TEACHERS[0].Addr , "A1");
    strcpy(TEACHERS[0].Dob  , "23/05/1995");
    TEACHERS[0].timeout =  TM_DEF_TEACHER;
    TEACHERS[0].reg_stat = REG_STAT_FULL;

    TEACHERS[1].loginID = hash(1200);
    strcpy(TEACHERS[1].Fname, "Kim");
    strcpy(TEACHERS[1].Lname, "Long");
    strcpy(TEACHERS[1].Addr , "A2");
    strcpy(TEACHERS[1].Dob  , "23/07/1995");
    TEACHERS[1].timeout =  TM_DEF_TEACHER;
    TEACHERS[1].reg_stat = REG_STAT_FULL;

    TEACHERS[2].loginID = hash(1300);
    strcpy(TEACHERS[2].Fname, "Mercy");
    strcpy(TEACHERS[2].Lname, "James");
    strcpy(TEACHERS[2].Addr , "A3");
    strcpy(TEACHERS[2].Dob  , "17/04/1986");
    TEACHERS[2].timeout =  TM_DEF_TEACHER;
    TEACHERS[2].reg_stat = REG_STAT_FULL;

    TEACHERS[3].loginID = hash(1400);
    strcpy(TEACHERS[3].Fname, "John");
    strcpy(TEACHERS[3].Lname, "Jackson");
    strcpy(TEACHERS[3].Addr , "A4");
    strcpy(TEACHERS[3].Dob  , "17/04/1987");
    TEACHERS[3].timeout =  TM_DEF_TEACHER;
    TEACHERS[3].reg_stat = REG_STAT_FULL;

    TEACHERS[4].loginID = hash(1500);
    strcpy(TEACHERS[4].Fname, "Paul");
    strcpy(TEACHERS[4].Lname, "Wright");
    strcpy(TEACHERS[4].Addr , "A5");
    strcpy(TEACHERS[4].Dob  , "17/01/2001");
    TEACHERS[4].timeout =  TM_DEF_TEACHER;
    TEACHERS[4].reg_stat = REG_STAT_FULL;

    TEACHERS[5].loginID = hash(1600);
    strcpy(TEACHERS[5].Fname, "Kerry");
    strcpy(TEACHERS[5].Lname, "Truman");
    strcpy(TEACHERS[5].Addr , "A6");
    strcpy(TEACHERS[5].Dob  , "05/12/1991");
    TEACHERS[5].timeout =  TM_DEF_TEACHER;
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

FILE *reloadData(void *list, int *list_sz_ptr, const char *DATA_FNAME, const int read_only_flg) 
{  // used to refresh data in public domain context
    FILE* fptr = stageData(list, list_sz_ptr, DATA_FNAME, read_only_flg);  
    if (!fptr) {  // data refresh failure
        sys_err (read_only_flg? NULL: LVL_FATAL, read_only_flg? MSG_ACTION_CONTD: MSG_ACTION_ABORT);
    } 
    return fptr;
}

FILE *refreshData(void *list, int *list_sz_ptr, const char *DATA_FNAME, const int read_only_flg, const int auth_mode_flg) 
{  // used to refresh data in authenticated user context    
    if (auth_mode_flg && loggedOut() || !auth_mode_flg && !CURRENT_USR) {
        if (!auth_mode_flg) {
            sys_err(LVL_FATAL, MSG_ACTION_ABORT);
        }
        return NULL;
    }

    int loginID = CURRENT_USR->loginID;
    FILE* fptr  = reloadData(list, list_sz_ptr, DATA_FNAME, read_only_flg);

    if (!(CURRENT_USR = userSearch(loginID, NULL)))  // current user refresh failure
    {   
        if (fptr && !read_only_flg) {
            commitData(list, list_sz_ptr, fptr);    // recommit previous data as reloading in write mode always clears the source file
        }
        if (auth_mode_flg) {
            displayLogoutScreen(LGO_SESS_EXPIRED);
        } else {
            sys_err(LVL_FATAL, MSG_ACTION_ABORT);
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

void displayScreenHdr(const char* main_title, const char* sub_title) 
{
    const int repeat = SCR_SIZE / 2 - 1;

    clearScr();  //refreshes the screen
    printScrPat   (NULL, "- ", repeat, "\n\n");
    printScrTitle (NULL, main_title, "\n\n");
    printScrPat   (NULL, "- ", repeat, "\n");
    printScrTitle (NULL, sub_title, "\n");
    printScrPat   (NULL, "- ", repeat, "\n\n");
}

void displayScreenSubHdr(const char* sub_title) {
    displayScreenHdr(TTL_MAIN, sub_title);
}

void displayRetrySubScreen(int rt_mode) {
    int ttl_sz = SCR_SIZE / 2, opt_sz = ttl_sz - 4;
    char ttl[ttl_sz], rt_opt[opt_sz], ex_opt[opt_sz];

    switch (abs(rt_mode)) {
    case RT_CONFIRM:
        strcpy (ttl, "Are you finished with the current action?");
        strcpy (rt_opt, "No, I want to continue");
        strcpy (ex_opt, "Yes, I am finished");
        break;
    default:
        strcpy (ttl, "Select an option below to proceed");
        strcpy (rt_opt, "Re-try current action");
        strcpy (ex_opt, "Return to previous menu");
    }

    printf ("\n%s (? means any value except 0)\n", ttl);
    printf ("[?] %s\n", rt_opt);
    printf ("[0] %s\n", ex_opt);    
}

int retry(int rt_mode) {
    int choice = -1;

    displayRetrySubScreen(rt_mode);
    readOption(&choice);
    return choice;
}

void displayLogoutScreen(int lg_type) {
    
    displayScreenSubHdr("YOU ARE SIGNED OUT!!!");

    printScrMargin(3);

    switch (lg_type) {
        case LGO_SESS_EXPIRED:
            printf ("Your user session has expired. Please sign in again to renew your session.");
            printScrMargin(1);
            printf ("(You can adjust the session expiration time via the EDIT PROFILE option on your HOME menu.)");
            break;
        case LGO_SESS_INVALID:
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

    /* TODO: kill timer*/
}

int loggedOut() {
    if (CURRENT_USR) {
        /* TODO:*/  // reset session timer
    } else {
        displayLogoutScreen(LGO_SESS_EXPIRED);
    }
    return !CURRENT_USR;
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
    
    displayScreenSubHdr("STUDENT MENU");
    
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

void studentRegScreen() 
{
    int reg_chkpnt = CURRENT_USR ? CURRENT_USR->reg_stat : -1;

    displayScreenSubHdr(getTitle(reg_chkpnt < 0 ? NULL: "CONTINUE ", USR_STUDENT, " REGISTRATION"));

    if (reg_chkpnt <= REG_STAT_PROF) {
        if (!registerUserProfile(USR_STUDENT, reg_chkpnt, 0))
            return;
    }
    if (CURRENT_USR && reg_chkpnt <= REG_STAT_SUBJ) {
        subjectRegScreen(CURRENT_USR->loginID, reg_chkpnt > 0 ? 0: reg_chkpnt);
    }
}

int registerUserProfile(int usr_type, int reg_chkpnt, const int edit_mode_flg) {

    printf ("Please provide your profile information below (press ENTER to skip optional details)\n");

    if (edit_mode_flg || reg_chkpnt < REG_STAT_PROF) 
    {
        if (!processUserAccount(usr_type, edit_mode_flg)) {
            return 0;
        }
    }

    if (CURRENT_USR && (edit_mode_flg || reg_chkpnt <= REG_STAT_PROF))
    {
        User usr_cpy = *CURRENT_USR;

        processUserName(&usr_cpy, edit_mode_flg);
        processUserAddr(&usr_cpy, edit_mode_flg);
        processUserDob(&usr_cpy, edit_mode_flg);
        processUserTimeout(&usr_cpy, edit_mode_flg);

        int*  list_sz_ptr = getDataListSz(usr_type);
        void* list        = getDataList(usr_type);
        char* dat_fn      = getDataFileName(usr_type);
        
        FILE* fwptr  = refreshData(list, list_sz_ptr, dat_fn, 0, edit_mode_flg);  // begin txn operation   
        if (fwptr) 
        {
            *CURRENT_USR = usr_cpy;
            CURRENT_USR -> reg_stat = REG_STAT_SUBJ;
            commitData(list, *list_sz_ptr, fwptr);  // end txn operation

            char msg[32];
            sprintf(msg, "\nProfile %s successfully.\n", edit_mode_flg? "updated":"created");
            pauseScr(msg, 1);
        }
    }

    return 1;
}

int processUserAccount(int usr_type, const int edit_mode_flg) 
{
    int   loginID, id_ok;
    int*  list_sz_ptr = getDataListSz(usr_type);
    void* list        = getDataList(usr_type);
    char* dat_fn      = getDataFileName(usr_type);
    FILE* fwptr;
    User* usr;

    do {
        loginID = promptID(edit_mode_flg);

        if (loginID < 0) {  
            return 1;  // skip login ID edit
        }

        if (loginID > 0) {   
                                 // begin txn operation
            if (edit_mode_flg) {  
                fwptr = refreshData(list, list_sz_ptr, dat_fn, 0, edit_mode_flg); 
            } else {
                fwptr = reloadData(list, list_sz_ptr, dat_fn, 0);
            }
            if (!fwptr) return;  // abort txn operation

            usr = getUser(loginID, list, *list_sz_ptr);

            id_ok = !usr || (edit_mode_flg && usr == CURRENT_USR);

            if (id_ok)
            {
                if (edit_mode_flg) {
                    CURRENT_USR->loginID = loginID;  
                } else {
                    CURRENT_USR = registerUser(loginID, usr_type);
                }
            }

            commitData (getDataList(usr_type), *list_sz_ptr, fwptr);  // end txn operation
            
            if (id_ok) 
            {
                return 1;
            }
        }

        printf("\nThe login ID entered is either already taken or invalid. Please provide a different ID between %d and %d.\n", PASSCODE_MN, PASSCODE_MX);

        if (retry(0))
            displayScreenSubHdr (getTitle(NULL, USR_STUDENT, " REGISTRATION"));
        else 
            return 0;

    } while (1);
}

void processUserName(User* usr, const int edit_mode_flg) {
    char fn[FNAME_SZ + 1], ln[LNAME_SZ + 1];

    do {
        promptLn("\nEnter First Name: ", fn, FNAME_SZ);

        if (edit_mode_flg && skpdtl(fn))
            break;

        trim(fn, -1, fn, 0);

        if (strlen(fn) > 0) {
            strcpy(usr->Fname, fn);
            break;
        }

        printf("The name is invalid! Please specify a non-blank first name.");
   
    } while (1);

    promptLn("Enter Last Name: ", ln, LNAME_SZ);

    if (!(edit_mode_flg && skpdtl(ln))) 
    {
        trim(ln, -1, usr->Lname, 0);
    }
}

void processUserAddr(User* usr, const int edit_mode_flg) {
    char addr[ADDR_SZ + 1];

    promptLn("Enter Address: ", addr, ADDR_SZ);

    if (!(edit_mode_flg && skpdtl(addr))) 
    {
        trim(addr, -1, usr->Addr, 0);
    }
}

void processUserDob(User* usr, const int edit_mode_flg) {
    char dob[DOB_SZ + 1];
    Date tm;

    do {
        promptLn("Enter Date of Birth (DD/MM/YYYY): ", dob, DOB_SZ);

        if (edit_mode_flg && skpdtl(dob))
            break;

        if (convertDate(trim(dob, -1, NULL, 0), &tm)) 
        {
            if (tm.tm_year < 0) {
                printf("Invalid date of birth! Please specify a date with year 1900 or later.\n");
            } else if (calculateAge(&tm, NULL) < 0) {
                printf("Date of birth cannot be in the future!\n");
            } else {
                strcpy(usr->Dob, dob);
                break;
            }
        } else {
            printf("The date is invalid! Please specify date in the correct format.\n");
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

        int tm_out = atoi(trim(tout, -1, NULL, 0));
        if (tm_out > 0) {
            usr->timeout = tm_out;
            break;
        }

        printf("The timeout duration is invalid! Please enter a positive integer value.\n");
   
    } while (1);
}

void subjectRegScreen(int loginID, int reg_mode_flg)  // NOTE: subject registration specifically for students
{
    const int ITEM_SZ    = ITEM_NO_SZ   + SCR_PADDING;
    const int SUBJ_SZ    = SUBJ_NAME_SZ + SCR_PADDING;
    const int NAME_SZ    = FULL_NAME_SZ;

    int   fst_run_flg    = 1; 
    int   edit_mode_flg  = reg_mode_flg < 0? 0: reg_mode_flg;
    int   tbl_margin;

    int  *list_sz_ptr   = &enrollCapacity;
    void *list          = Enrollments;
    char *dat_fn        = ENROLL_DATA_FILENAME;

    int   enroll_cnt    = 0, subj_total; 
    int   subjects        [SUBJECTS_SZ];  // refers to indices of available subjects, i.e. subjects not already enrolled in by student
    Enrollment enroll_buf [SUBJECTS_SZ];

    int   input_sz      = ITEM_NO_SZ * 2 + 2;
    int   input_len, subj_no, tchr_no, i,  j;
    char  input [input_sz];
    char* inptr; 
    
top:    
    displayScreenSubHdr(getTitle(reg_mode_flg < 0 ? NULL: "CONTINUE ", LST_SUBJECT, " REGISTRATION"));
    
    tbl_margin = print3ColTblHdr("No.", ITEM_SZ, "Subject", SUBJ_SZ, "Teacher", NAME_SZ);

    if (fst_run_flg && !refreshData(list, list_sz_ptr, dat_fn, 1, edit_mode_flg))
        return;

    if (fst_run_flg) {  // filter for subjects not yet enrolled by student  
        subj_total = getAvlSubjects(loginID, USR_STUDENT, subjects);
    }

    // populate table with subject & teacher listings
    if (subj_total == 0) 
        printScrTitle(NULL, "No items available\n\n", NULL);
    else
    for (i=0, j=0; i < subj_total || j < TEACHERS_SZ; i++, j++) {
        printScrHMargin(tbl_margin); 
        printScrColVal(i + 1, ITEM_SZ, 0, NULL);      
        printScrColText(i >= subj_total ? "" : SUBJECTS[subjects[i]], SUBJ_SZ, NULL);
        printScrColText(j >= TEACHERS_SZ ? "" : getFullName(TEACHERS+j), NAME_SZ, "\n");
    }
 
    // prompt for and process enrollment input
    printf ("\n\nPlease provide your course information below (press ENTER when finished)\n\n");

    do {
        promptLn("Enter course info ([subject no.] [teacher no.]): ", input, input_sz);

        trim(input, -1, NULL, 0);

        if (!(input_len = strlen(input)))  // input is blank
        {
            if (retry(RT_CONFIRM)) { 
                fst_run_flg = 0;
                goto top;      
            } 
            else break;
        }
        else  // extract values from input line
        {  
            if ((subj_no=atoi(input)) > 0 && ((i=strcspn(input, " ")) < input_len || (i=strcspn(input, "\t")) < input_len)) 
            {
                inptr = input + i + 1; 
                trim(inptr, input_len-i-1, NULL, 0);

                if (isDigitStr(inptr) && (tchr_no = atoi(inptr)) > 0) 
                {
                    if (subj_no > subj_total || getEnrollment(subj_no, NULL, enroll_buf, enroll_cnt)) {
                        printf("The entered subject no. is incorrect or already added. Please specify a valid subject no. from the list above.\n");
                    } else if (tchr_no > TEACHERS_SZ) {
                        printf("The entered teacher no. is incorrect. Please specify a teacher no. from the list above.\n");
                    } else {  // buffer specified course info as new enrollment
                        Enrollment new_enroll = {loginID, edit_mode_flg? subjects[subj_no-1] + 1 :subj_no, TEACHERS[tchr_no-1].loginID};
                        enroll_buf [enroll_cnt++] = new_enroll;

                        if (enroll_cnt == subj_total)
                            break;
                    }

                    continue;
                }
            } 

            printf("The entered course information is invalid. Please specify enrollment options according to the syntax given.\n");
        }
    } while (1);

    // save selected enrollments
    FILE* fwptr;
    Enrollment* e;
    
    if (enroll_cnt > 0 && (fwptr = refreshData(list, list_sz_ptr, dat_fn, 0, edit_mode_flg)))  // begin txn operation 
    {
        for (j = 0; j < enroll_cnt; j++) {
            e = enroll_buf + j;
            registerEnrollment(e->studentID, e->subjectID, e->teacherID);
        }
        commitData(Enrollments, *list_sz_ptr, fwptr);  // end txn operation
    }

    else return;

    // update registration status for new students only
    if (!edit_mode_flg) 
    {
        list        = getDataList(USR_STUDENT);
        list_sz_ptr = getDataListSz(USR_STUDENT);
        dat_fn      = getDataFileName(USR_STUDENT);

        if (fwptr = refreshData(list, list_sz_ptr, dat_fn, 0, edit_mode_flg))  // begin txn operation 
        {
            CURRENT_USR->reg_stat = REG_STAT_FULL;
            commitData(list, *list_sz_ptr, fwptr);  // end txn operation
        } 
        else return;
    }

    char msg[45];
    sprintf(msg, "\nCourse information %s successfully.\n", edit_mode_flg? "updated":"created");
    pauseScr(msg, 1);
}

int promptID(const int edit_mode_flg) {
    char pswd [PASSCODE_SZ + 1];

    promptLgn("\nEnter login ID: ", &pswd, PASSCODE_SZ);

    if (edit_mode_flg && skpdtl(pswd))
        return -1;
    if (strlen(pswd) != PASSCODE_SZ)
        return 0;

    int loginID = atoi(pswd);
    
    return (PASSCODE_MN <= loginID && loginID <= PASSCODE_MX)? hash(loginID): 0;
}

void loginScreen() {
    int   reg_err_flg;
    int   loginID     = 0;
    int*  list_sz_ptr = getDataListSz(NULL);
    void* list        = getDataList(NULL);
    char* dat_fn      = getDataFileName(NULL);

    do {
        displayScreenSubHdr(getTitle(NULL, NULL, " LOGIN"));

        loginID = promptID(0);

        if (!reloadData(list, list_sz_ptr, dat_fn, 1)) {
            pauseScr("\n\nLogin failed! Please contact system administrator.", 1);
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
            printf ("\nThe entered login ID is invalid. Please enter a different ID or report this incident to your administrator.\n");
        } else {
            printf ("\nThe entered login ID is either not found or incorrect.");
            if(CURRENT_USR_TYPE == USR_STUDENT)
            printf ("\n(If you do not have an account you can create one via the SIGN UP option on the previous screen.)\n");
        }
   
        if (!retry(0)) {
            return;
        }

    } while(1);
}

void displayUserHomeScreen() {

    displayScreenSubHdr(getTitle(NULL, NULL, " HOME"));
    
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
            continue;

            case 2:
            userProfileScreen(CURRENT_USR, NULL, 1);
            continue;
        }

        if (CURRENT_USR_TYPE == USR_STUDENT) {
            switch (choice) {
                case 3:
                subjectListScreen();
                continue;

                case 4:
                subjectRegScreen(CURRENT_USR->loginID, 1);
                continue;

                case 5:
                
                continue;
            }
        } else if (CURRENT_USR_TYPE == USR_TEACHER) {
            switch (choice) {
                case 3:
                studentListScreen(0);
                continue;

                case 4:
                studentListScreen(1);
                continue;
            }
        } else if (CURRENT_USR_TYPE == USR_PRINCIPAL) {
            switch (choice) {
                case 3:
                enrollmentListScreen(0);
                continue;

                case 4:
                enrollmentListScreen(1);
                continue;

                case 5:
                enrollmentListScreen(0);
                continue;

                case 6:
                enrollmentListScreen(1);
                continue;

                case 7:
                enrollmentListScreen(1);
                continue;
            }
        }

        pauseScr ("Invalid input. Please enter a valid option.", 1);

    } while (1);
}

void userProfileScreen(User* usr, int usr_type, const int edit_mode_flg) {
    const int MAX_COL_SZ = 18 + SCR_PADDING;  //max size for the field names column
    
    int hmargin = 8, len;
    float avg;

    int*  list_sz_ptr = getDataListSz(NULL);
    void* list        = getDataList(NULL);
    char* dat_fn      = getDataFileName(NULL);

    if ((len = strlen(usr->Fname)) > hmargin)
        hmargin = len;
    if ((len = strlen(usr->Lname)) > hmargin)
        hmargin = len;
    if ((len = strlen(usr->Addr)) > hmargin)
        hmargin = len;

    hmargin = (SCR_SIZE - MAX_COL_SZ - hmargin) / 2;

    if (usr_type <= 0) usr_type = CURRENT_USR_TYPE;

    displayScreenSubHdr(getTitle(NULL, usr_type, " PROFILE"));

    if (!refreshData(list, list_sz_ptr, dat_fn, 1, 1))
        return;

    printScrTitle(NULL, "----- PERSONAL INFORMATION -----", "\n\n");

    printScrHMargin(hmargin);
    printScrColText("First Name:", MAX_COL_SZ, NULL);
    printScrColText(usr->Fname, 0, "\n");
    printScrHMargin(hmargin);
    printScrColText("Last Name:", MAX_COL_SZ, NULL);
    printScrColText(vsan(usr->Lname), 0, "\n");
    printScrHMargin(hmargin);
    printScrColText("Address:", MAX_COL_SZ, NULL);
    printScrColText(vsan(usr->Addr), 0, "\n");
    printScrHMargin(hmargin);
    printScrColText("Date of Birth:", MAX_COL_SZ, NULL);
    printScrColText(usr->Dob, 0, "\n");
    printScrHMargin(hmargin);
    printScrColText("Age:", MAX_COL_SZ, NULL);
    printScrColVal(calculateAge(NULL, usr->Dob), 0, 0, "\n\n");

    printScrTitle(NULL, "----- COURSE INFORMATION -----", "\n\n");

    avg = calculateGradeAvg(usr_type==USR_PRINCIPAL? NULL: usr->loginID);

    if (usr_type == USR_STUDENT) 
    {
        printScrHMargin(hmargin);
        printScrColText("Total Subjects:", MAX_COL_SZ, NULL);
        printScrColVal(calculateTally(usr->loginID, usr_type, NULL), 0, 0, "\n");
        printScrHMargin(hmargin);
        printScrColText("Total Teachers:", MAX_COL_SZ, NULL);
        printScrColVal(calculateTally(usr->loginID, usr_type, USR_TEACHER), 0, 0, "\n");  
        printScrHMargin(hmargin);
        printScrColText("Average Grade:", MAX_COL_SZ, NULL);
        if(avg < 0)
        printScrColText(UNSPEC_DATA, 0, "\n\n");   
        else 
        printScrColVal(avg, 0, 0, "%%\n\n"); 
    }
    else if (usr_type == USR_TEACHER) 
    {
        printScrHMargin(hmargin);
        printScrColText("Total Students:", MAX_COL_SZ, NULL);
        printScrColVal(calculateTally(usr->loginID, usr_type, USR_STUDENT), 0, 0, "\n"); 
        printScrHMargin(hmargin);
        printScrColText("Total Subjects:", MAX_COL_SZ, NULL);
        printScrColVal(calculateTally(usr->loginID, usr_type, NULL), 0, 0, "\n");
        printScrHMargin(hmargin);
        printScrColText("Student Avg Grade:", MAX_COL_SZ, NULL);
        if(avg < 0)
        printScrColText(UNSPEC_DATA, 0, "\n\n");   
        else 
        printScrColVal(avg, 0, 0, "%%\n\n"); 
    }
    else if (usr_type == USR_PRINCIPAL) 
    {
        printScrHMargin(hmargin);
        printScrColText("Total Subjects:", MAX_COL_SZ, NULL);
        printScrColVal(SUBJECTS_SZ, 0, 0, "\n");
        printScrHMargin(hmargin);
        printScrColText("Total Teachers:", MAX_COL_SZ, NULL);
        printScrColVal(TEACHERS_SZ, 0, 0, "\n");
        printScrHMargin(hmargin);
        printScrColText("Total Students:", MAX_COL_SZ, NULL);
        printScrColVal(getListEntryCnt(getDataList(usr_type), getDataListSz(usr_type)), 0, 0, "\n");
        printScrHMargin(hmargin);
        printScrColText("Student Avg Grade:", MAX_COL_SZ, NULL);
        if(avg < 0)
        printScrColText(UNSPEC_DATA, 0, "\n\n");   
        else 
        printScrColVal(avg, 0, 0, "%%\n\n"); 
    }

    printScrTitle(NULL, "----- ACCOUNT INFORMATION -----", "\n\n");

    printScrHMargin(hmargin);
    printScrColText("Session Duration:", MAX_COL_SZ, NULL);
    printScrColVal(usr->timeout, 0, 0, " minutes(s)\n\n");


    if (edit_mode_flg) {
        registerUserProfile(usr_type, CURRENT_USR->reg_stat, edit_mode_flg);
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