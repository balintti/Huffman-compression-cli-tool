#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>

#define AsciiLenght 128

typedef struct leaf{
    bool haveValue;
    char value;
    int appearence;
    struct leaf *left;
    struct leaf *right;
}leaf;

typedef struct dict{
    int *path;
    char character;
    int pathLenght;
}dict;

typedef struct binaryWriter{
    FILE *file;
    unsigned char buffer;
    int bitCount;
}binaryWriter;

typedef struct binaryReader{
    FILE * file;
    unsigned char buffer;
    int bitCount;
}binaryReader;

int categarizeChars(int *AsciiCounter, char *text);
int compress(char * text, char *destFile);
int matchingFileExtension(char *fileName, char *extension);
void help();
int strCompare(char *a, char *b);
long int sizeOfFile(FILE *file);
void copyFilesContent(FILE *file, long int lenght, char *stringDest);
long int findEndOfStr(char *str);
leaf * buildTree(int *asciiCounter);
dict ** initDict(leaf *treeTop, int *charFrequency);
void treeScan(leaf *node, dict ** dictionary, int * index, int sizeOfPath, int * path);
void freeTree(leaf *node);
long int countCompressedBitsequencLenght(dict **dictionary, char *str, int lenghtOfDict);
int saveConversion(binaryWriter *bitwriter, int *characterCount, long int bitSequenceLenght, dict** dictionary, char * text, int dictLenght);
int decompress(char *compressedFile, char *dest);
int readBit(binaryReader *reader);

//        o
//       / |
//      0   1




int main(int argc, char *argv[]){

    // check for sufficiently many arguments argc should be 4 (program name), (flag), (input file), (output file name) or with help option
    if (argc != 4 && (argc != 2 && strCompare(argv[1], "-h"))){
        printf("--ERROR-- [CALLING ERROR]\n");
        printf("wrong argument count: %d\n", argc);
        // prints cli tools uses
        help(); 
        return 1;
    }


    if (strCompare(argv[1], "-c") && argc == 4){

        // checks if inputfile is a text file
        if (!matchingFileExtension(argv[2], ".txt")){
            printf("--ERROR-- [FILE ERROR]\n");
            printf("The compressor only compresses .txt files\n");
            // prints cli tools uses
            help();
            return 1;
        }

        FILE *fileToCompress = fopen(argv[2], "r");
        if (fileToCompress == NULL){
            printf("--ERROR-- [FILE ERROR]\n");
            printf("Error occurs when program opens file. Please check whether a file with this name exists.\n");

            help();
            return 1;
        }

        long int fileSize = sizeOfFile(fileToCompress); // calculate size of file

        char *stringToCompress = (char*) malloc(fileSize * sizeof(char));

        // copy content form txt into string
        copyFilesContent(fileToCompress, fileSize, stringToCompress);

        fclose(fileToCompress);


        if (compress(stringToCompress, argv[3])){
            
            free(stringToCompress);
            return 1;
        }

        free(stringToCompress);
    }
    else if (strCompare(argv[1], "-d") && argc == 4){

        // checks if inputfile is a binary file
        if (!matchingFileExtension(argv[2], ".bin")){
            printf("--ERROR-- [FILE ERROR]\n");
            printf("The compressor only compresses .txt files\n");
            // prints cli tools uses
            help();
            return 1;
        }
        if (decompress(argv[2], argv[3])){
            return 1;
        }

    }
    else if (strCompare(argv[1], "-h") && argc == 2){
        // prints cli tools uses
        help();
        return 0;
    }
    else {
        printf("--ERROR-- [CALLING ERROR]\n");
        printf("Non recognized flag\n");
        // prints cli tools uses
        help();
        return 1;
    }

    return 0;
}

// decompress function handle all file operations and decompression
int decompress(char *compressedFile, char *dest){

    // init bitreader
    binaryReader bitReader;
    bitReader.buffer = 0;
    bitReader.bitCount =0;
    
    // opens compressed file
    bitReader.file = fopen(compressedFile, "rb");
    if (bitReader.file == NULL){
        printf("--ERROR-- [FILE ERROR]\n");
        printf("Error occurs when program opens file. Please check whether a file with this name exists.\n");

        help();

        return 1;
    }

    // character counter ascii array
    int *characterCounter = (int *) calloc(AsciiLenght , sizeof(int));
    int errorInCharacterCountRead = 0;

    // read character appearance from compressed foles key
    while (1){
        char character = fgetc(bitReader.file);

        // break on separator
        if (character == '\0') break;

        int count;
        fread(&count, sizeof(int), 1, bitReader.file);

        if ( (int) character < 0 || (int) character > AsciiLenght-1 ){
            errorInCharacterCountRead = 1;
            break;
        }

        characterCounter[(int) character] = count;
    }

    // handles error when there is a non ascci character in the key
    if (errorInCharacterCountRead){
        printf("--ERROR-- [NON ASCII CHARACTER]\n");
        printf("Program found a non ascii character in compressed file\n");
        fclose(bitReader.file);
        free(characterCounter);
        return 1;
    }

    // reads the lenght of the sequence
    long int sequenceLenght;
    fread(&sequenceLenght, sizeof(long int), 1, bitReader.file);

    // claculate size of the original file from charaterCounter
    long int charactersInOriginal = 1; // 1 beacuse i have to add \0 manually

    for (int i = 0; i < AsciiLenght; i++){
        charactersInOriginal += characterCounter[i];
    }

    // mallock string for decompressed string
    char *decompressed = (char *) malloc(charactersInOriginal*sizeof(char));

    // create tree for decompression
    leaf *topLeaf =buildTree(characterCounter);
    
    // process sequence
    int bit;
    leaf *currentLeaf = topLeaf;
    long int index = 0;

    for (int i = 0; i < sequenceLenght; i++) {
        // if leaf has value add it to the string and go back to the top
        if (currentLeaf -> haveValue){
            decompressed[index] = currentLeaf -> value;
            index++;
            currentLeaf = topLeaf;
        }

        bit = readBit(&bitReader);
        if (bit == -1) break;

        // travels down the tree to find value
        currentLeaf = ((bit) ? currentLeaf -> right : currentLeaf -> left);

    }

    // add \0 manually
    decompressed[charactersInOriginal-1] = '\0';

    int sizeOfFileName = findEndOfStr(dest);
    char outputFileName[sizeOfFileName+5];
    sprintf(outputFileName, "%s.txt", dest);

    FILE *destFile = fopen(outputFileName, "w");
    if (destFile == NULL){
        printf("--ERROR-- [FILE ERROR]\n");
        printf("Error occurs when program opens file.\n"); 
        help();

        freeTree(topLeaf);
        fclose(bitReader.file);
        free(decompressed);
        free(characterCounter);
        
        return 1;
    }

    fputs(decompressed, destFile);

    printf("---DECOMPRESSION COMPLETED SUCCESSFULLY---\n");
    printf("Decompressed file: %s\n", outputFileName);

    fclose(destFile);
    freeTree(topLeaf);
    fclose(bitReader.file);
    free(decompressed);
    free(characterCounter);
    return 0;
}

// reads one bit at a time from file with buffer returns the bit or -1 if end of file
int readBit(binaryReader *reader){
    // if buffer is empty fills it
    if (reader -> bitCount == 0){
        // reads byte
        int byte = fgetc(reader->file);
        // return -1 if end of file
        if (byte == EOF) return -1;

        // add byte to buffer
        reader-> buffer = byte;
        reader-> bitCount = 8;
    }

    // read bit from buffer
    reader -> bitCount--;
    int bit = (reader -> buffer >> reader -> bitCount) & 1;
    
    return bit;

}

// compress function called when the program called with flag -c
int compress(char * text, char *destFile){ 

    // every entry will store the count of the character what is represented with the same index in the ascii table
    int *characterCounter = (int *) calloc(AsciiLenght , sizeof(int));
    
    // count characters with error handling
    if (categarizeChars(characterCounter, text)){
        free(characterCounter);
        return 1;
    }

    // removeing \0 from the string and adding them manually so i can use it as separator it shouldnt count it but better to be sure
    characterCounter[(int)'\0'] = 0;

    // create tree
    leaf *topLeaf =buildTree(characterCounter);
    

    // create dict for compressing
    dict **dictionary = initDict(topLeaf, characterCounter);
    
    // count different characters
    int differentCharacters = 0;

    for (int i = 0; i < AsciiLenght; i++){
        if (characterCounter[i] > 0) differentCharacters++;
    }
    

    // returns the lengt of the bitsequence after compression
    long int compressedLenght= countCompressedBitsequencLenght( dictionary, text, differentCharacters);
    

    // writing into binary file
    int sizeOfFileName = findEndOfStr(destFile);
    char outputFileName[sizeOfFileName+5];
    sprintf(outputFileName, "%s.bin", destFile);

    // init bitwriter for sequence
    binaryWriter  bitWriter;
    bitWriter.file = fopen(outputFileName, "wb");
    if (bitWriter.file == NULL){
        printf("--ERROR-- [FILE ERROR]\n");
        printf("Error occurs when program opens file.\n"); 
        help();
        for (int i = 0; i < differentCharacters; i++){
            free(dictionary[i]->path);
            free(dictionary[i]);
        }
        //free dictionary!!!!!!!!!!!!!!!!!!!!!!!!!!
        free(dictionary);

        //free tree!!!!!!!!!!
        freeTree(topLeaf);

        free(characterCounter);
        return 1;
    }
    bitWriter.buffer = 0;
    bitWriter.bitCount = 0;

    // save conversion into the begining of the file key + sequence
    if(saveConversion(&bitWriter, characterCounter, compressedLenght, dictionary, text, differentCharacters)){
        printf("--ERROR-- [FILE ERROR]\n");
        printf("Error occurs when program opens file.\n"); 
        help();
        fclose(bitWriter.file);
        for (int i = 0; i < differentCharacters; i++){
            free(dictionary[i]->path);
            free(dictionary[i]);
        }
        //free dictionary!!!!!!!!!!!!!!!!!!!!!!!!!!
        free(dictionary);

        //free tree!!!!!!!!!!
        freeTree(topLeaf);

        free(characterCounter);
        return 1;
    }

    printf("---COMPRESSION COMPLETED SUCCESSFULLY---\n");
    printf("Compressed file: %s\n", outputFileName);


    // close output file
    fclose(bitWriter.file);
    //free dictionary!!!!!!!!!!!!!!!!!!!!!!!!!!
    for (int i = 0; i < differentCharacters; i++){
        free(dictionary[i]->path);
        free(dictionary[i]);
    }
    free(dictionary);

    //free tree!!!!!!!!!!
    freeTree(topLeaf);

    free(characterCounter);
    return 0;
}

// vrites conversion into binary file returs 0 if runs without error and 1 with error key + sequence
// different characters appearence | sep | lenght of bit sequence | sequence
// char | int | ... | char | int | \0 | long int | bitsequence
// 1byt | 2byt| ... | 1byt | 2byt| 1byt| 4byte | sequence
// the conversion key is the part before the sequence
// the sequence is converted into bits with the dictionary
int saveConversion(binaryWriter *bitwriter, int *characterCount, long int bitSequenceLenght, dict** dictionary, char * text, int dictLenght){
    for (int i = 0; i < AsciiLenght; i++){
        if (characterCount[i] != 0){
            // character
            fputc((char) i, bitwriter->file);
            // appearence
            fwrite(&characterCount[i], sizeof(int), 1, bitwriter->file);
        }
    }
    // separator
    fputc((char) '\0', bitwriter->file);
    // sequence lenght
    fwrite(&bitSequenceLenght, sizeof(long int), 1, bitwriter->file);

    // sequence
    long int index = 0;

    while(1){
        if (text[index] == '\0') break;

        char value = text[index];
        
        for (int i = 0; i < dictLenght; i++){
            
            if (dictionary[i]->character == value){
                
                for (int j =0; j < dictionary[i]->pathLenght; j++){
                    // add bit to buffer
                    bitwriter->buffer = (bitwriter->buffer << 1) | dictionary[i]->path[j];
                    bitwriter->bitCount++;

                    // if buffer is full write it to the file
                    if (bitwriter->bitCount == 8){
                        fputc(bitwriter->buffer, bitwriter->file);
                        bitwriter->buffer = 0;
                        bitwriter->bitCount = 0;
                    }
                }
                
                break;
            }
        }
        index++;
    }

    // put end of the buffer into the file
    if (bitwriter->bitCount != 0){
        bitwriter->buffer = (bitwriter->buffer << (8 - bitwriter->bitCount));
        fputc(bitwriter->buffer, bitwriter->file);
        bitwriter->buffer = 0;
        bitwriter->bitCount = 0;
    }

    return 0;

}

// returns the lenght of the compressed bit sequence
long int countCompressedBitsequencLenght(dict **dictionary, char *str, int lenghtOfDict){

    long int index = 0;
    long int lenght = 0;
    while(1){
        if (str[index] == '\0') break;

        char value = str[index];
        
        for (int i = 0; i < lenghtOfDict; i++){
            
            if (dictionary[i]->character == value){
                lenght += dictionary[i]->pathLenght;
                break;
            }
        }
        index++;
    }
    
    return lenght;
}

// frees tree LRF <- method
void freeTree(leaf *node){
    if (node == NULL) return;

    // go left 
    freeTree(node->left);
    // than right
    freeTree(node->right);
    // and free after all of its children is freed
    free(node);
}

// save tree into binary

// init dictionary for compression
dict ** initDict(leaf *treeTop, int *charFrequency){

    int differentCharacters = 0;

    for (int i = 0; i < AsciiLenght; i++){
        if (charFrequency[i] > 0) differentCharacters++;
    }

    dict ** dictionary = (dict**) calloc(differentCharacters, sizeof(dict*));
    int index = 0;
    int *indexPointer = &index;
    int sizeOfPath = 0;
    int * pathHolder = (int *) calloc(AsciiLenght, sizeof(int));
    
    treeScan(treeTop, dictionary, indexPointer, sizeOfPath, pathHolder);

    free(pathHolder);
    return dictionary;

}

// travels down the tree and build the dictionary for compression left: 0; right: 1
void treeScan(leaf *node, dict ** dictionary, int * index, int sizeOfPath, int * path){
    
    if (node == NULL) {
        return;
    }
    
    if (node ->haveValue){
        
        // add to dictionary
        dict *newDict = (dict*) malloc(sizeof(dict));
        newDict -> character = node -> value;
        newDict -> pathLenght = sizeOfPath;
        newDict -> path = (int*) malloc(sizeOfPath*sizeof(int));

        // fill new dictionary elements path with the correct path
        for(int i = 0; i < sizeOfPath; i++){
            newDict -> path[i] = path[i];
        }

        // adding new element to dictionary
        dictionary[*index] = newDict;
        
        // incrementing index for dictionary
        (*index)++;
        return;
    }

    
    // left path
    path[sizeOfPath] = 0;
    treeScan(node->left, dictionary, index, sizeOfPath + 1, path);

    // right path
    path[sizeOfPath] = 1;
    treeScan(node->right, dictionary, index, sizeOfPath + 1, path);

}

// builds binarytree from charFrequency
leaf * buildTree(int *charFrequency){

    int differentCharacters = 0;

    for (int i = 0; i < AsciiLenght; i++){
        if (charFrequency[i] > 0) differentCharacters++;
    }
    
    // an array what holds the leafs and build a tree from them
    leaf **leafHolder = (leaf **) malloc(differentCharacters*sizeof(leaf*));

    // init leaf array "waiting room"
    int counter = 0;
    for (int i = 0; i < AsciiLenght; i++){
        if (charFrequency[i] > 0){
            // create and init new leaf
            leaf * newLeaf = (leaf*) malloc(sizeof(leaf));
            newLeaf -> haveValue = true;
            newLeaf -> value = (char) i;
            newLeaf -> appearence = charFrequency[i];
            newLeaf -> left = NULL;
            newLeaf -> right = NULL;
            // add leaf to holder
            leafHolder[counter] = newLeaf;
            counter++;
        }
    }

    
    if(differentCharacters == 1){

        // init new leaf
        leaf *newLeaf = (leaf *) malloc(sizeof(leaf));
        newLeaf -> haveValue = false;
        newLeaf -> appearence = leafHolder[0] -> appearence;
        newLeaf -> left = leafHolder[0];
        newLeaf -> right = leafHolder[0];
        
        leaf *topLeaf = newLeaf;
        free(leafHolder);
    
        return topLeaf;
    }
    else{
        // it needs n-1 steps to build the tree
        for (int i = 0; i < differentCharacters-1; i++){
            int minA = -1;
            int minB = -1;

            // finding the two smallest leaves
            for (int j = 0; j < differentCharacters; j++){
                // check whether a position holds a leaf
                if(leafHolder[j] != NULL){
                    // first two leaf
                    if (minA == -1) {
                        minA = j;
                        continue;
                    }
                    if (minB == -1){
                        if (leafHolder[j]->appearence < leafHolder[minA]->appearence){
                            minB = minA;
                            minA = j;
                            continue;
                        }
                        else{
                            minB = j;
                            continue;
                        }
                    }

                    // other leaves
                    if (leafHolder[j]->appearence < leafHolder[minA]->appearence){
                        minB = minA;
                        minA = j;
                        continue;
                    }
                    else if (leafHolder[j]->appearence < leafHolder[minB]->appearence){
                        minB = j;
                        continue;
                    }
                }
            }
            // init new leaf
            leaf *newLeaf = (leaf *) malloc(sizeof(leaf));
            newLeaf -> haveValue = false;
            newLeaf -> appearence = leafHolder[minA] -> appearence + leafHolder[minB] -> appearence;
            newLeaf -> left = leafHolder[minA];
            newLeaf -> right = leafHolder[minB];
            // add new leaf to "waiting room"
            leafHolder[minA] = newLeaf;
            // remove used leaf
            leafHolder[minB] = NULL;
        }
        leaf *topLeaf;
        for (int i = 0; i < differentCharacters; i++){
            if(leafHolder[i] != NULL){
                topLeaf = leafHolder[i];
                break;
            }
        }

        free(leafHolder);
        
        return topLeaf;
    }

    
}

// copies a files content ito a variable
void copyFilesContent(FILE *file, long int lenght, char *stringDest){
    long int maxCount = lenght;
    long int startingIndexInString = 0;
    
    // appends every line to the string
    while (fgets(&stringDest[startingIndexInString], maxCount, file) != NULL){
        long int indexOfEndOfStr = findEndOfStr(stringDest);
        startingIndexInString = indexOfEndOfStr;
        maxCount = lenght - startingIndexInString;
    }

    // printf("%ld, %ld\n", startingIndexInString, lenght);
}

// return the index of \0 in a str
long int findEndOfStr(char *str){
    long int index = 0;

    while(1){
        if(str[index] == '\0'){
            return index;
        }
        index++;
    }
    return -1;
}

// return the size of file int bytes
long int sizeOfFile(FILE *file){
    // moves "curzor" end of file
    fseek(file, 0L, SEEK_END);
    // cehck its position
    long int lenght = ftell(file);
    // setback the starting state
    fseek(file, 0L, SEEK_SET);

    return lenght;
}

// returns 1 on matching strings
int strCompare(char *a, char *b){ 
    
    int i = 0;
    while (1){
        // checks if their end is the same and breaks from cycle
        if(a[i] == '\0' &&  b[i] == '\0') break;
        // cheks if their characters match returns 0 if not
        if(a[i] != b[i]) return 0;
        
        i++;
    }
    return 1;
}

// list possible uses for the program
void help(){

    printf("HELP----------------\n");
    printf("The Huffman cli tool have the follwing three options of use:\n");
    printf("(./)huffman -c input.txt outputFileName  <-- compresses the given input file\n");
    printf("(./)huffman -d input.bin outputFileName  <-- decompresses the given input file\n");
    printf("(./)huffman -h  <-- lists these three options\n");
    printf("--------------------\n");
}

// returns 1 if file extension matches
int matchingFileExtension(char *fileName, char *extension){ 
    
    int index = 0;
    // find end of strings index
    while (1){
        if (fileName[index] == '\0') break;
        index++;
    }

    int extensionLenght = 0;
    // ind extensions lenght
    while (1){
        if (extension[extensionLenght] == '\0') break;
        extensionLenght++;
    }

    // check if extension match
    for (int i = 0; i < extensionLenght; i++){
        if(extension[i] != fileName[index-(extensionLenght-i)]) return 0;
    }

    return 1;
}

// this is a function for counting each unique characters appearence in a text separetly
int categarizeChars(int *AsciiCounter, char *text){ 
    long int index = 0;
    while (1){

        // breaks at end of string
        if (text[index] == '\0') break;

        // checks if charcter is in the ascii table
        if ((int)text[index] >= 0 && (int)text[index] <= 127){
            // increments the entry in the counter whats index represents the char 
            AsciiCounter[(int)text[index]]++;
        }

        // exits whit error if character not in ascii table
        else {
            printf("--ERROR-- [VALUE ERROR]\n");
            printf("The compressor only supports characters in the Ascii table.\n");
            return 1;
        }
        index++;
    }
    return 0;
}