#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <ctype.h>


char *path[10];

// This function changes the directory of the bash
void changeDir(char *path[]){ 
    
    int change = chdir(path[0]);

    if (change == -1){
        char error_message[30] = "An error has occurred\n";
        write(STDERR_FILENO, error_message, strlen(error_message)); 
    }
}

// Set or update the search path of the shell
void setPath(char *inputs[]){

    // initialize the counter
    int count = 0;

    // Iterate through the inputs and place them in the path list
    while (inputs[count] != NULL){

        // Create space for the path and place the path in the index
        path[count] =(char *) malloc(strlen(inputs[count]) + 1);
        strcpy(path[count], inputs[count]);

        count++;
    }

    // Overwrite the current path in the list when the input is NULL
    path[count] = NULL;
    
}

// Get the path of the stated executable in the input
int getPath(char *inputs[]){

    // initialize variable for temporary paths
    char tempPath[20];

    // initialize the iterator
    int count = 0;

    // Check if the current path is null and iterate through them to check if the executable is located in the indicated paths
    while (path[count] != NULL){

        // Set the temporary path to the path at the current index of the path list
        strcpy(tempPath, path[count]);

        // Concactenate the executable and the temporary path
        strcat(tempPath, "/");
        strcat(tempPath, inputs[0]);

        //  Check if the path exists
        if (access(tempPath, X_OK) == 0){

            // Create space for the path and replace the executable with the temporary path if it exists and stop
            inputs[0] = (char *)malloc(strlen(tempPath) + 1);
            strcpy(inputs[0], tempPath);

            return 0;
        }

        // increment the counter
        count++;
    }

    return -1;

}

// Check if there are only spaces in input
char *onlySpace(char *str){

    // Trim leading space
    char *end;

    while( isspace(*str) )
    {
        str++;
    }

    end = str + strlen(str) -1;
    while( (end > str) && (isspace(*end)) )
    {
        end--;
    }

    *(end +1) = '\0';

    return str;
}

// This functions splits the input into a list 
int splitString(char *line, char *list[]){

    // initialize the iterator
    int count = 0;
    const char delimiter = ' ';

    // Remove any trailing new line character
    strtok(line, "\n");

    while((list[count] = strsep(&line, &delimiter)) != NULL){
        // Break the user input into inputs using a fixed delimiter

        count++;
    }

    // Return the number of items in the list
    return count;
}

// Check if the command contains any redirection to an output file
int checkRedirection(char *inputs[]){
    
    int count = 0;

    // iterate through the command to identify any redirection symbols
    while (inputs[count] != NULL){
        if (strcmp(inputs[count], ">") == 0){
            return count;
        }

        count++;
    }

    // if there's no redirection
    return -1;
}

// Check if the string contains any redirection to an output file
int isRedirectOrParallel(char *str, char delim){
    
    char * t; // first copy the pointer to not change the original

    for (t = str; *t != '\0'; t++) {
        if (*t == delim){
            
            return 0;
        }

    }
    // if there's no redirection
    return -1;
}

// Check for redirection
int checkSpacelessRedirection(char *line){

    // Clone the value of line to prevent any alterations
    char *lin = line;

    // Check if the redirection symbol exists
    int isRe = isRedirectOrParallel(lin, '>');

    if (isRe == -1){
        return -1;
    }

    const char delimiter = '>';
    // Split the line by the redirection symbols
    char *cmd = strtok(lin, &delimiter);

    char *redirectCheck[20];
    
    int count = 0;

    // Populate the array with the segments of the commands
    while (cmd != NULL){
        
        redirectCheck[count] = cmd;

        redirectCheck[count] = onlySpace(redirectCheck[count]);
        
        cmd = strtok(NULL, &delimiter);
        count++;
    }


    redirectCheck[count] = NULL;

    // Check if the are more than 1 redirection files
    char *inputs[10];
    char *tempRed = redirectCheck[1];

    int len = splitString(tempRed, inputs);

    // Check if the redirection symbols are more than 1
    if (len > 1){
        char error_message[30] = "An error has occurred\n";
        write(STDERR_FILENO, error_message, strlen(error_message));
        return 0;
    }



    // Check if the redirection symbols are more than 1
    if (count > 2){
        char error_message[30] = "An error has occurred\n";
        write(STDERR_FILENO, error_message, strlen(error_message));
        return 0;
    }

    if (count == 1){
        char error_message[30] = "An error has occurred\n";
        write(STDERR_FILENO, error_message, strlen(error_message));
        return 0;
    }

    // Check if there is no redirect file
    if (strcmp(redirectCheck[1], "\n") == 0){
        char error_message[30] = "An error has occurred\n";
        write(STDERR_FILENO, error_message, strlen(error_message));
        return 0;
    }
    
    int status;

    char *pathGen[10];

    int pg = splitString(redirectCheck[0], pathGen);

    if (pg == 0){
        return -1;
    }

    // Get the path of the stated executable
    int path = getPath(pathGen);

    // Check if the path was found
    if (path == -1){
        char error_message[30] = "An error has occurred\n";
        write(STDERR_FILENO, error_message, strlen(error_message));
        return -1;
    }    

    mode_t mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;

    // int fd = open(redirectCheck[1], O_CREAT | O_WRONLY | O_TRUNC, S_IRWXU);

    // redirectCheck[1][strlen(redirectCheck[1])-1] = '\0';

    int fd = creat(redirectCheck[1], mode);
    
    
    switch (status = fork()) {
        case -1:
            return -1;

        case 0: /* child process */

            if (dup2(fd, 1) == -1) { 
                return -1;
            }
            
            // execute the program
            if (execv( pathGen[0], pathGen ) == -1){// child: call execv with the path and the args
                char error_message[30] = "An error has occurred\n";
                write(STDERR_FILENO, error_message, strlen(error_message)); 
                return -1;
            }

        default:
            wait(&status);
            close(fd);
            return 0;
    }
}

// Split the input by the parallel symbol to detect parallel commands
int splitParallel(char *input, char *parallels[]){
    // initialize the iterator
    int count = 0;
    const char delimiter = '&';

    // Remove any trailing new line character
    strtok(input, "\n");

    while((parallels[count] = strsep(&input, &delimiter)) != NULL){
        // Break the user input into inputs using a fixed delimiter
        
        count++;
    }

    // Return the number of items in the list
    return count;
}





int decision(char ch[]){

    // Remove leading spaces in the input string
    char *str = ch;

    char *newStr = onlySpace(str);

    if (strlen(newStr) == 0){
        return 0;
    }

    // Set the original string to the string without spaces
    ch = newStr;


    // Check if there is a spaceless redirect
    int spaceless = checkSpacelessRedirection(ch);

    if (spaceless == 0){
        return 0;
    }

    char *inputs[10];

    // Split the commands by spaces
    int strLen = splitString(ch, inputs);

    // Check if we're looking for the path
    if (strcmp(inputs[0], "path") == 0){
        setPath(inputs + 1);
        return 0;
    }

    // Check if the command is a change directory
    if (strcmp(inputs[0], "cd") == 0){

        // Check if there is more than one argument and return an error if so
        if (strLen > 2){
            char error_message[30] = "An error has occurred\n";
            write(STDERR_FILENO, error_message, strlen(error_message)); 
            return 0;

        }
        else{

            // Change the directory
            changeDir(inputs + 1);
            return 0;
        }
    }
    // Check if the command is exit
    else if (strcmp(inputs[0], "exit") == 0){

        // Check if there are arguments with the exit
        if (strLen > 1){
            char error_message[30] = "An error has occurred\n";
            write(STDERR_FILENO, error_message, strlen(error_message)); 
            return 0;
        }

        exit(0);

    }

    // Check if no path has been set
    else if (path[0] == NULL){
        char error_message[30] = "An error has occurred\n";
        write(STDERR_FILENO, error_message, strlen(error_message)); 
        return 0;
    }

    

    // Check if the command contains any redirection
    int redirect = checkRedirection(inputs);

    // Get the path to the redirection file if redirection exists
    char *redirectFile;

    // Check if there is a redirect symbol and if the redirection is valid, meaning that there is only one arg after the symbol
    if (redirect != -1 && ((strLen > redirect + 2) || (strLen - redirect) <= 1)) {
        char error_message[30] = "An error has occurred\n";
        write(STDERR_FILENO, error_message, strlen(error_message)); 
        return 0;
    }
    
    else {
        // extract the redirection file
        redirectFile = inputs[redirect + 1];

        // remove everything from the redirection symbol
        inputs[redirect] = NULL;
    }

    // Get the path of the stated executable
    if (getPath(inputs) == -1){
        char error_message[30] = "An error has occurred\n";
        write(STDERR_FILENO, error_message, strlen(error_message));
        return 1;
    }

    int status;

    // Create a child process 
    if ( fork() == 0 ){
        
         // execute the program
        if (execv( inputs[0], inputs ) == -1){   // child: call execv with the path and the args
            char error_message[30] = "An error has occurred\n";
            write(STDERR_FILENO, error_message, strlen(error_message)); 
        }

        // Check if there is a redirect option and redirect the output to the indicated file
        if (redirect != -1){

            // Create the redirection file
            int fd = open(redirectFile, O_WRONLY);

            // Direct standard output and error to the redirection file
            dup2(fd, STDOUT_FILENO);
            dup2(fd, STDERR_FILENO);

            // Close the file
            close(fd);
        }
    }
    
    else{
        wait( &status );
    }

    return 0;

}

// Check if command is parallel
int checkParallel(char ch[]){

    const char delimiter = '&';
    

    int para = isRedirectOrParallel(ch, delimiter);

    if (para != 0){
        return -1;
    }

    char *parallels[20];
    
    int count = 0;

    // Split the line by the redirection symbols
    char *cmd = strtok(ch, &delimiter);

    if (cmd == NULL){
        return 0;
    }

    // Populate the array with the segments of the commands
    while (cmd != NULL){
        parallels[count] = cmd;
        // printf("%s",  cmd);
        cmd = strtok(NULL, &delimiter);
        count++;
    }

    parallels[count] = NULL;

    for (int i = 0; i < count; i++){
        // printf("%s\n", parallels[i]);
        decision(parallels[i]);
    }

    return 0;
    
}

// Ooen and read the file that was passed for bash mode
void fileReader(FILE *f){

    char ch[1000];

    // iterate through each line in the file
    while(fgets(ch, 1000, f)){

        // Check if there are parallel commands
        int para = checkParallel(ch);

        if (para == 0){
            continue;
        }

        // Execute the commands
        decision(ch);
    }

    // Close the file and exit the function
    fclose(f);
    exit(0);
}


int main(int argc, char *argv[]){

    // Check if there are more than 2 arguments
    if (argc > 2){
        // Return an error
        char error_message[30] = "An error has occurred\n";
        write(STDERR_FILENO, error_message, strlen(error_message)); 
        exit(1);
    }


    // Set the initial path to /bin
    char *iniPath[1];
    char *inPath = "/bin";
    splitString(inPath, iniPath);
    setPath(iniPath);

    

    // Check if 2 arguments were added
    if (argc == 2){

        // Enter batch moode and open the file 
        FILE *fp;
        fp = fopen(argv[1], "r");

        // Check if the file could not open
        if (fp == NULL) {
            char error_message[30] = "An error has occurred\n";
            write(STDERR_FILENO, error_message, strlen(error_message));
            exit(1);
        }

        fseek(fp, 0, SEEK_END); // goto end of file

        if (ftell(fp) == 0){

            //file empty
            char error_message[30] = "An error has occurred\n";
            write(STDERR_FILENO, error_message, strlen(error_message));

            exit(1);
        }

        fseek(fp, 0, SEEK_SET); // goto begin of file

        fileReader(fp);
        
    }

    // check if there's only one paramter and if the input is not "exit" and go to interactive mode
    else if (argc == 1){
        // Initialize input variables
        char *str = NULL;
        size_t strsize = 0;
      
        while (1){

            // indicate interactive mode and take user input
            printf("wish> ");
            int args = getline(&str, &strsize, stdin);

            // Check if there is no input from the user 
            if (args == 1){
                continue;
            }

            // Check if there are parallel commands
            int para = checkParallel(str);

            if (para == 0){
                return 0;
            }

            // // Initialize the parallel array to store all the parallel commands
            // char *parallels[10];

            // //  Check for any parallel commands and Split the input into parallels
            // int parallel = splitParallel(str, parallels);

            // if (parallel > 1){

            //     // Exexute the parallel commands
            //     checkParallel(parallels);
            // }

            // Execute the commands
            decision(str);
            
        }
    }
}