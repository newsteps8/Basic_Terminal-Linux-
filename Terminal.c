#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <dirent.h>
#include <sys/wait.h>
#include <fcntl.h>

#define MAX_LINE 80 /* 80 chars per line, per command, should be enough. */

/* The setup function below will not return any value, but it will just: read
in the next command line; separate it into distinct arguments (using blanks as
delimiters), and set the args array entries to point to the beginning of what
will become null-terminated, C-style strings. */
char *paths[100];
char *searchCommand(char *command);
char arg[100];
int n = 0;
int ioType = 0;
int i;

struct node
{
    char str[100];
    struct node *next;
};

struct node *head1 = NULL;
struct node *head2 = NULL;
struct node *p1 = NULL;
struct node *p2 = NULL;

void execute(char *args[], int background)
{
    pid_t pid;
    char *path = searchCommand(args[0]);
    pid = fork();

    if (pid < 0)
    {
        perror("Couldn't create child!");
    }
    else if (pid == 0)
    {
        if (path != NULL)
        {
            strncat(path, "/", 1);
            strncat(path, args[0], strlen(args[0]));
            if (execv(path, args))
            {
                perror("Couldn't run the command!\n");
            }
        }
        else
        {
            printf("There is no command like this\n");
        }
        exit(0);
    }
    else if (background == 0)
    { //we only wait if it is a foreground process
        wait(NULL);
    }
}

char *searchCommand(char *command)
{ //this function searches the command in every path in every environment
    DIR *dp = NULL;
    struct dirent *entry;

    int i;
    for (i = 0; paths[i] != NULL; i++)
    {
        if ((dp = opendir(paths[i])) != NULL)
        {
            while ((entry = readdir(dp)) != NULL)
            {
                if (strcmp(entry->d_name, command) == 0)
                { //command found
                    closedir(dp);
                    return strndup(paths[i], strlen(paths[i])); //returning it's pointer
                }
            }
        }
    }

    closedir(dp);
    return NULL;
}

void setup(char inputBuffer[], char *args[], int *background)
{
    int length, /* # of characters in the command line */
        i,      /* loop index for accessing inputBuffer array */
        start,  /* index where beginning of next command parameter is */
        ct;     /* index of where to place the next parameter into args[] */

    ct = 0;

    /* read what the user enters on the command line */
    length = read(STDIN_FILENO, inputBuffer, MAX_LINE);

    /* 0 is the system predefined file descriptor for stdin (standard input),
       which is the user's screen in this case. inputBuffer by itself is the
       same as &inputBuffer[0], i.e. the starting address of where to store
       the command that is read, and length holds the number of characters
       read in. inputBuffer is not a null terminated C-string. */

    start = -1;
    if (length == 0)
        exit(0); /* ^d was entered, end of user command stream */

    /* the signal interrupted the read system call */
    /* if the process is in the read() system call, read returns -1
  However, if this occurs, errno is set to EINTR. We can check this  value
  and disregard the -1 value */
    if ((length < 0) && (errno != EINTR))
    {
        perror("error reading the command");
        exit(-1); /* terminate with error code of -1 */
    }

    for (i = 0; i < length; i++)
    { /* examine every character in the inputBuffer */

        switch (inputBuffer[i])
        {
        case ' ':
        case '\t': /* argument separators */
            if (start != -1)
            {
                args[ct] = &inputBuffer[start]; /* set up pointer */
                ct++;
            }
            inputBuffer[i] = '\0'; /* add a null char; make a C string */
            start = -1;
            break;

        case '\n': /* should be the final char examined */
            if (start != -1)
            {
                args[ct] = &inputBuffer[start];
                ct++;
            }
            inputBuffer[i] = '\0';
            args[ct] = NULL; /* no more arguments to this command */
            break;

        default: /* some other character */
            if (start == -1)
                start = i;
            if (inputBuffer[i] == '&')
            {
                *background = 1;
                inputBuffer[i - 1] = '\0';
            }
        }            /* end of switch */
    }                /* end of for */
    args[ct] = NULL; /* just in case the input line was > 80 */

} /* end of setup routine */

void addnodeforpath(char *new_path)
{
    if (head1 == NULL)
    {
        struct node *New = (struct node *)malloc(sizeof(struct node));
        head1 = New;
        strcpy(New->str, new_path);
        New->next = NULL;
    }

    else
    {
        struct node *New = (struct node *)malloc(sizeof(struct node));
        strcpy(New->str, new_path);
        New->next = NULL;
        for (p1 = head1; p1->next != NULL; p1 = p1->next)
            ;
        p1->next = New;
    }
}

void deleteItem(char *item)
{
    struct node *temp, *previous;
    temp = head1;

    // First, locate the node that contains the item.
    for (; temp->next != NULL; temp = temp->next)
    {
        if (strcmp(item, temp->str) == 0)
        {
            break;
        }
        previous = temp;
    }

    // Now take care of deleting it.
    if (temp != NULL)
    {
        if (temp == head1)
        {
            head1 = temp->next;
        }
        else
        {
            previous->next = temp->next;
        }

        free(temp);
    }
}
void PATH()
{

    char *value = getenv("PATH");
    char *token;

    token = strtok(value, ":");
    paths[0] = token;

    int i;
    for (i = 1; token != NULL; i++)
    {
        paths[i] = token;
        token = strtok(NULL, ":");
        addnodeforpath(paths[i]);
    }
}

void pushToHistory(struct node **head_ref, char *new_data)
{
    if (strcmp(new_data, "history") != 0)
    {
        struct node *new_node = (struct node *)malloc(sizeof(struct node));
        strcpy(new_node->str, new_data);
        new_node->next = (*head_ref);
        (*head_ref) = new_node;
    }
}

/* Given a reference (pointer to pointer) to the head of a list
and a key, deletes the first occurrence of key in linked list */
void changeFromHistory(struct node **head_ref, char *key)
{
    // Store head node
    struct node *temp = *head_ref, *prev;

    // If head node itself holds the key to be deleted
    if (temp != NULL && strcmp(temp->str, key) == 0)
    {

        return;
    }

    // Search for the key to be deleted, keep track of the
    // previous node as we need to change 'prev->next'
    while (temp != NULL && strcmp(temp->str, key) != 0)
    {
        prev = temp;
        temp = temp->next;
    }

    // If key was not present in linked list
    if (temp == NULL)
        return;

    // Unlink the node from linked list
    prev->next = temp->next;
    struct node *new_node = (struct node *)malloc(sizeof(struct node));
    new_node->next = (*head_ref)->next;
    (*head_ref)->next = new_node;
    strcpy(new_node->str, (*head_ref)->str);
    strcpy((*head_ref)->str, key);
    free(temp); // Free memory
}

void printList(struct node *node)
{
    int i = 0;
    while (node != NULL && i < 10)
    {
        printf(" \n%s ", node->str);
        node = node->next;
        i++;
    }
}

int searchIO(char *args[])
{
    int i;
    for (i = 0; args[i + 1] != NULL; i++)
    {
        if (strcmp(args[i], ">") == 0)
        {
            if (strcmp(args[i + 1], "file.out") == 0)
            {
                args[i] = NULL;
                args[i + 1] = NULL;
                return 1;
            }
            else
            {
                return -1;
            }
        }
        else if (strcmp(args[i], ">>") == 0)
        {

            if (strcmp(args[i + 1], "file.out") == 0)
            {
                args[i] = NULL;
                args[i + 1] = NULL;
                return 2;
            }
            else
            {
                return -1;
            }
        }
        else if (strcmp(args[i], "<") == 0)
        {

            if (strcmp(args[i + 1], "file.in") == 0)
            {
                args[i] = "myfile.txt";
                args[i + 1] = NULL;
                return 3;
            }
            else
            {
                return -1;
            }
        }
        else if (strcmp(args[i], "2>") == 0) //place holder
        {

            if (strcmp(args[i + 1], "file.out") == 0)
            {
                args[i] = NULL;
                args[i + 1] = NULL;
                return 4;
            }
            else
            {
                return -1;
            }
        }
    }
    return 0;
}

// Takes head pointer of
// the linked list and index
// as arguments and return
// data at index
char *GetNthFromHistory(struct node *head, int index)
{

    struct node *current = head;

    // the index of the
    // node we're currently
    // looking at
    int count = 0;
    int control = 0;
    while (current != NULL)
    {
        if (count == index)
        {
            return (current->str);
        }

        count++;
        current = current->next;
        control = 1;
    }

    if (control == 0)
    {
        printf("There are no index like this.");
        return;
    }
    control = 0;
}

int main(void)
{
    struct node **head = NULL;

    char inputBuffer[MAX_LINE];   /*buffer to hold command entered */
    int background;               /* equals 1 if a command is followed by '&' */
    char *args[MAX_LINE / 2 + 1]; /*command line arguments */
    PATH();                       //initialize environment paths

    while (1)
    {
        background = 0;
        printf("\nmyshell: ");
        fflush(stdout);
        setup(inputBuffer, args, &background);
        ioType = searchIO(args);

        while (args[n] != NULL)
        {
            strcat(arg, args[n]);
            strcat(arg, " ");
            n++;
        }

        if (strcmp(args[0], "path") == 0)
        { //program terminates when user enters "exit"
            if (args[1] == NULL)
            {
                pushToHistory(&head, "path");
                printf("\nPATH LIST:");
                for (p1 = head1; p1 != NULL; p1 = p1->next)
                {
                    printf("\n%s", p1->str);
                }
            }

            else if (strcmp(args[1], "+") == 0)
            {
                addnodeforpath(args[2]);
                printf("\nPATH LIST:");
                for (p1 = head1; p1 != NULL; p1 = p1->next)
                {
                    printf("\n%s", p1->str);
                }
                char add1[100];
                strcat(add1, "path +");
                strcat(add1, " ");
                strcat(add1, args[2]);
                pushToHistory(&head, add1);
                *add1 = '\0';
            }
            else if (strcmp(args[1], "-") == 0)
            {
                deleteItem(args[2]);
                printf("\nPATH LIST:");
                for (p1 = head1; p1 != NULL; p1 = p1->next)
                {
                    printf("\n%s", p1->str);
                }
                char add2[100];
                strcat(add2, "path -");
                strcat(add2, " ");
                strcat(add2, args[2]);
                pushToHistory(&head, add2);
                *add2 = '\0';
            }
        }
        else if (strcmp(args[0], "cd") == 0)
        {
            pushToHistory(&head, "cd");
            if (args[1] == NULL)
            {
                printf("Enter a directory to use cd command!\n");
            }
            else if (chdir(args[1]) == -1)
            { //chdir changes directory
                printf("No directory like \"%s\".\n", args[1]);
            }
        }
        else if (strcmp(args[0], "history") == 0)
        { //program terminates when user enters "exit"
            if (args[1] != NULL)
            {
                changeFromHistory(&head, GetNthFromHistory(head, atoi(args[2]))); //çağırılan komutu listenin en başına taşır
                char *strng;
                strng = GetNthFromHistory(head, atoi(args[2]));
                char delim[] = " ";
                char *ptr = strtok(strng, delim);
                char *path = searchCommand(ptr);
                execv(path, args);
            }
            else
            {
                printList(head);
            }
        }

        else if (strcmp(args[0], "exit") == 0)
        { //program terminates when user enters "exit" and there no backround process
            if (background == 1)
            {
                printf("there are background processes still running");
            }
            else
            {
                printf("Bye\n");
                exit(0);
            }
        }
        else
        {
            if (ioType == 0)
            {
                execute(args, background); //processes are created here
            }
            else if (ioType == 1)
            {
                //First, we're going to open a file
                int saved_stdout = dup(1);
                int file = open("myfile.txt", O_CREAT | O_TRUNC | O_RDWR);
                if (file < 0)
                    return 1;

                //Now we redirect standard output to the file using dup2
                if (dup2(file, 1) < 0)
                {
                    close(file);
                    return 1; // this can return 0 normally
                }

                //Now standard out has been redirected, we can write to
                // the file
                execute(args, background); //processes are created here

                //At the end the file has to be closed:
                dup2(saved_stdout, 1);
                close(saved_stdout);
            }
            else if (ioType == 2)
            {
                //First, we're going to open a file
                int saved_stdout = dup(1);
                int file = open("myfile.txt", O_APPEND | O_CREAT | O_RDWR);
                if (file < 0)
                    return 1;

                //Now we redirect standard output to the file using dup2
                if (dup2(file, 1) < 0)
                {
                    close(file);
                    return 1; // this can return 0 normally
                }

                //Now standard out has been redirected, we can write to
                // the file
                execute(args, background); //processes are created here

                //At the end the file has to be closed:
                dup2(saved_stdout, 1);
                close(saved_stdout);
            }
            else if (ioType == 3)
            {
                //First, we're going to open a file
                int saved_stdout = dup(1);
                int file = open("myfile.txt", O_RDWR);
                if (file < 0)
                    return 1;

                //Now we redirect standard output to the file using dup2
                if (dup2(file, 1) < 0)
                {
                    close(file);
                    return 1; // this can return 0 normally
                }

                //Now standard out has been redirected, we can write to
                // the file
                execute(args, background); //processes are created here

                //At the end the file has to be closed:
                dup2(saved_stdout, 1);
                close(saved_stdout);
            }
            else if (ioType == 4)
            {
                //yazılacak
            }

            if (args[1] != NULL)
            {
                pushToHistory(&head, arg);
            }
            else
            {
                pushToHistory(&head, args[0]);
            }
        }
    }
}