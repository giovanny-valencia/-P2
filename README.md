# cs214 Project 2: Spelling Checker Spring 2024 Solo Version

author: gv206

# Changes from previous group submission:

1. Recursive Traversal was built from scratch. There was some segmentation fault/crash and an instance where some files were skipped. It was easier to rewrite my partners portions than attempt to debug.

This was the original recursive directory traversal logic

```c
    void spellCheckDirectory(char *directory, DIR *currentDirectory)
    { // Spell checks the paths to all text file in a given directory that does NOT begin with a '.'
        // DIR *currentDirectory = opendir(directory); //opens the directory passed in as input (THIS NEEDS TO BE DONE BEFORE ENTERING RECURSION BECAUSE WE CAN'T OPEN THE SAME DIRECTORY AGAIN AND AGAIN)

        struct dirent *currentEntry; // reads in the next entry in the directory
        currentEntry = readdir(currentDirectory);
        char *completePath = malloc(strlen(directory) + 200); // creates a new char array that will contain the full path we're desiring to search which is done on this line and the line below
        struct stat check;                                    // Create a new stat struct that will contain the mode of the path contained by completePath

        while (currentEntry != NULL)
        { // while in current directory level
            strcpy(completePath, directory);
            completePath = strcat(completePath, "/");
            completePath = strcat(completePath, currentEntry->d_name);
            stat(completePath, &check);
            if (DEBUG)
            {
                printf("%s\n", completePath);
            }

            if (currentEntry->d_name[0] == '.') // skips files that start with a . and going back to the previous directory ".."
            {
                ;
            }
            else if (S_ISDIR(check.st_mode))
            { // recursively calls directory search on directory that's found
                DIR *newDirectory = opendir(completePath);
                char *newPath = malloc(1000);
                strcpy(newPath, completePath);
                spellCheckDirectory(newPath, newDirectory);
                free(newPath);
                free(newDirectory);
            }
            else if ((S_ISREG(check.st_mode) || S_ISLNK(check.st_mode)) && currentEntry->d_name[strlen(currentEntry->d_name) - 4] == '.' && currentEntry->d_name[strlen(currentEntry->d_name) - 3] == 't' && currentEntry->d_name[strlen(currentEntry->d_name) - 2] == 'x' && currentEntry->d_name[strlen(currentEntry->d_name) - 1] == 't')
            {
                int fileDetails = openFile(completePath);
                if (fileDetails == -1)
                {
                    perror(completePath);
                }
                else
                {
                    int poop = 0;
                    printf("reading file: %s\n", completePath);
                    readFile(fileDetails, completePath);
                }
                close(fileDetails);
            }
            currentEntry = readdir(currentDirectory);
        }

        return;
    }
```

I rewrote it as:

```c
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
```
2. Reading the file and generating sequence of position-annotated words

Like above, I opted for rewriting the respective code responsibility as opposed to debugging my partners code.

The issues with the initial codebase were:
- weird instance where the first and last words of a file were not being passed to the binary search function
- column positioning per word did NOT work whatsoever
- some issue when attempting to fix the trailing punctuation so some words weren't recorded properly?
- printing out every single word encountered in the file..?

My implementation:
1. Open the file. If it failed, return to caller function and increment errorCounter

2. Read the file one bye at a time until done
- here we can easily track the start of the word, found by when the first char is found and the var StartOfWord == -1. True ending of the word must be processed later but for now we can keep track of the last character/byte that is part of the word, i.e. NOT a white space or newline but punctuation has to be removed later since technically in the write up, punctuation could be part of the word if it's in the middle. Not a problem, just an extra step.

- we process the word when we find a space, tab, or newline. NOTE: there was SOME issue that occurred once when a period and multiple spaces were encountered after resulting in a hangup. I think I fixed it without using ispunct(readBuffer[0]) since this kind of broke the word processing as again, punctuation could be part of a word.. Wasn't able to replicate the hangup so I think we're good. 

```c

void processFile(char const *pathToFile)
{
#ifdef fileDebug
    printf("processing file: %s\n", pathToFile);
#endif

    // open the file
    int fd = open(pathToFile, O_RDONLY);
    if (fd == -1)
    {
        printf("could not open file\n");
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
        // ispunct(readBuffer[0])
        if (readBuffer[0] == ' ' || readBuffer[0] == '\t' || readBuffer[0] == '\n') // white space, process word
        {
            columnPosition++;                   // increments column position to account white space
            wordBufferIndex++;                  // increments word buffer index to account for white space -> null terminator
            wordBuffer[wordBufferIndex] = '\0'; // null terminate the word buffer

            // process word will add this function call later
            // printf("process word\n");

#ifdef tokenDebug
            printf("column position: %d\t", columnPosition);
            printf("start of word: %d\t", startOfWord);
            printf("end of word: %d\n", endOfWord);
#endif

            if (startOfWord != -1 && endOfWord != -1)
            {
                processWord(wordBuffer, startOfWord, endOfWord, line, columnPosition, pathToFile);
            }
            // currentLine;

            // reset our buffer and tracking variables and continue

            // currentLine;
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
```

3. We then actually process the word

    - we eliminate the trailing punctuation to get the 'true' ending of the word, whenever the first char is encountered if we walk through it from the end of the word until we encounter the first char

    so something like: this}}}} -> this

    while still keeping validity if "thi}}...s" 
    was found in the dictionary, making it valid. Basically, middle punctuation of a word is kept, only removed if no char stands between the punctuation and the next delimiter (space/newline) 

```c
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

    // re-creates the word with the true end of the word
    char legalTextWord[100]; // arbitrary size, should be enough for any word
    for (int i = 0; i <= endOfWord; i++)
    {
        legalTextWord[i] = wordBuffer[i];
    }

    legalTextWord[endOfWord + 1] = '\0';

#ifdef tokenDebug
    for (unsigned i = 0; i < strlen(legalTextWord); i++)
    {
        printf("%c", legalTextWord[i]);
    }
    printf("\n");
#endif

    if (startOfWord > 0)
    {
        spellCheckWord(legalTextWord, line, startOfWord, currentFilePath);
    }
}
```

# Final notes, explanation of program flow, test cases

Flow:
1. make the dictionary of accepted/correct words (this remains mostly identical to my previous implementation, but has been altered to be simpler and more efficient)
- read the entire file to get wordCount
- make an array of size 3*wordCount to store all possible accepted word cases
- read the file again, grabbing and processing each word
- examine each word, checking the index positioning to see if a initialCapitalized and/or allUppercase variation of the word is accepted
- add the base case word which was in the dictionary, then any variant accepted case
- this also keeps of the global wordCount of our dictionary words
- when all the words are in the array, qsort it, wordCount variable is handy here since we likely have empty space

2. start processing the n files given via cmd line
- check if the initial files are txt files or directories. Send to proper functionality for processing. If text file -> process. If directory -> recursive processing (open dir, read entries for their type, send entries to respective functions)
- paths need to be constructed to be passed
This was explained in greater detail above in my from scratch implementation.

3. For text files, grab each word. Process was in detail explained above in my revised from scratch implementation.

- each word is sent to a binary search alongside its column, line number, and path of file for error reporting. If not found in the dictionary array we created and sorted earlier, then it's invalid and we report as such.

4. When we're done processing each file/directory passed in the cmd line or found nested/recursively within those files and so forth, we return to the main function where we free the dictionary array.

5. when the program ends, it checks the variable "errorCount", if it's 0 then no incorrect spellings were detected so it exits successfully and all files were accessible, if at least 1 errorCount was found then it exits with failure.


Worth while mentions:
- non-character values such as numerical literals are not picked up by the spellChecker, at least standalone values like "1" are not, perhaps if they're part of a word like "th1s". At this point, it's a design choice to not include them.

- Hyphenated-words: This seemed open to interpretation, but for the column error reporting (when at least one word is spelled incorrectly) I decided to report the column position of where the word started, so the column positioning of the first word. This was an intended design choice as it was simpler and it also made sense the the whole word is "one instance" as if one was incorrect, the whole thing was incorrect. With that said, it stops checking when it encounters the first incorrect word in the hyphen. No point to continue, one deems it wrong anyways.

- Tried to free baseCase and the firstCapital case and Uppercase but it broke, so it seems those variables are used else where. Design choice to leave them.

- While common practice, every text file must contain an empty newline to indicate the file is done. Otherwise the last word is lost and not processed.

Test Cases:

- Used a handful of dictionaries and some passages written by ChatGPT to test the spellChecker. Once the testing seem satisfactory I used "/usr/share/dict/words" as my dictionary.

- Recursive traversal by using a simple dictionary that lacked many correct words then running it against the folder "testcases" which contained all of my text files, including other dictionaries. It displayed at least one error report for each document, proving that the recursive traversal was working as intended. Additionally, I used the trusty print statements to trace through it when I was designing it.

- final test: ./spchk /usr/share/dict/words /usr/share/dict/words

and:          ./spchk /usr/share/dict/words /usr/share/dict/words testcases

looks good