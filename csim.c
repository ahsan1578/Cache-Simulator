/*
 * Author: D M Raisul Ahsan
 * NetID: ahsan1578
 * Date: 11/22/2019
 *
 *
 * This is a cache simulator.
 * For given trace file of a program (produced by valgrind), the simulator simulates the cache activities
 * It finds out the hits and misses in the cache memory
 * It also finds the evictions
 * When run with verbose mode, the program outputs the hits, misses and evictions for each load, store or modification
 * If run without verbose mode, it only prints the number of hits, misses and evictions
 */

#include "cachelab.h"
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

int lines;

/*
 * The line represents the line of a set
 * We are not considering the blocks
 * if the tag and set bits matches, the block bits (block number) must be available in the line
 * because the block has a block number for every combination of block bits
 */
struct Line{
    int validBit;
    unsigned long tag;
    int operationNumber;
};

/*
 * This function is used to check if the string contains any numbers only
 * that if the string is a string representation of a number
 */
bool checkIfNumber(const char *string, size_t size){
    for(size_t i = 0; i<size; i++){
        if(string[i]<'0' || string[i]>'9'){
            return false;
        }
    }
    return true;
}


/*
 * This function checks if the arguments passed are in valid format
 * If there are 10 arguments, 2nd argument should be -v
 * If there are nine arguments, 2nd argument should be -s
 */
bool checkValidArgs(char *argv[], int numArgs){
    int x = 1;
    if(numArgs==10){
        x = 2;
        if(strcmp(argv[1],"-v")!=0){
            return false;
        }
    }
    if(strcmp(argv[x+0], "-s") != 0){
        return false;
    }
    if(strcmp(argv[x+2], "-E") != 0){
        return false;
    }
    if(strcmp(argv[x+4], "-b") != 0){
        return false;
    }
    if(strcmp(argv[x+6], "-t") != 0){
        return false;
    }
    if(!checkIfNumber(argv[x+1], strlen(argv[x+1])) ||
       !checkIfNumber(argv[x+3], strlen(argv[x+3]) )||
       !checkIfNumber(argv[x+5], strlen(argv[x+5]))){
        return false;
    }
    return true;
}

/*
 * This function splits the set bits and tag bits from the address
 * We are considering that the address is 64 bits
 */
void setTagAndSet(unsigned int *set, unsigned long *tag, unsigned long addr, int s, int b){
    *tag = addr>>(s+b);
    unsigned long x = addr >> b;
    *set = (unsigned int)((x<< ((sizeof(long)<<3)-s))>>((sizeof(long)<<3)-s));
}

/*
 * This is the function that actually runs the algorithm for checking hits and misses
 * First it goes to the correct set
 * It checks the valid bit and tag for each line
 * If the valid bit is 1 and the tag matches, it is a hit
 * In that case, the the function just increases of the number of operations by 1
 * Then it changes the operation number of the block to current number of operations
 * The number of operations is the total operations done on the cache so far and used to find the LRU line
 * If it's a miss, a loop finds the LRU line of that set
 * Then it changes the tag of the line to the tag of the address and increases the number of operations as before
 * When changing the tag, if the valid bit is 1, it means a eviction
 * If valid bit bit is 0, it changes valid bit to 1
 */
bool checkHitAndMiss(struct Line cache[][lines], unsigned long tag, unsigned int set, int E, int *operationNum, int *numHits, int *numMisses, bool withVerbose){
    bool isHit = false;
    bool isEviction = false;
    for(int i = 0; i<E; i++){
        if(cache[set][i].validBit == 1 &&
                (cache[set][i].tag == tag)){
            *operationNum = *operationNum+1;
            cache[set][i].operationNumber = *operationNum;
            *numHits = *numHits + 1;
            isHit = true;
            if(withVerbose)
                printf("hit ");
            break;
        }
    }
    if(!isHit){
        int lru_line = 0;
        int lru_op = *operationNum;
        for(int i = 0; i<E; i++){
            if(cache[set][i].operationNumber<=lru_op){
                lru_op = cache[set][i].operationNumber;
                lru_line = i;
            }
        }
        if(cache[set][lru_line].validBit == 1){
            isEviction =true;
        }
        cache[set][lru_line].validBit = 1;
        cache[set][lru_line].tag = tag;
        *operationNum = *operationNum+1;
        cache[set][lru_line].operationNumber = *operationNum;
        *numMisses = *numMisses + 1;
        if(withVerbose)
            printf("miss ");
    }
    return isEviction;
}


/*
 * This function at the beginning initiates the cache with all values 0s
 * The cache is essentially a 2D array of line struct
 * The rows are the sets, and the columns are the lines in the set
 * Then the function reads the trace file line by line and checks the hits and misses using the above function
 * At the end, it prints the number of hits and misses
 */
void runSimulation(int s, int E, int b, FILE *filePointer, bool withVerbose, int *hits, int *misses, int *evictions){
    int S = (int)pow(2,s);
    int operationNum = 0;
    int numEviction = 0;
    int numHits = 0;
    int numMisses = 0;
    struct Line cache[S][E];
    for(int i = 0; i<S; i++){
        for(int j = 0; j<E; j++){
            cache[i][j].validBit = 0;
            cache[i][j].tag = 0;
            cache[i][j].operationNumber = operationNum;
        }
    }
    char line[128];
    while (fgets(line, sizeof(line), filePointer) != NULL){
        unsigned long addr;
        char *operation;
        char *split = strtok(line," ");
        operation = split;
        char *addToken = strtok(NULL, ",");
        addr = (unsigned long)strtol(addToken,NULL,16);
        unsigned int set = 0;
        unsigned long tag = 0;
        setTagAndSet(&set, &tag, addr,s,b);
        if(strcmp(operation, "M")==0){
            if(withVerbose)
                printf("%s %lx ",operation,addr);
            bool isEviction = checkHitAndMiss(cache, tag, set, E, &operationNum, &numHits, &numMisses,withVerbose);
            if(isEviction)
                numEviction++;
            if(withVerbose)
                if(isEviction)
                    printf("eviction ");
            isEviction = checkHitAndMiss(cache, tag, set, E, &operationNum, &numHits, &numMisses,withVerbose);
            if(isEviction)
                numEviction++;
            if(withVerbose){
                if(isEviction)
                    printf("eviction\n");
                else
                    printf("\n");
            }
        }else if(strcmp(operation,"I") != 0){
            if(withVerbose)
                printf("%s %lx ",operation,addr);
            bool isEviction = checkHitAndMiss(cache, tag, set, E, &operationNum, &numHits, &numMisses,withVerbose);
            if(isEviction)
                numEviction++;
            if(withVerbose){
                if(isEviction)
                    printf("eviction\n");
                else
                    printf("\n");
            }
        }
    }
    fclose(filePointer);
    *hits = numHits;
    *misses = numMisses;
    *evictions = numEviction;
}


/*
 * This function prints the instruction to run the program when the program is run only with -h (help) flag
 */
void printHelp(){
    printf("Usage: ./csim-ref [-hv] -s <s> -E <E> -b <b> -t <tracefile>\n");
    printf("\t-h: Optional help flag that prints usage info\n");
    printf("\t-v: Optional verbose flag that displays trace info\n");
    printf("\t-s <s>: Number of set index bits (S = 2^s is the number of sets)\n");
    printf("\t-E <E>: Associativity (number of lines per set)\n");
    printf("\t-b <b>: Number of block bits (B = 2^b is the block size)\n");
    printf("\t-t <tracefile>: Name of the valgrind trace to replay/run simulation on\n");
}


/*
 * Main function gets the arguments
 * The it checks validity of arguments
 * It terminates the program if the arguments are not valid
 * Otherwise it runs the program according to the arguments provided
 * When everything is fine (valid arguments), it essentially calls the runSimulation function
 */
int main(int argc, char *argv[]) {
    bool withVerbose = false;
    int hits;
    int misses;
    int evictions;
    char *arg1;
    FILE *filePointer;
    int setVal, lineVal, blockVal;
    if(argc <2){
        printf("Invalid argument. Try using -h argument for help\n");
        return 0;
    }else if(argc == 2){
        arg1 = argv[1];
        if(arg1[1] != 'h'){
            printf("Invalid argument. Try using -h argument for help\n");
            return 0;
        }else{
            printHelp();
            return 0;
        }
    }else if(argc == 9){
        if(checkValidArgs(argv, argc)){
            setVal = atoi(argv[2]);
            lineVal = atoi(argv[4]);
            blockVal = atoi(argv[6]);
            filePointer = fopen(argv[8],"r");
        } else{
            printf("Invalid argument. Try using -h argument for help\n");
            return 0;
        }
    } else if(argc == 10){
        if(checkValidArgs(argv, argc)){
            withVerbose = true;
            setVal = atoi(argv[3]);
            lineVal = atoi(argv[5]);
            blockVal = atoi(argv[7]);
            filePointer = fopen(argv[9], "r");
        } else{
            printf("Invalid argument. Try using -h argument for help\n");
            return 0;
        }
    } else{
        printf("Invalid argument. Try using -h argument for help\n");
        return 0;
    }
    if(filePointer == NULL){
        printf("Invalid trace file name\n");
        return 0;
    }
    lines = lineVal;
    runSimulation(setVal,lineVal,blockVal,filePointer, withVerbose, &hits, &misses, &evictions);
    printSummary(hits,misses,evictions);
    return 0;
}
