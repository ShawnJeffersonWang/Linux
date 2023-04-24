#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <pwd.h>
#include <signal.h>
#include <readline/readline.h> //用命令下载库,然后在makefile里面-lreadline
#include <readline/history.h>

#define PATH_MAX 4096
#define MAX_ARGS 64
#define MAX_LINE 1024
#define MAX_HISTORY 100
#define MAX_NAME 100

void prompt(void);
int parse_args(char *line, char **args);
void change_dir(char **args, char **history);
void execute(char **args, int args_cnt);
void div_by_pipe(int left, int right, char **args);
int find(char *command);
void redirect(int left, int right, char **args);
void sigint_handler(int signum);
void my_err(const char *err_string, int line);

int cd_cnt = 0;

int main(int argc, char *argv[])
{
    char **history = (char **)malloc(sizeof(char *) * MAX_HISTORY);
    for (int i = 0; i < MAX_HISTORY; i++)
    {
        history[i] = (char *)malloc(sizeof(char) * PATH_MAX);
    }

    strcpy(history[0], "/home/shawn/");
    int args_cnt;

    char **args = (char **)malloc(sizeof(char *) * MAX_ARGS);
    for (int i = 0; i < MAX_ARGS; i++)
    {
        args[i] = (char *)malloc(sizeof(char) * MAX_LINE);
    }

    signal(SIGINT, sigint_handler); // 信号处理函数,好nb

    while (1)
    {
        prompt(); // bug每次循环都要打印

        char *line = (char *)malloc(sizeof(char) * MAX_LINE);
        fgets(line, MAX_LINE, stdin);
        line[strlen(line) - 1] = '\0'; // 防止栈溢出

        args_cnt = parse_args(line, args);
        // printf("%d\n",args_cnt);//打印参数个数
        if (args_cnt == 0) // 处理直接enter
        {
            continue;
        }

        if (strcmp(args[0], "cd") == 0)
        {
            change_dir(args, history);
            continue;
        }

        if (strcmp(args[0], "exit") == 0)
        {
            break;
        }

        // args_cnt--; // bug
        execute(args, args_cnt);

        free(line);
        free(args);
    }

    return 0;
}

void prompt(void)
{
    char cusername[MAX_NAME];
    char at = '@';
    char hostname[MAX_NAME];
    char chostname[MAX_NAME + 50];
    char path[PATH_MAX];
    char cpath[PATH_MAX + 50];

    struct passwd *pw;
    pw = getpwuid(getuid());
    gethostname(hostname, sizeof(hostname));
    getcwd(path, sizeof(path));
    char m = '$';

    sprintf(cusername, "\033[1;32m%s\033[0m", pw->pw_name);
    sprintf(chostname, "\033[1;32m%s\033[0m", hostname);
    sprintf(cpath, "\033[1;34m%s\033[0m", path);

    printf("%s\033[1;32m%c\033[0m%s:%s%c", cusername, at, chostname, cpath, m); // bug可以直接打印有颜色的字符@

    return;
}

int parse_args(char *line, char **args)
{
    int pos;
    int len = strlen(line);

    for (int i = 0; i < len; i++)
    {
        if (line[i] == '|')
        {
            pos = i;
            for (int j = len; j >= pos + 1; j--) // ls| wc -l  ->  ls|  wc -l  // ls |wc -l  ->  ls | wc -l  // ls|wc -l -> ls| wc -l -> ls |wc -l
            {
                line[j + 1] = line[j];
            }
            line[pos + 1] = ' '; // 此时已经处理完ls |wc -l

            if (line[pos + 2] == ' ') // 处理ls| wc -l
            {
                i++; // bug i如果不++ 会继续解析移动过的'|'
            }

            if (line[pos - 1] != ' ') // 处理ls| wc-l和ls|wc -l  //bug多用tui enable调试 p *args@5
            {
                line[pos + 1] = '|';
                line[pos] = ' ';
            }
        }
    }

    int args_cnt = 0;
    char *token;
    char *delim = " ";

    token = strtok(line, delim);

    while (token != NULL)
    {
        args[args_cnt++] = token; // 更先进的处理方式,不用在main()函数里args_cnt--并且能处理无命令直接enter的情况,这样可以直接反映参数个数
        token = strtok(NULL, delim);
    }

    args[args_cnt] = NULL;
    return args_cnt;
}

void change_dir(char **args, char **history)
{
    if (args[1] == NULL) // bug比如cd后啥也没输
    {
        return;
    }

    if (strcmp(args[1], "~") != 0 && strcmp(args[1], "-") != 0)
    {
        char buf[PATH_MAX];
        getcwd(buf, sizeof(buf));
        strcpy(history[++cd_cnt], buf);
    }

    if (strcmp(args[1], "~") == 0)
    {
        char buf[PATH_MAX];
        getcwd(buf, sizeof(buf));
        strcpy(history[++cd_cnt], buf);
        strcpy(args[1], "/home/shawn/");
    }

    if (strcmp(args[1], "-") == 0)
    {
        strcpy(args[1], history[cd_cnt]);
        cd_cnt--;
        char buf[PATH_MAX];
        getcwd(buf, sizeof(buf));
        strcpy(history[++cd_cnt], buf);
    }

    if (chdir(args[1]) == -1)
    {
        printf("chdir error\n"); // bug不能因为chdir错误就退出程序
    }

    return;
}

void execute(char **args, int args_cnt)
{
    pid_t pid;
    pid = fork();
    if (pid == -1)
    {
        my_err("fork", __LINE__);
    }

    if (pid == 0)
    {
        // int infd = dup(0);
        // int outfd = dup(1);
        div_by_pipe(0, args_cnt, args);
        // dup2(infd, STDIN_FILENO);
        // dup2(outfd, STDOUT_FILENO);
        exit(0);
    }
    else if (pid > 0)
    {
        waitpid(pid, NULL, 0);
    }
}

void div_by_pipe(int left, int right, char **args)
{
    int pipepos = -1;

    for (int i = left; i < right; i++)
    {
        if (strcmp(args[i], "|") == 0) // 大bug没有加=0
        {
            pipepos = i;
            break;
        }
    }

    if (pipepos == -1) // bug先于管道|后缺命令执行
    {
        redirect(left, right, args);
        return;
    }

    if (pipepos + 1 == right)
    {
        printf("behind the | need a command\n");
        return;
    }

    int fd[2];
    if (pipe(fd) == -1)
    {
        my_err("pipe", __LINE__);
    }

    pid_t pid = fork();
    if (pid == -1)
    {
        my_err("fork", __LINE__);
    }

    if (pid == 0)
    {
        close(fd[0]);
        dup2(fd[1], STDOUT_FILENO);
        // close(fd[1]);
        redirect(left, pipepos, args);
        exit(0);
    }
    else if (pid > 0)
    {
        waitpid(pid, NULL, 0);

        close(fd[1]);
        dup2(fd[0], STDIN_FILENO);
        // close(fd[0]);
        div_by_pipe(pipepos + 1, right, args);
    }
}

void redirect(int left, int right, char **args)
{
    int fd;
    int is_background = 0;

    if (!find(args[left]))
    {
        return;
    }

    for (int i = left; i < right; i++)
    {
        if (strcmp(args[i], "&") == 0)
        {
            is_background = 1;
            args[i] = NULL;
            break;
        }
    }

    for (int i = left; i < right; i++)
    {
        if (strcmp(args[i], "<") == 0)
        {
            fd = open(args[i + 1], O_RDONLY);
            dup2(fd, STDIN_FILENO);
            close(fd);
            args[i] = NULL;
            // args[i+1]=NULL;//处理<的同时处理文件
            i++;
        }
        if (strcmp(args[i], ">") == 0)
        {
            fd = open(args[i + 1], O_WRONLY | O_CREAT | O_TRUNC, 0644);
            dup2(fd, STDOUT_FILENO);
            close(fd);
            args[i] = NULL;
            // args[i+1]=NULL;
            i++;
        }
        if (strcmp(args[i], ">>") == 0)
        {
            fd = open(args[i + 1], O_WRONLY | O_CREAT | O_APPEND, 0644);
            dup2(fd, STDOUT_FILENO);
            close(fd);
            args[i] = NULL;
            // args[i+1]=NULL;
            i++;
        }
    }

    pid_t pid = fork();
    if (pid == -1)
    {
        my_err("fork", __LINE__);
    }

    if (pid == 0)
    {
        // execvp(args[left], args + left);//bug
        char *command[MAX_ARGS];
        for (int i = left; i < right; i++)
        {
            // strcpy(command[i], args[i]);//bug
            command[i] = args[i];
        }
        command[right] = NULL; // bug有效参数的个数j
        execvp(command[left], command + left);
    }
    else if (pid > 0)
    {
        if (is_background == 0)
        {
            waitpid(pid, NULL, 0);
        }
    }
}

int find(char *command)
{
    DIR *dir;
    struct dirent *sdp;
    char *path[] = {"/bin", "/usr/bin", "./", NULL};

    if (strncmp(command, "./", 2) == 0) // 类似于./a.out
    {
        command = command + 2; // 去除./,方便与d_name比较
    }

    for (int i = 0; path[i] != NULL; i++)
    {
        dir = opendir(path[i]);
        while ((sdp = readdir(dir)) != NULL)
        {
            if (strcmp(command, sdp->d_name) == 0)
            {
                closedir(dir);
                return 1;
            }
        }
    }

    closedir(dir);
    return 0;
}

// 信号处理函数
void sigint_handler(int signum)
{
    printf("\n");
    fflush(stdout);
}

void my_err(const char *err_string, int line)
{
    fprintf(stderr, "line:%d", __LINE__);
    perror(err_string);
    exit(1);
}