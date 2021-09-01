#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <ctype.h>
#include <sys/wait.h>
#define MAX_HOSTNAME 256
#define IGNORE_NULL_CMD 1
#define INIT_IS_ACTIVE 1
#define INIT_PID -1
#define BAR "***********************************************************"
#define NUM_SEP 2
#define INIT_SIZE 4
#define INC_FACTOR 2
#define MAX_PIPES 1024

// SUBMITTED BY: UMESH YADAV (2018UCS0078) IIT JMU

// CODE contains utilities as well (coded by me) which were used in the shell
// Main/ Core functions of shell starts from end. So better view the code flow from bottom to top

// Utility for handling to skip double quotes in cmd
int skip_single_quote(const char *string, int start) {
    start++;
    int len = strlen(string);
    while (start <= len-1 && string[start] != '\'') start++;
    if (start == len) return -1;
    return start;
}

// Utility for handling to skip single quotes in cmd
int skip_double_quote(const char *string, int start) {
    start++;
    int len = strlen(string);
    while (start <= len - 1 && (string[start] != '"' || string[start-1] =='\\')) start++;
    if (start == len) return -1;
    return start;
}

// Utility for handling to skip quotes in cmd
int skip_quote(const char *string, int start, int length, const char ignore_char, char **msg) {
    if ((start < length) && ((start == 0 && string[start] == '\'') || (string[start] == '\'' && string[start-1] != ignore_char)))
        start = skip_single_quote(string, start);
    if (start < 0) { if (msg) *msg = "' is missing at end. "; return start; }
    if ((start < length) && ((start == 0 && string[start] == '"') || (string[start] == '"' && string[start-1] != ignore_char)))
        start = skip_double_quote(string, start);
    if (start < 0) if (msg) *msg = "\" is missing at end. ";
    return start;
}

// Utility function to get first string
char *get_first_string_out(const char *string, const char stopper, const char ignore_char, int include_stopper, int want_quotation) {
    if (!string|| !strlen(string)) return NULL;
    // i -> first occurance of stopper + 1 and then stores len of modified string
    int i = 0, len = strlen(string);
    if (string[i] == stopper) i++;
    else while (i < len) {
        if (want_quotation) i = skip_quote(string, i, len, ignore_char, NULL);
        if (want_quotation && i < 0) return 0;      
        if (i < len) if (string[i++] == stopper && string[i-2] != ignore_char) break;
    }
    char *first_string = malloc((i + 1) * sizeof(*first_string));
    // if memory not allocated
    if (!first_string) return NULL;
    for (int j = 0; j < i; j++) first_string[j] = string[j];
    if (i != strlen(string))
        if (include_stopper) first_string[i] = '\0';
        else first_string[--i] = '\0';
    else first_string[i] = '\0';
    return first_string;
}

// 4. Get First String (not including Stopper)
// Utility funciton to get first string till stopper (not including it)
char * get_first_string_till_out(const char *string, const char stopper, const char ignore_char, int want_quotation) {
    return get_first_string_out(string, stopper, ignore_char, 0, want_quotation);
}

// VECTOR Utilities for storing PROCESSES

// we can't initialise struct in its definition it will throw error
// so, we need to define a init function for that, just like a constructor
// and also a free function just like a destructor.
typedef struct Command {
    int is_back;
    int is_active;
    pid_t pid;
    char *cmd;
} command;

// Init Command
void init_command(command *cmd_ptr) {
    cmd_ptr->is_back = 0;
    cmd_ptr->is_active = 1;
    cmd_ptr->pid = -1;
    cmd_ptr->cmd = NULL;
    return;
}

// Free Command
void free_command(command **ptr) {
    free(*ptr);
    *ptr = NULL;
}

// Free a char ptr (string)
void free_char(char **ptr) {
    free(*ptr);
    *ptr = NULL;
}

// Free a char arr *ptr (string arr)
void free_char_arr(char ***ptr) {
    free(*ptr);
    *ptr = NULL;
}

// Validate a given command
int validate_command(command **cmd_ptr, int is_alloc) {
    if (!*cmd_ptr) {
        if (is_alloc) 
            printf("Unable to allocate memory, So not able to initialise.\n");
        else printf("Unable to reallocate memory, So, not able to add element.\n");
        free_command(&(*cmd_ptr));
        return 0;
    }
    return 1;
}

// Vector to store commands
typedef struct commandVector {
    int len;
    size_t size;
    command *array;
} command_vector;

// validate a given vector
int validate_vector(command_vector *vec) {
    if (!vec) return 0;
    if (!vec->len) {
        printf("No commands exist!\n");
        return 0;
    }
    return 1;
}

// validate a given vector and index
int validate_vector_index(command_vector *vec, int index) {
    if (validate_vector(vec)) {
        if (index > vec->len || index < 0) {
        printf("Invalid Index!. So not able to update.\n");
        return 0;
        }
        return 1;  
    }
    return 0;
}

// Init Vector
void init_vector (command_vector *vec) {
     command *cmd_ptr;
    cmd_ptr = (command *) malloc(INIT_SIZE * sizeof(command));
    if (!validate_command(&cmd_ptr, 1)) return;
    vec->array = cmd_ptr; 
    for (int i = 0; i < INIT_SIZE; i++)
        init_command(&vec->array[i]);
    vec->size = INIT_SIZE;
    vec->len = 0;
    cmd_ptr = NULL;
    return;
}

// push elements (commands) into given vector
void push_back(command_vector *vec, int is_back, char *cmd) {
    // Cannot pushback without init_vector
    if (!vec) return;
    command *cmd_ptr = vec->array;
    if (vec->len == vec->size) {
        vec->size *= INC_FACTOR;
        cmd_ptr = (command *)realloc(vec->array, vec->size * sizeof(command));
        if (!validate_command(&cmd_ptr, 0)) return;
    }
    vec->array = cmd_ptr;
    vec->array[vec->len].is_back = is_back;
    vec->array[vec->len].is_active = INIT_IS_ACTIVE;
    vec->array[vec->len].pid = INIT_PID;
    char *string = malloc((strlen(cmd)+1) * sizeof(*string));
    if (string) strcpy(string, cmd);
    vec->array[vec->len].cmd = string;
    vec->len++;
    cmd_ptr = NULL;
    return;
}

// update is_active of a command with given index
void update_is_active(command_vector *vec, int index, int is_active) {
    if(!validate_vector_index(vec, index)) return;
    vec->array[index].is_active = is_active;
    return;
}

// update pid of a command with given index
void update_pid(command_vector *vec, int index, pid_t pid) {
    if(!validate_vector_index(vec, index)) return;
    vec->array[index].pid = pid;
    return;
}

// check if any command is active
int is_any_command_active(command_vector *vec) {
    if(!validate_vector(vec)) return 0;
    for (int i = 0; i < vec->len; i++)
        if (vec->array[i].is_active == 1) return 1;
    printf("No commands are active right now.\n");
    return 0;
}

// get index from given pid of a command
int get_index_from_pid(command_vector *vec, pid_t pid, int is_active) {
    if (!vec || !vec->len) return -1;
    for (int i = 0; i < vec->len; i++)
        if (vec->array[i].pid == pid && vec->array[i].is_active == is_active) return i;
    return -1;
}

// Utility function for pid all
void print_commands(command_vector *vec) {
    if(!validate_vector(vec)) return;
    for (int i = 0; i < vec->len; i++)
        printf("cmd: %s\t pid: %d\t active: %d\n",vec->array[i].cmd, vec->array[i].pid, vec->array[i].is_active);
    return;
}

// Utility function for pid current
void print_active_commands(command_vector *vec) {
    if(!validate_vector(vec)) return;
    for (int i = 0; i < vec->len; i++)
        if (vec->array[i].is_active)
            printf("cmd: %s\t pid: %d\t active: %d\n",vec->array[i].cmd, vec->array[i].pid, vec->array[i].is_active);
    return;
}

// Free Vector
void free_vector(command_vector *vec) {
    if (vec->array) for (int i = 0; i < vec->len; i++) free_char(&vec->array[i].cmd);
    free_command(&vec->array);
    vec->len = vec->size = 0;
    return;
}

// Validate a given command
int validate_string(char ***string_ptr, int is_alloc) {
    if (!*string_ptr) {
        if (is_alloc) 
            printf("Unable to allocate memory, So not able to initialise.\n");
        else printf("Unable to reallocate memory, So, not able to add element.\n");
        free_char_arr(&(*string_ptr));
        return 0;
    }
    return 1;
}

// VECTOR Utilities for storing HISTORY

// Vector to store integers
typedef struct stringVector {
    int len;
    size_t size;
    char **array;
} string_vector;

// validate a given vector
int validate_string_vector(string_vector *vec) {
    if (!vec) return 0;
    if (!vec->len) {
        printf("History is empty!\n");
        return 0;
    }
    return 1;
}

// validate a given vector and index
int validate_string_vector_index(string_vector *vec, int index) {
    if (validate_string_vector(vec)) {
        if (index > vec->len || index < 0) {
        printf("Invalid Index!. So not able to update.\n");
        return 0;
        }
        return 1;  
    }
    return 0;
}

// Init Vector
void init_string_vector (string_vector *vec) {
    char **string_ptr;
    string_ptr = malloc(INIT_SIZE * sizeof(*string_ptr));
    if (!validate_string(&string_ptr, 1)) return;
    vec->array = string_ptr; 
    for (int i = 0; i < INIT_SIZE; i++) vec->array[i] = NULL;
    vec->size = INIT_SIZE;
    vec->len = 0;
    string_ptr = NULL;
    return;
}

// push elements (indices) into given vector
void push_back_string_vector(string_vector *vec, char *value) {
    // Cannot pushback without init_vector
    if (!vec) return;
    char **string_ptr = vec->array;
    if (vec->len == vec->size) {
        vec->size *= INC_FACTOR;
        string_ptr = realloc(vec->array, vec->size * sizeof(*string_ptr));
        if (!validate_string(&string_ptr, 0)) return;
    }
    vec->array = string_ptr;
    char *string = malloc((strlen(value)+1) * sizeof(*string));
    if (string) strcpy(string, value);
    vec->array[vec->len] = string;
    vec->len++;
    string_ptr = NULL;
    return;
}

// Utility function for HISTN
void print_last_n_commands(string_vector *vec, int n) {
    if(!validate_string_vector(vec)) return;
    if (n > vec->len) n = vec->len;
    for (int i = vec->len - 1, j = 0; i >= vec->len - n; i--)
        printf("%d. %s\n", ++j, vec->array[i]);
    return;
}

// Utility function for !HISTN
char *get_last_nth_command(string_vector *vec, int n) {
    if(!validate_string_vector(vec)) return NULL;
    if (n > vec->len) {
        printf("Invalid index! %dth command not exist in History!\n", n);
        return NULL;
    }
    return vec->array[vec->len - n];
}

// Utility function for HIST FULL
void print_full_commands(string_vector *vec) {
    if(!validate_string_vector(vec)) return;
    for (int i = 0; i < vec->len; i++)
        printf("%d. %s\n", i+1, vec->array[i]);
    return;
}

// Utility function for HIST BRIEF
void print_brief_commands(string_vector *vec) {
    if(!validate_string_vector(vec)) return;
    for (int i = 0; i < vec->len; i++) {
        char *first_cmd = get_first_string_till_out(vec->array[i], ' ', '\\', 1);
        printf("%d. %s\n", i+1, first_cmd);
        free_char(&first_cmd);
    }
    return;
}

// free string vector
void free_string_vector(string_vector *vec) {
    if (vec->array) for (int i = 0; i < vec->len; i++) free_char(&vec->array[i]);
    free_char_arr(&vec->array);
    vec->len = vec->size = 0;
    return;
}

// Global home for home dir, shell_dir for shell dir, p_name for program name
char *home = NULL;
char *shell_dir = NULL;
char *p_name = NULL;
// Global
pid_t child_id = 0;
string_vector history;
command_vector processes;
// Global array
const char *valid_cmds[] = {"STOP", "pid", "HIST", "INST", "CMD", "SWITCH", NULL};
const char *valid_pid_args[] = {"all", "current", NULL};
const char *valid_hist_args[] = {"FULL", "BRIEF", "CLEAR", NULL};
const char *no_args_cmds[] = {"STOP", "INST", "CMD", "SWITCH", NULL};
char *INSTRUCTIONS[] = {
    "Use: ./a.out (set during compliation) to Start the Shell.",
    "User can type different supported commands to\n be executed by Shell.",
    "User can enter commands after the Shell prompt \n(<user>@<host>: <home_dir> > <cmd> to execute command <cmd>)",
    "Two types of commands are supported -> USER and\n SYSTEM commands.",
    "User can choose to run any SYSTEM command as a\n Background process by appending & at end of command.",
    "Background processes once started cannot be stopped\n by user until STOP command is executed.",
    "Upon Termination of any started Background process,\n User will be notified with status code and process id.",
    "Only Way to Stop the Shell is by using STOP command.\n STOP commands end all current active processes started by SHELL.",
    "By Default the current Shell Directory is HOME Directory and is\n represented by ~ in prompt.",
    "USER can change HOME Directory using SWITCH command to switch\n HOME between Defualt UNIX/LINUX HOME and SHELL Directory",
    "Currently use of ~ as HOME in commands is supported for CD commands only.",
    "USER can seperate multiple commands in a single commands by &. \nUSER can also pipes in the commands.",
    "SHELL also supports quoting and character escaping just like BASH shell.",
    "SHELL also supports redirection using > < and pipes | just like BASH shell.",
    "EG: echo hello & mkdir 'AREN\"T U' & echo hi > f.txt | wc -m -> valid command.",
    NULL
};
char *COMMANDS[] = {
    "<user>@SHELL >> <cmd> to execute command <cmd> with Shell \nwhere valid <cmd> as follows:",
    "Use: HIST<index> to view last (recent) <index> commands\n (with parameters) executed by shell.",
    "Use: !HIST<index> to execute last (recent) <index> command.",
    "Only !HIST<index> is a USER command that can be used executed\n as a Background process.",
    "Use: HIST FULL to view all previously executed commands\n (with parameters) and in Order of Execution.",
    "Use: HIST BRIEF to view all previously executed commands\n (without parameters) and in Order of Execution.",
    "Use: HIST CLEAR to remove all previously executed commands\n from HISTORY.",
    "Use: INST to view Instructions for using Shell.",
    "Use: CMD to to view valid Commands accepted by Shell.",
    "Use: SWITCH to switch HOME directory (detail given in instructions).",
    "Use: STOP to exit/ stop the Shell.",
    "Use: pid to view pid of Shell",
    "Use: pid current to veiw pid of current active process/ command",
    "Use: pid all to view active/ inactive status alongwith pid",
    "All other commands are treated as (UNIX/ LINUX) System Commands.",
    "USER can use & to seperate multiple system commands.",
    "USER can use & and | together but Pipe commands can't be run in background.",
    NULL
};

// Utility function to kill active commands
void kill_active_commands() {
    if (is_any_command_active(&processes)) {
        for (int i = 0; i < processes.len; i++) {
            if (processes.array[i].is_active) {
                printf("Killing %s\t\tpid: %d\n", processes.array[i].cmd, processes.array[i].pid);
                kill(processes.array[i].pid, SIGKILL);
            }
        }
        printf("\n");
    }
}

// 1. Set Shell Dir
// Set shell_dir assuming shell_dir is global and init with NULL
void set_shell_dir() {
    // free shell_dir first
    free_char(&shell_dir);
    shell_dir = getcwd(NULL, 0);
    if (!shell_dir) {
        printf("Error while setting shell directory! It is now default Home");
        shell_dir = malloc(strlen(getenv("HOME")) * sizeof(*shell_dir));
        if (!shell_dir) {
            printf("Error while setting shell directory! Exiting!\n");
            exit(1);
        }
        strcpy(shell_dir, getenv("HOME"));
    }
}

// 2. Set Default Home Dir
// set home to defualt home dir assuming home is global and init with NULL
void set_default_home() {
    home = malloc(strlen(getenv("HOME")) * sizeof(*home));
    if (!home) {
        printf("Error while setting home directory! Exiting!\n");
        exit(1);
    }
    strcpy(home, getenv("HOME"));
}

// 3. Set Home Dir (for switching)
// Set home dir -> assuming home is global and init with NULL
void set_home(int mode) {
    // free home first
    free_char(&home);
    // home is getenv("HOME") -> switch to shell dir
    if (mode == 0) {
        home = malloc((strlen(shell_dir) + 1) * sizeof(*home));
        if (!home) {
            printf("Error while setting home directory! Home is now default Home");
            set_default_home();
        }
        else strcpy(home, shell_dir);
    }
    // home is shell_dir -> switch to getenv("HOME")
    else set_default_home();
}

// 8. Utility function to clear screen
void clear_screen() {
    char *argv[] = {"clear", NULL};
    pid_t p;
    if ((p = fork()) < 0) exit(1);
    else if (p==0) execvp(*argv, argv);
    else wait(NULL);
    return;
}

// Utility function to search string
int char_is_in(const char sep, const char *char_arr) {
    if (!char_arr || !strlen(char_arr)) return -1;
    for (int i = 0; char_arr[i] != '\0'; i++) if (sep == char_arr[i]) return i;
    return -1;
}

// Utility function to get number of strings in array
int num_strings(char **string_arr) {
    if (!string_arr || !*string_arr || !strlen(*string_arr)) return 0;
    int j = 0;
    while(string_arr[j++] != NULL);
    return --j;
}

void print_argv(char **argv) {
    if (!argv || !*argv || !strlen(*argv)) return;
    int j = 0;
    printf("\nArgv: ");
    while(argv[j] != NULL) printf("%s,", argv[j++]);
    printf("\n");
    return;
}

// Utility function to calculate digits in int
int get_num_digits(int num) {
    int count = 1;
    while(num /= 10) count ++;
    return count;
}

// Utility function to convert string (only numeric otherwise return 0) to int
int string_to_int(const char* string) {
    if (!string || !strlen(string)) return 0;
    // convert to int
    int index = atoi(string);
    // assert with no. of digits
    int string_len = strlen(string), digits = get_num_digits(index);
    if ((index < 0 && string_len-1 == digits) || string_len == digits) return index;
    return 0;
}

// Utility function to check if string is integeric
int is_integeric(const char* string) {
    if (!string || !strlen(string)) return 0;
    int i = (!isdigit(string[0]) && string[0] == '-') ? 1 : 0;
    while (i < strlen(string)) if (!isdigit(string[i++])) return 0;
    return 1;
}

// Utility function to search string
int is_present(const char *string, const char *strings[]) {
    if (!*strings || !strlen(*strings) || !string || !strlen(string)) return 0; 
    int i = 0;
    while(strings[i]) if (strcmp(string, strings[i++]) == 0) return 1;
    return 0;
}

// 1. Utilitfy function for Header
void header() {
    printf("\n********************* Welcome to My SHELL *****************\n");
    printf("\t%% NOTE: Use this SHELL at your own Risk %%*\n");
}

// 2. Utilitfy function for Header
void footer() {
    printf("%s\n", BAR);
    printf("\t© Copyright © Umesh Yadav (2021) ©\n");
}

// 3. Utility function printing helpful details
void print_details(char *details[], char msg[]) {
    header();
    printf("%s\n", BAR);
    printf("\t<- %s TO USE SHELL ->\n", msg);
    printf("%s\n", BAR);
    for (int i = 0; details[i]; i++)
        printf("%d. %s\n", i+1, details[i]);
    footer();
    printf("%s\n", BAR);
}

// 9. slicing string, end will be included
char *slice_string_out(const char *string, int start, int end) {
    if (!string || !strlen(string)) return NULL;
    int len = strlen(string);
    if (start < 0) {
        start = len + start; // len - (-start)
        if (start < 0) start = 0;
    }
    if (start > len) start = len - 1;
    if (end < 0) {
        end = len + end; // len - (-end)
        if (end < 0) end = 0;
    }
    if (end > len) end = len - 1;
    int new_len = (start <= end) ? end - start + 1 : start - end + 1;
    char *sliced_string = malloc((new_len + 1) * sizeof(*sliced_string));
    // memory allocation failed
    if (!sliced_string) return NULL;
    if (start <= end)
        for (int i = 0; i <= (end - start); i++) sliced_string[i] = string[i + start];
    else 
        for (int i = 0; i <= (start - end); i++) sliced_string[i] = string[start - i];
    sliced_string[new_len] = '\0';
    return sliced_string;
}

// 10. slicing string from given index till end 
char *slice_string_from_out(const char *string, int start) {
    return slice_string_out(string, start, strlen(string)-1);
}

// 11. slicing string till given index from start
char *slice_string_till_out(const char *string, int end) {
    return slice_string_out(string, 0, end);
}

// count frequency of given char in string
int count_freq(const char* string, const char c, const char ignore_char, int want_quotation) {
    if (!string || !strlen(string)) return 0;
    int i = 0, len = strlen(string);
    if (string[0] == c) i++;
    int j = 1;
    while (j < len) {
        if (want_quotation) j = skip_quote(string, j, len, ignore_char, NULL);
        if (want_quotation && j < 0) return 0;  
        if (j < len) if (string[j++] == c && string[j-2] != ignore_char) i++;
    }
    return i;
}

// count frequency of a given pattern in string
int count_pattern(char *string, char *pattern, const char ignore_char, int want_quotation) {
    if (!string || !strlen(string)) return 0;
    if (!pattern || !strlen(pattern)) return 0;
    int len = strlen(string), pattern_len = strlen(pattern), j = 0, is_equal = 1, count = 0;
    while (j + pattern_len <= len) {
        if (want_quotation) j = skip_quote(string, j, len - pattern_len + 1, ignore_char, NULL);
        if (want_quotation && j < 0) return 0; 
        if (j + pattern_len <= len) {
            for (int k = j, l = 0; k < j + pattern_len; k++) {
                if (string[k] != pattern[l++]) {
                    is_equal = 0;
                    break;
                }
            }
            if (j >= 1) is_equal = is_equal && (string[j-1] != ignore_char); 
            if (is_equal) {
                count++;
                j += pattern_len;
            }
            else j++;
            is_equal = 1;
        }
    }
    return count;
}

// 8. Utility function to strip string
char *strip_string_arr_in_ignore_after(char *string, const char *stripper, char ignore_char) {
    if (!string || !strlen(string)) return string;
    int j = 0, len = strlen(string);
    while(j < len && char_is_in(string[j], stripper) >= 0) j++;
    if (j > 0) for (int i = 0; i < (len - j + 1); i++) string[i] = string[i + j];
    j = strlen(string) - 1;
    while (j >= 1 && char_is_in(string[j], stripper) >= 0 && string[j-1] != ignore_char) string[j--] = '\0';
    return string;
}

char **split_string_arr_out_ignore_after(const char *string, const char splitters[], int **sep_list, const char ignore_char, int want_quotation, int want_strip) {
    if (!string || !strlen(string)) return NULL;
    char *line = malloc((strlen(string) + 1) * sizeof(*line));
    // if memory not allocated
    if (!line) return NULL;
    strcpy(line, string);
    int num_splitter = 0, len = strlen(line), i = 0;
    for (int i = 0; splitters[i] != '\0'; i++)
        num_splitter += count_freq(string, splitters[i], '\\', 1);
    if (sep_list) *sep_list = malloc((num_splitter + 1) * sizeof(*sep_list));
    int ind = char_is_in(line[i], splitters), j = 0;
    if (ind >= 0) {
        line[i] = '\0';
        if (sep_list) (*sep_list)[j++] = ind;
    }
    while (++i < len) {
        if (want_quotation) i = skip_quote(string, i, len, ignore_char, NULL);
        if (want_quotation && i < 0) return NULL; 
        if (i < len) {
            ind = char_is_in(line[i], splitters);
            if ( ind >= 0 && line[i-1] != ignore_char) {
                line[i] = '\0';
                if (sep_list) (*sep_list)[j++] = ind;
            }
        }
    }
    if (sep_list) (*sep_list)[j] = -1;
    char **string_arr = malloc((num_splitter + 1 + 1) * sizeof(*string_arr));
    // if memory not allocated
    if (!string_arr) return NULL;
    i = 0, j = 0;
    string_arr[j] = NULL;
    const char stripper[] = {' ', '\t', '\n', '\0'};
    // line[0] must be not = '\0' to avoid seg fault error because we can't free_argv if line[0] = '\0'
    // as *string_arr will not be line[0] if line[0] = '\0'
    while (i < len && line[i] == '\0') i++;
    // if line is fully null i.e all line[i] = '\0'
    if (i == len) {
        free_char(&line);
        return string_arr;
    }
    else if (i > 0) for (int k = 0; k < len - i + 1; k++) line[k] = line[k+i];
    i = 0;
    while (i < len) {
        while (i < len && line[i] == '\0') i++;
        if (i < len) string_arr[j++] = line + i;
        if (want_strip && i < len) strip_string_arr_in_ignore_after(string_arr[j-1], stripper, ignore_char);
        while (i < len && line[i] != '\0') i++;
    }
    // end of array
    string_arr[j] = NULL;
    return string_arr;
}

char *quote_removal(char *string) {
    if (!string || !strlen(string)) return NULL;
    int len = strlen(string);
    int i = 0, j = 0; // i -> writer and j -> reader
    while (j < len) { // 0 as char is same as \0
        if (string[j] == '\'' && ((j >= 1 && string[j-1] != '\\') || (j == 0))) {
            while (++j <= len-1 && string[j] != '\'') string[i++] = string[j];
            j++;
        }
        if (j < len && string[j] == '"' && ((j >= 1 && string[j-1] != '\\') || (j == 0))) {
            char allowed_char[] = {'\\', '"', 0};
            while (++j <= len-1 && (string[j] != '"' || string[j-1] =='\\')) {
                if (string[j] != '\\' || (j+1 < len-1 && char_is_in(string[j+1], allowed_char) < 0)) {
                    string[i++] = string[j];
                }
            }
            j++;
        }
        if (j < len && string[j] == '\\' && ((j >= 1 && string[j-1] != '\\') || (j == 0))) j++;
        else if (j < len) string[i++] = string[j++];
        // j++;
    }
    string[i] = 0;  // 0 as char is same as \0 so it is same as string[i] = '\0'
    return string;
}

// 1. Free shell -> assumes history, shell_dir & home are Global
void free_shell() {
    free_char(&shell_dir);
    free_char(&home);
    free_string_vector(&history);
    free_vector(&processes);
    return;
}

// Free argv
void free_argv(char ***argv) {
    if (*argv) free_char(&(**argv));
    free_char_arr(&(*argv));
    return;
}

// Free shell cmd
void free_shell_cmd(char **line, char **prompt) {
    free_char(&(*line));
    free_char(&(*prompt));
    return;
}

int num_splitters(int *splitters_list) {
    if (!splitters_list) return 0;
    int i = 0;
    while (splitters_list[i++] >= 0);
    return --i;
}

// Command Utilities

// 4. Handle home
// Replace home by ~ in given path
void modify_path(const char *home, char *path) {
    if (!path || !strlen(path)) return;
    if (strlen(home) <= strlen(path)) {
        int i = 0;
        while (home[i] == path[i] && i < strlen(home)) i++;
        if (i == strlen(home)) {
            path[0] = '~';
            for (int j = 1; j < (strlen(path) - i + 1); j++) path[j] = path[i + j - 1];
            path[strlen(path) - i + 1] = '\0';  
        }
    }
}

// 5. Handle ~
// Replace ~ by home in given arg [only if ~ is first char]
char *modify_arg(const char *home, char *arg) {
    if (!arg || !strlen(arg)) return NULL;
    int i = 0, j = 0;
    if (arg[i] == '~') j++;
    char *modified_arg = malloc((j * strlen(home) + strlen(arg) - j + 1) * sizeof(char));
    if (!modified_arg) return NULL;
    i = 0, j = 0; 
    if (arg[i] == '~') {
        strcpy(modified_arg, home);
        j += strlen(home);
        i++;
    }
    while (i <= strlen(arg)) modified_arg[j++] = arg[i++];
    return modified_arg;
}

// 6. Utility function to create prompt for shell
char *create_prompt(const char *home) {
    // can't do char *user = getenv("USER"); as it will modify env USER varible if user is modified
    char host_name[MAX_HOSTNAME];
    if (gethostname(host_name, sizeof(host_name)) < 0) {
        printf("Error while getting hostname. So, using 'USER' as hostname!");
        strcpy(host_name, "USER");
    }
    char *cwd = getcwd(NULL, 0);
    if (cwd == NULL) {
        printf("Error while getting hostname. So, using 'USER' as hostname!");
        *cwd = '~';
    }
    // modify_path("/Users/umeshyadav", cwd);
    modify_path(home, cwd);
    char *user = malloc((strlen(getenv("USER")) + 1 + strlen(host_name) + strlen(cwd) + strlen(": ~ > ")) * sizeof(char));
    strcpy(user, getenv("USER"));
    strncat(user, "@", 1);
    strcat(user, host_name);
    strcat(user, ": ");
    strcat(user, cwd);
    strcat(user, " > ");
    // free cwd
    free(cwd);
    cwd = NULL;
    return user;
}

// 7. Utility function to handle signal
void signal_handler(int sig_num) {
    int print_again = 0;
    if (sig_num == SIGINT) {
        printf("\nCaught the Signal! Killing the Process\n");
        if (child_id != 0) kill(child_id, SIGKILL);
        else printf("\nUse STOP command to exit\n");
        print_again = 1;
    }
    // read here: https://www.geeksforgeeks.org/zombie-processes-prevention/
    else if (sig_num == SIGCHLD) {
        // read here: IMP: https://stackoverflow.com/a/2596788
        pid_t p_id;
        int wstatus;
        while ((p_id = waitpid(-1, &wstatus, WNOHANG)) > 0) {
            // printf("i am pid: %d\n", p_id);
            int ind = get_index_from_pid(&processes, p_id, 1);
            if (ind >=0 && processes.array[ind].is_back) {
                update_is_active(&processes, ind, 0);
                char *msg = NULL;
                int status = -1;
                if (WIFEXITED(wstatus)) {msg = "finished with status: "; status = WEXITSTATUS(wstatus);}
                else if (WIFSIGNALED(wstatus)) {msg = "killed by signal with status: "; status = WTERMSIG(wstatus);}
                else if (WIFSTOPPED(wstatus)) {msg = "stopped by signal with status: "; status = WSTOPSIG(wstatus);}
                else if (WIFCONTINUED(wstatus)) {msg = "continued";}
                else msg = "exited";
                printf("cmd: %s\t pid: %d\t %s", processes.array[ind].cmd, p_id, msg);
                if (status >= 0) printf("%d\n", status);
                else printf("\n");
                print_again = 1;
            }
        }
    }
    // below helps in presenting prompt at right position
    fflush(stdout); // can be removed
    if (print_again) {
        char *prompt = create_prompt(home);
        printf("\n%s", prompt);
        free_char(&prompt);
        fflush(stdout); // must be here after displaying prompt above 
        // (check codevault tutorial for signal handling for more details)
    }
    return;
}

// 2. Handle CD system command (CD not works in pipe commands)
void handle_cd(char **cmd_argv) {
    int num_args = num_strings(cmd_argv);
    if (num_args > 1) {
        char *dir = modify_arg(home, cmd_argv[1]);
        if ((num_strings(cmd_argv) > 2) || chdir(dir) < 0) fprintf(stderr, "SHELL: Failed to change directory to %s\n", dir);
        free_char(&dir);
    }
    return;
}

char **handle_redirects(char *cmd) {
    const char splitters[] = {'>', '<', '\0'};
    int *splitters_list;
    char **cmd_argv = split_string_arr_out_ignore_after(cmd, splitters, &splitters_list, '\\', 1, 1);
    int num_args = num_strings(cmd_argv);
    int num_redirects = num_splitters(splitters_list);
    int fd = -2, i = 0, j = 0;
    int spaces = count_freq(cmd, ' ', '\\', 1);
    char **argv = malloc((count_freq(cmd, ' ', '\\', 1) + 1 + 1) * sizeof(*argv));
    if (!argv) return NULL;
    argv[j] = NULL;
    while (i < num_args) {
        const char splitter[] = {' ', '\0'};
        char **args = split_string_arr_out_ignore_after(cmd_argv[i], splitter, NULL, '\\', 1, 1);
        int num = num_strings(args);
        if (i == 0 || num > 1) {
            for (int k = 0; k < num; k++)
                if (i == 0 || k != 0)
                    argv[j++] = args[k];
            if (i == 0) {
                i++;
                continue;
            }
        }        
        fd = -2;
        if (splitters_list[i-1] == 0)
            fd = open(args[0], O_WRONLY | O_CREAT | O_TRUNC, 0777);
        else if (splitters_list[i-1] == 1)
            fd = open(args[0], O_RDONLY, 0777);
        if (fd == -1) {
            fprintf(stderr, "Opening file: %s failed\n", args[0]);
            free_argv(&args);
            free_argv(&cmd_argv);
            free_argv(&argv);
            return NULL;
        }
        if (fd != -2 && splitters_list[i-1] == 0) dup2(fd, STDOUT_FILENO);
        else if (fd != -2) {
            dup2(fd, STDIN_FILENO);
            close(fd);
        }
        i++;
    }
    argv[j] = NULL;
    int num = num_strings(argv);
    for (int i = 0; i < num; i++) quote_removal(argv[i]);
    return argv;
}

// 3. Utility function to execute commands using execvp
void execute_cmd_execvp(char *simple_cmd, int is_back) {
    if (!simple_cmd || !strlen(simple_cmd)) return;
    char *cmd_name = get_first_string_till_out(simple_cmd, ' ', '\\', 1);
    // Create child process to execute cmd
    pid_t pid = fork();
    // Fork failed
    if (pid < 0) {
        printf("Error occured while using fork! Exiting\n");
        exit(1);
    }
    // Child Process
    else if (pid == 0) { 
        if (is_back) { // -> assumes & is stripped from cmd_argv
            printf("\ncmd: %s\t pid: %d\t started in background\n", cmd_name, getpid());
            // fflush(stdout);
            setpgid(0, 0);
        }
        if (strcmp(cmd_name, "cd") != 0)  {// if not cd
            // Call execvp to execute cmd
            char **cmd_argv = handle_redirects(simple_cmd);
            if (!cmd_argv) exit(1);
            if (execvp(*cmd_argv, cmd_argv) < 0) fprintf(stderr, "SHELL: %s command not found\n", *cmd_argv);
        }
        exit(1);
    }
    // Parent Process
    else {
        // Update pid in history -> assumes cmd is added to history
        update_pid(&processes, processes.len - 1, pid);
        // IMP: chdir can change dir of calling process only: https://stackoverflow.com/a/56031268
        // so calling it in parent process i.e main process of our shell
        if (strcmp(cmd_name, "cd") == 0) {
            char **cmd_argv = handle_redirects(simple_cmd);
            handle_cd(cmd_argv);
        }
        if (!is_back) {
            // read here: https://stackoverflow.com/a/42107099
            // assign p_id as fork() return spawned child's process id to parent
            child_id = pid;
            // Wait till 'this' child finish its process
            waitpid(pid, NULL, 0);
            // Update is_active to 0 in history -> assumes cmd is added to history
            update_is_active(&processes, processes.len - 1, 0);
            // Reinitialise child_id
            child_id = 0;
        }
        return;
    }
}

// Utility function to handle system commands
void simple_cmd_handler(char *simple_cmd, int is_back) {
    if (!simple_cmd || !strlen(simple_cmd)) return;
    // Execute cmd using execvp
    push_back(&processes, is_back, simple_cmd);
    execute_cmd_execvp(simple_cmd, is_back);
    return;
}

// Utility function to execute pipes
void execute_pipes(char **cmds) {
    int n = num_strings(cmds);
    // array for storing pids
    pid_t pids[MAX_PIPES+1] = {0};
    // 2 file descriptors for n-1 pipes for <cmd1> | <cmd2> | .... <cmdn-1> | <cmdn> -> n-1 pipes
    int pipe_fd[MAX_PIPES][2];
    for (int i = 0; i < n-1; i++) {
        if (pipe(pipe_fd[i]) < 0) {
            printf("Pipe creation failed! for pipe_fd[%d]\n", i);
            for (int j = 0; j < i; j++) {
                close(pipe_fd[j][0]);
                close(pipe_fd[j][1]);
            }
            return;
        }
    }
    // n child processes for n commands
    for (int i = 0; i < n; i++) {
        pids[i] = fork();
        if (pids[i] < 0) {
            printf("Fork failed for child[%d]\n", i);
        }
        else if (pids[i] == 0) {
            // signal(SIGPIPE, signal_handler);
            if (i == 0)
                dup2(pipe_fd[i][1], STDOUT_FILENO);
            else if (i == n-1)
                dup2(pipe_fd[i-1][0], STDIN_FILENO);
            else {
                dup2(pipe_fd[i-1][0], STDIN_FILENO);
                dup2(pipe_fd[i][1], STDOUT_FILENO);
            }
            for (int j = 0; j < n-1; j++) {
                close(pipe_fd[j][0]);
                close(pipe_fd[j][1]);
            }
            char **cmd_argv = handle_redirects(cmds[i]);
            if (!cmd_argv) exit(1);
            if (execvp(*cmd_argv, cmd_argv) < 0) 
                // read: https://stackoverflow.com/a/39002219
                fprintf(stderr, "SHELL: %s command not found\n", *cmd_argv);
            exit(1);
        }
    }
    child_id = pids[n-1];
    // close pipe in parent
    for (int j = 0; j < n-1; j++) {
        close(pipe_fd[j][0]);
        close(pipe_fd[j][1]);
    }
    // update processes in history
    int ind = 0;
    for (int j = 0; j < n; j++) {
        //ind = get_index_from_pid(&processes, pids[n-1], 1); // do this when is_back is supported for pipe cmds
        ind = processes.len - (n-j);
        update_pid(&processes, ind, pids[j]);
    }
    // parent process
    int w_status, i = n-2;
    pid_t pid;
    // wait for last child
    pid = waitpid(pids[n-1], &w_status, 0);
    // ind = get_index_from_pid(&processes, pids[n-1], 1); // do this when is_back is supported for pipe cmds
    ind = processes.len - 1;
    update_is_active(&processes, ind, 0);
    // printf("Child with pid: %d exited with status: %d\n", pid, w_status);
    for (int i = 0; i < n-1; i++) {
        kill(pids[i], SIGKILL);
        // printf("Process with pid: %d exited with SIGPIPE\n", pids[i]);
        // ind = get_index_from_pid(&processes, pids[n-1], 1); // do this when is_back is supported for pipe cmds
        ind = processes.len - (n-i);
        update_is_active(&processes, ind, 0);
    }
    child_id = 0;
    free_argv(&cmds);
    return;
}

// Utiltity function to handle piped commands
void pipe_cmd_handler(char *pipe_cmd, int is_back) {
    if (!pipe_cmd || !strlen(pipe_cmd)) return;
    const char splitter[] = {'|', '\0'};
    char **cmds = split_string_arr_out_ignore_after(pipe_cmd, splitter, NULL, '\\', 1, 1);
    int num_cmds = num_strings(cmds);
    for (int i = 0; i < num_cmds; i++)
        push_back(&processes, is_back, cmds[i]);
    execute_pipes(cmds);
    return;
}

// below definition is required when && and || is allowed (not now)
// void **cmd_handler(char *cmd, int *is_simple, int *is_back)
void cmd_handler(char *cmd);

// 7. Utility function to handle user commands
void user_cmd_handler(char **argv, int is_back) {
    if (!*argv || !strlen(*argv) || !**argv || !strlen(*argv)) return;
    char *cmd = argv[0];
    int num_args = num_strings(argv);
    if (strcmp(cmd, "pid") == 0) {
        if (num_args == 1) printf("cmd: %s\t pid: %d\n", p_name, getpid());
        else if (strcmp(argv[1], "all") == 0) print_commands(&processes);
        else if (is_any_command_active(&processes)) print_active_commands(&processes);
    }
    else if (strcmp(cmd, "HIST") == 0) {
        if (strcmp(argv[1], "FULL") == 0) print_full_commands(&history);
        else if (strcmp(argv[1], "BRIEF") == 0) print_brief_commands(&history);
        else {
            free_string_vector(&history);
            init_string_vector(&history);
        }
    }
    else if (strcmp(cmd, "INST") == 0) print_details(INSTRUCTIONS, "Instructions");
    else if (strcmp(cmd, "CMD") == 0) print_details(COMMANDS, "Commands");
    else if (strcmp(cmd, "SWITCH") == 0) {
        set_home(strcmp(home, getenv("HOME")));
        printf("Home dir is changed to %s\n", home);
    }
    else if (num_args == 1) {
        int sep_ind = (cmd[0] == '!') ? strlen("!HIST") - 1 : strlen("HIST") - 1;
        char *first = slice_string_till_out(cmd, sep_ind);
        char *last = slice_string_from_out(cmd, sep_ind + 1);
        int index = string_to_int(last);
        if (strcmp(first, "HIST") == 0) print_last_n_commands(&history, index);
        else {
            char *cmd = get_last_nth_command(&history, index);
            if (cmd) cmd_handler(cmd);
        }
        free_char(&first);
        free_char(&last);
    }
    // stop command is handled seperately at begin in user mode
    return;
}

// 8. Utility function to validate user commands
int is_user_cmd(char *line, int is_back) {
    if (!line || !strlen(line)) return 0; 
    const char stripper[] = {' ', '\0'};
    char **argv = split_string_arr_out_ignore_after(line, stripper, NULL, '\\', 1, 1);
    char *cmd = argv[0];
    int num_args = num_strings(argv);
    int i = 0, is_user = 0, is_valid = 0, is_hist = 0;
    if (num_args == 1) {
        const char *temp[] = {"HIST", "!HIST", NULL};
        int sep_ind = (cmd[0] == '!') ? strlen("!HIST") - 1 : strlen("HIST") - 1;
        char *first = slice_string_till_out(cmd, sep_ind);
        char *last = slice_string_from_out(cmd, sep_ind + 1);
        is_hist = is_present(first, temp) ? 1 : 0;
        if (is_hist) {
            // below is must condition to check for is_vaild 
            is_valid = is_hist && is_integeric(last) && string_to_int(last) > 0 ? 1 : 0;
            if (!is_valid && is_hist) printf("%s<index> requires a positive integral index! Try Again!\n", first);
            // !HIST<index> can be background cmd
            is_valid = (is_valid && is_back && strcmp(first, "HIST") == 0) ? 0 : is_valid;
            if (!is_valid && is_back) printf("Only !HIST<index> USER command can be executed with &. Try Again!\n");
        }
        free_char(&first);
        free_char(&last);
    }
    is_user = is_hist ? 1 : 0;
    if (!is_user) while(valid_cmds[i]) if (strcmp(cmd, valid_cmds[i++]) == 0) is_user = 1;
    if (is_user && !is_hist) {
        if (strcmp(cmd, "pid") == 0) {
            is_valid = (num_args == 1 || num_args == 2 && is_present(argv[1], valid_pid_args)) ? 1 : 0;
            if (!is_valid) printf("pid USER command expects all OR current as argument! Try Again!\n");
        }
        else if (strcmp(cmd, "HIST") == 0) {
            is_valid = (num_args == 2 && is_present(argv[1], valid_hist_args)) ? 1 : 0;
            if (!is_valid) printf("HIST user command expects FULL or BRIEF or CLEAR as argument! Try Again!\n");
        }
        else if (is_present(cmd, no_args_cmds)) is_valid = (num_args == 1) ? 1 : 0;
        if (is_back) printf("Only !HIST<index> USER command can be executed with &. Try Again!\n");
    }
    // No user cmd can be background cmd except history cmd (!HIST<index>)
    is_valid = (is_user && !is_hist && is_back) ? 0 : is_valid;
    if (is_user && is_valid) user_cmd_handler(argv, is_back);
    return is_user;
}

// assumes cmd given is valid and is stripped
void cmd_handler(char *cmd) {
    if (!cmd || !strlen(cmd)) return;
    const char splitter[] = {'&', '\0'};
    char **pipe_cmds = split_string_arr_out_ignore_after(cmd, splitter, NULL, '\\', 1, 1);
    int num_pipes_cmds = num_strings(pipe_cmds);
    int is_back = 0, is_simple = 0;
    if (num_pipes_cmds == 1) {
        if (is_user_cmd(pipe_cmds[0], cmd[strlen(cmd)-1] == '&'))
            return;
    }
    push_back_string_vector(&history, cmd);
    for (int i = 0; i < num_pipes_cmds; i++) {
        is_simple = !count_pattern(pipe_cmds[i], "|", '\\', 1);
        is_back = (i != num_pipes_cmds-1) || (cmd[strlen(cmd)-1] == '&');
        if (is_simple) simple_cmd_handler(pipe_cmds[i], is_back);
        else pipe_cmd_handler(pipe_cmds[i], is_back);
    }
    free_argv(&pipe_cmds);
    return;
}

// error handler
int error_handler(char *cmd) {
    const char stripper[] = {' ', '\t', '\n', 0};
    char *msg = NULL;
    int is_valid = 1, i =0, j = 0, k = 0;
    strip_string_arr_in_ignore_after(cmd, stripper, '\\');
    int is_null = cmd == NULL || *cmd == '\0' || *cmd == ' ' || *cmd == '\t' || *cmd == '\n';
    is_valid = !is_null;
    // check for quotation
    int l = 0, len = strlen(cmd);
    while (l < len && is_valid) {
        l = skip_quote(cmd, l, len, '\\', &msg);
        if (l++ < 0) is_valid = 0;
    }
    if (is_valid && count_pattern(cmd, "&&", '\\', 1)) {
        msg = "&& is not allowed in the command. ";
        is_valid = 0;
    }
    if (is_valid) {
        const char splitter[] = {'&', '\0'};
        char **pipe_cmds = split_string_arr_out_ignore_after(cmd, splitter, NULL, '\\', 1, 1);
        int num_pipes_cmds = num_strings(pipe_cmds);
        if (num_pipes_cmds == 0) {
            msg = "& expects a command before it -> <cmd> &.";
            is_valid = 0;
        }
        if (is_valid && count_pattern(cmd, "||", '\\', 1)) {
            msg = "|| is not allowed in command. ";
            is_valid = 0;
        }
        if (is_valid) {
            const char splitter[] = {'|', '\0'};
            char **cmds = NULL;
            int is_simple = 0;
            int is_back = 1;
            for (i = 0; i < num_pipes_cmds; i++) {
                if (!strcmp(pipe_cmds[i],"\0") || pipe_cmds[i] == NULL || pipe_cmds[i][0] == '|' || pipe_cmds[i][strlen(pipe_cmds[i])-1] == '|') {
                    if (!strcmp(pipe_cmds[i],"\0") || pipe_cmds[i] == NULL) 
                        msg = "& expects a command before it -> <cmd> &.";
                    else if (pipe_cmds[i][0] == '|' || pipe_cmds[i][strlen(pipe_cmds[i])-1] == '|') 
                        msg = "| expects commands before and after it -> <cmd1> | <cmd2>. ";
                    is_valid = 0;
                    break;
                }
                is_simple = (!count_pattern(pipe_cmds[i], "|", '\\', 1));
                is_back = (i != num_pipes_cmds-1) || (cmd[strlen(cmd)-1] == '&');
                if (!is_simple && is_back) {
                    msg = "Running Pipe commands in Background is not currently supported by the shell.";
                    is_valid = 0;
                    break;
                }
                cmds = split_string_arr_out_ignore_after(pipe_cmds[i], splitter, NULL, '\\', 1, 1);
                int num_cmds = num_strings(cmds);
                if (num_cmds == 0) {
                    msg = "| expects a command before and after it -> <cmd1> | <cmd2>. ";
                    is_valid = 0;
                    break;
                }
                char **args = NULL;
                const char splitters[] = {'>', '<', '\0'};
                for (j = 0; j < num_cmds; j++) {
                    if (!strcmp(cmds[j], "\0") || cmds[j] == NULL || count_pattern(cmds[j], "<<", '\\', 1) || count_pattern(cmds[j], ">>", '\\', 1)) {
                        if (!strcmp(cmds[j], "\0") || cmds[j] == NULL) msg = "| expects a command before it -> <cmd1> |. ";
                        else if (count_pattern(cmds[j], "<<", '\\', 1)) msg = "<< is not allowed in the command. ";
                        else if (count_pattern(cmds[j], ">>>", '\\', 1)) msg = ">>> is not allowed in the command. ";
                        is_valid = 0;
                    }
                    if (cmds[j][0] == '>' || cmds[j][strlen(cmds[j])-1] == '>' || cmds[j][0] == '<' || cmds[j][strlen(cmds[j])-1] == '<') {
                        if (cmds[j][0] == '>') msg = "> expects a command before it -> <cmd> > <file>. ";
                        else if (cmds[j][strlen(cmds[j])-1] == '>') msg = "> expects a file after it -> <cmd> > <file>. ";
                        else if (cmds[j][0] == '<') msg = "< expects a command before it -> <cmd> < <file>. ";
                        else msg = "< expects a file after it -> <cmd> < <file>. ";
                        is_valid = 0;
                    }
                    args = split_string_arr_out_ignore_after(cmds[j], splitters, NULL, '\\', 1, 1);
                    int num_args = num_strings(args);
                    if (num_args == 0) {
                        msg = "> and < expects a non blank command. ";
                        is_valid = 0;
                        break;
                    } 
                    // print_argv(args);
                    for (k = 0; k < num_args; k++) {
                        if (!strlen(args[k]) ||!args[k]) {
                            msg = "> and < expects a command before them and a file after them. ";
                            is_valid = 0;
                            break;
                        }
                    }
                    free_argv(&args);
                    if (!is_valid) break;
                    k = 0;
                }
                free_argv(&cmds);
                if (!is_valid) break;
                j = 0;
            }
        }
        free_argv(&pipe_cmds);
    }
    if (!is_valid && !is_null) {
        printf("Invalid Command Entered Try Again!\n");
        printf("Error is found at pipe_cmd num: %d and cmd_num: %d\n", i+1, j+1);
        if (msg) printf("%s\n", msg);
    }
    return is_valid;
}

// E. Main Utilities

// 1. Utility function for user_interactive mode in Shell
void shell(int ignore_null_cmd) {
    while(1) {
        // prompt
        char *prompt = create_prompt(home);
        printf("\n%s", prompt);
        // take input and -> imp read here for strings in c: https://www.studymite.com/blog/strings-in-c
        char *line = NULL;
        size_t len = 0;
        if (getline(&line, &len, stdin) == -1) {
            printf("Error Occured while taking input! Try Again!\n");
            free_shell_cmd(&line, &prompt);
            continue;
        }
        // Also handling exception case below: 
        int is_valid = error_handler(line);
        if (is_valid) {
            if (strcmp(line, "STOP") == 0) {
                free_shell_cmd(&line, &prompt);
                return;
            }
            cmd_handler(line);
            free_shell_cmd(&line, &prompt);
        }
    }
}

// 2. Main function
int main(int argc, char *argv[]) {
    // sigaction
    struct sigaction sa;
    sa.sa_handler = &signal_handler;
    sa.sa_flags = SA_RESTART;
    // Register signal
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGCHLD, &sa, NULL);

    // set program name for pid command
    if (argc > 0) p_name = argv[0];
    else p_name = "./a.out";

    // init history vector for storing commands
    init_vector(&processes);
    init_string_vector(&history);

    // Set shell dir
    set_shell_dir();

    // Set home dir
    set_home(0);

    // Print Instructions
    print_details(INSTRUCTIONS, "Instructions");
    printf("Wait for 15 sec. Read Instructions Carefully. OR press CTRL+C to continue\n");
    sleep(15);
    clear_screen();

    // Print Commands
    print_details(COMMANDS, "Commands");
    printf("Wait for 15 sec. Read Commands Carefully. OR press CTRL+C to continue\n");
    sleep(15);
    clear_screen();

    // // User mode
    printf("\nWelcome to SHELL User Mode!\n");
    printf("Happy Shelling! Please enter your command after the SHELL prompt below.\n");
    shell(IGNORE_NULL_CMD);
    
    // STOP
    printf("\nThanks for using my SHELL! Bye! Exiting!\n");
    // Kill active if any
    kill_active_commands();
    printf("\n");
    // Free
    free_shell();
    return 0;
}
