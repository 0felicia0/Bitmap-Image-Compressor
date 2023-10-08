#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <ctype.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <time.h>

typedef unsigned short WORD;
typedef unsigned int DWORD;
typedef unsigned int LONG;
typedef unsigned char BYTE;

/*STRUCTS FROM PROJECT 1 PDF*/
typedef struct tagBITMAPFILEHEADER {
    WORD bfType; /*specifies the file type*/
    DWORD bfSize; /*specifies the size in bytes of the bitmap file*/
    WORD bfReserved1; /*reserved; must be 0*/
    WORD bfReserved2; /*reserved; must be 0*/
    DWORD bfOffBits; /*species the offset in bytes from the bitmapfileheader to the bitmap bits*/
}tagBITMAPFILEHEADER;

typedef struct tagBITMAPINFOHEADER {
    DWORD biSize; /*specifies the number of bytes required by the struct*/
    LONG biWidth; /*specifies width in pixels*/
    LONG biHeight; /*species height in pixels*/
    WORD biPlanes; /*specifies the number of color planes, must be 1*/
    WORD biBitCount; /*specifies the number of bit per pixel*/
    DWORD biCompression;/*specifies the type of compression*/
    DWORD biSizeImage; /*size of image in bytes*/
    LONG biXPelsPerMeter; /*number of pixels per meter in x axis*/
    LONG biYPelsPerMeter; /*number of pixels per meter in y axis*/
    DWORD biClrUsed; /*number of colors used by th ebitmap*/
    DWORD biClrImportant; /*number of colors that are important*/
}tagBITMAPINFOHEADER;

typedef struct Node{
    int gray;
    int freq;
    struct Node *left, *right;
}Node;

typedef struct HeapNode{
    struct HeapNode *next;
    Node *node;
}HeapNode;

typedef struct PriorityQueue{
    HeapNode *head;
}PriorityQueue;

typedef struct HuffmanCode {
    unsigned char gray;
    int freq;
    char code[256];
} HuffmanCode;

Node* create_node(int gray, int freq);
HeapNode* create_heap_node(Node *node);
PriorityQueue* create_priority_queue();
void enqueue(PriorityQueue *p, Node *node);
Node* dequeue(PriorityQueue *p);

Node* build_huff_tree(int *freq);

void get_huff_codes(Node *root,char *code, int depth, HuffmanCode *huff_code, int *i, int *num_codes);

char* find_code(unsigned char gray, HuffmanCode *huff_codes, int num_codes);

BYTE get_bit(BYTE byte, int i);

/*functions*/
BYTE str2char(char *str); /*week 3 activity*/
void grayscale(int height, int width, BYTE *pixel_data, int *table);

void update_freq_table(int *table, int index);

int ret_leaf(int bit, Node *cur);

void freeHuffmanTree(Node* root);

int main(int argc, char *argv[]){
    /*argv[2] = bmp, argv[1] = flag*/
    /*using similar structure to my lab 4*/
    FILE *in, *out;
    tagBITMAPFILEHEADER file_head;
    tagBITMAPINFOHEADER info_head;
    BYTE *pixel_data, *start;
    BYTE padding;
    int height, width, i, j, k, num_pairs;
    int compression, decompression;

    char *extension, *file_name;

    int file_name_len;

    clock_t start_time, end_time;
    double total_time;

    Node *root;
    Node *cur;
    HuffmanCode huff_codes[256];
    char code[256];
    int index;

    /*info from compressed*/
    int c_num_pairs;
    
    char *found_code;

    char bit_buffer[100];
    char eight_bits[9];
    int  remaining_buffer_len, leading_0;

    BYTE conversion, last_code_len;

    BYTE gray, byte, bit;
    int freq, leaf_data;
    BYTE final_code, int_to_byte_gray;

    char end[] = "end";

    /*frequency table*/
    int freq_table[256] = {0};
    int *freq_ptr = freq_table;

    compression = 0;
    decompression = 0;

    index = 0;
    num_pairs = 0;
    freq = 0;
    leaf_data = 0;



    start_time = clock();
    /*check argv[1] for -g or -c*/
    if (strcmp(argv[1], "-g") != 0 && strcmp(argv[1], "-c") != 0){
        perror("Invalid flag. Exiting.\n");
        exit(EXIT_FAILURE);
    }
 
    file_name = argv[2];
/*check extensions for compression or decompression*/
    /*in file extension check*/
    /*if in file has ext .cbmp -> decompress, .bmp -> compress*/
    extension = strrchr(argv[2], '.');
    if (!extension){
        perror("Could not get extension. Exiting.\n");
        exit(EXIT_FAILURE);
    }
    else if (strcmp(extension + 1, "bmp") == 0){
        compression = 1;
    }
    else if (strcmp(extension + 1, "cbmp") == 0){
        decompression = 1;
    }
    else if (strcmp(extension + 1, "bmp") != 0 && strcmp(extension + 1, "cbmp") != 0){
        perror("In file does not have .bmp or .cbmp extension. Exiting.\n");
        exit(EXIT_FAILURE);
    }
    
    /*open in file*/
    in = fopen(argv[2], "rb");

/*read in in file-------------------------------------------------------------------------------*/
    if (fread(&(file_head.bfType), sizeof(WORD), 1, in) != 1){
        fclose(in);
        perror("Error reading file.bfType. Exiting.\n");
        exit(EXIT_FAILURE);
    }

    if (fread(&(file_head.bfSize), sizeof(DWORD), 1, in) != 1){
        fclose(in);
        perror("Error reading file.bfSize. Exiting.\n");
        exit(EXIT_FAILURE);
    }

    if (fread(&(file_head.bfReserved1), sizeof(WORD), 1, in) != 1){
        fclose(in);
        perror("Error reading file.bfReserved1. Exiting.\n");
        exit(EXIT_FAILURE);
    }

    if (fread(&(file_head.bfReserved2), sizeof(WORD), 1, in) != 1){
        fclose(in);
        perror("Error reading file.bfReserved2. Exiting.\n");
        exit(EXIT_FAILURE);
    }

    if (fread(&(file_head.bfOffBits), sizeof(DWORD), 1, in) != 1){
        fclose(in);
        perror("Error reading file.bfOffBits. Exiting.\n");
        exit(EXIT_FAILURE);
    }
    

    if (fread(&(info_head.biSize), sizeof(DWORD), 1, in) != 1){
        fclose(in);
        perror("Error reading file.biSize. Exiting.\n");
        exit(EXIT_FAILURE);
    }

    if (fread(&(info_head.biWidth), sizeof(LONG), 1, in) != 1){
        fclose(in);
        perror("Error reading file.biWidth. Exiting.\n");
        exit(EXIT_FAILURE);
    }

    if (fread(&(info_head.biHeight), sizeof(LONG), 1, in) != 1){
        fclose(in);
        perror("Error reading file.biHeight. Exiting.\n");
        exit(EXIT_FAILURE);
    }

    if (fread(&(info_head.biPlanes), sizeof(WORD), 1, in) != 1){
        fclose(in);
        perror("Error reading file.biPlanes. Exiting.\n");
        exit(EXIT_FAILURE);
    }

    if (fread(&(info_head.biBitCount), sizeof(WORD), 1, in) != 1){
        fclose(in);
        perror("Error reading file.biBitCount. Exiting.\n");
        exit(EXIT_FAILURE);
    }
    
    if (fread(&(info_head.biCompression), sizeof(DWORD), 1, in) != 1){
        fclose(in);
        perror("Error reading file.biCompression. Exiting.\n");
        exit(EXIT_FAILURE);
    }
    
    if (fread(&(info_head.biSizeImage), sizeof(DWORD), 1, in) != 1){
        fclose(in);
        perror("Error reading file.biSizeImage. Exiting.\n");
        exit(EXIT_FAILURE);
    }

    if (fread(&(info_head.biXPelsPerMeter), sizeof(LONG), 1, in) != 1){
        fclose(in);
        perror("Error reading file.biXPelsPerMeter. Exiting.\n");
        exit(EXIT_FAILURE);
    }

    if (fread(&(info_head.biYPelsPerMeter), sizeof(LONG), 1, in) != 1){
        fclose(in);
        perror("Error reading file.biYPelsPerMeter. Exiting.\n");
        exit(EXIT_FAILURE);
    }

    if (fread(&(info_head.biClrUsed), sizeof(DWORD), 1, in) != 1){
        fclose(in);
        perror("Error reading file.biClrUsed. Exiting.\n");
        exit(EXIT_FAILURE);
    }
    
    if (fread(&(info_head.biClrImportant), sizeof(DWORD), 1, in) != 1){
        fclose(in);
        perror("Error reading file.biClrImportant. Exiting.\n");
        exit(EXIT_FAILURE);
    }
/*end of reading in in file headers--------------------------------------------------------------------*/
/*make out file with proper extension*/
/*code from stack overflow*/
    if (compression){
        file_name_len = strlen(argv[2]);
        file_name[file_name_len - 4] = '\0';
        out = fopen(strcat(file_name, ".cbmp"), "wb");
    }
    else if (decompression){
        file_name_len = strlen(argv[2]);
        file_name[file_name_len - 5] = '\0';
        out = fopen(strcat(file_name, ".bmp"), "wb");
    }

    /*format file*/
    if (fwrite(&(file_head.bfType), sizeof(WORD), 1, out) != 1){
        fclose(out);
        fclose(in);
        perror("Error writing bfType to outfile. Exiting.\n");
        exit(EXIT_FAILURE);
    }
    if (fwrite(&(file_head.bfSize), sizeof(DWORD), 1, out) != 1){
        fclose(out);
        fclose(in);
        perror("Error writing bfSize to outfile. Exiting.\n");
        exit(EXIT_FAILURE);
    }
    if (fwrite(&(file_head.bfReserved1), sizeof(WORD), 1, out) != 1){
        fclose(out);
        fclose(in);
        perror("Error writing bfReserved1 to outfile. Exiting.\n");
        exit(EXIT_FAILURE);
    }
    if (fwrite(&(file_head.bfReserved2), sizeof(WORD), 1, out) != 1){
        fclose(out);
        fclose(in);
        perror("Error writing bfReserved2 to outfile. Exiting.\n");
        exit(EXIT_FAILURE);
    }
    if (fwrite(&(file_head.bfOffBits), sizeof(DWORD), 1, out) != 1){
        fclose(out);
        fclose(in);
        perror("Error writing bfOffBits to outfile. Exiting.\n");
        exit(EXIT_FAILURE);
    }

    if (fwrite(&(info_head.biSize), sizeof(DWORD), 1, out) != 1){
        fclose(out);
        fclose(in);
        perror("Error writing biSize to outfile. Exiting.\n");
        exit(EXIT_FAILURE);
    }
    if (fwrite(&(info_head.biWidth), sizeof(LONG), 1, out) != 1){
        fclose(out);
        fclose(in);
        perror("Error writing biWidth to outfile. Exiting.\n");
        exit(EXIT_FAILURE);
    }
    if (fwrite(&(info_head.biHeight), sizeof(LONG), 1, out) != 1){
        fclose(out);
        fclose(in);
        perror("Error writing biHeight to outfile. Exiting.\n");
        exit(EXIT_FAILURE);
    }
    if (fwrite(&(info_head.biPlanes), sizeof(WORD), 1, out) != 1){
        fclose(out);
        fclose(in);
        perror("Error writing biPlanes to outfile. Exiting.\n");
        exit(EXIT_FAILURE);
    }
    if (fwrite(&(info_head.biBitCount), sizeof(WORD), 1, out) != 1){
        fclose(out);
        perror("Error writing biBitCount to outfile. Exiting.\n");
        exit(EXIT_FAILURE);
    }
    if (fwrite(&(info_head.biCompression), sizeof(DWORD), 1, out) != 1){
        fclose(out);
        fclose(in);
        perror("Error writing biCompression to outfile. Exiting.\n");
        exit(EXIT_FAILURE);
    }
    if (fwrite(&(info_head.biSizeImage), sizeof(DWORD), 1, out) != 1){
        fclose(out);
        fclose(in);
        perror("Error writing biSizeImage to outfile. Exiting.\n");
        exit(EXIT_FAILURE);
    }
    if (fwrite(&(info_head.biXPelsPerMeter), sizeof(LONG), 1, out) != 1){
        fclose(out);
        fclose(in);
        perror("Error writing biXPelsPerMeter to outfile. Exiting.\n");
        exit(EXIT_FAILURE);
    }
    if (fwrite(&(info_head.biYPelsPerMeter), sizeof(LONG), 1, out) != 1){
        fclose(out);
        fclose(in);
        perror("Error writing biYPelsPerMeter to outfile. Exiting.\n");
        exit(EXIT_FAILURE);
    }
    if (fwrite(&(info_head.biClrUsed), sizeof(DWORD), 1, out) != 1){
        fclose(out);
        fclose(in);
        perror("Error writing biClrUsed to outfile. Exiting.\n");
        exit(EXIT_FAILURE);
    }
    if (fwrite(&(info_head.biClrImportant), sizeof(DWORD), 1, out) != 1){
        fclose(out);
        fclose(in);
        perror("Error writing biClrImportant to outfile. Exiting.\n");
        exit(EXIT_FAILURE);
    }
/*read pixel data------------------------------------------------------------------------------*/
    if(compression){
        padding = (4 - ((info_head.biWidth*3) % 4)) % 4;
        width = info_head.biWidth + padding;
        height = info_head.biHeight;

    /*
        printf("Padding: %d\n", padding);
        printf("original width: %d\n", info_head.biWidth);
        printf("width: %d\n", width);*/
    
        /*will have to fork accordding to pdf, use mmap*/
        pixel_data = mmap(NULL, 3*info_head.biHeight*width, PROT_READ|PROT_WRITE, MAP_SHARED|0x20, -1, 0);
        
        if (pixel_data == MAP_FAILED){
            fclose(in);
            perror("Unable to allocate space for pixel data. Exiting.\n");
            exit(EXIT_FAILURE);
        }
        
        start = pixel_data;
        /*read in pixel data: from lab 4*/
        /*make sure to test properly bc this had issues in lab 4*/
        if(fread(pixel_data, sizeof(BYTE), 3*info_head.biHeight*width, in) == 0){
            perror("Unable to read pixel data. Exiting.\n");
            pixel_data = start;
            munmap(pixel_data, 3*info_head.biHeight*width);
            fclose(in);
            exit(EXIT_FAILURE);
        }

        pixel_data = start;

 
    }
    else if(decompression){
        fread(&c_num_pairs, sizeof(int), 1, in);

        /*for storing info
        c_huff_codes = (HuffmanCode*)malloc(sizeof(HuffmanCode) * c_num_pairs);
        */
        for (i = 0; i < c_num_pairs; i++){
            fread(&gray, sizeof(BYTE), 1, in);
            fread(&freq, sizeof(int), 1, in);

            /*remake freq table*/
            freq_table[gray] = freq;
        }

        root = build_huff_tree(freq_ptr);
        cur = root;

        /*while end of file not reached*/
        while(fread(&byte, sizeof(BYTE), 1, in) != 0){
            if (byte == 'e'){
                fread(&byte, sizeof(BYTE), 1, in);

                if (byte == 'n'){

                    fread(&byte, sizeof(BYTE), 1, in);
                    if (byte == 'd'){
                 
                        fseek(in, 1, SEEK_CUR);
                        fread(&final_code, sizeof(BYTE), 1, in); /*get how long the final code is*/

                        fread(&byte, sizeof(BYTE), 1, in);
                        for (j = 0; j <= 9 - final_code; j++){
                            bit = get_bit(byte, j);

                            leaf_data = ret_leaf(bit, cur);
                            int_to_byte_gray = (BYTE)leaf_data;

                            if (leaf_data != -1){
                                /*leaf hit*/
                                /*add to pixel data array 3 times*/
                                fwrite(&int_to_byte_gray, sizeof(BYTE), 1, out);
                                fwrite(&int_to_byte_gray, sizeof(BYTE), 1, out);
                                fwrite(&int_to_byte_gray, sizeof(BYTE), 1, out);
                                cur = root;
                            }
                            else{
                                if (bit == 1){
                                    cur = cur->right;
                                }
                                else if (bit == 0){
                                    cur = cur->left;
                                }
                            }
                           /* printf("bit: %u\n", bit);*/
                        }
                        break;

                    }
                    else{
                        fseek(in, -3, SEEK_CUR);
                        fread(&byte, sizeof(BYTE), 1, in);
                    }

                }
                else{
                    fseek(in, -2, SEEK_CUR);
                    fread(&byte, sizeof(BYTE), 1, in);
                }
            }
            for (i = 7; i >= 0; i--){
                bit = get_bit(byte, i);

                leaf_data = ret_leaf(bit, cur);

                int_to_byte_gray = (BYTE)leaf_data;

                if (leaf_data != -1){
                    /*leaf hit*/
                    /*add to pixel data array 3 times*/
                    fwrite(&int_to_byte_gray, sizeof(BYTE), 1, out);
                    fwrite(&int_to_byte_gray, sizeof(BYTE), 1, out);
                    fwrite(&int_to_byte_gray, sizeof(BYTE), 1, out);
                    cur = root;
                }
                else{
                    if (bit == 1){
                        cur = cur->right;
                    }
                    else if (bit == 0){
                        cur = cur->left;
                    }
                }
            }
        }

        freeHuffmanTree(root);
    }
    
/*end reading pixel data-----------------------------------------------------------------------*/

    /*close in file*/
    fclose(in);

    /*check argv[1] for gray or color*/
    if (strcmp(argv[1], "-g") == 0 && compression){
        /*issue with 2by2 (maybe padding?)*/
        grayscale(height, width, pixel_data, freq_ptr);
        pixel_data = start;
    }

    if (compression){
        /*generate huffman codes*/
        root = build_huff_tree(freq_ptr);
        get_huff_codes(root, code, 0, huff_codes, &index, &num_pairs);
        freeHuffmanTree(root);

        /*codes match with my drawing, but test further
        for (i = 0; i < index; i++) {
            printf("Grayscale value: %d, Code: %s\n", huff_codes[i].gray, huff_codes[i].code);
            
        }*/

        /*write num of pairs*/
        fwrite(&num_pairs, sizeof(int), 1, out);

        for (i = 0; i < num_pairs; i++){
            /*write out freq gray pairs*/
            fwrite(&huff_codes[i].gray, sizeof(BYTE), 1, out);
            fwrite(&huff_codes[i].freq, sizeof(int), 1, out);

        }

        /*write pixel data: compressed*/
        memset(bit_buffer, 0, 100);
        for (i = 0; i < height; i++){
            for (j = 0; j < width; j++){
                pixel_data = start + 3*(width * i + j);

                found_code = find_code(*pixel_data, huff_codes, num_pairs);

                if (found_code == NULL){
                    perror("Could not find code for pixel. Exiting\n");
                    pixel_data = start;
                    munmap(pixel_data, 3*info_head.biHeight*width);
                    exit(EXIT_FAILURE);
                }
                
                /*printf("strlen of code: %d\n", code_len);*/

                strncat(bit_buffer, found_code, 100 - strlen(bit_buffer) - 1);


                /*if last element and buffer len less than 8, make it 8 by adding leading zeros*/
                if (i == height - 1 && j == width - 1){
                    /*add leading zeros or trailing zeros?*/
                    /*calc how many 0's needed*/
                    
                    if ((strlen(bit_buffer) < 8 && strlen(bit_buffer) > 0)){
                        leading_0 = 8 - strlen(bit_buffer);
                        if (leading_0 != 0){
                            memmove(bit_buffer + leading_0, bit_buffer, strlen(bit_buffer) + 1);
                            for (k = 0; k < leading_0; k++){
                                bit_buffer[k] = '0';
                            }
                        }
                    }
                    
                    /*write out some indication that this is the last*/
                    /*see what the actual code is and if need to handle */
                    fwrite(end, sizeof(end), 1, out);
                    /*write out length of final piece of code*/
                    last_code_len = 8 - leading_0;
                    fwrite(&last_code_len, sizeof(BYTE), 1, out);
                }

                while (strlen(bit_buffer) >= 8){

                    /*convert and write*/
                    memcpy(eight_bits, bit_buffer, 8);
                    eight_bits[8] = '\0';
                
                    /*clear from buffer*/
                    remaining_buffer_len = strlen(bit_buffer + 8);
                    memmove(bit_buffer, bit_buffer + 8, remaining_buffer_len);
                    memset(bit_buffer + remaining_buffer_len, 0, 100 - remaining_buffer_len);
                    
               
                    conversion = str2char(eight_bits);
                    
                    fwrite(&conversion, sizeof(BYTE), 1, out);
                }
            }
        }
    }
    
    fclose(out);
    if (compression){
        pixel_data = start;
        munmap(pixel_data, 3*info_head.biHeight*width);
    }
    end_time = clock();
    total_time = (double)(end_time - start_time)/CLOCKS_PER_SEC;
    total_time *= 1000; /*milliseconds*/

    printf("Time in ms: %f\n", total_time);
    return 0;
}

BYTE str2char(char *str){
    /*
    FROM WEEK 3 ACTIVITY
    Write a C function str2char() that takes a valid C String (possibly NULL) consisting
    exclusively of the digits 0 and 1 strips of any leading zeros and builds a char out of the
    remaining values. It should return the char on success or -1 if the number cannot be
    represented due to overflow.
    For example, str2char(“110000”) returns ‘O’ or 48 and str2char(NULL) returns
    0.
    */
    int i, ascii_value;

    char *end;
    char *temp = str;
    int len = strlen(temp);

    /*null case*/
    if (str == NULL){
        return 0;
    }

    /*move pointer*/
    while(*temp == '0'){
        temp++;
        len--; /*len of remaining*/
    }


    /*now at proper position*/
    /*convert string to ascii value of a char*/
    end = temp + (len - 1); 

    i = 0;
    ascii_value = 0;
    while (end != temp - 1){
        if (*end == '1'){
            ascii_value += 1 << i; /*keep running total*/
        }
        end--;
        i++;
    }

    if (ascii_value < 0){
        /*overflow*/
        return -1;
    }

    return (BYTE)ascii_value;
}

void grayscale(int height, int width, BYTE *pixel_data, int *table){
    int i, j, red, blue, green, gray;
    BYTE *start = pixel_data;

    for(i = 0; i < height; i++){
        for(j = 0; j < width; j++){
            pixel_data = start + 3*(width * i + j);
            blue = *pixel_data;
            green = *(pixel_data + 1);
            red = *(pixel_data + 2);

            gray = (int)(blue + green + red)/3;
/*
            printf("blue: %d\n", blue);
            printf("green: %d\n", green);
            printf("red: %d\n", red);
               printf("Gray: %d\n", gray);
*/
         

            *pixel_data = gray;
            *(pixel_data + 1) = gray;
            *(pixel_data + 2) = gray;

            update_freq_table(table, gray);
        }
    }
}

void update_freq_table(int *table,int index){
    table[index] ++;
}

/*------------------------------------------------------------------------------------*/
Node* create_node(int gray, int freq){
    Node *new = (Node*)malloc(sizeof(Node));
    new->freq = freq;
    new->gray = gray;
    new->left = NULL;
    new->right = NULL;

    return new;
}

HeapNode* create_heap_node(Node *node){
    HeapNode *new = (HeapNode*)malloc(sizeof(HeapNode));
    new->node = node;
    new->next = NULL;

    return new;
}

PriorityQueue* create_priority_queue(){
    PriorityQueue *p = (PriorityQueue*)malloc(sizeof(PriorityQueue));
    p->head = NULL;

    return p;
}

void enqueue(PriorityQueue *p, Node *node){
    HeapNode *cur;
    HeapNode *h = create_heap_node(node);

    if (p->head == NULL || p->head->node->freq > node->freq){
        /*new head*/
        h->next = p->head;
        p->head = h;
    }
    else{
        cur = p->head;
        while(cur->next != NULL && (cur->next->node->freq < node->freq ||
               (cur->next->node->freq == node->freq && cur->next->node->gray < node->gray))){
            
            cur = cur->next;
            
        }

        h->next = cur->next;
        cur->next = h;
    }
}

Node* dequeue(PriorityQueue *p){
    HeapNode *h;
    Node *n;

    if (p->head == NULL){
        return NULL;
    }

    h = p->head;
    n = h->node;

    p->head = h->next;

    free(h);

    return n;
}
/*--------------------------------------------------------------------------------------------------*/
Node* build_huff_tree(int *freq){
    int i, combined_key;
    Node *combined, *root;
    PriorityQueue *p = create_priority_queue();

    for (i = 0; i < 256; i++){
        if (freq[i] > 0){
            /*create node*/
            Node *n = create_node(i, freq[i]);
            enqueue(p, n);
        }
    }

    while (p->head->next != NULL) {
        Node *left = dequeue(p);
        Node *right = dequeue(p);
        

        if (left->gray <= right->gray){
            combined_key = left->gray;
        }
        else{
            combined_key = right->gray;
        }

        combined = create_node(combined_key, left->freq + right->freq);
        combined->left = left;
        combined->right = right;
        enqueue(p, combined);
    }
    root = dequeue(p);
    free(p);
    return root;
}

void get_huff_codes(Node *root, char *code, int depth, HuffmanCode *huff_code, int *i, int *num_codes){
    if (root->left == NULL && root->right == NULL) {
        huff_code[*i].gray = root->gray;
        huff_code[*i].freq = root->freq;
        strcpy(huff_code[*i].code, code);
        (*i)++;
        (*num_codes)++; /*returns number of codes*/
    }


    code[depth] = '0';
    code[depth + 1] = '\0';
    if (root->left != NULL) {
        get_huff_codes(root->left, code, depth + 1, huff_code, i, num_codes);
    }


    code[depth] = '1';
    code[depth + 1] = '\0';
    if (root->right != NULL) {
        get_huff_codes(root->right, code, depth + 1, huff_code, i, num_codes);
    }
}

char* find_code(unsigned char gray, HuffmanCode *huff_codes, int num_codes){
    int i;
    for (i = 0; i < num_codes; i++){
        if (huff_codes[i].gray == gray){
            return huff_codes[i].code;
        }
    }

    return NULL;
}

BYTE get_bit(BYTE byte, int i){
    BYTE mask, ret;

    mask = 1 << i;

    ret = (byte & mask) >> i;

    return ret;

}

int ret_leaf(int bit, Node *cur){
    if (bit == 1){
        cur = cur->right;
    }

    else if (bit == 0){
        cur = cur->left;
        
    }

    if (cur->left == NULL && cur->right == NULL){
        return cur->gray;
    }

    return -1;
}


void freeHuffmanTree(Node* root) {
    if (root == NULL)
        return;


    freeHuffmanTree(root->left);
    freeHuffmanTree(root->right);


    free(root);
}