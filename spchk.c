#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>

// macros
#define dictionaryDebug
#define tokenDebug
#define currentLine printf("Current line: %d\n", __LINE__)

// global variables
char **dictionaryArray;
int wordCount = 0;

// function prototypes
void dictionaryCreation(char const argv[1]);

int main(int argc, char const *argv[])
{

    printf("start!\n");

    if (argc < 3)
    {
        printf("invalid arguments, min is 3 (prog name, dictionary, n files)\n");
        exit(EXIT_FAILURE);
    }

    // The first argument to spchk will be a path to a dictionary file. Subsequent arguments will be paths to text files or directories.
    dictionaryCreation(argv[1]);

#ifdef dictionaryDebug
    printf("word count: %d\t", wordCount);
    printf("dictionary array \n");
    for (int i = 0; i < wordCount; i++)
    {
        printf("i: %d\t", i);
        printf("%s\n", dictionaryArray[i]);
    }
#endif

    printf("end!\n");
    exit(EXIT_SUCCESS);
}

//  step one, construct the dictionary
void dictionaryCreation(char const *pathToDictionaryFile)
{

#ifdef dictionaryDebug
    printf("dictionary creation\n");
#endif

    // first check to see if the dictionary file exists/can be opened
    int fd = open(pathToDictionaryFile, O_RDONLY);
    if (fd == -1)
    {
        perror("open\n");
        exit(EXIT_FAILURE);
    }
    // else valid file descriptor, continue

    // read the dictionary file to get word count
    char countBuffer[1];
    int bytesRead = 0;

    // reads the file one byte at a time (no need to process just yet). Write up says dictionary contains 1 word per line.
    // So counting just the lines will give us the word count
    while ((bytesRead = read(fd, countBuffer, 1)) > 0)
    {
        if (countBuffer[0] == '\n')
        {
            wordCount++;
        }
    }
#ifdef dictionaryDebug
    printf("num of words: %d\n", wordCount);
#endif

    close(fd);
    // given the wordCount, create array of size wordCount to store the dictionary words (accepted cases handled later)
    dictionaryArray = malloc(wordCount * sizeof(char *)); // Allocate memory for the array of pointers

    fd = open(pathToDictionaryFile, O_RDONLY);

    char wordBuffer[100]; // arbitrary size, should be enough for any word
    int wordIndex = 0;
    int dictionaryIndex = 0;

    while ((bytesRead = read(fd, &wordBuffer[wordIndex], 1)) > 0)
    {
        if (wordBuffer[wordIndex] == '\n')
        {
            wordBuffer[wordIndex] = '\0';

            // end of word, add word to dictionary, reset wordIndex, and increment dictionaryIndex
            char *token = strtok(wordBuffer, "\n");
            dictionaryArray[dictionaryIndex] = strdup(token);
            dictionaryIndex++;
            wordIndex = 0;
        }
        else
        {
            wordIndex++;
        }
    }
}

//  step two, check the files