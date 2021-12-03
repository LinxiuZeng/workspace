#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/sysinfo.h>
#include <pthread.h>


pthread_mutex_t lock;

//this is the raw data read in from the input file
//keep track of the contents and the length of the contents
typedef struct zipRaw{
    char* contents;
    int size;
}z_r;

typedef struct zipCompressed{
    int length;
    char* contents;
    int* count;
    char last_c;
    int last_num;
}z_c;

//global variables
//things producer should update
int single_file_size=0;
int last_chunk_size=0;

int num_procs=0;
int numfull=0;
int max;
char last_c;
int last_n;
int fd;
int threshold; //chunk size
z_r** read_buffer;  //array of pointers pointing to each chunk location
                    //space to store all the file_chunks within one file
                    //last character and corresponds counts



//zip the input file
void* zip(void *input_v){
    if (input_v == NULL){
        return NULL;
    }

    z_r* input = (z_r*)input_v;
    //printf("input_size:%d, thread_id:%ld\n", input->size, syscall(__NR_gettid));  
    if(input->size<=0) return NULL;

    int track_input = 0;
    z_c *output=(z_c*) malloc(sizeof(z_c));
    output->contents=(char*)malloc(input->size*sizeof(char));
    output->count=(int*)malloc(input->size*sizeof(int));
    output->length=0;
    char current = input->contents[0];
    int index = 0;
    int num = 0;

    while(track_input < input->size){
        if(input->contents[track_input] =='\0'){
            track_input++;
            if(track_input<input->size&&current=='\0') current=input->contents[track_input];
            continue;
        }
        else if(current==input->contents[track_input]){
            num++;
        }
        if(track_input==input->size-1||current!=input->contents[track_input+1]){         
            output->count[index]=num;
            output->contents[index]=current;
            output->length++;
            index++;
            if(track_input+1<input->size) current=input->contents[track_input+1];
            num=0;
        }  
    track_input++;
    }

    if(index>0){
        output->last_num=output->count[index-1];
        output->last_c=output->contents[index-1];
    }

    //if(output->count[2]!='\0'&& output->length>=2){
    if(output->length>=1){
     output->length--;
    }else {
        //free(output);
        return NULL;
    }

    return (void*)output;
}


//producer function
void* magic_mmap(void* chunk_start_pos){

    z_r *input = (z_r*) malloc(sizeof(z_r));
    //reaches the last chunk of current file
    if (*(int*)chunk_start_pos + threshold >= single_file_size+1){
        input->contents = (char*)malloc(sizeof(char)*last_chunk_size);
        input->contents = (char*) mmap(NULL, last_chunk_size, PROT_READ, MAP_PRIVATE, fd, *(int*)chunk_start_pos);
        input->size = last_chunk_size;
    }else{
        input->contents = (char*) malloc(sizeof(char)*threshold+1);
        input->contents = (char*) mmap(NULL, threshold, PROT_READ, MAP_PRIVATE, fd, *(int*)chunk_start_pos);
        input->size = threshold;
    }

    return (void*) input;
}



//main function
int main(int argc, char *argv[]){
    //no enough argument are provided
    if (argc < 2){
        printf("pzip: file1 [file2 ...]\n");
        exit(1);
    }

    //number of files in total
    int num_files = argc - 1;

    //all the input filenames
    char *in_files[num_files];

    //take the input files
    for (int i = 0; i < num_files; i ++) {
        in_files[i] = strdup(argv[i+1]);
    }

    //producer create thread for each file
    for (int i = 0; i < num_files; i ++){
        struct stat s;
        fd = open((const char*)in_files[i], O_RDONLY);

        //get the status of the file
        int status = fstat(fd, &s);
       
        //check if fstat fails
        if (status < 0) {
            close(fd);
            continue;
        }
        //check if file didn't open    
        if(fd == -1){
            continue;
        }

        //check if couldn't retrieve file stats
        if( s.st_size == -1){
            close(fd);
            continue;
        }
        // check if the file contains any content
        if( s.st_size == 0){
            continue;
        }

        single_file_size = s.st_size;
        pthread_mutex_init(&lock, NULL);

        threshold= getpagesize();
        //use single thread to run a small file
        if (s.st_size <= threshold){
            //create 1 thread mmap
            z_r *input = (z_r*) malloc(sizeof(z_r));
            read_buffer=(z_r**)malloc(sizeof(z_r*));
            last_chunk_size = s.st_size;
            input->contents = (char*) malloc(sizeof(char)* single_file_size);
            input->contents = (char*) mmap(NULL, s.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
            input->size = s.st_size;
            read_buffer[0] = input;
            numfull++;
        //use multiple threads to run big files    
        }else{        
            //create multi thread, thread number equals to num_procs
            //calculate num_chunks
            int num_chunks = s.st_size/threshold + 1;
            last_chunk_size = s.st_size%threshold;
            max = num_chunks;
            int** current_size_chunks;
            current_size_chunks = (int**)malloc(sizeof(int*)*num_chunks); // the starting pos of each chunk
            int zero=0;
            int* last = (int*)malloc(sizeof(int)*num_chunks);
            for (int chunk_index = 0; chunk_index < num_chunks; chunk_index++){
                if (chunk_index == 0){
                    current_size_chunks[chunk_index] = &zero;
                }else{
                    last[chunk_index] = *(current_size_chunks[chunk_index-1])+threshold;
                    current_size_chunks[chunk_index] = &last[chunk_index];
                }              
            }

            read_buffer=(z_r**)malloc(sizeof(z_r*)* num_chunks);
                       
            //create new threads list
            pthread_t thread_prod[num_chunks];  
            for(int thd=0; thd < num_chunks; thd++){
                pthread_create(&thread_prod[thd], NULL, magic_mmap, (void*)current_size_chunks[thd]);
            }

            //join all the threads together
            for (int thd = 0; thd < num_chunks; thd++){
                void * output;
                pthread_join(thread_prod[thd], &output);
                read_buffer[thd] = (z_r*) output;
            }
        }


       
    //consumer create thread
    int current_size = 0;
    int total_proc=0;
    while(current_size<single_file_size){
        num_procs=0;
        //update num_procs
        for(int proc=0; proc < get_nprocs(); proc++){
            //check last chunk size
            if(current_size+threshold >= single_file_size){
                current_size+=last_chunk_size;
                num_procs++;
                break;
            }
            else{
                num_procs++;
                current_size+=threshold;
            }
        }

        //create new threads list for the consumer!
        pthread_t threads[num_procs];  
        for(int thd=0; thd < num_procs; thd++){
            pthread_create(&threads[thd], NULL, zip, (void*)read_buffer[total_proc]);
            total_proc++;
        }

        //join all the threads together
        z_c* joins[num_procs];
        //printf("num_procs:%d\n", num_procs);        
        //save the output of thread      
        for(int j=0; j< num_procs;j++){
            //save the output of thread
            void* output;
            
            pthread_join(threads[j], &output);
            
                       
            if (output== NULL) continue;
            z_c *result = (z_c*)output;            
            joins[j]=result;

            int check_single=0;
            check_single=(joins[j]->contents[1]=='\0');
            
            //first thread of first file
            if(j==0&&i==0){
                last_c=result->last_c;
                last_n=result->last_num;
            }
            //switch file and check first thread, then init new file's
            //first thread last char
            else if(j==0&&i>0){
                if(joins[j]->contents[0]==last_c) {
                    joins[j]->count[0]+=last_n;
                    if(check_single==1){
                        last_n=joins[j]->count[0];
                    }
                    else{
                        last_c=result->last_c;
                        last_n=result->last_num;
                    }
                }
                else{
                    fwrite(&(last_n), 1, sizeof(int), stdout);
                    fwrite(&(last_c), 1, sizeof(char), stdout);
                    last_c=result->last_c;
                    last_n=result->last_num;
                }                
            }
            //siwtch thread check
            //if not first thread, then check last thread's last character
            else if(j>0){
            
                if(joins[j]->contents[0]==last_c) {
                        joins[j]->count[0]+=last_n;

                        if(check_single==1){
                            last_n=joins[j]->count[0];
                        }
                        else{
                            last_c=result->last_c;
                            last_n=result->last_num;
                        }
                }
                else{
                    fwrite(&(last_n), 1, sizeof(int), stdout);
                    fwrite(&(last_c), 1, sizeof(char), stdout);
                    last_c=result->last_c;
                    last_n=result->last_num;
                }

             }

                //i is file num
                //j is thread num
                //check_single is checking thread with single char              
                if(check_single==0){
                    for(int m=0; m<joins[j]->length; m++){                  
                        fwrite(&(joins[j]->count[m]), 1, sizeof(int), stdout);
                        fwrite(&(joins[j]->contents[m]), 1, sizeof(char), stdout);
                    }
                }
            }
        //pthread_mutex_unlock(&lock);            
    }
    free(read_buffer);

    }
    fwrite(&(last_n), 1, sizeof(int), stdout);
    fwrite(&(last_c), 1, sizeof(char), stdout);

    return(0);
}