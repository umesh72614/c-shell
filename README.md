# C (unix type) Shell
A shell similar to Bash (Linux) using C (system call APIs) with support of IO redirection (pipes and file handlers) and running multiple fore/ background commands along with various custom user commands.

## Usage
Clone the repository
```
git clone https://github.com/umesh72614/c-shell.git
cd c-shell
make
```
To run the shell
```
./cshell.out
```
To clean the executables
```
make clean
```

## Instructions
1. Use: ./a.out (set during compliation) to Start the Shell.
2. User can type different supported commands to\n be executed by Shell.
3. User can enter commands after the Shell prompt \n(<user>@<host>: <home_dir> > <cmd> to execute command <cmd>)
4. Two types of commands are supported -> USER and\n SYSTEM commands.
5. User can choose to run any SYSTEM command as a\n Background process by appending & at end of command.
6. Background processes once started cannot be stopped\n by user until STOP command is executed.
7. Upon Termination of any started Background process,\n User will be notified with status code and process id.
8. Only Way to Stop the Shell is by using STOP command.\n STOP commands end all current active processes started by SHELL.
9. By Default the current Shell Directory is HOME Directory and is\n represented by ~ in prompt.
10. USER can change HOME Directory using SWITCH command to switch\n HOME between Defualt UNIX/LINUX HOME and SHELL Directory
11. Currently use of ~ as HOME in commands is supported for CD commands only.
12. USER can seperate multiple commands in a single commands by &. \nUSER can also pipes in the commands.
13. SHELL also supports quoting and character escaping just like BASH shell.
14. SHELL also supports redirection using > < and pipes | just like BASH shell.
15. EG: echo hello & mkdir 'AREN\"T U' & echo hi > f.txt | wc -m -> valid command.
  
## Commands
1. <user>@SHELL >> <cmd> to execute command <cmd> with Shell \nwhere valid <cmd> as follows:
2. Use: HIST<index> to view last (recent) <index> commands\n (with parameters) executed by shell.
3. Use: !HIST<index> to execute last (recent) <index> command.
4. Only !HIST<index> is a USER command that can be used executed\n as a Background process.
5. Use: HIST FULL to view all previously executed commands\n (with parameters) and in Order of Execution.
6. Use: HIST BRIEF to view all previously executed commands\n (without parameters) and in Order of Execution.
7. Use: HIST CLEAR to remove all previously executed commands\n from HISTORY.
8. Use: INST to view Instructions for using Shell.
9. Use: CMD to to view valid Commands accepted by Shell.
10. Use: SWITCH to switch HOME directory (detail given in instructions).
11. Use: STOP to exit/ stop the Shell.
12. Use: pid to view pid of Shell.
13. Use: pid current to veiw pid of current active process/ command.
14. Use: pid all to view active/ inactive status alongwith pid.
15. All other commands are treated as (UNIX/ LINUX) System Commands.
16. USER can use & to seperate multiple system commands.
17. USER can use & and | together but Pipe commands can't be run in background.

## Features
Support of the following functionality:
1. Input/Output redirect with "<" and ">"
2. Pipes
3. Combination of both.

### Specification -1

**Output redirection with ">"**

```
<daksh@iitjammu:~>wc temp.c > output.txt
<daksh@iitjammu:~>cat output.txt
xx xx xx
<daksh@iitjammu:~>
```

### Specification -2

**Input redirection with "<"**

```
<daksh@iitjammu:~>wc < temp.c
xx xx xx
```

### Specification -3

**Input/Output redirection with "<" ">"**

```
<daksh@iitjammu:~>wc < temp.c > output.txt
<daksh@iitjammu:~>cat output.txt
xx xx xx
```

### Specification -4

**Redirection with pipes (multiple pipes should be supported**

```
<daksh@iitjammu:~>cat temp.c | wc
xx xx xx
```

### Specification -5

**Redirection with pipes and ">"**

```
<daksh@iitjammu:~>cat temp.c | wc | output.txt
<daksh@iitjammu:~>cat output.txt
xx xx xx
```

### License
This project is distributed under [MIT license](https://opensource.org/licenses/MIT). Any feedback, suggestions are higly appreciated.
