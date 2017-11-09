#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/time.h>

int rows_int;
int cols_int;
FILE* fileC;
struct argus{ // arguments for threaded functions
    int AR;
    int AC;
    int BR;
    int BC;
    int *ArrA;
    int *ArrB;
    long *ArrC;
    int index_i;
    int index_j;
};
void setRowAndColInt(FILE* file);
int fromStrToDigit(char* str);
void readArr(FILE* file, int arr[rows_int][cols_int]);
void writeToFile(char* file_name, int rows, int cols, long matrix[rows][cols]);
void serialMultiplication(int ra, int ca, int arrA[ra][ca], int rb, int cb, int arrB[rb][cb] , long arrC[ra][cb]);
void case1Multiplication(int ra, int ca, int arrA[ra][ca], int rb, int cb, int arrB[rb][cb] , long arrC[ra][cb]);
void * rowMultiplyer(void * args);
void case2Multiplication(int ra, int ca, int arrA[ra][ca], int rb, int cb, int arrB[rb][cb] , long arrC[ra][cb]);
void * cellMultiplyer(void * args);

int main(int argc[], char * argv[])
{

    struct timeval stop, start; // to measure time of each case
    char * mat1, * mat2, * mat_out;
    if(argc == 4){
        mat1 = argv[1];
        mat2 = argv[2];
        mat_out = argv[3];
    }else{
        mat1 = "a.txt";
        mat2 = "b.txt";
        mat_out = "c.out";
    }

    // reading array A
    FILE* fileA = fopen(mat1,"r");
    if(fileA == NULL){
        printf("Error.. File: %s not Exist!\n", mat1);
        exit(1);
    }
    setRowAndColInt(fileA); // to set global vars with size of rows and cols of A
    int arrA[rows_int][cols_int];
    readArr(fileA, arrA);
    fclose(fileA);
    int a_row_size = rows_int;
    int a_col_size = cols_int;

    // reading array B
    FILE* fileB = fopen(mat2,"r");
    if(fileB == NULL){
        printf("Error.. File: %s not Exist!\n", mat2);
        exit(1);
    }
    setRowAndColInt(fileB); // to set global vars with size of rows and cols of B
    int arrB[rows_int][cols_int];
    readArr(fileB, arrB);
    fclose(fileB);
    int b_row_size = rows_int;
    int b_col_size = cols_int;
    if(b_row_size != a_col_size){
        printf("Error in matrices dimensions, A cols number must equal B rows number!!\n");
        exit(1);
    }

    long arrC[a_row_size][b_col_size];


    // multiplication
    remove(mat_out);
    gettimeofday(&start, NULL); //start checking time
    serialMultiplication(a_row_size,a_col_size,arrA, b_row_size, b_col_size, arrB, arrC);
    gettimeofday(&stop, NULL); //end checking time
    printf("Normal case - Microseconds taken: %lu\n", stop.tv_usec - start.tv_usec);
    writeToFile(mat_out, a_row_size, b_col_size, arrC); // write array C -the output- to the file

    // multiplication with thread with each row - case 1
    fileC = fopen(mat_out, "a");
    fprintf(fileC, "\nmethod 1 using row threading\n");
    gettimeofday(&start, NULL); //start checking time
    case1Multiplication(a_row_size,a_col_size,arrA, b_row_size, b_col_size, arrB, arrC);
    gettimeofday(&stop, NULL); //end checking time
    printf("Row case - Microseconds taken: %lu\n", stop.tv_usec - start.tv_usec);
    fclose(fileC);
    writeToFile(mat_out, a_row_size, b_col_size, arrC); // write array C -the output- to the file
    // multiplication with thread with each element - case 2
    fileC = fopen(mat_out, "a");
    fprintf(fileC, "\nmethod 2 using cell threading\n");
    gettimeofday(&start, NULL); //start checking time
    case2Multiplication(a_row_size,a_col_size,arrA, b_row_size, b_col_size, arrB, arrC);
    gettimeofday(&stop, NULL); //end checking time
    printf("Cell case - Microseconds taken: %lu\n", stop.tv_usec - start.tv_usec);
    fclose(fileC);
    writeToFile(mat_out, a_row_size, b_col_size, arrC); // write array C -the output- to the file


    return 0;
}

/*
* multiply matrices by row threads
*/
void case1Multiplication(int ra, int ca, int arrA[ra][ca], int rb, int cb, int arrB[rb][cb] , long arrC[ra][cb]){

    int i,j;
    pthread_t thread[ra];
    for(i = 0; i < ra; i++){
        struct argus *args = malloc(sizeof(struct argus)); // argumens for threaded function
        args->AR = ra;
        args->AC = ca;
        args->BR = rb;
        args->BC = cb;
        args->index_i = i;
        args->ArrA = arrA;
        args->ArrB = arrB;
        args->ArrC = arrC;
        pthread_create(&thread[i], NULL, rowMultiplyer, args);
    }
    for(i = 0; i < ra; i++){
        pthread_join(thread[i], NULL);
    }
    printf("Row case - Number of threads :  %i\n", ra);
}


void * rowMultiplyer(void * args){ // the function done by each thread

    struct argus *arguments;
    arguments = (struct argus *) args;
    int i = arguments->index_i;
    int cb = arguments->BC;
    int ca = arguments->AC;

    int *pa = arguments->ArrA;//ponter for first element of Array a
    int *pb = arguments->ArrB;//ponter for first element of Array b
    long *pc = arguments->ArrC;//ponter for first element of Array c

    int j, c;

    for(j = 0; j < cb; j++){
            int indA, indB, indC; // offset to the array pointer
            indC = (i*cb + j); // i is the rows index , cb is number of cols in B
            *(pc+ indC) = 0;
            for(c = 0; c < ca; c++){
                indA = (i*ca + c);
                indB = (c*cb + j);
                *(pc+ i*cb + j) += (long)(*(pa+indA) * *(pb+indB));
            }
    }
}

/*
* multiply matrices by cell threads
*/
void case2Multiplication(int ra, int ca, int arrA[ra][ca], int rb, int cb, int arrB[rb][cb] , long arrC[ra][cb]){

    int i,j,tidIndex = 0;
    pthread_t thread[ra*cb];
    for(i = 0; i < ra; i++){
        for(j = 0; j < cb; j++){
            struct argus *args = malloc(sizeof(struct argus));
            args->AR = ra;
            args->AC = ca;
            args->BR = rb;
            args->BC = cb;
            args->index_i = i;
            args->index_j = j;
            args->ArrA = arrA;
            args->ArrB = arrB;
            args->ArrC = arrC;
            pthread_create(&thread[tidIndex], NULL, cellMultiplyer, args);
            tidIndex++;
        }
    }
    for(i = 0; i < ra*cb; i++){
        pthread_join(thread[i], NULL);
    }
    printf("Cell case - Number of threads :  %i\n", (ra*cb));
}

void * cellMultiplyer(void * args){ // the function done by each thread

    struct argus *arguments;
    arguments = (struct argus *) args;
    int ca = arguments->AC;
    int i = arguments->index_i;
    int j = arguments->index_j;
    int cb = arguments->BC;

    int *pa = arguments->ArrA;//ponter for first element of Array a
    int *pb = arguments->ArrB;//ponter for first element of Array b
    long *pc = arguments->ArrC;//ponter for first element of Array c

    int c,indA, indB, indC;
    indC = (i*cb + j); // i is the rows index , cb is number of cols in B
    *(pc+ indC) = 0;
    for(c = 0; c < ca; c++){
        indA = (i*ca + c); // i is the rows index ,c is number of cell index -counter-, ca is number of cols in A
        indB = (c*cb + j);
        *(pc+ i*cb + j) += (long) (*(pa+indA) * *(pb+indB));
    }
}

/* multiply matrix a with matrix b then put the output in c*/
void serialMultiplication(int ra, int ca, int arrA[ra][ca], int rb, int cb, int arrB[rb][cb] , long arrC[ra][cb]){

    int i, j,c;
    for(i = 0; i < ra; i++){
        for(j = 0; j < cb; j++){
            arrC[i][j] = 0;
            for(c = 0; c < ca; c++)
                arrC[i][j] += (long)(arrA[i][c] * arrB[c][j]);
        }
    }
}


/* write an array[rows][cols] to a file*/
void writeToFile(char* file_name, int rows, int cols, long matrix[rows][cols]){

    FILE* file = fopen(file_name, "a");
    int i , j;
    long number;
    for ( i = 0 ; i < rows ; i++){
        for (j = 0 ; j < cols-1 ; j++){
            number = matrix[i][j];
            fprintf(file, "%ld ", number);
        }
        number = matrix[i][j];
        fprintf(file, "%ld", number); // write last element at the end of the line without space
        fprintf(file, "\n");
    }
    fclose(file);

}

/* Rad Array from file and put it into arr*/
void readArr(FILE* file, int arr[rows_int][cols_int]){
    int i = 0 , j = 0;
    while(!feof(file)){
        int num = 0;
        fscanf (file, "%d", &num);
        arr[i][j] = num;
        j++;
        if(j == cols_int){
            j = 0;
            i++;
        }
    }
}

/* function to get rows size and cols size from the file*/
void setRowAndColInt(FILE* file){

    char* rows = (char*)malloc(50* sizeof(char));
    char* cols = (char*)malloc(50* sizeof(char));
    char ch ;
    int i;
    for(i = 0; i < 4; i++) fgetc(file);// to reach number of rows char - remove "row="
    i = 0;
    ch = fgetc(file);
    while(ch != ' '){
        *(rows+i) = ch;
        i++;
        ch = fgetc(file);
    }
    *(rows+i) = '\0';
    for(i = 0; i < 4; i++) fgetc(file);// to reach number of cols char - remove "col="
    i = 0;
    ch = fgetc(file);
    while(ch != '\n'){
        *(cols+i) = ch;
        i++;
        ch = fgetc(file);
    }
    *(cols+i) = '\0';
    rows_int = fromStrToDigit(rows);
    cols_int = fromStrToDigit(cols);
    free(rows); // deallocate size
    free(cols);// deallocate size

}

/*convert string digits to integer*/
int fromStrToDigit(char* str){
    int i = 0;
    int result = 0;
    while (str[i] != '\0') {
        int digit = str[i] - 48; // ascii code of zero = 48
        int j;
        result = result*10;
        result = result + digit;
        i++;
    }
    return result;
}
