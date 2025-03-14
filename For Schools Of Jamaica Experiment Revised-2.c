#include "screenio.h"

#define APPLICATION_MODE DEVELOPER_MODE   // primarily used to toggle DEBUG_MODE 

// Application mode enumeration
#define DEVELOPER_MODE LG_MODE_CONSL
#define RELEASE_MODE LG_MODE_OFF

// Application resource properties
#define TTL_MAIN "FOR SCHOOLS OF JAMAICA"
#define DAT_MIN_SZ 1   // minimum allocatable capacity for a dynamic list
#define DAT_EXT_SZ 10  // fixed capacity increment to be applied when extending a dynamic list

// Screen display column sizes (measured in characters)
#define ITEM_NO_SZ 4   
#define SUBJ_TTL_SZ 30
#define GRADE_SZ 10
#define REG_STAT_SZ 10

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

// Log message type enumeration
#define FILE_UNREADABLE 1
#define FILE_UNWRITABLE 2
#define FILE_CORRUPT    3
// Other warn type enumeration
#define NO_ENTRY_ERROR  4
#define REG_ENTRY_ERROR 5
#define REFRESH_ERROR   6

// Data refresh file mode parameter enumeration
#define READ_WRITE 0
#define READ_ONLY  1
// Data refresh auth mode parameter enumeration
#define PUBLIC  0
#define SECURED 1

// Application log resources
#define APP_LOG_FILENAME        "AppLog.txt"
#define APP_LOG_TITLE           "'FOR SCHOOLS OF JAMAICA' Application Log"
// Log messages
#define MSG_ACTN_CONTD          "The current action may continue but render inconsistent results."
#define MSG_ACTN_ABORT          "The current action will be aborted."
#define MSG_SAVE_ERROR          "The current changes may not be preserved."
//Screen messages
#define MSG_INVALID_OPTION      "Invalid input. Please enter a valid option."
#define MSG_EMPTY_LIST          "No items available\n\n"

// Void data placeholders
#define UNSPEC_DATA "<NONE>"   // frontend view label shown in the place of values not yet assigned or set
#define BLANK_DATA "<BLANK>"   // backend data label used in place of string values that are empty 
// Data entry file resources
#define PRINCIPAL_DATA_FILENAME "Principal.txt"
#define TEACHER_DATA_FILENAME   "Teachers.txt"
#define STUDENT_DATA_FILENAME   "Students.txt"
#define ENROLL_DATA_FILENAME    "Enrollments.txt"
#define SUBJECT_DATA_FILENAME   "Subjects.txt"

// User type enumeration
#define USR_STUDENT   1
#define USR_TEACHER   2
#define USR_PRINCIPAL 3
// Other data list type enumeration
#define LST_ENROLL    4
#define LST_SUBJECT   5
#define LST_PTR       6

// User registration status enumeration
#define REG_STAT_PROF 0
#define REG_STAT_SUBJ 1
#define REG_STAT_FULL 2
// User actions enumeration
#define USR_ACTN_ADD  2
#define USR_ACTN_REM  3
#define USR_ACTN_EDIT 4

// User default timeout (in minutes)
#define TM_DEF_STUDENT   10
#define TM_DEF_TEACHER   15
#define TM_DEF_PRINCIPAL 30


struct ListEntry {
/* transient fields */
    int deleted_flg;
    int index;
/* persistent fields */    
    int ID;
}; 
typedef struct ListEntry Entry;

struct SubjectEntry {
    Entry entry;
    char title [SUBJ_TTL_SZ + 1];
}
DEF_SUBJECT = {TRUE}; // default non-active subject entry;

struct UserEntry {
    Entry entry;
    char Fname [FNAME_SZ + 1];
    char Lname [LNAME_SZ + 1];
    char Addr  [ADDR_SZ + 1];
    char Dob   [DOB_SZ + 1];
    int  timeout;   //measured in minutes
    int  reg_stat;
}
DEF_USER = {TRUE};    // default non-active user entry

struct EnrollEntry {
    Entry entry;
    int studentID;
    int teacherID;   
    float grade;
}
DEF_ENROLL = {TRUE};  // default non-active enrollment entry

typedef struct tm Date;
typedef struct UserEntry User;
typedef struct SubjectEntry Subject;
typedef struct EnrollEntry Enrollment;

// List entry type size definition caching
const int PTR_SZ = sizeof(void*);
const int CHAR_SZ = sizeof(char);
const int USER_SZ = sizeof(User);
const int ENROLL_SZ = sizeof(Enrollment);
const int SUBJECT_SZ = sizeof(Subject);

const int FULL_NAME_SZ = FNAME_SZ + LNAME_SZ + 1;

int subjectCapacity;
Subject *Subjects;

int enrollCapacity;
Enrollment *Enrollments;

int principalCapacity = 1, studentCapacity, teacherCapacity;
User *Principal, *Students, *Teachers;

int CURRENT_USR_TYPE;
User* CURRENT_USR;

// Function prototype declarations for functions whose invocations occur BEFORE their declaration.
// Note that this is only necessary for static functions or functions that return pointer types.
char *fsan(char*);
char *vsan(char* attr, const char* mask);
char *getDataFileName(int); 
int  *getDataListSzPtr(int);
void *getDataList(int);
void *setDataListSz(void*, int);
void *getEntry(int, void*, int);
void *setEntry(int, void*, int, void*);
FILE *refreshListData(int, const int, const int, const int);


/********************************************************************/
/***************** Convenience Auxilliary Functions *****************/
/********************************************************************/

int datSz(int list_sz) {
    return list_sz < DAT_MIN_SZ ? DAT_MIN_SZ : list_sz;
}

void* syncDataListSz(void *list, int *list_sz_ptr, FILE *fptr) {
    int data_sz;

    if (list_sz_ptr)
        data_sz = *list_sz_ptr;
    else
        return NULL;

    if (!freadInt(&data_sz, fptr)) 
        return NULL;

    if (data_sz != *list_sz_ptr) 
    {
        if (list = setDataListSz(list, data_sz))
            *list_sz_ptr = data_sz;
        else 
            return NULL; 
    }
    return list;
}

User *loadUserData(User *list, int *list_sz_ptr, FILE *fptr)  // NOTE: can produce partial loads upon failure
{
    if (list = syncDataListSz(list, list_sz_ptr, fptr))

    for (int i=0; i < *list_sz_ptr; i++) { 
        if (freadInt (&list[i].entry.ID, fptr)) {
            readChars (list[i].Fname, FNAME_SZ, fptr);
            readChars (list[i].Lname, LNAME_SZ, fptr);
            readChars (list[i].Addr , ADDR_SZ, fptr);
            readChars (list[i].Dob  , DOB_SZ,   fptr);
            freadInt  (&list[i].timeout, fptr);
            freadInt  (&list[i].reg_stat, fptr);
            list[i].entry.deleted_flg = FALSE;
            list[i].entry.index = i;
        } 
        else return NULL;   // assert parity between expected and actually loaded data
    }

    return list;
}

Enrollment *loadEnrollData(Enrollment *list, int *list_sz_ptr, FILE *fptr)  // NOTE: can produce partial loads upon failure 
{
    if (list = syncDataListSz(list, list_sz_ptr, fptr))

    for (int i=0; i < *list_sz_ptr; i++) {
        if (freadInt  (&list[i].entry.ID, fptr)) {
            freadInt  (&list[i].studentID, fptr);
            freadInt  (&list[i].teacherID, fptr);
            freadFloat (&list[i].grade, fptr);
            list[i].entry.deleted_flg = FALSE;
            list[i].entry.index = i;
        }
        else return NULL;   // assert parity between expected and actually loaded data
    }

    return list;
}

Subject *loadSubjectData(Subject *list, int *list_sz_ptr, FILE *fptr)  // NOTE: can produce partial loads upon failure 
{
    if (list = syncDataListSz(list, list_sz_ptr, fptr))

    for (int i=0; i < *list_sz_ptr; i++) {
        if (freadInt (&list[i].entry.ID, fptr)) {
            readChars (list[i].title, SUBJ_TTL_SZ, fptr);
            list[i].entry.deleted_flg = FALSE;
            list[i].entry.index = i;
        }
        else return NULL;   // assert parity between expected and actually loaded data
    }

    return list;
}

FILE *saveUserData(User *list, int list_sz, FILE *fwptr) 
{
    User* u;

    if (fprintf (fwptr, "%d\n", getListEntryCnt(list, list_sz)) <= 0)
        return NULL;

    for (int i = 0; i < list_sz; i++) {
        u = list + i;
        if (!u->entry.deleted_flg) {
            if (fprintf (fwptr, "%d\n%s\n%s\n%s\n%s\n%d\n%d\n", 
                         u->entry.ID, fsan(u->Fname), fsan(u->Lname), fsan(u->Addr), fsan(u->Dob), u->timeout, u->reg_stat) <= 0)
                return NULL;
        }
    } 
    return fwptr;
}

FILE *saveEnrollData(Enrollment *list, int list_sz, FILE *fwptr) 
{
    Enrollment* e;

    if (fprintf (fwptr, "%d\n", getListEntryCnt(list, list_sz)) <= 0)
        return NULL;

    for (int i = 0; i < list_sz; i++) {
        e = list + i;
        if (!e->entry.deleted_flg) {
            if (fprintf (fwptr, "%d\n%d\n%d\n%.2f\n", e->entry.ID, e->studentID, e->teacherID, e->grade) <= 0)
                return NULL;
        }
    }
    return fwptr;
}

FILE *saveSubjectData(Subject *list, int list_sz, FILE *fwptr) 
{
    Subject* subj;

    if (fprintf (fwptr, "%d\n", getListEntryCnt(list, list_sz)) <= 0)
        return NULL;

    for (int i = 0; i < list_sz; i++) {
        subj = list + i;
        if (!subj->entry.deleted_flg) {
            if (fprintf (fwptr, "%d\n%s\n", subj->entry.ID, subj->title) <= 0)
                return NULL;
        }
    }
    return fwptr;
}

FILE *stageListData(void *list, int *list_sz_ptr, const char *dat_fn, const int read_only_flg) { // used to initiate a save session
    void* ptr;

    FILE* fptr = fopen (dat_fn, "r");
    if (fptr) 
    {
        if (list == getDataList(LST_ENROLL)) {
            ptr = loadEnrollData((Enrollment*)list, list_sz_ptr, fptr);
        } else
        if (list == getDataList(LST_SUBJECT)) {
            ptr = loadSubjectData((Subject*)list, list_sz_ptr, fptr);
        } else {
            ptr = loadUserData((User*)list, list_sz_ptr, fptr);
        }
        fclose(fptr); 

        if (!ptr) {
            fptr = NULL;
        }
        if (!read_only_flg && ptr) {
            fptr = fopen (dat_fn, "w");
        }
    }
    return fptr;
}

int commitListData(void *list, int list_sz, FILE *fwptr) { // used to conclude a save session
    void *ptr;
  
    if (list == getDataList(LST_ENROLL)) {
        ptr = saveEnrollData((Enrollment*)list, list_sz, fwptr);
    } else 
    if (list == getDataList(LST_SUBJECT)) {
        ptr = saveSubjectData((Subject*)list, list_sz, fwptr);
    } else {
        ptr = saveUserData((User*)list, list_sz, fwptr);
    }
    fclose(fwptr);

    return ptr != NULL;
}

int restoreListData(int lst_type, void *list, void *bkp_list, int bkp_sz) {
    int cnt = 0;
    Entry* e;

    for (int i = 0; i < bkp_sz; i++) 
    {
        if (e = getEntry(-lst_type, bkp_list, i)) {
            setEntry(lst_type, list, e->index, e);
        }
    }    
    return cnt;
}

int backupListData(int lst_type, void**list, int list_sz, void *bkp_list)
{

}

FILE *reloadListData(int lst_type, const int read_only_flg, const int scr_psd_mode)  // used to reload data in public domain context
{  
    int*  list_sz_ptr = getDataListSzPtr(lst_type);
    void* list        = getDataList(lst_type);
    char* dat_fn      = getDataFileName(lst_type);

    FILE* fptr = stageListData(list, list_sz_ptr, dat_fn, read_only_flg);  
    if (!fptr) {
        sys_err (read_only_flg? NULL: LVL_FATAL, read_only_flg? MSG_ACTN_CONTD: MSG_ACTN_ABORT, scr_psd_mode, FALSE);
    } 
    return fptr;
}

int persistListData(int lst_type, FILE *fwptr, const int scr_psd_mode) 
{
    void* list = getDataList(lst_type);

    if (!(list && commitListData(list, getDataListSz(lst_type, FALSE), fwptr))) {
        sys_err (NULL, MSG_SAVE_ERROR, scr_psd_mode, FALSE);
        return FALSE;
    }
    return TRUE;
}


char* getRegStatDesc(int reg_stat_code, const int short_desc_flg) {
    switch (reg_stat_code) {
        case REG_STAT_PROF:
            return short_desc_flg? "CREATED":"Pending Profile";
        case REG_STAT_SUBJ:
            return short_desc_flg? "PROFILED":"Pending Enrollment";
        case REG_STAT_FULL:
            return short_desc_flg? "FULL":"Completed";
        default:
            return NULL;
    }
}

char* getFullName(User* usr) {
    if (usr) {
        static char full_name [FNAME_SZ + LNAME_SZ + 2];

        strcpy(full_name, vsan(usr->Fname, ""));
        strcat(full_name, " ");
        strcat(full_name, vsan(usr->Lname, ""));  

        return full_name;         
    }
    return NULL;
}

char* getTitle(const char* pre_ttl, int lst_type, const char* pst_ttl) {

    static char* title;

    if (!title)
        title = malloc(SCR_SIZE * CHAR_SZ);

    strcpy(title, "");  // clears any previously set title
    
    if (pre_ttl)
        strcat(title, pre_ttl);

    if (!lst_type) lst_type = CURRENT_USR_TYPE;

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

char* getDataFileName(int lst_type) {    //NOTE: references global application list resources

    if (!lst_type) lst_type = CURRENT_USR_TYPE;

    switch (lst_type) {
        case USR_STUDENT:
            return STUDENT_DATA_FILENAME;
        case USR_TEACHER:
            return TEACHER_DATA_FILENAME;
        case USR_PRINCIPAL:
            return PRINCIPAL_DATA_FILENAME; 
        case LST_ENROLL:
           return ENROLL_DATA_FILENAME;
        case LST_SUBJECT:
           return SUBJECT_DATA_FILENAME;
        default:
           return NULL;        
    }
}

void* getDataList(int lst_type) {    //NOTE: references global application list resources

    if (!lst_type) lst_type = CURRENT_USR_TYPE;

    switch (lst_type) {
        case USR_STUDENT:
            return Students;
        case USR_TEACHER:
            return Teachers;
        case USR_PRINCIPAL:
            return Principal;
        case LST_ENROLL:
            return Enrollments;  
        case LST_SUBJECT:
            return Subjects;    
        default:
            return NULL;        
    }
}

int* getDataListSzPtr(int lst_type) {    //NOTE: references global application data list resources
    
    if (!lst_type) lst_type = CURRENT_USR_TYPE;

    switch (lst_type) {
        case USR_STUDENT:
            return &studentCapacity;
        case USR_TEACHER:
            return &teacherCapacity;
        case USR_PRINCIPAL:
            return &principalCapacity;
        case LST_ENROLL:
            return &enrollCapacity;
        case LST_SUBJECT:
            return &subjectCapacity; 
        default:
            return NULL;            
    }
}

int getDataListSz(int lst_type, const int dat_sz_flg) {
    int* list_sz_ptr = getDataListSzPtr(lst_type);
    int  list_sz     = list_sz_ptr? *list_sz_ptr:- 1; 
    
    return dat_sz_flg? datSz(list_sz): list_sz;
}

int getDataListCnt(int lst_type) {
    int* list_sz_ptr = getDataListSzPtr(lst_type);
    if (!list_sz_ptr) {
        return -1;
    }
    return getListEntryCnt (getDataList(lst_type), *list_sz_ptr);
}

int getListEntryCnt(void *list, int list_sz) {
    int cnt = 0;
    int lst_type;
    Entry* e;

    // determine data list type of list
    if (list == getDataList(LST_ENROLL)) {
        lst_type = LST_ENROLL;  
    } 
    else 
    if (list == getDataList(LST_SUBJECT)) {
        lst_type = LST_SUBJECT;
    }
    else {
        lst_type = -1;      // turn off global entry type checking for user lists
    }

    for (int i = 0; i < list_sz; i++) {
        e = getEntry(lst_type, list, i);
        if (!e->deleted_flg)
            cnt++;
    }   
    return cnt;
}

void* setDataListSz(void *list, int new_sz) {    //NOTE: references global application data list resources
    void* new_list;
    int data_sz;

    // determine the entry size based on the supported list type
    if (list == NULL) {
        return list;
    } else if (list == Enrollments) {
        data_sz = ENROLL_SZ;
    } else if (list == Subjects) {
        data_sz = SUBJECT_SZ;
    } else {
        data_sz = USER_SZ;
    }

    new_list = realloc(list, datSz(new_sz) * data_sz);

    if (new_list) {
        // determine the list to set based on the supported list type
        if (list == Enrollments) {
            return Enrollments = new_list;
        } else if (list == Subjects) {
            return Subjects = new_list;
        } else if (list == Students) {
            return Students = new_list;
        } else if (list == Teachers) {
            return Teachers = new_list;
        }
    }
    return NULL;
}

void reset(void *list, int lst_type, int offset, int count) 
{
    void* def_value;

    switch (lst_type) {
        case LST_PTR:
            def_value = NULL;
            break;
        case LST_ENROLL:
            def_value = &DEF_ENROLL;
            break;
        case LST_SUBJECT:
            def_value = &DEF_SUBJECT;
            break;
        default:
            def_value = &DEF_USER;
    }

    for (int i=offset; i < offset + count; i++) {
        setEntry(-lst_type, list, i, def_value);
    }
}

void* extDataList(int lst_type) 
{
    void* list = getDataList(lst_type);
    if   (list) 
    {
        int* list_sz_ptr = getDataListSzPtr(lst_type);

        if (list = setDataListSz(list, *list_sz_ptr + DAT_EXT_SZ)) {
            reset (list, lst_type, *list_sz_ptr, DAT_EXT_SZ);   // initializes the list
            (*list_sz_ptr) += DAT_EXT_SZ;
        }
    }
    return list;
}

void* initDataList(int lst_type)    //NOTE: references global application data list resources
{
    int*  list_sz_ptr = getDataListSzPtr(lst_type);
    void* list        = getDataList(lst_type);

    if  (list_sz_ptr && !list) 
    {
        switch (lst_type) {
            case LST_ENROLL:
                return Enrollments = calloc(DAT_MIN_SZ, ENROLL_SZ);
            case LST_SUBJECT:
                return Subjects = calloc(DAT_MIN_SZ, SUBJECT_SZ);  
            case USR_STUDENT:
                return Students = calloc(DAT_MIN_SZ, USER_SZ);
            case USR_TEACHER:
                return Teachers = calloc(DAT_MIN_SZ, USER_SZ);  
             case USR_PRINCIPAL:
                return Principal = calloc(DAT_MIN_SZ, USER_SZ);   
        }
    }
    return list; 
}

User* initUser(User *usr, int usr_type, int loginID) 
{
    if (usr && loginID > 0) {    
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
                break;
            default:
                return NULL;
        }

       *usr = DEF_USER;  // resets entity to a freshly initialized user
        usr -> entry.ID = loginID;
        usr -> timeout  = tm_out;
        usr -> entry.deleted_flg = FALSE; 

        return usr;
    }
    return NULL;
}

Enrollment* initEnroll(Enrollment *enroll, int subjID, int studID, int tchrID) 
{
    if (enroll && subjID > 0 && studID > 0 && tchrID > 0) 
    {
       *enroll = DEF_ENROLL;  // resets entity to a freshly initialized enrollment
        enroll -> entry.ID  = subjID;
        enroll -> studentID = studID;
        enroll -> teacherID = tchrID;
        enroll -> grade     = -1;
        enroll -> entry.deleted_flg = FALSE;

        return enroll;
    }
    return NULL;
}

Subject* initSubject(Subject *subj, int ID, const char* title) 
{
    if (subj && ID > 0 && !isEmptyStr(title, TRUE, NULL)) 
    {
       *subj = DEF_SUBJECT;  // resets entity to a freshly initialized subject
        subj -> entry.ID = ID;
        strcpy(subj->title, title);
        subj -> entry.deleted_flg = FALSE;

        return subj;
    }
    return NULL;
}

int isValidEntry(Entry* entry) {
    return entry && !entry->deleted_flg && entry->ID > 0;
}

int isValidEnroll(Enrollment* enroll) {
    return isValidEntry(enroll) && enroll->studentID > 0 && enroll->teacherID > 0;
}

int isValidSubject(Subject* subj) {
    return isValidEntry(subj) && !isEmptyStr(subj->title, TRUE, NULL);
}

int isEntryType(int lst_type) {      // determies if lst_type is a valid global, application-defined list entry type
    return lst_type == LST_ENROLL || isSnglEntryType(lst_type);
}

int isSnglEntryType(int lst_type) {  // determies if lst_type is a valid single-ID list entry type
    return lst_type == LST_SUBJECT || isUsrType(lst_type);
}

int isUsrType(int lst_type) {
    switch (lst_type) {
        case USR_STUDENT:
        case USR_TEACHER:
        case USR_PRINCIPAL:
            return TRUE;
        default:
            return !lst_type;   // current user type
    }
}

void* getEntry(int lst_type, void* list, int index) {
    return setEntry(lst_type, list, index, NULL);
}

void* setEntry(int lst_type, void* list, int index, void* entry) 
{   // when lst_type is -ve, given list is not treated as a global data list
    // i.e. global is not used if list is NULL and index is not checked for global list size exceed violation

    if (index < 0)
        return NULL;    
                                
    if (lst_type < 0) {           
        lst_type = abs(lst_type);
    } 
    else if (index >= getDataListSz(lst_type, FALSE)) {
        return NULL;
    } 
    else if (!list) {
        list = getDataList(lst_type);
    }

    if (!list)
        return NULL;

    void* lst_entry;
    int entry_flg = TRUE;


    if (lst_type == LST_PTR) 
    {
        entry_flg = FALSE;
        lst_entry = (void**) list + index;
        *(void**) lst_entry = entry;
    }
    else
    if (lst_type == LST_ENROLL) 
    {
        lst_entry = (Enrollment*) list + index;
        if (entry)
          *(Enrollment*) lst_entry = *(Enrollment*) entry;
    }
    else
    if (lst_type == LST_SUBJECT) 
    {
        lst_entry = (Subject*) list + index;
        if (entry)
          *(Subject*) lst_entry = *(Subject*) entry;
    }    
    else
    if (isUsrType(lst_type))
    {
        lst_entry = (User*) list + index;
        if (entry)
          *(User*) lst_entry = *(User*) entry;
    }
    else
    {
        entry_flg = FALSE;
    }

    if (entry && entry_flg) {
        ((Entry*) lst_entry)->index = index;
    }

    return lst_entry;
}

Entry* addListEntry(int lst_type, void *entry) {
    // handle null reference
    if (!(entry && isEntryType(lst_type))) {
        return entry;
    }

    // handle invalid data list type
    void*list = getDataList(lst_type);
    if (!list) {
        return NULL;
    }

    // add user to available tombstoned slot (if any)
    int list_sz = getDataListSz(lst_type, FALSE);
    int i; Entry* e;
    for (i = 0; i < list_sz; i++) 
    {    
        e = getEntry(lst_type, list, i);
        if (e->deleted_flg) {
            return setEntry(lst_type, list, i, entry);
        }
    }

    // increase slot capacity and add user at the end
    if (list = extDataList(lst_type)) {
        return setEntry(lst_type, list, i, entry);
    }

    return NULL;
}

void deleteListEntry(Entry *entry) {
    if (entry) { 
         entry->deleted_flg = TRUE;
    } 
}

Entry* getListEntry(int lst_type, int entryID, int offset)  // supports only single-ID entry list types
{
    void* list  = getDataList(lst_type);
    int list_sz = getDataListSz(lst_type, FALSE);

    if (list && isSnglEntryType(lst_type)) {
        Entry* e;

        for (int i = offset<0? 0:offset; i < list_sz; i++) 
        {
            e = getEntry(lst_type, list, i);

            if ((!entryID || entryID == e->ID) && (entryID >= 0 && !e->deleted_flg || entryID < 0 && e->deleted_flg)) {
                return e;
            }
        }
    }   
    return NULL;
}

int userSearch(int loginID, int usr_type, int offset, User**res_list, int res_limit) 
{ 
    int entry_cnt = 0;

    if (isUsrType(usr_type)) 
    {
        offset = abs(offset);

        if (usr_type >= 0) {

            return getListEntry(loginID, usr_type, offset);
        }

          User* usr;  // alternative search mode where all user lists are scanned instead of a single target list

    if (usr = getListEntry(loginID, USR_PRINCIPAL, offset))
        return usr;

    if (usr = getListEntry(loginID, USR_TEACHER, offset))
        return usr;

    return getListEntry(loginID, USR_STUDENT, offset);
    } 

    return entry_cnt;
} 

int getEnrollEntryID(int usr_type, Enrollment* e) 
{
    if (!e || usr_type >= 0 && e->entry.deleted_flg || usr_type < 0 && !e->entry.deleted_flg)
        return 0;

    switch (abs(usr_type)) {
        case USR_STUDENT:
            return e->studentID;
        case USR_TEACHER:
            return e->teacherID;
        default:
            return e->entry.ID;     
    }
}

Enrollment* getEnrollEntry(int entryID, int usr_type, Enrollment* list, int list_sz) {
    Enrollment* e;
    int _active = entryID < 0 ? -1:1;

    for (int i=0; i < list_sz; i++) {
        e = list + i;
        if (entryID == getEnrollEntryID(_active * usr_type, e)) {
            return e;
        }
    }
    return NULL;
}

Enrollment* getEnrollPtr(int entryID, int usr_type, Enrollment**list, int list_sz) {
    Enrollment* e;
    int _active = entryID < 0 ? -1:1;

    for (int i=0; i < list_sz; i++) {
        e = list[i];
        if (entryID == getEnrollEntryID(_active * usr_type, e)) {
            return e;
        }
    }
    return NULL;
}

int enrollSearch(int entryID, int utyp_sbjID, int tgtyp_stdID, int tchrID, int offset, Enrollment**res_list, int res_limit) 
{ 
    /**
     * This function supports two major search modes: 
     * 
     * 1. When entryID is NOT zero(0), searches are based on the given entryID, its user type, and a target user type 
     *    that enforces non-duplication of its values in the result list; the tchrID parameter is ignored.
     *    For a +ve entryID, only matching ACTIVE entries are returned.
     *    For a -ve entryID, only matching DELETED entries are returned.
     * 
     * 2. When entryID is zero, searches are narrowed by the specified _ID parameter triplet and only active (i.e not deleted) 
     *    entries are returned. 
     *    Note that zero(0) or -ve ID values will be ommitted from the search.
     * 
     * If res_limit is zero(0) or -ve, searches are not paged and will return full results, beginning at the given offset.
     * The function returns a +ve count of entries found, except in the case where the number of matching entries 
     * in the search list exceeds res_limit. In this case, the index of the next matching entry in the search list, 
     * after the last entry populated in the result list, is returned as a -ve value. 
     * The offset parameter, which also supports -ve indexing, may be used in tandem with res_limit to page search results.
     * 
     * WARNING: If a +ve res_list is provided, it should not exceed the actual capacity of res_list.
     */

    const int kc_srch_flg = !entryID;        //determines if function should operate in key combo or default search mode
    const int _active = entryID < 0 ? -1:1;  //user type co-efficient that determines if function will search for active or deleted entries

    int usr_type = utyp_sbjID;
    int subjID   = utyp_sbjID;
     
    int tgt_usr_type = tgtyp_stdID;
    int studID       = tgtyp_stdID;

    Enrollment* e;
    int entry_id, entry_cnt = 0;

    Enrollment*enrolls = getDataList(LST_ENROLL);
    int enrolls_sz     = getEnrollListSz();

    if (res_limit <= 0) {
        res_limit = enrolls_sz;
    }

    if (!(res_list || kc_srch_flg)) 
    {
        int enrolls_cnt = getListEntryCnt(enrolls, enrolls_sz);

        if (entryID < 0) {
            enrolls_cnt = enrolls_sz - enrolls_cnt;
        }
        if (res_limit > enrolls_cnt) {
            res_limit = enrolls_cnt;
        }
        if (res_limit > 1) {
            res_list = calloc(res_limit, PTR_SZ);  //provide local buffer list 
        }
    }

    for (int i = abs(offset), match_flg = FALSE; i < enrolls_sz; i++, match_flg = FALSE) {
        e = enrolls + i;

        if (kc_srch_flg) 
        {
            if (!e->entry.deleted_flg && (subjID <= 0 || subjID == e->entry.ID) && (studID <= 0 || studID == e->studentID) && (tchrID <= 0 || tchrID == e->teacherID)) {    
                match_flg = TRUE;
            }
        }
        else
        {
            if (entryID == getEnrollEntryID(_active * usr_type, e)) 
            {
                entry_id = getEnrollEntryID(_active * tgt_usr_type, e);

                // prevents duplicate addition of enrollment entries
                if (!getEnrollPtr(_active * entry_id, tgt_usr_type, res_list, entry_cnt)) {
                    match_flg = TRUE;
                }
            }
        }

        if (match_flg)
        if (entry_cnt < res_limit) {
            res_list[entry_cnt++] = e;

            if (kc_srch_flg && subjID > 0 && studID > 0 && tchrID > 0)  // unique entry search short-circuit optimization
                break;
        } else
            return -i;  //search results exceed specified limit
    }

    return entry_cnt;
}

int getSubjects(int entryID, int usr_type, int* subj_buf, const int avl_subj_flg)  // gets enrolled/available subjects for user
{  
    int subj_total = 0;
    int subjects_sz = getSubjectListSz();

    if (subjects_sz) {
        int total = 0;
        int enrolls_sz  = getEnrollListSz();
        Enrollment** enrolls_ptr;

        if (enrolls_sz) {
            enrolls_ptr = malloc(subjects_sz * PTR_SZ);
            total = enrollSearch(entryID, usr_type, NULL, -1, 0, enrolls_ptr, subjects_sz); 
        }

        if (!avl_subj_flg) 
        {
            for (int i=0; i < total; i++) {   
                if (subj_buf) { 
                    subj_buf [i] = enrolls_ptr[i]->entry.index;   // populate subj_buf with entry indexes from global enrollment list 
                }
            }

            subj_total = total;
        }
        else
        {
            Subject* subjects = getDataList(LST_SUBJECT);
            Subject* subj;

            for (int i=0; i < subjects_sz; i++) {   
                subj = subjects + i;

                if (!(subj->entry.deleted_flg || total && getEnrollPtr(subj->entry.ID, NULL, enrolls_ptr, total))) {
                    if (subj_buf) { 
                        subj_buf [subj_total++] = subj->entry.index;   // populate subj_buf with entry indexes from global subject list 
                    }
                }
            }
        }
    }

    return subj_total;
}

int calculateTally(int entryID, int usr_type, int tgt_usr_type) {
    return enrollSearch(entryID, usr_type, tgt_usr_type, -1, 0, NULL, 0);
}

float calculateAvgGrade(int entryID, int usr_type) {
    float sum = 0;
    int count = 0;
    Enrollment* e;

    Enrollment* enrolls = getDataList(LST_ENROLL);
    int enrolls_sz      = getEnrollListSz();

    for (int i=0; i < enrolls_sz; i++) {
        e = enrolls + i;
        
        if (!e->entry.deleted_flg && e->grade >= 0 && (entryID <= 0 || entryID == getEnrollEntryID(usr_type, e))) {
            sum += e->grade;
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
    if (!tm_dob) 
    {
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
    if (!dstr) return FALSE;

    int date_len = 3;
    int date_buf [date_len];

    //copy date to editable string
    int str_len = strlen(dstr);
    char* dt = malloc((str_len + 1) *CHAR_SZ);
    char c;

    for (int i=0; i < str_len; i++) {
        c = dstr[i];
        if (!(isdigit(c) || c == '/') )
            return FALSE;
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
        return FALSE;   
    }

    //check date format semantics
    int dy = date_buf [0];
    int mn = date_buf [1];
    int yr = date_buf [2];

    if (dy > 31 || mn > 12)
        return FALSE;

    if (dy == 31 && (mn == APR || mn == JUN || mn == SEP || mn == NOV))
        return FALSE;

    if (dy > (isLeapYear(yr)? 29:28) && mn == FEB) 
        return FALSE;
    
    if (dptr) {
        //use local time settings to properly initialize date
        time_t curr_tm = time(NULL);
        *dptr = *localtime(&curr_tm);

        dptr->tm_mday = dy;
        dptr->tm_mon  = mn - 1;
        dptr->tm_year = yr - 1900;
    }
    return TRUE;
}

int hashID(int loginID) {
    const int SEED  = -579;
    const int SALT  = 10000;
    const int PRIME = 7;

    return (SEED + loginID) * PRIME + SALT;
}


////////////// Application Convenience Functions Wrappers //////////////

char *fsan(char* attr) { // sanitize blank user data attribute for file storage
    return isEmptyStr(attr, FALSE, NULL) ? BLANK_DATA : attr;
}

char *vsan(char* attr, const char* mask) { // sanitize blank user data attribute for on-screen viewing
    return isEmptyStr(attr, FALSE, NULL) || !strcmp(attr, BLANK_DATA)? mask? mask:UNSPEC_DATA : attr;
}

int skpdtl(char *input) {
    return isEmptyStr(input, FALSE, NULL);
}

void printMargin() {
    printScrMargin (0);
}

void printTopic(char* caption) {
    printScrTopic (caption, -1, "-", 0, TRUE);
}

int print2ColTblHdr(const char* col_txt1, int col_sz1, const char* col_txt2, int col_sz2) 
{
    return print4ColTblHdr (col_txt1, col_sz1, col_txt2, col_sz2, NULL, 0, NULL, 0);
}

int print4ColTblHdr(const char* col_txt1, int col_sz1, const char* col_txt2, int col_sz2, 
                     const char* col_txt3, int col_sz3, const char* col_txt4, int col_sz4) 
{
    return print6ColTblHdr (col_txt1, col_sz1, col_txt2, col_sz2, col_txt3, col_sz3, col_txt4, col_sz4, NULL, 0, NULL, 0);
}

int print6ColTblHdr(const char* col_txt1, int col_sz1, const char* col_txt2, int col_sz2, 
                     const char* col_txt3, int col_sz3, const char* col_txt4, int col_sz4, 
                     const char* col_txt5, int col_sz5, const char* col_txt6, int col_sz6) 
{
    return printScrHeader (col_txt1, col_sz1, col_txt2, col_sz2, col_txt3, col_sz3, col_txt4, col_sz4, col_txt5, col_sz5, col_txt6, col_sz6, "=", -1);
}

printGrade(float grade, const char* pst_txt) 
{
    if(grade < 0) {
        printScrColText(UNSPEC_DATA, 0, NULL);   
    } else {
        printScrColVal(grade, 0, 1, NULL);
    }
    if (pst_txt) {
        printf(pst_txt); 
    }
}

int initLogFile() 
{    
    FILE* fptr  = fopen(APP_LOG_FILENAME, "r");
    
    if (fptr) {  // log file already created
        fclose(fptr);
        return 0;
    } else {  // create and prep log file
        int fresult = scrLogInit(NULL);

        if (fresult == 0) {
            fresult = -1;
        }
        return fresult;
    }  
}

void warn(int msg_type, char* msg_arg, char* msg_pst, const int cntd_lg_flg) 
{
    char msg[SCR_SIZE]; msg[0] = '\0';   // properly initialize declared char array

    switch(msg_type) {
        case FILE_UNREADABLE:
            sprintf(msg, "%s does not exist or cannot be opened. ", msg_arg);
            break;
        case FILE_UNWRITABLE:
            sprintf(msg, "%s either cannot be created or cannot be written to. ", msg_arg);
            break;
        case FILE_CORRUPT:
            sprintf(msg, "%s is empty, corrupted or cannot be read. ", msg_arg);
            break;
        case NO_ENTRY_ERROR:
            sprintf(msg, "Cannot find the entry %s: ", msg_arg);
            break;
        case REG_ENTRY_ERROR:
            sprintf(msg, "Could not register entry %s: ", msg_arg);
            break;
        case REFRESH_ERROR:
            sprintf(msg, "Unable to load %s data: An error occurred while refreshing the list. ", msg_arg);
            break;
    }

    if (msg_pst) {
        strcat(msg, msg_pst);
    }

    scrWrn(NULL, msg, cntd_lg_flg);
}

void sys_err(char* alt_level, char* msg_pst, const int scr_psd_mode, const int cntd_lg_flg) 
{
    char msg[SCR_SIZE];

    strcpy(msg, cntd_lg_flg? "":"An unexpected system error has occurred! ");
    if(msg_pst)
    strcat(msg, msg_pst);

    scrLog(LVL_ERROR, alt_level, LG_MODE_FILE, -1, cntd_lg_flg, msg);
    scrCnslErr(0, 0, alt_level, msg, scr_psd_mode, cntd_lg_flg);
}

void inform(int tMargin, int bMargin, char* msg, const int scr_psd_mode, const int cntd_lg_flg) 
{
    if ( !(cntd_lg_flg && isEmptyStr(msg, FALSE, NULL)) ) 
    {
        if (!msg) msg = "";
        scrLog(LVL_INFO, "INFO: ", LG_MODE_FILE, -1, cntd_lg_flg, msg);
        scrCnslInf(tMargin, bMargin, NULL, msg, scr_psd_mode, cntd_lg_flg);
    }
}

void dbug(char* msg, int bMargin, const int scr_psd_mode, const int cntd_lg_flg) {
    scrCnslDbg(0, bMargin, NULL, msg, scr_psd_mode, cntd_lg_flg);
}

int promptID(const int edit_mode_flg) {
    char pswd [PASSCODE_SZ + 1];

    promptLgn("\nEnter login ID: ", pswd, PASSCODE_SZ);

    if (edit_mode_flg && skpdtl(pswd))
        return -1;
    if (strlen(pswd) != PASSCODE_SZ)
        return 0;

    int loginID = atoi(pswd);
    
    return (PASSCODE_MN <= loginID && loginID <= PASSCODE_MX)? hashID(loginID): 0;
}

int promptOptions(const char* msg, int* opt1, int* opt2) {
    float val;
    int result = promptOptVal(msg, opt1, &val, TRUE);

    if (result > 0) {
        *opt2 = val;
    }
    return result;
}

int promptOptVal(const char* msg, int* opt, float* val, const int opt_strict_flg) {  // when opt_strict_flg is true, val is treated like another opt
    int   input_sz = ITEM_NO_SZ + GRADE_SZ + 1;
    int   input_len, i;
    char  input [input_sz];
    char* inptr; 

    promptLn(msg, input, input_sz);

    trim(input, -1, NULL, 0);

    if (!(input_len = strlen(input)))   // input is blank
        return -1;

    if ((*opt=atoi(input)) > 0 && ((i=strcspn(input, " ")) < input_len || (i=strcspn(input, "\t")) < input_len))  // extract values from input line
    {
        inptr = input + i + 1; 
        if (opt_strict_flg) {
            trim(inptr, input_len-i-1, NULL, 0);
        }
        return isDigitStr(inptr, FALSE, !opt_strict_flg) && ((*val = atof(inptr)) > 0 || !opt_strict_flg);
    }
    return 0;
}

FILE *reloadData(int lst_type, const int read_only_flg) {  
    return reloadListData(lst_type, read_only_flg, SCR_PSD_NO_PRMPT);
}

FILE *refreshData(int lst_type, const int read_only_flg, const int auth_mode_flg) {   
    return refreshListData(lst_type, read_only_flg, auth_mode_flg, SCR_PSD_NO_PRMPT); 
}

int persistData(int lst_type, FILE *fwptr) {
    return persistListData(lst_type, fwptr, SCR_PSD_NO_PRMPT);
}

int currentUsr() {
    return isValidEntry(CURRENT_USR);
}

int getEnrollListSz() {
    return getDataListSz(LST_ENROLL, FALSE);
}

int getSubjectListSz() {
    return getDataListSz(LST_SUBJECT, FALSE);
}

int getPrincipalListSz() {
    return getDataListSz(USR_PRINCIPAL, FALSE);
}

int getTeacherListSz() {
    return getDataListSz(USR_TEACHER, FALSE);
}

int getStudentListSz() {
    return getDataListSz(USR_STUDENT, FALSE);
}

int getUserListSz() {       // represents the cumulative capacity of all user lists in the application
    return getPrincipalListSz() + getTeacherListSz() + getStudentListSz();
}

int getSnglEntryListSz() {  // represents the cumulative capacity of all single entry lists in the application
    return getSubjectListSz() + getUserListSz();
}

Subject* getSubject(int subjectID) 
{
    return getListEntry(LST_SUBJECT, subjectID, 0);
}

User* getUser(int loginID, int usr_type) 
{
    if (!isUsrType(usr_type)) {
        return NULL;
    }
    return getListEntry(usr_type, loginID, 0);
}

User* registerUser(int usr_type, int loginID)
{
    static User new_usr;

    return addListEntry(usr_type, initUser(&new_usr, usr_type, loginID));
}

Subject* registerSubject(int subjectID, const char* title) 
{
    static Subject new_subj;

    return addListEntry(LST_SUBJECT, initSubject(&new_subj, subjectID, title));
}

Enrollment* registerEnrollment(Enrollment* new_enroll) 
{    
    return addListEntry(LST_ENROLL, new_enroll);
}

int submitEnrollData(Enrollment *enroll_buf, int enroll_cnt, const int auth_mode_flg, const int edit_mode_flg) 
{
    if (enroll_cnt > 0) {

        FILE* fwptr = refreshData(LST_ENROLL, READ_WRITE, auth_mode_flg);  // begin mod session 

        if (!fwptr) return -1;  // abort mod session

        int edt_err_flg, edt_err_cnt = 0;
        char entry_key [12];
        Enrollment *e, *enrolls_ptr [1];

        for (int i = 0; i < enroll_cnt; i++) 
        {
            e = enroll_buf + i;

            edt_err_flg = -TRUE;

            if (edit_mode_flg) 
            {
                if (enrollSearch(NULL, e->entry.ID, e->studentID, e->teacherID, 0, enrolls_ptr, 1)) {
                    **enrolls_ptr = *e; edt_err_flg = FALSE;
                }
            } 
            else 
                edt_err_flg = !registerEnrollment(e);
            
            if (edt_err_flg) {
                edt_err_cnt++;
                
                strcpy(entry_key, e->entry.ID);
                strcat(entry_key, "+");
                strcat(entry_key, e->studentID);

                warn (edit_mode_flg? 
                      NO_ENTRY_ERROR:REG_ENTRY_ERROR, 
                      entry_key, 
                      edit_mode_flg? 
                      "The enrollment may have been altered from an external source.":"An unknown application error has occurred.", 
                      FALSE);
            } 
        }

        if (!persistData(LST_ENROLL, fwptr))  // end mod session
            return -1;

        enroll_cnt -= edt_err_cnt;
    }

    return enroll_cnt < 0 ? 0 : enroll_cnt;
}


/*******************************************************************/
/******************** Screen Navigation Functions ******************/
/*******************************************************************/

prtEntry(Entry *e) {
printf("\nAs Entry: %d %d %d\n", e->deleted_flg, e->index, e->ID);   
}

prtUsr(User *u) {
printf("Usr: %d %d %d %s %s %s %s %d %d", u->entry.deleted_flg, u->entry.index, u->entry.ID, u->Fname, u->Lname, u->Addr, u->Dob, u->timeout, u->reg_stat);
prtEntry(u);
}

prtSub(Subject *s) {
printf("Subj: %d %d %d %s", s->entry.deleted_flg, s->entry.index, s->entry.ID, s->title);
prtEntry(s);
}

prtEnroll(Enrollment *e) {
printf("Enroll: %d %d %d %d %d", e->entry.deleted_flg, e->entry.index, e->entry.ID, e->studentID, e->teacherID);
prtEntry(e);
}

prtLst0(void* list, int list_sz) {
    printf("Lst Capacity: %d\n", list_sz);
    for (int i=0; i<list_sz; i++) 
    if (list == Enrollments)
    prtEnroll(((Enrollment*)list)+i);
    else if (list == Subjects)
    prtSub(((Subject*)list)+i);
    else
    prtUsr(((User*)list)+i);
    printf("\n");
}

prtLst(char* ttl, int lst_type) {
    printf(ttl);
    printf(getTitle("", lst_type, ":\n"));
    prtLst0(getDataList(lst_type), getDataListSz(lst_type, FALSE));
    pauseScr(NULL,1);
}

prtAll(char* ttl) {
prtLst(ttl, LST_SUBJECT);
prtLst(ttl, USR_PRINCIPAL);
prtLst(ttl, USR_TEACHER);
prtLst(ttl, USR_STUDENT);
prtLst(ttl, LST_ENROLL);   
}

int main() {
    initSystem();
    initDefaultUsers(); 
    loadSchoolData();
    mainscreen();
    return 0;
}

void initSystem()
{
    // configure log framework for application logging
    SYS_LG_FILE = APP_LOG_FILENAME;  
    SYS_LG_TITLE = APP_LOG_TITLE;
    WARN_MODE = ERROR_MODE = LG_MODE_ALL;
    DEBUG_MODE = APPLICATION_MODE;
    LOG_NULL_VALUE = "{null}";

    if (initLogFile() < 0) {
        warn(FILE_UNWRITABLE, APP_LOG_FILENAME, "Application file logging will not be performed.", FALSE);
    }

    // initialize system resources
    User* prin          = initDataList(USR_PRINCIPAL);
    User* tchrs         = initDataList(USR_TEACHER);
    User* studs         = initDataList(USR_STUDENT);
    Subject* subjs      = initDataList(LST_SUBJECT);
    Enrollment* enrolls = initDataList(LST_ENROLL);

    if (!(prin && tchrs && studs && subjs && enrolls)) {
        sys_err(LVL_FATAL, "One or more critical system components could not be initialized. ", SCR_PSD_OFF_PRMPT, FALSE);
        inform(2, 1, "The application will now exit...", SCR_PSD_NO_PRMPT, TRUE);
        exit(1);
    }
}

void initDefaultUsers() 
{
    // Initialize default subjects 
    registerSubject(1, "Mathematics");
    registerSubject(2, "English");
    registerSubject(3, "Spanish");
    registerSubject(4, "Computing");
    registerSubject(5, "Biology");
    registerSubject(6, "Physics");
    
    // Initialize default users
    User* usr = initUser(getDataList(USR_PRINCIPAL), USR_PRINCIPAL, hashID(1000));
    strcpy(usr->Fname, "Paul");
    strcpy(usr->Lname, "Duncanson");
    strcpy(usr->Addr , "Portsmouth, Lesser Portmore");
    strcpy(usr->Dob  , "23/05/1975");
    usr->reg_stat = REG_STAT_FULL;

    usr = registerUser(USR_TEACHER, hashID(1100));
    strcpy(usr->Fname, "Grace");
    strcpy(usr->Lname, "Peters");
    strcpy(usr->Addr , "12 Jones Lane, Kingstin");
    strcpy(usr->Dob  , "23/05/1995");
    usr->reg_stat = REG_STAT_FULL;

    usr = registerUser(USR_TEACHER, hashID(1200));
    strcpy(usr->Fname, "Kim");
    strcpy(usr->Lname, "Long");
    strcpy(usr->Addr , "8 Lord's Way, Sandy Gully, Clarendon");
    strcpy(usr->Dob  , "23/07/1995");
    usr->reg_stat = REG_STAT_FULL;
/*
    usr = registerUser(USR_TEACHER, hashID(1300));
    strcpy(usr->Fname, "Mercy");
    strcpy(usr->Lname, "James");
    strcpy(usr->Addr , "Lot A3, Norbrook, Apt 6, Kingston");
    strcpy(usr->Dob  , "17/04/1986");
    usr->timeout =  TM_DEF_TEACHER;
    usr->reg_stat = REG_STAT_FULL;

    usr = registerUser(USR_TEACHER, hashID(1400));
    strcpy(usr->Fname, "John");
    strcpy(usr->Lname, "Jackson");
    strcpy(usr->Addr , "Grove Lane, Jacks Hill, St. Andrew");
    strcpy(usr->Dob  , "17/04/1987");
    usr->reg_stat = REG_STAT_FULL;

    usr = registerUser(USR_TEACHER, hashID(1500));
    strcpy(usr->Fname, "Paul");
    strcpy(usr->Lname, "Wright");
    strcpy(usr->Addr , "3 Sand Lane, Annotto Bay, St. Mary");
    strcpy(usr->Dob  , "17/01/2001");
    usr->reg_stat = REG_STAT_FULL;

    usr = registerUser(USR_TEACHER, hashID(1600));
    strcpy(usr->Fname, "Kerry");
    strcpy(usr->Lname, "Truman");
    strcpy(usr->Addr , "Lot 853, 8 West, Greater Portmore, St. Catherine");
    strcpy(usr->Dob  , "05/12/1991");
    usr->reg_stat = REG_STAT_FULL;*/
}

void *loadAppData(int lst_type) 
{
    int*  list_sz_ptr = getDataListSzPtr(lst_type);
    void* list        = getDataList(lst_type);
    char* dat_fn      = getDataFileName(lst_type);

    void *ptr  = NULL;
    FILE *fptr = fopen (dat_fn, "r");

    if (fptr) 
    {
        if (lst_type == LST_ENROLL) {
            ptr = loadEnrollData((Enrollment*) list, list_sz_ptr, fptr);
        } else if (lst_type == LST_SUBJECT) {
            ptr = loadSubjectData((Subject*) list, list_sz_ptr, fptr);
        } else {
            ptr = loadUserData((User*) list, list_sz_ptr, fptr);
        }  
        fclose(fptr);

        if (!ptr) {
            warn(FILE_CORRUPT, dat_fn, NULL, FALSE);
        }
    } 
    else 
    {
        warn(FILE_UNREADABLE, dat_fn, "(Resetting to default state...", FALSE);  

        if (list && (fptr = fopen(dat_fn, "w")) && commitListData(list, *list_sz_ptr, fptr)) {
            warn(NULL, NULL, "Success)", TRUE);
        } else {
            warn(NULL, NULL, "Fail)", TRUE);
        }
    }

    return ptr;
}

void loadSchoolData() {
    void *ptr;

    clearScr();

    ptr = loadAppData(LST_SUBJECT) && ptr;
    ptr = loadAppData(USR_PRINCIPAL) && ptr; 
    ptr = loadAppData(USR_TEACHER) && ptr; 
    ptr = loadAppData(USR_STUDENT) && ptr;
    ptr = loadAppData(LST_ENROLL) && ptr;

    if (!ptr) {
        pauseScr ("\n", TRUE);
    }
}

FILE *refreshListData(int lst_type, const int read_only_flg, const int auth_mode_flg, const int scr_psd_mode)  // used to reload data in active user context  
{    
    if (auth_mode_flg && loggedOut() || !auth_mode_flg && !currentUsr()) {
        if (!auth_mode_flg) {
            sys_err(LVL_FATAL, MSG_ACTN_ABORT, scr_psd_mode, FALSE);
        }
        return NULL;
    }

    int loginID = CURRENT_USR->entry.ID;
    FILE* fptr  = reloadListData(lst_type, read_only_flg, scr_psd_mode);

    if (lst_type && lst_type != CURRENT_USR_TYPE) {
        reloadListData(NULL, TRUE, scr_psd_mode);
    }

    CURRENT_USR = getUser(loginID, NULL);

    if (!currentUsr())  // current user refresh/validation failure
    {   
        if (fptr && !read_only_flg) {
            persistListData(lst_type, fptr, scr_psd_mode);    // recommit previous data since reloading in write mode always clears the source file
        }
        if (auth_mode_flg) {
            displayLogoutScreen(LGO_SESS_INVALID);
        } else {
            sys_err(LVL_FATAL, MSG_ACTN_ABORT, FALSE, scr_psd_mode);
        }
        return NULL;
    }
    return fptr;
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

    pauseScr(NULL, TRUE);
}

void logout(int lg_type) 
{
    CURRENT_USR = NULL;

    /* TODO: kill timer*/
}

int loggedOut() {
    int result = currentUsr();
    if (result) {
        /* TODO:*/  // reset session timer
    } else {
        displayLogoutScreen(LGO_SESS_EXPIRED);
    }
    return !result;
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
            inform(0, 1, "Exiting the program....", SCR_PSD_OFF_PRMPT, FALSE);
            exit(0);

            default:
            pauseScr (MSG_INVALID_OPTION, TRUE);
        }

    } while (TRUE);
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
            CURRENT_USR = NULL;
            studentRegistration();
            break;

            case 0:
            return; // to mainscreen

            default:
            pauseScr (MSG_INVALID_OPTION, TRUE);
        }

    } while (TRUE);
}

int displayProfRegScreen(int usr_type, int reg_chkpnt, const int edit_mode_flg, const int frsh_dat_flg) {

    displayScreenSubHdr(getTitle(reg_chkpnt < 0 ? NULL: edit_mode_flg ? "EDIT ":"CONTINUE ", usr_type, " REGISTRATION"));

    if (edit_mode_flg) 
    { 
        if (frsh_dat_flg && !refreshData(usr_type, READ_ONLY, edit_mode_flg))
            return FALSE;

        displayProfileView(NULL, usr_type); 
    }

    printf ("\nPlease provide your profile information below (press ENTER to skip optional details)\n");

    return TRUE;
}

void studentRegistration() 
{
    int reg_chkpnt = CURRENT_USR ? CURRENT_USR->reg_stat : -1;

    if (reg_chkpnt <= REG_STAT_PROF) {
        if (!userProfileRegScreen(USR_STUDENT, reg_chkpnt))
            return;
    }
    if (reg_chkpnt <= REG_STAT_SUBJ) {
        subjectRegScreen(CURRENT_USR->entry.ID, reg_chkpnt);
    }

    if (CURRENT_USR->reg_stat == REG_STAT_FULL) {
        pauseScr("\nCONGRATULATIONS, YOU ARE FULLY REGISTERED!!\n", TRUE);
    } else {
        pauseScr("\nNOTE: Registration is not completed!\n", TRUE);
    }
}

int userProfileRegScreen(int usr_type, int reg_chkpnt) {

    const int edit_mode_flg = reg_chkpnt == REG_STAT_FULL;
    int result = TRUE;

    if (edit_mode_flg || reg_chkpnt < REG_STAT_PROF) 
    {
        if (!(result = processUserAccount(usr_type, reg_chkpnt, edit_mode_flg)))
            return FALSE;
    } 
    else {
        displayProfRegScreen(usr_type, reg_chkpnt, edit_mode_flg, FALSE);
    }

    if (edit_mode_flg || reg_chkpnt <= REG_STAT_PROF)
    {
        const int DTL_SKP_LMT = -6;
        int dtl_skp = result; 

        User usr_cpy = *CURRENT_USR;

        dtl_skp += processUserName(&usr_cpy, edit_mode_flg);
        dtl_skp += processUserAddr(&usr_cpy, edit_mode_flg);
        dtl_skp += processUserDob(&usr_cpy, edit_mode_flg);
        dtl_skp += processUserTimeout(&usr_cpy, edit_mode_flg);
             
        if (dtl_skp > DTL_SKP_LMT) 
        {       
            FILE* fwptr; 

            if (fwptr = refreshData(usr_type, READ_WRITE, edit_mode_flg))  // begin mod session
            {
               *CURRENT_USR = usr_cpy;

                if (!edit_mode_flg) 
                {
                    CURRENT_USR -> reg_stat = REG_STAT_SUBJ;
                }
                else
                if (result > FALSE) 
                {
                    FILE* efwptr;  

                    if (efwptr = refreshListData(LST_ENROLL, READ_WRITE, edit_mode_flg, SCR_PSD_OFF))   // begin enrollment mod session (in quiet mode)
                    {  
                        Enrollment* enrolls = getDataList(LST_ENROLL);
                        int enrolls_sz      = getEnrollListSz();

                        for (int i=0; i < getEnrollListSz(); i++) {
                            if (!enrolls[i].entry.deleted_flg) {
                                if (usr_type == USR_STUDENT)
                                    enrolls[i].studentID = CURRENT_USR->entry.ID;
                                else
                                if (usr_type == USR_TEACHER)
                                    enrolls[i].teacherID = CURRENT_USR->entry.ID;
                            }
                        }     
                        result *= persistListData(LST_ENROLL, efwptr, SCR_PSD_OFF);  // conclude enrollment mod session (in quiet mode)
                    } else {
                        warn(REFRESH_ERROR, "enrollment", NULL, FALSE);
                        result = FALSE;
                    }
                } 

                if (result *= persistData(usr_type, fwptr)) {   // end mod session 
                    char msg[32];
                    sprintf(msg, "Profile %s successfully.", edit_mode_flg? "updated":"created");

                    inform(0, 1, msg, SCR_PSD_ON, FALSE);
                }
            } else {
                result = FALSE;
            }
        }
    }

    return result;
}

int processUserAccount(int usr_type, const int reg_chkpnt, const int edit_mode_flg) 
{
    FILE* fptr;
    User* usr;

    int loginID, id_ok, result;
    int fst_run_flg = TRUE;

    do {
        if (!displayProfRegScreen(usr_type, reg_chkpnt, edit_mode_flg, fst_run_flg))
            return FALSE;

        loginID = promptID(edit_mode_flg);

        if (loginID < 0) {  
            return -TRUE;  // skip login ID edit
        }

        if (loginID > 0) {   
                                    
            if (edit_mode_flg) {                                         // begin user mod session
                fptr = refreshData(usr_type, READ_ONLY, edit_mode_flg); 
            } else {
                fptr = reloadData(usr_type, READ_WRITE);
            }
            if (!fptr) return FALSE;  // abort mod session

            usr = getUser(loginID, usr_type);

            id_ok = !usr || (edit_mode_flg && usr == CURRENT_USR);

            if (id_ok)
            {
                result = FALSE;

                if (!edit_mode_flg) {
                    CURRENT_USR = registerUser(usr_type, loginID);
                } 
                else if (CURRENT_USR->entry.ID == loginID) {
                    result = -TRUE;
                } else {
                    CURRENT_USR->entry.ID = loginID; 
                    result = TRUE;
                }
            }
            if (!edit_mode_flg) {
                result = persistData(usr_type, fptr);  // end user mod session
            }
            if (id_ok) 
            {
                if (!CURRENT_USR) {
                    sys_err(LVL_FATAL, "The user account could not be created.", SCR_PSD_NO_PRMPT, FALSE);
                    result = FALSE;
                }
                return result;
            }
        }

        printf("\nThe login ID entered is either already taken or invalid. Please provide a different ID between %d and %d.\n", PASSCODE_MN, PASSCODE_MX);

        if (!retry(0)) {
            return FALSE;
        }

        fst_run_flg = FALSE;

    } while (TRUE);
}

int processUserName(User* usr, const int edit_mode_flg) {
    char fn[FNAME_SZ + 1], ln[LNAME_SZ + 1];
    int result = -1;

    do {
        promptLn("\nEnter First Name: ", fn, FNAME_SZ);

        if (edit_mode_flg && skpdtl(fn))
            break;

        if (!isEmptyStr(fn, TRUE, fn)) {
            strcpy(usr->Fname, fn);
            result = 1;
            break;
        }

        printf("The name is invalid! Please specify a non-blank first name.");
   
    } while (TRUE);

    promptLn("Enter Last Name: ", ln, LNAME_SZ);

    if (!(edit_mode_flg && skpdtl(ln))) 
    {
        trim(ln, -1, usr->Lname, 0);

        if (result < 0) 
            result = 1;
        else
            result += 1;
    }
    else if (result < 0) { 
        result -= 1;
    }

    return result;
}

int processUserAddr(User* usr, const int edit_mode_flg) {
    char addr[ADDR_SZ + 1];

    promptLn("Enter Address: ", addr, ADDR_SZ);

    if (!(edit_mode_flg && skpdtl(addr))) 
    {
        trim(addr, -1, usr->Addr, 0);
        return TRUE;
    }

    return -TRUE;
}

int processUserDob(User* usr, const int edit_mode_flg) {
    char dob[DOB_SZ + 1];
    Date tm;

    do {
        promptLn("Enter Date of Birth (DD/MM/YYYY): ", dob, DOB_SZ);

        if (edit_mode_flg && skpdtl(dob))
            return -TRUE;

        if (convertDate(trim(dob, -1, NULL, 0), &tm)) 
        {
            if (tm.tm_year < 0) {
                printf("Invalid date of birth! Please specify a date with year 1900 or later.\n");
            } else if (calculateAge(&tm, NULL) < 0) {
                printf("Date of birth cannot be in the future!\n");
            } else {
                strcpy(usr->Dob, dob);
                return TRUE;
            }
        } else {
            printf("The date is invalid! Please specify date in the correct format.\n");
        }
   
    } while (TRUE);
}

int processUserTimeout(User* usr, const int edit_mode_flg) {
    char tout[TOUT_SZ + 1];
    Date tm;

    do {
        promptLn("Enter Session Timeout (mins): ", tout, TOUT_SZ);

        if (edit_mode_flg && skpdtl(tout))
            return -TRUE;

        int tm_out = atoi(trim(tout, -1, NULL, 0));
        if (tm_out > 0) {
            usr->timeout = tm_out;
            return TRUE;
        }

        printf("The timeout duration is invalid! Please enter a positive integer value.\n");
   
    } while (TRUE);
}

void subjectRegScreen(int loginID, const int actn_mode)  // NOTE: subject registration specifically for students
{
    const int ITEM_SZ = ITEM_NO_SZ   + SCR_PADDING;
    const int SUBJ_SZ = SUBJ_TTL_SZ  + SCR_PADDING;
    const int NAME_SZ = FULL_NAME_SZ + SCR_PADDING;

    const int edit_mode_flg = actn_mode >= REG_STAT_FULL;
    const int edit_subj_flg = actn_mode == USR_ACTN_EDIT;

    int   fst_run_flg = TRUE; 
    int   tbl_margin, result, idx, subj_no, subj_id, tchr_no, tchr_id;

    Subject* subjects = getDataList(LST_SUBJECT);
    int subjects_sz   = getDataListSz(LST_SUBJECT, TRUE);  // use minimal list size

    User* teachers    = getDataList(USR_TEACHER);
    int teachers_sz   = getTeacherListSz();

    Enrollment* enrolls = getDataList(LST_ENROLL);
    
    Enrollment*enroll;
    Enrollment enroll_buf  [subjects_sz];  // refers to newly created enrollment entries based on user selection
    int   subject_ids      [subjects_sz];  // refers to indexes of either enrolled or available subjects
    int   enroll_cnt  = 0, subj_total; 
    
top:    
    displayScreenSubHdr(getTitle(actn_mode < 0 ? NULL: edit_mode_flg ? "EDIT ":"CONTINUE ", LST_SUBJECT, " REGISTRATION"));
    
    tbl_margin = print4ColTblHdr("No.", ITEM_SZ, "Subject", SUBJ_SZ, "No.", ITEM_SZ, "Teacher", NAME_SZ);

    if (fst_run_flg) {

        if (!refreshData(LST_ENROLL, READ_ONLY, edit_mode_flg))
            return; 

        subj_total = getSubjects(loginID, USR_STUDENT, subject_ids, !edit_subj_flg);  // filter for subjects based on the subject action mode
    }

    // populate table with subject & teacher listings
    if (subj_total == 0) 
        printScrTitle(NULL, MSG_EMPTY_LIST, NULL);
    else
    for (int i = 0; i < subj_total || i < teachers_sz; i++) {

        idx = subject_ids[i];
        
        printScrHMargin(tbl_margin); 

        if (i < subj_total) {
            printScrColVal(i + 1, ITEM_SZ, 0, NULL);
            printScrColText((edit_subj_flg? getSubject(enrolls[idx].entry.ID) : subjects+idx)->title, SUBJ_SZ, NULL);
        } else {
            printScrColText("", ITEM_SZ + SUBJ_SZ, NULL);
        }

        if (i < teachers_sz && !teachers[i].entry.deleted_flg) {
            printScrColVal(i + 1, ITEM_SZ, 0, NULL);
            printScrColText(getFullName(teachers+i), NAME_SZ, NULL);
        } 

        printScrVMargin(1);
    }
 
    // prompt for and process enrollment input
    printf ("\n\nPlease provide your course information below (press ENTER when finished)\n");
    printf ("\n(Specify any pair of numbers associated with a subject and teacher from the list.");
    printf ("\n Note that a subject can only be paired with a single teacher, however the same");
    printf ("\n teacher may be paired with multiple subjects. Also note that, if enrolling for");
    printf ("\n the first time, at least one subject must be chosen in order to complete the");
    printf ("\n registration.)\n\n");

    do {
        result = promptOptions("Enter course info (Subject No. Teacher No.): ", &subj_no, &tchr_no);

        if (result < 0)  // input is blank
        {
            if (retry(RT_CONFIRM)) { 
                fst_run_flg = FALSE;
                goto top;      
            } 
            else break;
        }
        else if (result)
        {  
            if (subj_no > subj_total) {
                printf("The entered subject no. is incorrect. Please specify a valid subject no. from the list above.\n");
                continue;
            } 
            else if (tchr_no > teachers_sz || teachers[tchr_no-1].entry.deleted_flg) {
                printf("The entered teacher no. is incorrect. Please specify a teacher no. from the list above.\n");
                continue;
            }
            
            idx = subject_ids[subj_no-1];   

            if (edit_subj_flg)
                subj_id = (enrolls+idx)->entry.ID;
            else
                subj_id = (subjects+idx)->entry.ID;

            tchr_id = teachers[tchr_no-1].entry.ID;

            if (enroll = getEnrollEntry(subj_id, NULL, enroll_buf, enroll_cnt)) {  // re-edit already edited enrollment entry
                enroll->teacherID = tchr_id;
            }
            else if (edit_subj_flg) {  // clone existing enrollment and update with specified course info

                enroll_buf [enroll_cnt] = enrolls[idx];
                enroll_buf [enroll_cnt++].teacherID = tchr_id;
            } 
            else {  // buffer specified course info as new enrollment

                initEnroll(enroll_buf + enroll_cnt++, subj_id, loginID, tchr_id);
            } 
        }
        else 
            printf("The entered course information is invalid. Please specify enrollment options according to the syntax given.\n"); 

    } while (TRUE);

    // save selected enrollments
    if (submitEnrollData(enroll_buf, enroll_cnt, edit_mode_flg, edit_subj_flg) <= 0) 
        return;

    // update registration status for new students only
    if (!edit_mode_flg) 
    {
        FILE* fwptr;

        if (fwptr = refreshData(USR_STUDENT, READ_WRITE, PUBLIC))  // begin mod session 
        {
            CURRENT_USR->reg_stat = REG_STAT_FULL;
            if (!persistData(USR_STUDENT, fwptr))  // end mod session
                return;
        } 
        else return;
    }

    char msg[45];
    sprintf(msg, "Course information %s successfully.", edit_mode_flg? "updated":"created");

    inform(0, 1, msg, edit_mode_flg? SCR_PSD_ON: SCR_PSD_NO_PRMPT, FALSE);
}

void loginScreen() 
{
    int loginID = 0;
    int reg_err_flg;

    do {
        displayScreenSubHdr(getTitle(NULL, NULL, " LOGIN"));

        loginID = promptID(FALSE);

        reg_err_flg = FALSE;

        if (loginID > 0) {

            if (!reloadData(NULL, READ_ONLY)) {
                inform(1, 0, "Login failed! Please contact the system administrator.", SCR_PSD_ON, FALSE);
                return;
            }  

            if (CURRENT_USR = getUser(loginID, NULL)) {
                if (CURRENT_USR->reg_stat == REG_STAT_FULL) {
                    userHomeScreen();
                    return;
                }
                if (CURRENT_USR_TYPE == USR_STUDENT) {
                    studentRegistration();
                    return;
                }
                reg_err_flg = -TRUE;
            }
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

    } while (TRUE);
}

void displayUserHomeScreen() {

    displayScreenSubHdr(getTitle(NULL, NULL, " HOME"));
    
    printTopic ("Select an option below");

    printf ("[1] View Profile\n");
    printf ("[2] Edit Profile\n");
    printf ("[3] View Subjects\n");

    switch (CURRENT_USR_TYPE) {
    case USR_STUDENT:
        printf ("[4] Add Subjects\n");
        printf ("[5] Drop Subjects\n");
        break;
    case USR_TEACHER:
        printf ("[4] Edit Grades\n");
        break;
    case USR_PRINCIPAL:  
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
            viewProfileScreen(NULL, NULL);
            continue;

            case 2:
            userProfileRegScreen(NULL, REG_STAT_FULL);
            continue;
        }

        if (CURRENT_USR_TYPE == USR_STUDENT) {
            switch (choice) {
                case 3:
                subjectListScreen(FALSE);
                continue;

                case 4:
                subjectRegScreen(CURRENT_USR->entry.ID, USR_ACTN_ADD);
                continue;

                case 5:
                subjectListScreen(TRUE);
                continue;
            }
        } else if (CURRENT_USR_TYPE == USR_TEACHER) {
            switch (choice) {
                case 3:
                subjectListScreen(FALSE);
                continue;

                case 4:
                subjectListScreen(TRUE);
                continue;
            }
        } else if (CURRENT_USR_TYPE == USR_PRINCIPAL) {
            switch (choice) {
                case 3:
                viewEnrollmentScreen(LST_SUBJECT);
                continue;

                case 4:
                viewEnrollmentScreen(USR_TEACHER);
                continue;

                case 5:
                viewEnrollmentScreen(USR_STUDENT);
                continue;

                case 6:
                editEnrollmentScreen(USR_ACTN_EDIT);
                continue;

                case 7:
                editEnrollmentScreen(USR_ACTN_REM);
                continue;
            }
        }

        pauseScr (MSG_INVALID_OPTION, TRUE);

    } while (TRUE);
}

void displayProfileView(User* usr, int usr_type) 
{
    const int MAX_COL_SZ = 20 + SCR_PADDING;  //max size for the field names column
    
    int hmargin = 8, len;
    float avg;

    if (!usr) usr = CURRENT_USR;
    if (!usr_type) usr_type = CURRENT_USR_TYPE;

    if ((len = strlen(usr->Fname)) > hmargin)
        hmargin = len;
    if ((len = strlen(usr->Lname)) > hmargin)
        hmargin = len;
    if ((len = strlen(usr->Addr)) > hmargin)
        hmargin = len;

    hmargin = (SCR_SIZE - MAX_COL_SZ - hmargin) / 2;

    printScrTitle(NULL, "----- PERSONAL INFORMATION -----", "\n\n");

    printScrHMargin(hmargin);
    printScrColText("First Name:", MAX_COL_SZ, NULL);
    printScrColText(vsan(usr->Fname, NULL), 0, "\n");
    printScrHMargin(hmargin);
    printScrColText("Last Name:", MAX_COL_SZ, NULL);
    printScrColText(vsan(usr->Lname, NULL), 0, "\n");
    printScrHMargin(hmargin);
    printScrColText("Address:", MAX_COL_SZ, NULL);
    printScrColText(vsan(usr->Addr, NULL), 0, "\n");
    printScrHMargin(hmargin);
    printScrColText("Date of Birth:", MAX_COL_SZ, NULL);
    printScrColText(vsan(usr->Dob, NULL), 0, "\n");
    printScrHMargin(hmargin);
    printScrColText("Age:", MAX_COL_SZ, NULL);
    printScrColVal(calculateAge(NULL, usr->Dob), 0, 0, "\n\n");

    printScrTitle(NULL, "----- COURSE INFORMATION -----", "\n\n");

    avg = calculateAvgGrade(usr_type==USR_PRINCIPAL? NULL: usr->entry.ID, usr_type);

    if (usr_type == USR_STUDENT) 
    {
        printScrHMargin(hmargin);
        printScrColText("Total Subjects:", MAX_COL_SZ, NULL);
        printScrColVal(calculateTally(usr->entry.ID, usr_type, NULL), 0, 0, "\n");
        printScrHMargin(hmargin);
        printScrColText("Total Teachers:", MAX_COL_SZ, NULL);
        printScrColVal(calculateTally(usr->entry.ID, usr_type, USR_TEACHER), 0, 0, "\n");  
        printScrHMargin(hmargin);
        printScrColText("Average Grade:", MAX_COL_SZ, NULL);
        printGrade(avg, "\n\n");
    }
    else if (usr_type == USR_TEACHER) 
    {
        printScrHMargin(hmargin);
        printScrColText("Total Students:", MAX_COL_SZ, NULL);
        printScrColVal(calculateTally(usr->entry.ID, usr_type, USR_STUDENT), 0, 0, "\n"); 
        printScrHMargin(hmargin);
        printScrColText("Total Subjects:", MAX_COL_SZ, NULL);
        printScrColVal(calculateTally(usr->entry.ID, usr_type, NULL), 0, 0, "\n");
        printScrHMargin(hmargin);
        printScrColText("Students Avg Grade:", MAX_COL_SZ, NULL);
        printGrade(avg, "\n\n");
    }
    else if (usr_type == USR_PRINCIPAL) 
    {
        printScrHMargin(hmargin);
        printScrColText("Total Subjects:", MAX_COL_SZ, NULL);
        printScrColVal(getDataListCnt(LST_SUBJECT), 0, 0, "\n");
        printScrHMargin(hmargin);
        printScrColText("Total Teachers:", MAX_COL_SZ, NULL);
        printScrColVal(getDataListCnt(USR_TEACHER), 0, 0, "\n");
        printScrHMargin(hmargin);
        printScrColText("Total Students:", MAX_COL_SZ, NULL);
        printScrColVal(getDataListCnt(USR_STUDENT), 0, 0, "\n");
        printScrHMargin(hmargin);
        printScrColText("School Avg Grade:", MAX_COL_SZ, NULL);
        printGrade(avg, "\n\n");
    }

    printScrTitle(NULL, "----- ACCOUNT INFORMATION -----", "\n\n");

    if (CURRENT_USR_TYPE == USR_PRINCIPAL && usr_type != USR_PRINCIPAL) {
    printScrHMargin(hmargin);
    printScrColText("Registration Status:", MAX_COL_SZ, NULL);
    printScrColText(getRegStatDesc(usr->reg_stat, FALSE), 0, "\n");
    }
    printScrHMargin(hmargin);
    printScrColText("Session Duration:", MAX_COL_SZ, NULL);
    printScrColVal(usr->timeout, 0, 0, " minutes(s)\n\n");
}

void viewProfileScreen(User* usr, int usr_type) {

    if (!(refreshData(usr_type, READ_ONLY, SECURED) && currentUsr()))
        return;

    displayScreenSubHdr(getTitle(NULL, usr_type, " PROFILE"));

    displayProfileView(usr, usr_type);

    pauseScr("\n", TRUE);
}

int displaySubjListView(User* usr, int usr_type, Enrollment**enrolls_ptr, int* subj_total_ptr) 
{
    const int ITEM_SZ = ITEM_NO_SZ   + SCR_PADDING;
    const int NAME_SZ = FULL_NAME_SZ + SCR_PADDING;
    const int SUBJ_SZ = SUBJ_TTL_SZ  + SCR_PADDING;

    int uID, subj_total = *subj_total_ptr; 
    char* name;
    Subject* subj;
    Enrollment* e, *subj_enrolls_ptr [getDataListSz(LST_SUBJECT, TRUE)];
    

    if (!usr) usr = CURRENT_USR;
    if (!usr_type) usr_type = CURRENT_USR_TYPE;

    int tchr_flg = usr_type==USR_TEACHER;
    int utype = tchr_flg ? USR_STUDENT : USR_TEACHER;

    int tbl_margin = print4ColTblHdr("No.", ITEM_SZ, "Subject", SUBJ_SZ, tchr_flg?"Student":"Teacher", NAME_SZ, "Grade (%)", GRADE_SZ);

    if (subj_total < 0) {
        subj_total = enrollSearch(usr->entry.ID, usr_type, NULL, -1, 0, tchr_flg? subj_enrolls_ptr: enrolls_ptr, 0);

        if (tchr_flg) 
        {
            int stud_total = 0;

            for (int i=0; i < subj_total; i++) {
                stud_total += enrollSearch(NULL, subj_enrolls_ptr[i]->entry.ID, NULL, usr->entry.ID, 0, enrolls_ptr + stud_total, 0);
            }

            *subj_total_ptr = stud_total;
        } 
        else {
            *subj_total_ptr = subj_total;
        }
    }

    // populate table with subject & teacher listings
    if (subj_total == 0) 
        printScrTitle(NULL, MSG_EMPTY_LIST, NULL);
    else
    for (int i = 0; i < subj_total; i++) 
    {
        e    = enrolls_ptr[i];
        uID  = usr_type==USR_TEACHER? e->studentID : e->teacherID;
        name = getFullName(getUser(uID, utype));
        subj = getSubject(e->entry.ID);

        printScrHMargin(tbl_margin); 
        printScrColVal(i + 1, ITEM_SZ, 0, NULL);     
        printScrColText(subj? subj->title: "", SUBJ_SZ, NULL);
        printScrColText(name? name: "", NAME_SZ, NULL); 
        printGrade(e->grade, "\n");
    }

    printScrVMargin(2);
}

void subjectListScreen(const int edit_mode_flg) {

    if (!refreshData(LST_ENROLL, READ_ONLY, SECURED) && (edit_mode_flg || !currentUsr()))
        return;

    int subj_total = -1;
    int enroll_cnt =  0;
    int enrolls_sz = getDataListSz(LST_ENROLL, TRUE);
    
    Enrollment*enrolls_ptr[enrolls_sz];  // subject enrollment entries belonging to the given user
    Enrollment enroll_buf [enrolls_sz];  // updated enrollment entries based on user input
    
    do {

        displayScreenSubHdr(getTitle(NULL, NULL, " SUBJECTS"));

        displaySubjListView(NULL, NULL, enrolls_ptr, &subj_total);

        if (!edit_mode_flg) {
            pauseScr (NULL, TRUE);
            return;
        }
        
        if (CURRENT_USR_TYPE == USR_TEACHER) 
        {
            if (editGrades(enrolls_ptr, subj_total, enroll_buf, &enroll_cnt))
                return;
        } 
        else 
        {
            if (dropSubject(enrolls_ptr, subj_total))
                return;
        }

    } while (TRUE);
}

int editGrades(Enrollment**enrolls_ptr, int subj_total, Enrollment* enroll_buf, int* enroll_cnt_ptr) 
{
    int subj_no, subj_id, result;
    float grade;
    Enrollment* enroll;

    // prompt for and process subject grades
    printf ("\n\nPlease provide the subject grade information below (press ENTER when finished)\n");
    printf ("\n(Specify the subject no. from the list and the grade to be associated with the subject.");
    printf ("\n Note that subject grades can by edited multiple times, with the last edit used as the");
    printf ("\n final grade for each subject.)\n\n");

    do {
        result = promptOptVal("Enter course info (Subject No. Grade): ", &subj_no, &grade, FALSE);

        if (result < 0)    // input is blank
        {
            if (retry(RT_CONFIRM)) { 
                return FALSE;    
            } 
            else break;
        }
        else if (result)
        {  
            if (subj_no > subj_total) {
                printf("The entered subject no. is incorrect. Please specify a valid subject no. from the list above.\n");
                continue;
            } 
            else if (grade < 0) {
                printf("The entered grade is invalid. Please specify a grade greater than or equal to zero.\n");
                continue;
            }
            
            subj_id = enrolls_ptr[subj_no-1]->entry.ID;   

            if (enroll = getEnrollEntry(subj_id, NULL, enroll_buf, *enroll_cnt_ptr)) {  // re-edit already edited enrollment entry
                enroll->grade = grade;
            }
            else {  // clone existing enrollment and update with specified course info

                enroll_buf [*enroll_cnt_ptr] = *enrolls_ptr[subj_no-1];
                enroll_buf [(*enroll_cnt_ptr)++].grade = grade;
            } 
        }
        else 
            printf("The entered course information is invalid. Please specify options according to the syntax given.\n"); 

    } while (TRUE);

    // save selected enrollments
    if (submitEnrollData(enroll_buf, *enroll_cnt_ptr, TRUE, TRUE) <= 0) 
        return;

    inform(0, 1, "Course information edited successfully.", SCR_PSD_ON, FALSE);

    return TRUE;
} 

int dropSubject(Enrollment**enrolls_ptr, int subj_total) 
{
    int choice = 0;

    printf("Please enter the subject no. for the course you desire to remove.\n\n");

    promptInt("Enter Subject No.: ", &choice, ITEM_NO_SZ);

    if (!choice || choice > subj_total) {
        printf("The entered subject no. is incorrect. Please specify a valid subject no. from the list above.\n");
        return ! retry(0);
    } 
   
    Enrollment e_cpy = *enrolls_ptr[choice-1];
    FILE* fwptr; 

    if (fwptr = refreshData(LST_ENROLL, READ_WRITE, SECURED))  // begin mod session 
    {
        if (enrollSearch(NULL, e_cpy.entry.ID, e_cpy.studentID, NULL, 0, enrolls_ptr, 1)) {
            deleteListEntry(*enrolls_ptr);
            inform(0, 1, "Course removed successfully.", SCR_PSD_ON, FALSE);
        } else {
            char entry_key [12];

            strcpy(entry_key, e_cpy.entry.ID);
            strcat(entry_key, "+");
            strcat(entry_key, e_cpy.studentID);
            
            warn(NO_ENTRY_ERROR, entry_key, "It may have been removed externally.", FALSE);
        }

        persistData(LST_ENROLL, fwptr);  // end mod session
    } 

    return TRUE;
}

void displayEnrollStatsView(int usr_type, Entry* list, int list_sz, const int show_stats, const int show_grade) 
{
    const int usr_flg  =  usr_type==USR_STUDENT || usr_type==USR_TEACHER;
    const int ENTRY_SZ = (usr_flg? FULL_NAME_SZ *1/3 : SUBJ_TTL_SZ) + SCR_PADDING;
    const int RG_STAT_SZ  = REG_STAT_SZ + SCR_PADDING;
    const int ITEM_SZ     = ITEM_NO_SZ  + SCR_PADDING;
    const int STAT_SZ     = ITEM_NO_SZ  + SCR_PADDING *2;

    char *col_hdr2, *col_hdr3, *col_hdr4, *col_hdr5, *col_hdr6; 
    int entryID, sz_col5, tgt_col4, tgt_col5;

    col_hdr3 = usr_flg?    "Reg Status":NULL;
    col_hdr6 = show_grade? "Avg Grade (%)":NULL;

    switch (usr_type) {
        case USR_STUDENT:
            col_hdr2 = "Student";
            col_hdr4 = show_stats? "Teachers": NULL;
            col_hdr5 = show_stats? "Subjects": NULL;
            tgt_col4 = USR_TEACHER;
            tgt_col5 = NULL;
            break;
        case USR_TEACHER:
            col_hdr2 = "Teacher";
            col_hdr4 = show_stats? "Students": NULL;
            col_hdr5 = show_stats? "Subjects": NULL;
            tgt_col4 = USR_STUDENT;
            tgt_col5 = NULL;
            break;
        default:
            col_hdr2 = "Subject";
            col_hdr4 = show_stats? "Students": NULL;
            col_hdr5 = show_stats? "Teachers": NULL;
            tgt_col4 = USR_STUDENT;
            tgt_col5 = USR_TEACHER;
    }

    int tbl_margin = print6ColTblHdr("No.", ITEM_SZ, col_hdr2, ENTRY_SZ, col_hdr3, RG_STAT_SZ, col_hdr4, STAT_SZ, col_hdr5, STAT_SZ, col_hdr6, GRADE_SZ);

    for (int i = 0; i < list_sz; i++) 
    {
        if (list->deleted_flg)
            continue;

        entryID = list[i].ID;

        printScrHMargin(tbl_margin);

        printScrColVal(i + 1, ITEM_SZ, 0, NULL);
        
        printScrColText(usr_flg? getFullName(((User*) list) + i) : ((Subject*) list)[i].title, ENTRY_SZ, NULL);

        if (usr_flg) {
            printScrColText(getRegStatDesc(((User*) list)[i].reg_stat, TRUE), STAT_SZ, NULL);
        }
        if (show_stats) {
            printScrColVal(calculateTally(entryID, usr_type, tgt_col4), ITEM_SZ, 0, NULL);
            printScrColVal(calculateTally(entryID, usr_type, tgt_col5), ITEM_SZ, 0, NULL);    
        }
        if (show_grade){
            printGrade(calculateAvgGrade(entryID, usr_type), NULL);
        }

        printScrVMargin(1);
    }

    printScrVMargin(2);
}

void viewEnrollmentScreen(int usr_type) {

    int usr_flg = usr_type==USR_STUDENT || usr_type==USR_TEACHER;

    if (!(refreshData(usr_flg? usr_type: LST_SUBJECT, READ_ONLY, SECURED) && currentUsr()))
        return;

    if (!(refreshData(LST_ENROLL, READ_ONLY, SECURED) && currentUsr()))
        return;

    Entry* list = getDataList(usr_flg? usr_type :LST_SUBJECT);
    int list_sz = getDataListSz(usr_flg? usr_type :LST_SUBJECT, FALSE);

    int choice = 0;
    
    do {

        displayScreenSubHdr(getTitle(NULL, usr_flg? usr_type: LST_SUBJECT, " ENROLLMENTS"));

        displayEnrollStatsView(usr_type, list, list_sz, TRUE, TRUE);

        if (!usr_flg) {
            pauseScr (NULL, TRUE);
            return;
        }
        
        printf("\nSelect an item no. from the list in order to view additional details.\n\n");

        promptInt("Enter Item No.: ", &choice, ITEM_NO_SZ);

        if (!choice || choice > list_sz || list[choice-1].deleted_flg) {
            printf("The entered item no. is incorrect. Please specify a valid item no. from the list above.\n");
            if (!retry(0)) break;
        } else {
            viewProfileScreen(list + choice-1, usr_type);
        }

    } while (TRUE);
}

void editEnrollmentScreen(const int actn_mode) {

    if (!(refreshData(LST_ENROLL, READ_ONLY, SECURED) && currentUsr()))
        return;

    int subj_total = -1;
    int enroll_cnt =  0;
    int enrolls_sz = getDataListSz(LST_ENROLL, TRUE);
    
    Enrollment*enrolls_ptr[enrolls_sz];  // subject enrollment entries belonging to the given user
    Enrollment enroll_buf [enrolls_sz];  // updated enrollment entries based on user input
    
    do {

        displayScreenSubHdr(getTitle(NULL, NULL, " SUBJECTS"));

        displaySubjListView(NULL, NULL, enrolls_ptr, &subj_total);
        
        if (CURRENT_USR_TYPE == USR_TEACHER) 
        {
            if (editGrades(enrolls_ptr, subj_total, enroll_buf, &enroll_cnt))
                return;
        } 
        else 
        {
            if (dropSubject(enrolls_ptr, subj_total))
                return;
        }

    } while (TRUE);
}