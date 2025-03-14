#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>      // For isspace(), isdigit() & isprint() functions 
#include <time.h>       // For strftime() function
#if defined(_WIN32) || defined(__CYGWIN__)
#include <windows.h>    // For Windows Sleep() function and getpass() implementation
#else
#include <unistd.h>     // For Linux sleep() function
#include <termios.h>    // For getpass() implementation
#endif

// Log file default title
#define DEF_LG_TITLE "System Application Log"

// Log level enumeration
#define LVL_INFO ""
#define LVL_WARN "Warning: "
#define LVL_ERROR "ERROR: "
#define LVL_FATAL "FATAL: "
#define LVL_DEBUG "DEBUG: "

// Debug log mode enumeration
#define LG_MODE_OFF   0
#define LG_MODE_CONSL 1
#define LG_MODE_FILE  2
#define LG_MODE_ALL   3

// Screen pause mode enumeration
#define SCR_PSD_OFF      -2
#define SCR_PSD_OFF_PRMPT-1
#define SCR_PSD_NO_PRMPT  0
#define SCR_PSD_ON        1

// Trim mode enumeration
#define LTRIM -1
#define FTRIM  0
#define RTRIM  1

// Boolean flags enumeration
#define FALSE 0
#define TRUE  1

// Screen display properties (measured in characters)
static int SCR_SIZE = 120;
static int SCR_PADDING = 5;

// Synchronized read settings
static int FILE_READ_FRQ = 20;      // no. of retries upon read failure
static int FILE_READ_LAT = 50;      // wait interval between each retry (in milliseconds)

static int OPTION_MAX_SZ = 1;       // max. character width for any given menu option

// Log framework control settings
static char *LOG_NULL_VALUE;        // specifies how NULL messages should be represented in the logs

static char *SYS_LG_FILE, *INFO_LG_FILE, *WARN_LG_FILE, *ERROR_LG_FILE, *FATAL_LG_FILE, *DEBUG_LG_FILE;
static char *SYS_LG_TITLE,*INFO_LG_TITLE,*WARN_LG_TITLE,*ERROR_LG_TITLE,*FATAL_LG_TITLE,*DEBUG_LG_TITLE;
static int INFO_MODE = LG_MODE_CONSL, INFO_TMS_MODE = LG_MODE_FILE;
static int WARN_MODE = LG_MODE_CONSL, WARN_TMS_MODE = LG_MODE_FILE;
static int ERROR_MODE = LG_MODE_CONSL, ERROR_TMS_MODE = LG_MODE_FILE;
static int FATAL_MODE = LG_MODE_CONSL, FATAL_TMS_MODE = LG_MODE_FILE;
static int DEBUG_MODE, DEBUG_TMS_MODE = LG_MODE_FILE; 

// Mandatory function prototype declarations
static int   glbMode(char*); 
static int   glbTMSMode(char*); 
static char* glbFname(char*, const int);
static char* glbTitle(char*, const int); 
static char* tmstmp(char*);
static void* get_console();
static void* setnoecho_console(void*);
static void* restore_console(void*, void*);


/********************************************************************/
/******************** Auxilliary Screen Functions *******************/
/********************************************************************/

void clearScr() {
#if defined(_WIN32) || defined(__CYGWIN__)
    system ("cls");   // Windows OS
#else
    system ("clear"); // other OS
#endif
}

void printScrHMargin(int hMargin) {
    printf("%*s", hMargin < 0 ? 0 :hMargin, "");
}

void printScrVMargin(int vMargin) {
    printScrPat(NULL, "\n", vMargin - 1, NULL);
}

void printScrHVMargin(int hMargin, int vMargin) {
    printScrHMargin(hMargin);
    printScrVMargin(vMargin);
}

void printScrMargin(int vMargin) {
    printScrHVMargin(SCR_PADDING, vMargin);
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
    const int DEF_MRGN = (SCR_SIZE - TTL_SZ) / 2;

    int margin = DEF_MRGN;

    if (pre_pat) {
        margin -= strlen(pre_pat);
        printf (pre_pat);
    }
    if (title) 
        printf ("%*s", (margin < 0 ? 0 : margin) + TTL_SZ, title);
    if (pst_pat) 
        printf ("%*s", (margin < 0 ? margin : 0) + DEF_MRGN, pst_pat);    
}

void printScrTopic(const char* caption, int cap_sz, const char* pattern, int margin, const int ovr_bnd_flg) 
{
    if (caption || cap_sz >= 0) {
        if (caption) {
            printScrHMargin(margin);
            printf("%s\n", caption);
        }
        if (pattern) {
            int reps = 0;
            int pat_sz = strlen(pattern);

            if (pat_sz > 0) 
            {
                if (cap_sz < 0)
                    cap_sz = strlen(caption);
            
                reps = cap_sz / pat_sz - 1; 
                if (ovr_bnd_flg > FALSE && cap_sz % pat_sz > 0)
                    reps++;
            }
            printScrHMargin(margin);
            printScrPat(NULL, pattern, reps, "\n");
        }  
    }
}

int printScrHeader(const char* col_txt1, int col_sz1, const char* col_txt2, int col_sz2,   // supports up to 6 columns
                   const char* col_txt3, int col_sz3, const char* col_txt4, int col_sz4, 
                   const char* col_txt5, int col_sz5, const char* col_txt6, int col_sz6, 
                   const char* pattern, int margin) 
{   
    int hdr_sz = 0;

    if (col_txt1) {
        hdr_sz += (col_sz1 = abs(col_sz1));
    }
    if (col_txt2) {
        hdr_sz += (col_sz2 = abs(col_sz2));
    }
    if (col_txt3) {
        hdr_sz += (col_sz3 = abs(col_sz3));
    }
    if (col_txt4) {
        hdr_sz += (col_sz4 = abs(col_sz4));
    }
    if (col_txt5) {
        hdr_sz += (col_sz5 = abs(col_sz5));
    }
    if (col_txt6) {
        hdr_sz += (col_sz6 = abs(col_sz6));
    }

    if (margin < 0)
        margin = (SCR_SIZE - hdr_sz) / 2;

    printScrHMargin(margin);

    if (col_txt1) {
        printScrColText(col_txt1, col_sz1, NULL);
    }
    if (col_txt2) {
        printScrColText(col_txt2, col_sz2, NULL);
    }
    if (col_txt3) {
        printScrColText(col_txt3, col_sz3, NULL);
    }
    if (col_txt4) {
        printScrColText(col_txt4, col_sz4, NULL);
    }
    if (col_txt5) {
        printScrColText(col_txt5, col_sz5, NULL);
    }
    if (col_txt6) {
        printScrColText(col_txt6, col_sz6, NULL);
    }

    printScrTopic("", hdr_sz, pattern, margin, TRUE);

    return margin;
}

void printScrColText(const char* col_txt, int col_txt_sz, const char* pst_txt) {
    if (col_txt) 
        printf ("%-*s", col_txt_sz, col_txt); 
    if (pst_txt)
        printf (pst_txt);
}

void printScrColVal(float col_val, int col_val_sz, int col_val_prec, const char* pst_txt) {
    
    printf ("%-*.*f", col_val_sz, col_val_prec < 0 ? 0 :col_val_prec, col_val); 
    
    if (pst_txt)
        printf (pst_txt);
}


/********************************************************************/
/********************* Common Library Functions *********************/
/********************************************************************/

void msleep(int delay) {   // delay in milliseconds
#if defined(_WIN32) || defined(__CYGWIN__)  // Windows OS
    Sleep (delay);          
#else  // Linux OS
    int delay_sec = delay / 1000;  // convert milliseconds to seconds
    while ((delay_sec = sleep(delay_sec)) > 0); 
    usleep (delay % 1000 * 1000);
#endif
}

char *trim (char* str, int str_sz, char* trstr, const int tr_mode)  // if trstr is provided,   
{                                                                   // it must have a size at least 1 character greater than str
    if (str == NULL)  
        return str; 

    if (str_sz < 0)
        str_sz = strlen(str);

    if (trstr == NULL) {
        if(str_sz == 0)  
            return str; 
        else
            trstr = str;
    }

    int st, end = str_sz - 1;

    if (tr_mode <= FTRIM) 
    {
        st = 0;
        while (st++ < end && isspace(*str)) {
            str++; str_sz--;
        }
        end = str_sz - 1;
    }

    if (tr_mode >= FTRIM) {
        while (end >= 0 && isspace(str[end])) {
            end--; str_sz--;
        }
    }

    if (end >= 0 && str[end]) {
        str_sz++;
    }
    for (st = 0; st <= end; st++, str++) {
        trstr[st] = *str;
    }
    trstr[str_sz < 1 ? 0 : str_sz-1] = '\0'; 

    return trstr;
}

int isEmptyStr(char* str, const int is_blnk_flg, char* trstr) 
{
    if (str == NULL)
        return TRUE;

    if (!*str) 
    {
        if (is_blnk_flg > FALSE && trstr) {
            *trstr = '\0';
        }
        return TRUE;
    }

    if (is_blnk_flg < TRUE)
        return FALSE;

    int   strsz = -1;
    char* trtmp = trstr;

    if (!trtmp) {
        strsz = strlen(str);
        trtmp = malloc((strsz + 1) *sizeof(char));
    }
    return !*trim(str, strsz, trtmp, FTRIM);
}

int isDigitStr(char* str, const int is_zero_flg, const int len_mode_flg) 
{
    if (!str) { return FALSE; }
    int len = strlen(str), dot_cnt = 0, i = 0;
    if (!len) { return FALSE; }

    if (len_mode_flg > FALSE) 
    {
        while (i < len && (str[i] == ' ' || str[i] == '\t')) { i++; }    // ignore leading spaces

        if (i == len) { return FALSE; }
    }

    for (; i < len; i++) 
    {
        if (len_mode_flg > FALSE && (str[i] == ' ' || str[i] == '\t')) {
            break;
        }
        if (!isdigit(str[i]) && (len_mode_flg < TRUE || *str != '-' && str[i] != '.') || isdigit(str[i]) && is_zero_flg > FALSE && str[i] != '0') {
            return FALSE;
        } 
        if (i > 0 && str[i] == '-' || str[i] == '.' && ++dot_cnt > 1) {
            return FALSE;
        }
    }

    if (len_mode_flg > FALSE) 
    {
        while (i < len && (str[i] == ' ' || str[i] == '\t')) { i++; }     // ignore trailing spaces
    }

    return i == len;
}

int isPrintStr(char* str, const int len_mode_flg) 
{
    if (!str) { return TRUE; }
    if (!*str) { FALSE; }

    for (int i=0; i < strlen(str); i++) 
    {
        if (len_mode_flg && isprint(str[i]))
            return TRUE;
        else 
        if (!(len_mode_flg || isprint(str[i])))
            return FALSE;
    }

    return !len_mode_flg;
}


/*******************************************************************/
/******************** Rudimentary I/O Functions ********************/
/*******************************************************************/

void flush() {
    while (getchar() != '\n');  // clear console input stream (remove unconsumed input characters)
}

char *readChars(char* input, int input_sz, FILE* stream) {
    char *result = NULL;

    if (stream && input && input_sz > 0) {

        int spn_len, retry = 0;

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
    char str_input [input_sz];

    char* result = readChars(str_input, input_sz, stream);

    if (result && isDigitStr(str_input, FALSE, FALSE)) {
       *input = atoi(str_input);
        return input;
    }
    return NULL;
}

void* freadVal(void* input, const char* fmt, FILE* fptr) {
    int result = 0, retry = 0;
    if (fptr) 
    {
        result = fscanf(fptr, fmt, input); 

        while (result < 0 && retry++ < FILE_READ_FRQ) {
            msleep (FILE_READ_LAT);
            result = fscanf(fptr, fmt, input);
        }
    }
    return result > 0 ? input : NULL;
}

int* freadInt(int* input, FILE* fptr) {
    return freadVal(input, "%d\n", fptr);
}

long* freadLong(long* input, FILE* fptr) {
    return freadVal(input, "%ld\n", fptr);
}

float* freadFloat(float* input, FILE* fptr) {
    return freadVal(input, "%f\n", fptr);
}

void readOption(int* input) {
    readInt(input, OPTION_MAX_SZ + 1, stdin);
}

void promptInt(const char * message, int* input, int max_digits) {  // reads an integer value on the console input stream
    printf (message);
    readInt(input, max_digits + 1, stdin);
}

void promptLn(const char * message, char* input, int input_sz) {  // reads an entire line of characters on the console input stream
    printf (message);
    readChars(input, input_sz, stdin);
}

void promptLgn(const char * message, char* input, int input_sz) {  // captures line of characters on the console input stream without displaying it
    printf(message);
    void* rfCnsl = get_console();
    void* oMode = setnoecho_console(rfCnsl);
    readChars(input, input_sz, stdin);
    restore_console(rfCnsl, oMode);
}

void pauseScr(const char * message, const int alt_msg_flg) {
    printf (message);
    if (alt_msg_flg > FALSE) {
        printf ("\nPress ENTER key to continue");
    }
    void* rfCnsl = get_console();
    void* oMode = setnoecho_console(rfCnsl);
    flush();
    restore_console(rfCnsl, oMode);
}

int scrLogInit(char* level) {
    int fresult = -1;
    char* fname = glbFname(level, TRUE);
    FILE* fptr; 

    if (fname && (fptr = fopen(fname, "w"))) {
        fresult = fprintf(fptr, "*************** %s ***************", glbTitle(level, TRUE));
        fclose(fptr);
    }
    return fresult;
}

int scrLog(const char* level, char* alt_level, int mode, int tms_mode, const int cntd_lg_flg, char* msg, ...) 
{
    if (!msg) {                                     // formatting NULL message display
        msg = LOG_NULL_VALUE? LOG_NULL_VALUE:"";
    }

    if (cntd_lg_flg && !isPrintStr(msg, TRUE))     // short-circuiting unprintable message
        return 0; 

    const char* LOG_HDR_FMT = "%s%s%s";
    const char* NL = cntd_lg_flg > FALSE ? "":"\n";
    char fmt[strlen(LOG_HDR_FMT) + strlen(msg) + 1], tms[25];
    int result = 0;

    if (cntd_lg_flg > FALSE) {
        alt_level = "";
    } else if (!alt_level) {
        alt_level = level;
    }

    if (mode < 0) 
    {  // use global log mode settings
        mode = glbMode(level);
    }

    if (tms_mode < 0) 
    {  // use global timestamp mode settings
        tms_mode = glbTMSMode(level);
    }

    if (cntd_lg_flg > FALSE) {
        tms[0] = '\0';
    } 
    else 
    if (tms_mode > LG_MODE_OFF) {
        tmstmp(&tms);
    }

    if (mode >= LG_MODE_CONSL && mode != LG_MODE_FILE) 
    {
        strcpy(fmt, LOG_HDR_FMT); strcat(fmt, msg);
        result = fprintf(stdout, fmt, NL, tms_mode >= LG_MODE_CONSL && tms_mode != LG_MODE_FILE ? tms : "", alt_level);
    }

    if (mode >= LG_MODE_FILE) 
    {
        int fresult = -1;
        char* fname = glbFname(level, FALSE);
        FILE* fptr;

        if (fname && (fptr = fopen(fname, "a"))) {
            strcpy(fmt, LOG_HDR_FMT); strcat(fmt, msg);
            fresult = fprintf(fptr, fmt, NL, tms_mode >= LG_MODE_FILE ? tms : "", alt_level);
            fclose(fptr);
        }

        if (result < 0 && fresult < 0 || result > 0 && fresult > 0) {
            result += fresult;
        } else if (fresult < 0) {
            result = fresult;
        }
    }

    return result;
}

int scrCnslLog(int t_margin, int b_margin, const char* level, char* alt_level, char* msg, int tms_mode, int scr_psd_mode, const int cntd_lg_flg) 
{
    int result; 

    printScrVMargin(t_margin);

    result = scrLog(level, alt_level, scr_psd_mode <= SCR_PSD_OFF? LG_MODE_OFF:LG_MODE_CONSL, tms_mode, cntd_lg_flg, msg);

    if (result >= 0) 
    {
        if (scr_psd_mode >= 0) {
            if (scr_psd_mode > 0) {
                printScrVMargin(b_margin);
            }
            pauseScr(NULL, scr_psd_mode);
        } 
        
        if (scr_psd_mode <= 0) {
            printScrVMargin(b_margin);
        }    
    }

    return result;
}

int scrInf(char* alt_level, char* msg, const int cntd_lg_flg) {
    return scrLog (LVL_INFO, alt_level, -1, -1, cntd_lg_flg, msg);
}

int scrCnslInf(int t_margin, int b_margin, char* alt_level, char* msg, int scr_psd_mode, const int cntd_lg_flg) {
    return scrCnslLog (t_margin, b_margin, LVL_INFO, alt_level, msg, -1, scr_psd_mode, cntd_lg_flg);
}

int scrWrn(char* alt_level, char* msg, const int cntd_lg_flg) {
    return scrLog (LVL_WARN, alt_level, -1, -1, cntd_lg_flg, msg);
}

int scrCnslWrn(int t_margin, int b_margin, char* alt_level, char* msg, int scr_psd_mode, const int cntd_lg_flg) {
    return scrCnslLog (t_margin, b_margin, LVL_WARN, alt_level, msg, -1, scr_psd_mode, cntd_lg_flg);
}

int scrErr(char* alt_level, char* msg, const int cntd_lg_flg) {
    return scrLog (LVL_ERROR, alt_level, -1, -1, cntd_lg_flg, msg);
}

int scrCnslErr(int t_margin, int b_margin, char* alt_level, char* msg, int scr_psd_mode, const int cntd_lg_flg) {
    return scrCnslLog (t_margin, b_margin, LVL_ERROR, alt_level, msg, -1, scr_psd_mode, cntd_lg_flg);
}

int scrFtl(char* alt_level, char* msg, const int cntd_lg_flg) {
    return scrLog (LVL_FATAL, alt_level, -1, -1, cntd_lg_flg, msg);
}

int scrCnslFtl(int t_margin, int b_margin, char* alt_level, char* msg, int scr_psd_mode, const int cntd_lg_flg) {
    return scrCnslLog (t_margin, b_margin, LVL_FATAL, alt_level, msg, -1, scr_psd_mode, cntd_lg_flg);
}

int scrDbg(char* alt_level, char* msg, const int cntd_lg_flg) {
    return scrLog (LVL_DEBUG, alt_level, -1, -1, cntd_lg_flg, msg);
}

int scrCnslDbg(int t_margin, int b_margin, char* alt_level, char* msg, int scr_psd_mode, const int cntd_lg_flg) {
    return scrCnslLog (t_margin, b_margin, LVL_DEBUG, alt_level, msg, -1, scr_psd_mode, cntd_lg_flg);
}


static int glbMode(char* level) 
{
    if (level == NULL)
        return -1;
    else
    if (level == LVL_WARN)
        return WARN_MODE;
    else 
    if (level == LVL_ERROR)
        return ERROR_MODE;
    else 
    if (level == LVL_FATAL)
        return FATAL_MODE;
    else 
    if (level == LVL_DEBUG)
        return DEBUG_MODE;
    else
        return INFO_MODE;
}

static int glbTMSMode(char* level) 
{
    if (level == NULL)
        return -1;
    else
    if (level == LVL_WARN)
        return WARN_TMS_MODE;
    else 
    if (level == LVL_ERROR)
        return ERROR_TMS_MODE;
    else 
    if (level == LVL_FATAL)
        return FATAL_TMS_MODE;
    else 
    if (level == LVL_DEBUG)
        return DEBUG_TMS_MODE;
    else
        return INFO_TMS_MODE;
}

static char *glbFname(char* level, const int strict_mode_flg) 
{
    if (level == NULL) {
        return SYS_LG_FILE;
    }

    char* fname;

    if (level == LVL_WARN)
        fname = WARN_LG_FILE;
    else
    if (level == LVL_ERROR)
        fname = ERROR_LG_FILE;
    else 
    if (level == LVL_FATAL)
        fname = FATAL_LG_FILE;
    else 
    if (level == LVL_DEBUG)
        fname = DEBUG_LG_FILE;
    else
        fname = INFO_LG_FILE;

    if (strict_mode_flg < TRUE && !fname) {
        fname = SYS_LG_FILE;
    }
    return fname;
}

static char *glbTitle(char* level, const int strict_mode_flg) 
{
    if (level == NULL) {
        return SYS_LG_TITLE? SYS_LG_TITLE: DEF_LG_TITLE;
    }

    char* title; 

    if (level == LVL_WARN)
        title = WARN_LG_TITLE;
    else
    if (level == LVL_ERROR)
        title = ERROR_LG_TITLE;
    else 
    if (level == LVL_FATAL)
        title = FATAL_LG_TITLE;
    else 
    if (level == LVL_DEBUG)
        title = DEBUG_LG_TITLE;
    else 
        title = INFO_LG_TITLE;

    if (strict_mode_flg < TRUE && !title) {
        title = SYS_LG_TITLE? SYS_LG_TITLE: DEF_LG_TITLE;
    }
    return title;
}

static char *tmstmp(char* tms) {
    time_t t = time(NULL);
    struct tm* tm = localtime(&t);
    strftime(tms, 25, "[%Y-%m-%d %H:%M:%S] ", tm);
    return tms;
}

static void *get_console() {

    static void *rfCnsl;
    
    if (!rfCnsl) 
    {
#if defined(_WIN32) || defined(__CYGWIN__)  // Windows OS
    
    // Get the standard input handle.
    static HANDLE hStdin; hStdin = GetStdHandle(STD_INPUT_HANDLE);

    rfCnsl = (hStdin == INVALID_HANDLE_VALUE)? NULL: hStdin;

#else   // Linux OS
    
    /* Get the standard input file descriptor. */
    static int fdStdin; fdStdin = fileno (stdin);

    rfCnsl = (fdStdin == -1)? NULL: &fdStdin;

#endif
    }
    return rfCnsl;
}

static void *setnoecho_console(void* rfCnsl) 
{
    if (!rfCnsl) return NULL;

    static void *oMode, *nMode;

#if defined(_WIN32) || defined(__CYGWIN__)  // Windows OS
    if (!oMode) 
    {
        static DWORD fdwSaveOldMode, fdwMode;

        // Save the current input mode, to be restored on exit.
        if (!GetConsoleMode(rfCnsl, &fdwSaveOldMode) )
            return NULL;
        else
            oMode = &fdwSaveOldMode;

        // Disable the console's echo mode.
        fdwMode = fdwSaveOldMode & ~ENABLE_ECHO_INPUT;
        nMode = &fdwMode;
    }

    if (!SetConsoleMode(rfCnsl, *(DWORD*)nMode) )
        return NULL;
    else
        return oMode;

#else   // Linux OS
    if (!oMode) 
    {
        static struct termios old, new;

        /* Backup current terminal state */
        if (tcgetattr (*(int*)rfCnsl, &old) != 0)
            return NULL;
        else
            oMode = &old;

        /* Turn echoing off and fail if we can’t. */  
        new = old;
        new.c_lflag &= ~ECHO;
        nMode = &new;
    }

    if (tcsetattr (*(int*)rfCnsl, TCSAFLUSH, (struct termios*)nMode) != 0)
        return NULL;
    else
        return oMode;

#endif
}

static void *restore_console(void* rfCnsl, void* oMode) 
{
    if (!oMode) return NULL;

#if defined(_WIN32) || defined(__CYGWIN__)  // Windows OS

    // Restore input mode on exit.
    if (!SetConsoleMode(rfCnsl, *(DWORD*)oMode))
        return NULL;
    else
        return rfCnsl;

#else   // Linux OS
    
    /* Restore terminal. */
    if (tcsetattr (*(int*)rfCnsl, TCSAFLUSH, (struct termios*)oMode) != 0)
        return NULL;
    else
        return rfCnsl;

#endif
}
