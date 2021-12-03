#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct node{
    int key;
    char* value;  
    struct node *next; 
};

struct node *head = NULL;
//struct node *current = NULL; 

struct node* get(int target) {
    //printf("enter get function.\n");
    //printf("target = %d", target);
    
   struct node* current = head;

   while(current != NULL) {	  
    if(current->key != target){
        current = current->next;  
      }else{
          return current;
      }
   }    
   return NULL;
}

void put(int key, char *value){
    //printf("enter put function.\n");
    if (value == NULL){
        printf("bad command\n");
        return;
    }
    //handle duplicate node
    struct node* duplicate = get(key);

    if (duplicate != NULL){
        duplicate -> value = value;
        return;
    }

    struct node *new = (struct node*) malloc(sizeof(struct node));
    new -> key = key; 
    new -> value = value; 

    new -> next = head; 
    head = new; 
}

void all(){
    //printf("enter all function.\n");
    struct node *point = head;

    while(point != NULL){    
        printf("%d,%s\n", point->key, point->value);
        point = point->next;
    }
}

void delete(int target) {
    //printf("enter delete function.\n");
   struct node* current = head;
   struct node* previous = NULL;
	
   if(head == NULL) {
      printf("%d not found\n", target);
      return;
   }

    //not find target
   while(current->key != target) {
      if(current->next == NULL) {
         printf("%d not found\n", target);
      } else {
         previous = current;
         current = current->next;
      }
   }

    //find target! 
   if(current == head) {
       struct node* tmp = head;
      head = head->next;
      free(tmp);
   } else {
       struct node* tmp = current;
      previous->next = current->next;
        free(tmp);
   }    
}

void clear(){
        free(tmp);
        tmp = head;
    }
    head = NULL;
}

FILE* read_file(){
    FILE *store;   

    if ((store = fopen("store.txt", "r"))){ 
        return store;
    }else{
        return NULL;
    }
}

int main(int argc, char *argv[]) {
    //printf("enter here");
    char *line = NULL;
    size_t len = 0;
    ssize_t nread;
    FILE* store = read_file(); 

    if (store != NULL){
        while ((nread = getline(&line, &len, store)) != -1){
            //printf("should store sth.");
            char* file_store = strtok(line,",");
            int key = atoi(file_store);

            file_store = strtok (NULL, ",");
            char *value = (char *)malloc((argc+1) * sizeof(char));
            strncpy (value, file_store, strlen(file_store)-1);

            //printf("key = %d,value = %s ends!", key, value);
            put(key, value);   
        }
     fclose(store);
    }
    
   


    char *commands; 
    for (int i = 1; i < argc; i ++){
        char *expression = (char *)malloc((argc+1) * sizeof(char));
        strcpy (expression, argv[i]);
        commands = strtok(expression, ",");
        
        while(commands != NULL){
            if (strcmp(commands,"p") == 0){
                //printf("%s\n", commands);
                commands = strtok(NULL, ",");
                int key = atoi(commands);

                if (key == 0){
                    printf("bad command\n");
                    break;
                }

                //printf("%s\n", commands);
                commands =  strtok (NULL, ",");
                //printf("%s\n", commands);
                char *value = commands;
                
                //if (*value >= 97 && *value<= 122){
                    //printf("key = %d,value = %s ends!", key, value);
                put(key, value);
                break;
                    //free(value);
                // }else{
                //     break;
                // }
                
            }else if (strcmp (commands, "a") == 0){
                all();
                break;
            }else if (strcmp (commands, "g") == 0){
                commands = strtok(NULL, ",");
                if (commands == NULL){
                    printf("bad command\n");
                    break;
                }

                int key = atoi(commands);

                if (key == 0){
                    printf("bad command\n");
                    break;
                }

                if (strtok(NULL, ",") != NULL){
                    printf("bad command\n");
                    break;
                }

                struct node* get_value = get(key);

                if (get_value != NULL){
                    printf("%d,%s\n", get_value->key, get_value-> value);
                    break;
                }else{
                    printf("%d not found\n", key);  
                    break;
                }
            }else if (strcmp (commands, "d") == 0){
                commands = strtok(NULL, ",");

                if (commands == NULL){
                    printf("bad command\n");
                    break;
                }

                int key = atoi(commands);

                if (key == 0){
                    printf("bad command\n");
                    break;
                }

                if (strtok(NULL, ",") != NULL){
                    printf("bad command\n");
                    break;
                }

                delete(key);
                break;
            }else if (strcmp (commands, "c") == 0){
                clear();
                break;
            }else{
                //printf("%s",commands);
                printf("bad command\n");
                break;
            }
        }

        FILE *fp;
        fp = fopen ("store.txt", "w+");
        struct node* current = head;

        // if (fp == NULL){
        //     fprintf(stderr, "\nCouldn't Open File'\n");
        //     exit (1);
        // }
        
        while(current!= NULL){
            fprintf(fp, "%d,%s\n", current->key,current->value);
            current = current->next;
        }
            fclose (fp);
    }
    
    

return(0);
}