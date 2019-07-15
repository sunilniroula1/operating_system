/*
Sunil Niroula: 1001557601
Arjun Lamichhane: 1001427770
*/



#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <sys/stat.h>


#define MAX_COMMAND_SIZE 200   // maximum command size
#define MAX_ARGS 5              // max number of arguments
#define MAX_FS_NAME 100         // max File system name length
#define DELIMITER " \t\n"    // specify command delimiter

/*
 * File System Information Structure both in base 10 and Hexadecimal
 */
struct FS_Info {
    int16_t BPB_BytesPerSec;
    int8_t BPB_SecPerClus;
    int16_t BPB_RsvdSecCnt;
    int8_t BPB_NumFATS;
    int32_t BPB_FATSz32;
};

/*
 * Directory Attributes Structure
 */
struct __attribute__((__packed__)) Directory {
    char DIR_Name[11];
    uint8_t DIR_Attr;
    uint8_t ignored[8];
    uint16_t DIR_FirstClusterHigh;
    uint8_t ignored2[4];
    uint16_t DIR_FirstClusterLow;
    uint32_t DIR_FileSize;
};

/* Error Message wrappers */
void errorFileNotFound() { printf("Error: File not found\n"); }
void errorCloseClosedFile() { printf("Error: File system not open.\n"); }
void errorOpenOpenedFile() { printf("Error: File system image already open.\n"); }
void errorFSnotFound() { printf("Error: File system image not found.\n"); }
void errorFileNotOpen() { printf("Error: File system image must be opened first.\n"); }
void errorCdToInvalidDir(const char *dirName) { printf("Error: %s directory does not exist\n", dirName); }

/*
 *  Open file img specified by the user or report error.
 *  Handle Open of the File System image specified by the user
*/
int openFS_Image(char *token[MAX_FS_NAME], FILE **pFILE) {
    struct stat temp;
    // throw an error if the file is already open
    if (*pFILE != NULL) {
        errorOpenOpenedFile();
        return 0;
    }
        // open the file system image
    else if (stat(token[1], &temp) != -1) {
        *pFILE = fopen(token[1], "r");
        return 1;
    } // throw and error when the file image does not exist
    else {
        errorFSnotFound();
        return 0;
    }
}


/*
    Display file system information when the user requests for it via the "info" command.
    precondition: file system must be open.
*/
void showFS_Info(struct FS_Info *fs_info) {
    // display file system information
    printf("\nBPB_BytesPerSec \n  Hex: 0x%X\n  Dec: %d \n\n", fs_info->BPB_BytesPerSec, fs_info->BPB_BytesPerSec);
    printf("BPB_SecPerClus \n  Hex: 0x%X\n  Dec: %d \n\n", fs_info->BPB_SecPerClus, fs_info->BPB_SecPerClus);
    printf("BPB_RsvdSecCnt \n  Hex: 0x%X\n  Dec: %d \n\n", fs_info->BPB_RsvdSecCnt, fs_info->BPB_RsvdSecCnt);
    printf("BPB_NumFATS \n  Hex: 0x%X\n  Dec: %d \n\n", fs_info->BPB_NumFATS, fs_info->BPB_NumFATS);
    printf("BPB_FATSz32 \n  Hex: 0x%X\n  Dec: %d \n\n", fs_info->BPB_FATSz32, fs_info->BPB_FATSz32);
}


/*
    Open the File system along with the infoSet the file system specs
*/
void setUpFS_info(FILE **pFILE, struct FS_Info *fs_info) {
    short bytes, rsvdSectors, fatSize;
    int8_t fatNum, sectors;

    // fetch bytes per sector information
    fseek(*pFILE, 11, SEEK_SET);
    fread(&bytes, 1, 2, *pFILE);
    fs_info->BPB_BytesPerSec = bytes;

    // fetch sectors per cluster information
    fseek(*pFILE, 13, SEEK_SET);
    fread(&sectors, 1, 1, *pFILE);
    fs_info->BPB_SecPerClus = sectors;

    // fetch number of reserved sectors information
    fseek(*pFILE, 14, SEEK_SET);
    fread(&rsvdSectors, 1, 2, *pFILE);
    fs_info->BPB_RsvdSecCnt = rsvdSectors;

    // fetch number of FATS information
    fseek(*pFILE, 16, SEEK_SET);
    fread(&fatNum, 1, 1, *pFILE);
    fs_info->BPB_NumFATS = fatNum;

    // fetch FAT size count information
    fseek(*pFILE, 36, SEEK_SET);
    fread(&fatSize, 1, 4, *pFILE);
    fs_info->BPB_FATSz32 = fatSize;
}

/*
    For the ls command. Display contents of directory.
*/
void cmd_ls(struct Directory *dir) {
    //traverse through Directory entry, listing all files / and folders in that folder
    for (int i = 0; i < 16; i++) {
        if ((dir[i].DIR_Attr == 1 || dir[i].DIR_Attr == 16 || dir[i].DIR_Attr == 32 || dir[i].DIR_Attr == 48)
            &&
            ((unsigned int) dir[i].DIR_Name[0] != 0xffffffe5)) {
            // list found files and directories
            printf("%.*s\n", 11, dir[i].DIR_Name);
        }
    }
}

/*
 * calculate the starting address for a given data block
 */
int getFirsAddress(int32_t sect, struct FS_Info *fs_info) {
    const int32_t fAddress = ((sect - 2) * fs_info->BPB_BytesPerSec) + (fs_info->BPB_BytesPerSec * fs_info->BPB_RsvdSecCnt)
                             + (fs_info->BPB_NumFATS * fs_info->BPB_FATSz32 * fs_info->BPB_BytesPerSec);
    return fAddress;
}

/*
    Set directory structure value in the directory entry array.
*/
void AddDirectoryEntry(FILE **pFILE, struct Directory *dir, int *pInfo) {
    int i;
    for (i = 0; i < 16; i++) {
        fread(&dir[i], 1, 32, *pFILE);
        *pInfo += 32;
    }
}

/*
    compute the address of the corresponding clusters/data blocks when given the directory name and info
*/
int getAddress(char *dirNameToken, struct Directory *dir, struct FS_Info *fs_info) {
    int i, j = 0;
    int address = -1;

    //loop through directory entry array and find the directory name the user entered
    for (i = 0; i < 16; i++) {
        char *dirName;
        j = 0;
        while (dir[i].DIR_Name[j] != '\0' && j != 12) {
            if (dir[i].DIR_Name[j] == ' ') {
                dirName = (char *) malloc(sizeof(char) * j);
                strncpy(dirName, dir[i].DIR_Name, (size_t) j);
                break;
            }
            j++;
        }

        // compare user provided directory to that those known to exist
        if (strcmp(dirName, dirNameToken) == 0 && dir[i].DIR_Attr == 0x10) {
            //return file pointer address to the directory the user is switching to
            address = getFirsAddress(dir[i].DIR_FirstClusterLow, &(*fs_info));
            if (dir[i].DIR_FirstClusterLow == 0) {
                address = (fs_info->BPB_NumFATS * fs_info->BPB_FATSz32 * fs_info->BPB_BytesPerSec) +
                          (fs_info->BPB_RsvdSecCnt * fs_info->BPB_BytesPerSec);
            }
            break;
        }
    }
    return address;
}

/*
    Handle the Change Directory command
*/
void cmd_cd(char *dirName, FILE **pFILE, struct Directory *dir, int *pInfo,
            struct FS_Info *fs_info) {

    // cd without path to a specific directory should take user to their root directory
    if (dirName == NULL) {
        *pInfo = (fs_info->BPB_NumFATS * fs_info->BPB_FATSz32 * fs_info->BPB_BytesPerSec) +
                       (fs_info->BPB_RsvdSecCnt * fs_info->BPB_BytesPerSec);
        fseek(*pFILE, *pInfo, SEEK_SET);
        AddDirectoryEntry(&(*pFILE), dir, &(*pInfo));
        return;
    }

    // change the provided directory name to uppercase as is data in our FS
    int i = 0;
    while (dirName[i] != '\0') {
        dirName[i] = (char) toupper(dirName[i]);
        i++;
    }

    i = 0;
    int index = 0;
    int count = 0;   //directory name length

    /*
     *  Traverse the given directory name string, checking and changing to the it, if such directories exists
     */
    while (dirName[i] != '\0') {
        if (dirName[i] == '/') {
            //generate a token string to save directory name up to the '/' mark
            char token[count + 1];
            strncpy(token, dirName + index, (size_t) (i - index));
            token[count] = '\0';
            index = i + 1;
            count = 0;
            i++;

            *pInfo = getAddress(token, dir, &(*fs_info));    // get the address for current directory name

            if (*pInfo != -1) {

                fseek(*pFILE, *pInfo, SEEK_SET);    //reassign the file pointer to it
                //assign the current directory structure to the directory we just processed
                AddDirectoryEntry(&(*pFILE), dir,
                                  &(*pInfo));    // Set directory structure value in the directory entry array.
                continue;
            } else {
                errorCdToInvalidDir(dirName);
                return;
            }
        }
        count++;
        i++;
    }

    char token[count + 1];
    strncpy(token, dirName + index, (size_t) ((i + 1) - index));
    *pInfo = getAddress(token, dir, &(*fs_info));
    if (*pInfo != -1) {
        fseek(*pFILE, *pInfo, SEEK_SET);
        AddDirectoryEntry(&(*pFILE), dir, &(*pInfo));
    } else {
        errorCdToInvalidDir(dirName);
    }
}


/*
    get the index of a file location, given its name
*/
int getFileIndex(char *fName, struct Directory *dir) {
    const int MAX_FILE_NAME_LENGTH = 100;

    // change the provided filename to uppercase as is data in our FS
    int i = 0;
    while (fName[i] != '\0') {
        fName[i] = (char) toupper(fName[i]);
        i++;
    }

    //user typed file name too long

    if (i <= MAX_FILE_NAME_LENGTH) {

        char usersfName[14] = {0};
        // copy user provided file name
        strncpy(usersfName, fName, (size_t) i);

        // such for existence of the file in the directory
        for (i = 0; i < 16; i++) {
            char fileExtension[4] = {0};
            char newFName[14] = {0};     // directory entry
            char nulledDir[12] = {0};   // directory name with ending with nulls

            strncpy(nulledDir, dir[i].DIR_Name, 11);
            int j = 0;
            for (j = 0; j < 8; j++) {
                // locate the end of the name, space delimited
                if (nulledDir[j] != ' ') continue;
                break;
            }

            nulledDir[j] = '\0';   // null the character at the end of the name
            strncpy(newFName, nulledDir, j);  // strip off the null character when coping it

            // append existing extensions
            if (dir[i].DIR_Name[8] != ' ') {
                strncpy(fileExtension, dir[i].DIR_Name + 8, 3);
                strcat(newFName, ".");
                strcat(newFName, fileExtension);
            }
            // return result for the the match between user provided name to those in the FS
            if (strcmp(usersfName, newFName) == 0) {
                return i;
            }
        }
        return -1;
    } else {
        return -1;
    }
}

/*
 * Handle the 'stat' command.
 * Displays attributes of the file or directory, including the starting cluster number
*/
void cmd_stat(char *fileName, struct Directory *dir) {

    if (fileName != NULL) {
        // get the index of the file requested
        int i = getFileIndex(fileName, dir);
        if (i == -1) return;
        printf("Attribute\tSize\tStarting Cluster Number\n%u\t\t%u\t%u\n",
               dir[i].DIR_Attr, dir[i].DIR_FileSize, dir[i].DIR_FirstClusterLow);
    }
    else { // throw an error is the file does not exist
        errorFileNotFound();
    }
}

/*
 * Reads bytes from the given file position
*/
void cmd_readFile(char **token, FILE **fp, struct Directory *dir,
                  struct FS_Info *info) {
    int position, numberOfBytes, index;

    //throw an error if the file name not provided
    if (token[1] == NULL) {
        printf("Error: File name was not provided\n");
        return;
    }
        //if number of bytes is not valid, print error message
    else if (token[2] == NULL || token[3] == NULL) {
        printf("Enter a valid number of bytes to read.\n");
        return;
    }

    // store filename entered by user
    char fName[strlen(token[1])];
    strcpy(fName, token[1]);

    // cast position into an int
    char charPosition[strlen(token[2])];
    strcpy(charPosition, token[2]);
    position = atoi(charPosition);

    // cast number of bytes into an int
    char charBytes[strlen(token[3])];
    strcpy(charBytes, token[3]);
    numberOfBytes = atoi(charBytes);

    // get the file index and address
    index = getFileIndex(fName, dir);

    // throw an error when the file is not found
    if (index != -1) {

        int address = getFirsAddress(dir[index].DIR_FirstClusterLow, &(*info)) + position;

        //print the bytes of the file the user requested
        fseek(*fp, address, SEEK_SET);
        int i;
        for (i = 0; i < numberOfBytes; i++) {
            short val;
            //read one byte at a time and print the value in hex
            fread(&val, 1, 1, *fp);
            printf("%x ", val);
        }
        printf("\n");
    } else {
        errorFileNotFound();
    }
}

/* Handle the get command
 * copies the file from the File system to the current directory
 * */
void cmd_get(char *fName, FILE **pFILE, struct Directory *dir, struct FS_Info *fs_info) {
    // get file address
    int index = getFileIndex(fName, dir);
    int address = getFirsAddress(dir[index].DIR_FirstClusterLow, &(*fs_info));

    if (index != -1) {
        FILE *pNewFile;   // pointer to local host file
        pNewFile = fopen(fName, "w");
        fseek(*pFILE, address, SEEK_SET);  // pointer to the file to be copied
        int i;

        // read bytes one at a time while writing them
        for (i = 0; i < dir[index].DIR_FileSize; i++) {
            char val[1];
            fread(&val, 1, 1, *pFILE);
            fwrite(val, 1, sizeof(val), pNewFile);
        }
        fclose(pNewFile);
    } else {
        errorFileNotFound();
    }
}

/* Handles the put command
 * copies a file from the the current working directory to the File system
 * */
void cmd_put(char *fName, FILE **pFILE, struct Directory *dir, struct FS_Info *fs_info) {
    // get local host file address

    int index = getFileIndex(fName, dir);
    int address = getFirsAddress(dir[index].DIR_FirstClusterLow, &(*fs_info));

    if (index != -1) {
        FILE *pNewFile;   // pointer to local host file

        pNewFile = fopen(fName, "w");

        fseek(*pFILE, address, SEEK_SET);
        int i;

        // read bytes one at a time while writing them
        for (i = 0; i < dir[index].DIR_FileSize; i++) {
            char val[1];
            fread(&val, 1, 1, *pFILE);
            fwrite(val, 1, sizeof(val), pNewFile);
        }
        fclose(pNewFile);
    } else {
        errorFileNotFound();
    }
}

/*
 * Handle input and parse the command appropriately
*/
void handleUserInput(char **token, FILE **pFILE, int *infoPointer, struct FS_Info *info,
                     struct Directory *dir) {

    char *userCmd = token[0];  // get user input which is to be parsed as a command

    // on 'open' command, open the file system if it is not already open
    if (strcmp(userCmd, "open") == 0) {
        // open the file system image, otherwise inform the user that the FS is already open or does not exist
        if (openFS_Image(token, &(*pFILE)) == 0) {
            return;
        }

        // configure file system information
        setUpFS_info(&(*pFILE), &(*info));

        // initialize FS info pointer
        *infoPointer = (info->BPB_NumFATS * info->BPB_FATSz32 * info->BPB_BytesPerSec) +
                       (info->BPB_RsvdSecCnt * info->BPB_BytesPerSec);

        // initialize directory entry for the loaded file system
        fseek(*pFILE, *infoPointer, SEEK_SET);
        AddDirectoryEntry(&(*pFILE), dir, &(*infoPointer));
    }
    else if (strcmp(userCmd, "close") == 0) {  //if command entered is close
        if (*pFILE == NULL || fclose(*pFILE) != 0) {
            errorCloseClosedFile();
        } else {
            *pFILE = NULL;
        }
    }
    else if (strcmp(userCmd, "info") == 0) {  //if command entered is info

        if (*pFILE != NULL) {
            showFS_Info(&(*info));
        } else {  // display error message when other commands are issued before opening the file system
            errorFileNotOpen();
        }
    }
        //if command entered is ls
    else if (strcmp(userCmd, "ls") == 0) {
        if (*pFILE != NULL) {
            cmd_ls(dir);
        } else {
            errorFileNotOpen();
        }
    }
        //if command entered is cd
    else if (strcmp(userCmd, "cd") == 0) {
        if (*pFILE != NULL) {
            cmd_cd(token[1], &(*pFILE), dir, &(*infoPointer), &(*info));
        } else {
            errorFileNotOpen();
        }
    }
        //if command entered is stat
    else if (strcmp(userCmd, "stat") == 0) {
        if (*pFILE != NULL) {
            cmd_stat(token[1], dir);
        } else {
            errorFileNotOpen();
        }
    }
        //if command entered is read
    else if (strcmp(userCmd, "read") == 0) {
        if (*pFILE != NULL) {
            cmd_readFile(token, &(*pFILE), dir, &(*info));
        } else {
            errorFileNotOpen();
        }
    }
        //if command entered is get
    else if (strcmp(userCmd, "get") == 0) {
        if (*pFILE != NULL) {
            cmd_get(token[1], &(*pFILE), dir, &(*info));
        } else {
            errorFileNotOpen();
        }
    }
    else if (strcmp(userCmd, "put") == 0) {
        char **lFIle = &token[1];  // address for the file on local computer
        if (*lFIle != NULL) {
            cmd_put(token[1], &(*pFILE), dir, &(*info));
        } else {
            errorFileNotOpen();
        }
    }
    else {
        printf("Error: Unsupported command.\n");
    }
}



int main() {
    struct FS_Info info;
    struct Directory dir[16];

    int pFile = -1;
    char *cmd_str = (char *) malloc(MAX_COMMAND_SIZE);
    //initialize file pointer to NULL
    FILE *fName = NULL;

    while (1) {

        //  print out a prompt of mfs> as the program is ready to accept input.
        printf("mfs> ");

        // continuously listen for user input to be parsed as commands
        while (!fgets(cmd_str, MAX_COMMAND_SIZE, stdin));

        /* Parse input */
        char *token[MAX_ARGS];
        int token_count = 0;

        // Pointer to point to the token parsed by strsep
        char *argPtr;
        char *pwStr = strdup(cmd_str);

        // Tokenize the input stringswith whitespace used as the delimiter
        while (((argPtr = strsep(&pwStr, DELIMITER)) != NULL) &&
               (token_count < MAX_ARGS)) {
            token[token_count] = strndup(argPtr, MAX_COMMAND_SIZE);
            if (strlen(token[token_count]) == 0) {
                token[token_count] = NULL;
            }
            token_count++;
        }

        if (token[0] != NULL) {
            handleUserInput(token, &fName, &pFile, &info, dir);
        }
    }
}
