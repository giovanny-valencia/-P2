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

