#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>      // For isprint(), isspace() & isdigit() functions 
#if defined(_WIN32) || defined(__CYGWIN__)
#include <windows.h>    // For Windows Sleep() function and getpass() implementation
#else
#include <unistd.h>     // For Linux sleep() function
#include <termios.h>    // For getpass() implementation
#endif

// Log level enumeration
#define LVL_INFO ""
#define LVL_WARN "Warning: "
#define LVL_ERROR "ERROR: "
#define LVL_FATAL "FATAL: "
#define LVL_DEBUG "DEBUG: "

// Log message type enumeration
#define FILE_UNREADABLE 1
#define FILE_CORRUPT 2
#define SYS_ERROR 3

// Trim mode enumeration
#define LTRIM -1
#define FTRIM  0
#define RTRIM  1

// Screen display properties (measured in characters)
static int SCR_SIZE = 120;
static int SCR_PADDING = 5;

// Synchronized read settings
static int FILE_READ_FRQ = 20;      // no. of retries upon read failure
static int FILE_READ_LAT = 50;      // wait interval between each retry (in milliseconds)

// Debug log level activation flag
static int DEBUG_MODE = 0;

// Mandatory function prototype declarations
static void *get_console();
static void *setnoecho_console(void*);
static void *restore_console(void*, void*);


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
    printf("%*s", hMargin, "");
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
                if (ovr_bnd_flg > 0 && cap_sz % pat_sz > 0)
                    reps++;
            }
            printScrHMargin(margin);
            printScrPat(NULL, pattern, reps, "\n");
        }  
    }
}

int printScrHeader(const char* col_txt1, int col_sz1, const char* col_txt2, int col_sz2,   // supports up to 5 columns
                   const char* col_txt3, int col_sz3, const char* col_txt4, int col_sz4, 
                   const char* col_txt5, int col_sz5, const char* pattern, int margin) 
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

    printScrTopic("", hdr_sz, pattern, margin, 1);

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

    if (tr_mode <= 0) 
    {
        st = 0;
        while (st++ < end && isspace(*str)) {
            str++; str_sz--;
        }
        end = str_sz - 1;
    }

    if (tr_mode >= 0) {
        while (end >= 0 && isspace(str[end])) {
            end--; str_sz--;
        }
    }

    if (end >= 0 && str[end])
        str_sz++;

    for (st = 0; st <= end; st++, str++) {
        trstr[st] = *str;
    }
    trstr[str_sz < 1 ? 0 : str_sz-1] = '\0'; 

    return trstr;
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


/*******************************************************************/
/******************** Rudimentary I/O Functions ********************/
/*******************************************************************/

void flush() {
    while (getchar() != '\n');  // clear console input stream (remove unconsumed input characters)
}

char *readChars(char* input, int input_sz, FILE* stream) {
    char *result; int spn_len, retry = 0;

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

long* freadLong(long* input, FILE* fptr) {
    return freadVal(input, "%ld\n", fptr);
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

void promptLgn(const char * message, char* input, int input_sz) {  // captures line of characters on the console input stream without displaying it
    printf(message);
    void* rfCnsl = get_console();
    void* oMode = setnoecho_console(rfCnsl);
    readChars(input, input_sz, stdin);
    restore_console(rfCnsl, oMode);
}

void pauseScr(const char * message, const int alt_msg_flg) {
    printf (message);
    if (alt_msg_flg > 0) {
        printf ("\nPress ENTER key to continue");
    }
    void* rfCnsl = get_console();
    void* oMode = setnoecho_console(rfCnsl);
    flush();
    restore_console(rfCnsl, oMode);
}

void log(char* level, int msg_type, char* msg_arg, char* msg_pst, FILE* ostream) {

    if (!level) level = LVL_INFO;
    if (!msg_pst) msg_pst = "";

    if (level != LVL_DEBUG || DEBUG_MODE)
        switch(msg_type) {
            case FILE_UNREADABLE:
                fprintf(ostream, "%s%s does not exist or cannot be opened. %s", level, msg_arg, msg_pst);
                break;
            case FILE_CORRUPT:
                fprintf(ostream, "%s%s is empty, corrupted or cannot be read. %s", level, msg_arg, msg_pst);
                break;
            case SYS_ERROR:
                fprintf(ostream, "%sAn unexpected system error has occurred! %s", level, msg_pst);
                break;
            default:
                fprintf(ostream, "%s%s", level, msg_pst);
        }
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
        if (! GetConsoleMode(rfCnsl, &fdwSaveOldMode) )
            return NULL;
        else
            oMode = &fdwSaveOldMode;

        // Disable the console's echo mode.
        fdwMode = fdwSaveOldMode & ~ENABLE_ECHO_INPUT;
        nMode = &fdwMode;
    }

    if (! SetConsoleMode(rfCnsl, *(DWORD*)nMode) )
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
    if (! SetConsoleMode(rfCnsl, *(DWORD*)oMode))
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
