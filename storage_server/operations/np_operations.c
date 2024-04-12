#include "../../common/print/logging.h"
#include "operations.h"

ErrorCode GetFileInfo(char* path, struct stat* info) {
    if (stat(path, info) == -1) {
        eprintf("Could not read FileInfo, errno = %d, %s\n", errno, strerror(errno));
        return StatErrors();
    }
    return SUCCESS;
}

ErrorCode GetDirectoryInfo(char* path, struct stat* info) {
    if (stat(path, info) == -1) {
        eprintf("Could not read DirectoryInfo, errno = %d, %s\n", errno, strerror(errno));
        return StatErrors();
    }
    int size = 0;
    ErrorCode ret;
    if ((ret = GetDirectorySize(path, &size))) {
        eprintf("Could not read Directory size, errno = %d, %s\n", errno, strerror(errno));
        return ret;
    }
    info->st_size = size;
    return SUCCESS;
}
ErrorCode IsDirectory(char* path) {
    struct stat st;
    ErrorCode ret;
    if ((ret = stat(path, &st)) == 0 && S_ISDIR(st.st_mode)) {
        return IS_DIRECTORY;
    }
    if (ret == -1) {
        return StatErrors(ret);
    }
    return IS_FILE;
}

ErrorCode GetDirectorySize(char* path, int* size) {
    ErrorCode ret = IsDirectory(path);
    if (ret == IS_DIRECTORY) {
        DIR* d;
        if ((d = opendir(path)) == NULL) {
            eprintf("Could not open directory, errno = %d, %s\n", errno, strerror(errno));
            return OpenDirErrors();
        }
        struct dirent* dir;
        while ((dir = readdir(d)) != NULL) {
            if (strcmp(dir->d_name, ".") == 0 || strcmp(dir->d_name, "..") == 0) {
                continue;
            }
            ErrorCode ret;
            char entry_path[256];
            memset(entry_path, '\0', 256);
            snprintf(entry_path, sizeof(entry_path), "%s/%s", path, dir->d_name);
            if ((ret = GetDirectorySize(entry_path, size)))
                return ret;
        }
        if (closedir(d) == -1) {
            eprintf("Could not close directry, errno = %d, %s\n", errno, strerror(errno));
            return CloseDirErrors();
        }
    } else if (ret == IS_FILE) {
        struct stat st;
        if (stat(path, &st) != 0) {
            eprintf("Could not read directory size, errno = %d, %s\n", errno, strerror(errno));
            return StatErrors();
        }
        *size += st.st_size;
    } else {
        return ret;
    }
    return SUCCESS;
}

// ErrorCode readFromFile(char* path, char* buffer, size_t bufferSize) {
//     int fd;
//     if ((fd = open(path, O_RDONLY)) == -1) {
//         eprintf("Read:Could not open file, errno = %d, %s\n", errno, strerror(errno));
//         return FAILURE;
//     }
//     if (read(fd, buffer, bufferSize) == -1) {
//         eprintf("Read:Could not read file, errno = %d, %s\n", errno, strerror(errno));
//         return FAILURE;
//     }
//     if (close(fd) == -1) {
//         eprintf("Read:Could not close file, errno = %d, %s\n", errno, strerror(errno));
//         return FAILURE;
//     }
//     return SUCCESS;
// }
ErrorCode writeToFile(char* path, char* buffer) {
    FILE* fptr;
    if ((fptr = fopen(path, "w")) == NULL) {
        eprintf("Write:Could not open file '%s', errno = %d, %s\n", path, errno, strerror(errno));
        return FileOpenErrors();
    }
    lprintf("opened %s for write", path);
    if (fprintf(fptr, "%s", buffer) < 0) {
        eprintf("Write:Could not write into file '%s', errno = %d, %s\n", path, errno, strerror(errno));
        return FAILURE;
    }
    lprintf("data written successfully");
    if (fclose(fptr) == EOF) {
        eprintf("Write:Could not close file '%s', errno = %d, %s\n", path, errno, strerror(errno));
        return FAILURE;
    }
    return SUCCESS;
}

ErrorCode ExecuteWrite(WriteRequest* Req) {
    ErrorCode ret;
    if ((ret = writeToFile(Req->path, Req->Data))) {
        return ret;
    }
    lprintf("write executed successfully");
    return SUCCESS;
}

ErrorCode ExecuteRead(ReadRequest* Req, int clientfd) {
    char buffer[1024];
    memset(buffer, '\0', 1024);
    int fd;
    if ((fd = open(Req->path, O_RDONLY)) == -1) {
        eprintf("Read:Could not open file '%s', errno = %d, %s\n", Req->path, errno, strerror(errno));
        return FileOpenErrors();
    }

    lprintf("opened %s for read", Req->path);
    ErrorCode ret;
    while ((ret = read(fd, buffer, 1024)) > 0) {
        if (sendDataPacket(buffer, clientfd)) {
            close(fd);
            return FAILURE;
        }
        lprintf("data packet sent for read");
        memset(buffer, '\0', 1024);
    }
    if (ret == -1) {
        return ReadErrors();
    }
    close(fd);

    lprintf("closed %s after read", Req->path);
    if (send_STOP_PKT(clientfd)) {
        return FAILURE;
    }
    lprintf("stop packet sent");

    lprintf("read executed successfully");
    return SUCCESS;
}

ErrorCode ExecuteMD(MDRequest* Req, int clientfd) {
    struct stat st;
    ErrorCode ret;
    if (IsDirectory(Req->path)) {
        if ((ret = GetDirectoryInfo(Req->path, &st)))
            return ret;
        lprintf("got Directory Info for %s", Req->path);
    } else {
        if ((ret = GetFileInfo(Req->path, &st)))
            return ret;
        lprintf("got File Info for %s", Req->path);
    }
    if ((ret = socketSend(clientfd, &st, sizeof(struct stat)))) {
        eprintf("Could not Send metadata to Client");
        return FAILURE;
    }
    lprintf("metadata response sent");
    return SUCCESS;
}

ErrorCode ExecuteClientRequest(RequestType reqType, void* request, int clientfd) {
    switch (reqType) {
        case REQUEST_WRITE:
            return ExecuteWrite((WriteRequest*)request);
        case REQUEST_READ:
            return ExecuteRead((ReadRequest*)request, clientfd);
        case REQUEST_METADATA:
            return ExecuteMD((MDRequest*)request, clientfd);
        default:
    }
    return FAILURE;
}