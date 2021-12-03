#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>


char ERROR_MESSAGE[128] = "An error has occurred\n";
char WELCOME[10] = "wish> ";
char line[1000]; 
int batch = 0;
char *paths[4096];
int path_existed = 1;
int CLOSED = 0; 
char* file_store;

void printErr(){
    write(STDERR_FILENO, ERROR_MESSAGE, strlen(ERROR_MESSAGE));
    //exit(0);
}

void printInfo(){
    write(STDERR_FILENO, WELCOME, strlen(WELCOME));
}

void exec(char *path,char *command[]){
    execv(path, command);
}

void execProcess(char *command[], int count){
    int check_dir = 0;
    if (paths[0] == NULL){
        printErr();
    }
    
    for (int i = 1; i < count; i ++){
        int dir_tok = 0;
        int tok_pos = 0;

        if (command[i] == NULL){
            break;
        }
        if (strlen(command[i]) > 1){
            for (int j = 0; j < strlen(command[i]); j ++){                            
                if (command[i][j] == '>'){
                    dir_tok++;
                    check_dir = 1;
                    tok_pos = j;
                }
            }
        }else if (strcmp(command[i], ">") == 0){
            check_dir ++; 
            file_store = command[i + 1];
            if (file_store == NULL){
                printErr();
                exit(0);
            }

            command [i] = NULL;

            if (command[i+2] != NULL && command[i+1] != NULL){
                printErr();
                exit(0);
            }
            command[i+1] = NULL;
            
        }

        if (dir_tok == 1){
            // printf("enter here!\n");
            // printf("i:%d\n", i);
            char *end = strdup(command[i]);
            command[i] = strsep(&end,">");            
            file_store = strsep(&end,">");
            free(end);
            command[i+1] = NULL;
        }else if (dir_tok > 1){
            printErr();
            exit(0);
        }

    }

    for (int i = 0; i < path_existed; i ++){
        char *concate = malloc(strlen(command[0]) + strlen(paths[i]) + 1);
        strcpy(concate, paths[i]);
        strcat(concate, "/");
        strcat(concate, command[0]);
        concate[strcspn (concate, "\n")] = 0;

        if (*command[0] == '\0'){
            return;
        }

        if (access(concate, X_OK) == 0){
            int rc = fork();
            i = path_existed+1;
            if (rc < 0 ){
                printErr(0);
            }
            else if (rc == 0){
                if (check_dir >= 1){
                    close(STDOUT_FILENO);
                    open(file_store, O_CREAT | O_WRONLY | O_TRUNC, 00600);
                    exec(concate, command);
                }else{
                    
                    if (strcmp(command[0], "echo") == 0){ 
                        if (count >= 5 && command[count-1] != NULL){
                            for (int i = 0; i < strlen(command[count-1]); i ++){
                                if (command[count-1][i] == ' '){
                                    command[count-1][i] = 0;
                                }
                            }
                            command[count-1] = NULL;
                        }
                        
                    }

                    exec(concate, command);
                }                    
            }else{
                (void) wait(NULL);
            }
        }else if (i == (path_existed - 1)){
            printErr();
        }        
    }

}

int execute(char *command[], int count){
    //cd 
    if (strcmp (command[0], "cd") == 0){
        //printf("enter cd\n");                   
        if (count == 2){
            char *path = command[1];
            int ret_cd = chdir(path);

            //the execution fails
            if (ret_cd == -1){
                printErr();
            }

        }else{
            printErr();
        }
    }
    else if(command[0] == NULL){
        exit(0);
    }

    //path
    else if (strcmp(command[0], "path") == 0){
        path_existed = 0;
        //printf("enter path.\n"); 
        int i = 1; 

        while(command[i] != NULL){
            paths[path_existed] = command[i];
            i++;
            path_existed++;
        }
        paths[path_existed] = NULL; 
    }
    //loop
    else if (strcmp(command[0], "loop") == 0){
        
        if (count == 1){
            printErr();
            return 0;
        }

        int loop_time = atoi(command[1]);

        if (loop_time <= 0){
            printErr();
            return 0;
        }
        char *cmd[512];

        //$loop exists 
        int store_pos= 0; 
        for (int i = 0; i < count; i ++){
            if (strcmp(command[i], "$loop") == 0){
                store_pos = i;
            }
        }
        //printf("store pos: %d",store_pos);

        if (store_pos > 0){
            //printf("count the number of loop\n");
            //store the new command into another array

            int loop = 1;

            while(loop <= loop_time){
                for (int k = 2; k < count; k ++){
                    if (k == store_pos){
                        char number[4];
                        sprintf (number, "%i", loop);
                        cmd[k-2] = number;
                    }else{
                        cmd[k-2] = command[k];
                    }             
                }
                cmd[count - 2] = NULL;
                loop++;

                execProcess(cmd, count-2);
            }
        
            
        }else{ 
            for (int i = 2; i < count; i ++){
                cmd[i-2] = strdup(command[i]);
            }
            cmd[count-2] = NULL;
            int new_count = count-1;
            for (int j = 0; j < loop_time; j++){
                execProcess(cmd, new_count);
            }
        }
    }

    //exit
    else if (strcmp(command[0], "exit") == 0){
        if (count != 1){
            printErr();
        }else{
            exit(0);
        }           
    }
    else if (command[0] == NULL){
        printInfo();
        exit(0);
    }
    else{
        execProcess(command,count);
    }
    return 0;
}

int main(int argc, char *argv[]) {       
    FILE *file = NULL;
    char *buffer;
    size_t bufsize = 32;
    buffer = (char *)malloc(bufsize * sizeof(char));
    paths[0] = "/bin";
    paths[1] = NULL;
               
    //Interactive mode
    if(argc==1){ 
        file = stdin; //Store standard input to the file.
        printInfo();
    }

    //Batch mode
    else if(argc==2){                
        char *bFile= strdup(argv[1]);
        file = fopen(bFile, "r");
            if (file == NULL) {
                printErr();
                exit(1);
            }
        batch=1;
    }else{
        printErr();
        exit(1);
    }
           
    //characters hold the number of characters detected, buffer holds the content of the stdin
        while( getline(&buffer,&bufsize,file) != -1){
            char *str, *found;
            str = strdup(buffer);
            char *command[sizeof(buffer)];
            int count = 0; 

            while( (found = strsep(&str, " ")) != NULL ){
                if (*found == '\0'){
                    found = strsep(&str, " ");
                    continue;
                }
                
                found[strcspn (found, "\n")] = 0;
                command[count] = found;  
                count++;
            }           
            command[count] = NULL;
            execute(command, count);    

            if (argc == 1){
                printInfo();
            } 
            
        }
    fclose(file);
    free(buffer);
    return(0);
}