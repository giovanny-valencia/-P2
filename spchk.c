#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>
#include <dirent.h>
#include <limits.h>
#include <ctype.h>

// macros
#define TextFile 0
#define DirectoryFile 1
#define Invalid -1
#define INITIAL_CAPACITY 100
#define GROWTH_FACTOR 2
// #define dictionaryDebug
// #define tokenDebug
#define traverseDebug
#define fileDebug
#define currentLine printf("Current line: %d\n", __LINE__)

// global variables
char **dictionaryArray;
int wordCount = 0;
int errorCount = 0; // Global variable to keep track of errors for the wrap up of the program

// function prototypes
void dictionaryCreation(char const argv[1]);
void processFile(char const argv[]);
int file_or_dir(char const argv[]);
void traverseDirectory(char const argv[]);
const char *concatenatePath(const char *path, const char *entry);
void processWord(int wordBuffer[], int startOfWord, int endOfWord, int line, int columnPosition, const char *currentFilePath);
int isChar(char c);
int isQuotationOrBracket(char c);
void spellCheckWord(char *word, int line, int columnPosition, const char *currentFilePath);
int binarySearch(char **array, int size, char *target);

//  ProcessedFilesArray struct and utility functions for it

//  End of ProcessedFilesArray struct and utility functions for it

int main(int argc, char const *argv[])
{

    printf("\n");

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

    // start processing the files passed, starting at argv[2] -> argv[argc-1]
    for (int i = 2; i < argc; i++)
    {
        // check if file is .txt or directory
        int fileType = file_or_dir(argv[i]);

        if (fileType == TextFile)
        {
            // process the text file
            // printf("text file\n");
            processFile(argv[i]);
        }
        else if (fileType == DirectoryFile)
        {
            // process the directory
            // printf("directory\n");
            traverseDirectory(argv[i]);
        }
        else // shouldn't happen, but just in case
        {
            printf("invalid file type\n");
        }
    }

    printf("end!\n");
    if (errorCount > 0) // If there are errors, return EXIT_FAILURE. Otherwise, return EXIT_SUCCESS.
    {
        printf("Errors found\n");
        exit(EXIT_FAILURE);
    }
    else
    {
        printf("No errors found\n");
        exit(EXIT_SUCCESS);
    }
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

int file_or_dir(char const *pathToFile)
{
    struct stat fileStat;
    if (stat(pathToFile, &fileStat) < 0)
    {
        perror("stat\n");
        exit(EXIT_FAILURE);
    }

    if (S_ISREG(fileStat.st_mode))
    {
        return TextFile;
    }
    else if (S_ISDIR(fileStat.st_mode))
    {
        return DirectoryFile;
    }
    else // this shouldn't happen, but just in case
        return Invalid;
}

void processFile(char const *pathToFile)
{
#ifdef fileDebug
    printf("processing file: %s\n", pathToFile);
#endif

    // open the file
    int fd = open(pathToFile, O_RDONLY);
    if (fd == -1)
    {
        perror("could not open file\n");
        errorCount++;
        return;
    }

    char readBuffer[1]; // read one byte at a time
    int line = 1;
    int columnPosition = 0;
    int startOfWord = -1;
    int endOfWord = -1;
    int bytesRead = 0;

    int wordBufferIndex = 0;
    int wordBuffer[100]; // arbitrary size, should be enough for any word
    // read the file, one byte at a time, constructs one word at a time
    while ((bytesRead = read(fd, readBuffer, 1)) > 0)
    {
        if (readBuffer[0] == ' ' || readBuffer[0] == '\t' || readBuffer[0] == '\n') // white space, process word
        {
            columnPosition++;                   // increments column position to account white space
            wordBufferIndex++;                  // increments word buffer index to account for white space -> null terminator
            wordBuffer[wordBufferIndex] = '\0'; // null terminate the word buffer

            // process word will add this function call later
            // printf("process word\n");
            processWord(wordBuffer, startOfWord, endOfWord, line, columnPosition, pathToFile);

            // reset our buffer and tracking variables and continue

            if (readBuffer[0] == '\n')
            {
                line++;
                columnPosition = 0;
            }
            wordBufferIndex = 0;
            startOfWord = -1;
            endOfWord = -1;
            continue;
        }

        // if it's not a newline or whitespace, then need to grab and store the char, will further examine it later
        columnPosition++;

        // if the start of the word has not yet been found, and the char is a quotation or bracket ( ', ", (, [, { ) then dont store it, continue
        if ((startOfWord == -1) && (isQuotationOrBracket(readBuffer[0])))
        {
            continue;
        }

        if ((startOfWord == -1) && isChar(readBuffer[0])) // start of a word
        {
            startOfWord = columnPosition;
        }

        endOfWord = columnPosition;
        wordBuffer[wordBufferIndex] = readBuffer[0];
        wordBufferIndex++;
    }
}

void traverseDirectory(char const *pathToDirectory)
{
#ifdef traverseDebug
    printf("traversing directory: %s\n", pathToDirectory);
#endif

    // open directory
    DIR *dir = opendir(pathToDirectory);
    struct dirent *entry;

    if (dir == NULL)
    {
        printf("unable to open directory: %s\n", pathToDirectory);
        // increment error count but don't exit the program
        errorCount++;
        return;
    }

    // read directory
    while ((entry = readdir(dir)) != NULL)
    {
        // Ignore . and .. entries
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
        {
            continue;
        }

        // for each entry, check if it's a file or directory, and process accordingly
        if (entry->d_type == DT_REG)
        {
            // process the file
            printf("file: %s\n", entry->d_name);

            const char *newPath = concatenatePath(pathToDirectory, entry->d_name);
            if (newPath == NULL)
            {
                // error message already printed in concatenatePath
                printf("unable to process file: %s/%s\n", pathToDirectory, entry->d_name);
                errorCount++;
                continue;
            }
            else
                processFile(newPath);
        }
        else if (entry->d_type == DT_DIR)
        {
            // process the directory
            printf("directory: %s\n", entry->d_name);
            const char *newPath = concatenatePath(pathToDirectory, entry->d_name);
            if (newPath == NULL)
            {
                // error message already printed in concatenatePath
                printf("unable to process file: %s/%s\n", pathToDirectory, entry->d_name);
                errorCount++;
                continue;
            }
            else
                traverseDirectory(newPath);
        }
        else
        {
            // shouldn't happen, but just in case
            printf("invalid entry: %s\n", entry->d_name);
            errorCount++;
        }
    }
}

const char *concatenatePath(const char *path, const char *entry)
{
    // length of path + length of entry + 1 for the '/' + 1 for the null terminator
    size_t currentPathLength = strlen(path);
    size_t entryLength = strlen(entry);
    char *newPath = malloc(currentPathLength + entryLength + 2);

    if (newPath == NULL)
    {
        perror("malloc\n");
        return NULL;
    }

    // Copy the current path to the new path
    strcpy(newPath, path);

    // Add a '/' to the end of the current path if it doesn't already have one
    if (newPath[currentPathLength - 1] != '/')
    {
        strcat(newPath, "/");
    }

    // Append the entry name to the new path
    strcat(newPath, entry);

    return newPath;
}

int isChar(char c)
{
    return ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'));
}

int isQuotationOrBracket(char c)
{
    return (c == '\'' || c == '\"' || c == '(' || c == '[' || c == '{');
}

void processWord(int wordBuffer[], int startOfWord, int endOfWord, int line, int columnPosition, const char *currentFilePath)
{
    int wordLength = columnPosition - startOfWord;

    // this detects the true end of the word, accounting for any trailing punctuation
    for (int i = wordLength - 1; i >= 0; i--)
    {
        if (isChar(wordBuffer[i]))
        {
            // printf("char: %c\n", wordBuffer[i]);
            endOfWord = i;
            break;
        }
    }

    // printf("\n");
    //  then re-creates the word with the true end of the word
    char legalTextWord[100]; // arbitrary size, should be enough for any word
    for (int i = 0; i <= endOfWord; i++)
    {
        legalTextWord[i] = wordBuffer[i];
    }
    legalTextWord[endOfWord + 1] = '\0';

    spellCheckWord(legalTextWord, line, startOfWord, currentFilePath);
}

void spellCheckWord(char *word, int line, int startOfWord, const char *currentFilePath)
{
    // if hyphenated word, split into separate words checking each one
    // if they are ALL correct then the word is correct
    // if ONE or more is incorrect, then the word is incorrect, but the error report will be for the entire hyphenated word starting at initial
    char *wordCopy = strdup(word);
    char delimiter = '-';
    char *token = strtok(wordCopy, &delimiter); // get the first token or full word if no delimiter
    int report;
    while (token != NULL)
    {
        printf("token: %s\n", token);
        report = binarySearch(dictionaryArray, wordCount, token);
        if (report == -1)
        {
            break;
        }
        token = strtok(NULL, &delimiter);
    }

    if (report == -1)
    {
        printf("%s (%d, %d): %s\n", currentFilePath, line, startOfWord, word);
        errorCount++;
    }
    else
    {
        return;
    }
}

int binarySearch(char **array, int size, char *target)
{
    int left = 0;
    int right = size - 1;

    while (left <= right)
    {
        int mid = left + (right - left) / 2;

        int cmp = strcmp(array[mid], target);

        if (cmp == 0)
        {
            return mid; // target found
        }
        else if (cmp < 0)
        {
            left = mid + 1; // target is in the right half
        }
        else
        {
            right = mid - 1; // target is in the left half
        }
    }
    return -1; // target not found
}