// Mytar.c
// Student: Jorge Torró Fernández
//

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define BLOCK_SIZE 512

typedef struct node{
    char* content;
    struct node* next;
}LLnode;

typedef struct{
    char* fname;
    int t;
    LLnode* files;
} program_args;

typedef struct {
    char fname[100];
    char file_mode[8];
    char userid[8];
    char groupid[8];
    char file_size[12];
    char last_modification_time[12];
    char checksum[8];
    char typeFlag[1];
    char linkedfname[100];
    char ustarIndicator[6];
    char ustartVer[2];
    char ownerUserName[32];
    char ownerGroupName[32];
    char deviceMajorNo[8];
    char deviceMinorNo[8];
    char fnameprefix[155];
    char padding[12];

} tarHeader;

typedef struct {
    char content[512];
} tarContent;

LLnode* createNode(char* content){
    LLnode *new = (LLnode*)malloc(sizeof(LLnode));
    memset(new, 0, sizeof(LLnode));
    new->content = content;
    new->next = NULL;

    return new;
}

void append_to_list(LLnode *head, char* content){
    LLnode *new = createNode(content);
    if(head->next == NULL) head->next = new;
    else{
        LLnode *current = head;
        while(current->next != NULL) current = current->next;
        current->next = new;
    }
}

int endsWithTar (char *s){
    s = strrchr(s,'.');
    if (s != NULL) return (strcmp(s, ".tar"));
    return (-1);
}

int isOption(char* arg){
    if(arg != NULL && strlen(arg) > 0 && arg[0] == '-') return 1;
    return 0;
}

program_args* load_args(int argc, char *argv[]){
    program_args *args = (program_args*)malloc(sizeof(program_args));
    memset(args, 0, sizeof(program_args));
    if(argc < 2){
        printf("Not enough args.\n");
        exit(-1);
    }else{
        int i = 1;
        while(argv[i] != NULL){
            if(isOption(argv[i]) == 0){
                printf("Option expected.\n");
                exit(-1);
            }
            int option = argv[i][1];
            switch(option){
                case 'f':
                    if(i+1 >= argc || isOption(argv[i+1]) == 1 || endsWithTar(argv[i+1]) != 0){
                        printf("-f expected an argument ending in .tar.\n");
                        exit(-1);
                    }
                    args->fname = argv[i+1];
                    i+=2;
                break;
                case 't':
                    args->t = 1;
                    ++i;
                    if(i >= argc ||argv[i] == NULL || isOption(argv[i]) == 1){
                        break;
                    }
                    while(i < argc && isOption(argv[i]) == 0){
                        
                        if(args->files == NULL) args->files = createNode(argv[i]);
                        else append_to_list(args->files, argv[i]);
                        ++i;
                    }
                break;
                default:
                    printf("Unknown option: %s\n", argv[i]);
                    exit(-1); 
                }
            }
            ++i;
        }
        return args;
    }
int regularFile(tarHeader h){
    if(*h.typeFlag == '0' || *h.typeFlag == '\0') return 1;
    else return 0;
}

int isInTarFile(char* current_content, FILE* file){
    tarHeader h;
    int res = 0;
    int zeroBlocks = 0;

    fseek(file, 0, SEEK_SET);
    while(fread(&h, sizeof(h), 1, file) > 0) {
        if (h.fname[0] == '\0') {
            zeroBlocks++;
            if(zeroBlocks >= 2) break; 
            continue; 
        }                           // Check if empty block(end of .tar file)           
        if (regularFile(h) == 1){
            if (strcmp(current_content, h.fname) == 0) res = 1;
        }else{
            printf("Irregular archive detected.\n");
            exit(-1);
        }
        // Get the actual block's size and go to the next block
        int size = strtol(h.file_size, NULL, 8);
        int padding = 0; 
        if((size % BLOCK_SIZE) > 0) padding = BLOCK_SIZE - (size % BLOCK_SIZE);
        fseek(file, size + padding, SEEK_CUR);
        memset(&h, 0, sizeof(h));
    }
    if(zeroBlocks == 1) printf("Alone zero Block.");
    return res;
}


int main(int argc, char *argv[]){
    tarHeader h;
    int error = 0;
    program_args *args = load_args(argc, argv);
    
    if(args->fname == NULL){
        printf("-f option missing.\n");
        return -1;
    }
    FILE *file = fopen(args->fname, "rb");
    if(file == NULL){
        printf("Archive not found.");
        return 1;
    }

    if(args->t == 1 ){
        if(args->files == NULL){
            while (fread(&h, sizeof(h), 1, file) > 0) {
                if (h.fname[0] == '\0') break;                                               // Check if empty block(end of .tar file)           
                if (regularFile(h) == 1) printf("%s\n", h.fname);

                // Get the actual block's size and go to the next block
                int size = strtol(h.file_size, NULL, 8);
                int padding = 0; 
                if((size % BLOCK_SIZE) > 0) padding = BLOCK_SIZE - (size % BLOCK_SIZE);
                fseek(file, size + padding, SEEK_CUR);
                memset(&h, 0, sizeof(h));
            }
        }else{
            LLnode* current = args->files;
            LLnode* notFounds = NULL;
            while(current != NULL){
                if(isInTarFile(current->content, file) == 1){
                    printf("%s\n",current->content);
                }else{
                    if(notFounds == NULL) notFounds = createNode(current->content);
                    else append_to_list(notFounds, current->content);
                    error = -1;
                } 
                current = current->next;
            }
            current = notFounds;
            
            while(current != NULL){
                printf("Mytar: File %s not found.\n", current->content);
                current = current->next;
            }
            if(error == -1) printf("Exiting with failure.\n");
        }
    }
    fclose(file);
    return error;
}