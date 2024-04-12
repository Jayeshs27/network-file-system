#include "../../common/print/logging.h"
#include "operations.h"

ErrorCode send_INFOPKT(int sockfd, char* path, bool IsDir) {
    CopyPacket packet;
    CleanPacket(&packet);
    packet.header = INFO;
    packet.ObjectInfo.IsDirectory = IsDir;
    strcpy(packet.ObjectInfo.FullName, path);
    char* entity = strrchr(path, '/');
    if (entity != NULL) {
        strcpy(packet.ObjectInfo.ObjectName, entity + 1);
    } else {
        strcpy(packet.ObjectInfo.ObjectName, path);
    }
    if (socketSend(sockfd, &packet, sizeof(CopyPacket))) {
        return FAILURE;
    }
    return SUCCESS;
}

ErrorCode send_STOPPKT(int sockfd) {
    CopyPacket packet;
    CleanPacket(&packet);
    packet.header = STOP;
    if (socketSend(sockfd, &packet, sizeof(CopyPacket))) {
        return FAILURE;
    }
    return SUCCESS;
}

ErrorCode send_ENDPKT(int sockfd) {
    CopyPacket packet;
    CleanPacket(&packet);
    packet.header = END;
    lprintf("END PACKET");
    if (socketSend(sockfd, &packet, sizeof(CopyPacket))) {
        return FAILURE;
    }
    return SUCCESS;
}

ErrorCode send_DIR_END_PKT(int sockfd) {
    CopyPacket packet;
    CleanPacket(&packet);
    packet.header = DIR_END;
    printf("DIR_END PACKET\n");
    if (socketSend(sockfd, &packet, sizeof(CopyPacket))) {
        return FAILURE;
    }
    return SUCCESS;
}

ErrorCode send_DATAPKT(int sockfd, char* data, int count) {
    CopyPacket packet;
    CleanPacket(&packet);
    packet.header = DATA;
    packet.count = count;
    packet.ObjectInfo.IsDirectory = false;
    memcpy(packet.Data, data, MAX_DATA_LENGTH);
    if (socketSend(sockfd, &packet, sizeof(CopyPacket))) {
        return FAILURE;
    }
    return SUCCESS;
}

ErrorCode ExecuteCopyRead(char* src, int sockfd) {
    lprintf("Executing copy read on %s", src);
    ErrorCode ret = IsDirectory(src);
    if (ret == IS_DIRECTORY) {
        lprintf("Executing copy read on %s - directory", src);
        DIR* Dptr;
        if ((Dptr = opendir(src)) == NULL) {
            eprintf("CopyRead : Could not open Source directory, errno = %d, %s\n", errno, strerror(errno));
            return OpenDirErrors();
        }
        if (send_INFOPKT(sockfd, src, true)) {
            closedir(Dptr);
            return FAILURE;
        }

        struct dirent* dir;
        while ((dir = readdir(Dptr)) != NULL) {
            if (dir->d_name[0] == '.') {
                continue;
            }
            char entry_path[256];
            memset(entry_path, '\0', 256);
            snprintf(entry_path, sizeof(entry_path), "%s/%s", src, dir->d_name);

            if ((ret = ExecuteCopyRead(entry_path, sockfd))) {
                closedir(Dptr);
                return ret;
            }
        }

        closedir(Dptr);
        if (send_DIR_END_PKT(sockfd)) {
            return FAILURE;
        }
    } else if (ret == IS_FILE) {
        if (SendfileData(src, sockfd)) {
            lprintf("Executing copy read on %s - file", src);
            if (SendfileData(src, sockfd)) {
                return FAILURE;
            }
        }
    } else {
        return ret;
    }
    return SUCCESS;
}

ErrorCode SendfileData(char* filename,int sockfd){
    int fd;
    if(send_INFOPKT(sockfd,filename,false)){
        return FAILURE;
    }
    if ((fd = open(filename, O_RDONLY)) == -1) {
        eprintf("Could not open file '%s', errno = %d, %s\n", filename, errno, strerror(errno));
        return FileOpenErrors();
    }
    char buffer[MAX_DATA_LENGTH];
    memset(buffer,'\0',MAX_DATA_LENGTH);
    ErrorCode ret;
    while((ret = read(fd,buffer,MAX_DATA_LENGTH)) > 0){
         send_DATAPKT(sockfd,buffer,ret);
         memset(buffer,'\0',MAX_DATA_LENGTH);
    }
    close(fd);
    if(send_STOPPKT(sockfd)){
        return FAILURE;
    }
    if(ret == -1){
        eprintf("Could not read the file '%s', errno = %d, %s\n", filename, errno, strerror(errno));
        return ReadErrors();
    }

    FeedbackAck ack;
    recieveFeedbackAck(&ack, sockfd);
    usleep(10000);
    lprintf("destination wrote %s",filename);

    return SUCCESS;
}

ErrorCode ReceiveFileData(char* filename, int sockfd) {
    FILE* fptr;
    if ((fptr = fopen(filename, "w")) == NULL) {
        eprintf("Could not open file, errno = %d, %s\n", errno, strerror(errno));
        return FileOpenErrors();
    }
    CopyPacket packet;
    ReceivePacket(sockfd, &packet);
    lprintf("writing data to the path : %s", filename);
    while (packet.header == DATA) {
        if (fwrite(packet.Data, sizeof(char), packet.count, fptr) <= 0) {
            eprintf("Could not write into file, errno = %d, %s\n", errno, strerror(errno));
            fclose(fptr);
            return FAILURE;
        }
        ReceivePacket(sockfd, &packet);
    }
    FeedbackAck ack;
    ack.errorCode = SUCCESS;
    sendFeedbackAck(&ack, sockfd);
    lprintf("Finished receiving %s", filename);
    fclose(fptr);
    return SUCCESS;
}

// inside indst path
ErrorCode ExecuteCopyWrite(char* dst, int sockfd) {
    lprintf("Executing copy write on %s", dst);
    CopyPacket Packet;
    if (ReceivePacket(sockfd, &Packet)) {
        // closedir(Dptr);
        return FAILURE;
    }
    if (Packet.header == DIR_END) {
        return DIR_END_REACH;
    }
    ErrorCode ret;
    char entry_path[256];
    memset(entry_path, '\0', 256);
    snprintf(entry_path, sizeof(entry_path), "%s/%s", dst, Packet.ObjectInfo.ObjectName);
    if (Packet.header != INFO) {
        eprintf("invalid order of packets\n");
        return FAILURE;
    }
    if (Packet.ObjectInfo.IsDirectory) {
        if ((ret = createDirectory(entry_path, 0777))) {
            return ret;
        }
        while (1) {
            ErrorCode ret = ExecuteCopyWrite(entry_path, sockfd);
            if (ret == DIR_END_REACH)
                break;
            else if (ret == SUCCESS)
                continue;
            else
                return ret;
        }

    } else {
        if ((ret = ReceiveFileData(entry_path, sockfd))) {
            return ret;
        }
    }
    return SUCCESS;
}

ErrorCode createFile(char* path, mode_t perm) {
    printf("Trying to create file %s\n", path);
    int fd;
    if ((fd = open(path, O_CREAT, perm)) == -1) {
        eprintf("Could not create file, errno = %d, %s\n", errno, strerror(errno));
        return FileOpenErrors();
    }
    return SUCCESS;
}
ErrorCode createDirectory(char* path, mode_t perm) {
    printf("Trying to create directory %s\n", path);
    if (mkdir(path, perm) == -1) {
        eprintf("Could not create Directory, errno = %d, %s\n", errno, strerror(errno));
        return MKDirErrors();
    }
    return SUCCESS;
}
ErrorCode deleteFile(char* path) {
    if (remove(path) == -1) {
        eprintf("Could not Delete file, errno = %d, %s\n", errno, strerror(errno));
        return rmErrors();
    }
    return SUCCESS;
}

ErrorCode deleteDirectory(char* path) {
    ErrorCode ret = IsDirectory(path);
    if (ret == IS_DIRECTORY) {
        DIR* d;
        if ((d = opendir(path)) == NULL) {
            eprintf("Could not open directory, errno = %d, %s\n", errno, strerror(errno));
            return OpenDirErrors();
        }
        struct dirent* dir;
        ErrorCode ret;
        while ((dir = readdir(d)) != NULL) {
            if (strcmp(dir->d_name, ".") == 0 || strcmp(dir->d_name, "..") == 0) {
                continue;
            }
            char entry_path[256];
            memset(entry_path, '\0', 256);
            snprintf(entry_path, sizeof(entry_path), "%s/%s", path, dir->d_name);
            if ((ret = deleteDirectory(entry_path)))
                return ret;
        }
        closedir(d);
        if (rmdir(path) == -1) {
            eprintf("Could not delete directory, errno = %d, %s\n", errno, strerror(errno));
            return rmErrors();
        }
    } else if (ret == IS_FILE) {
        if (remove(path) == -1) {
            eprintf("Could not Delete file, errno = %d, %s\n", errno, strerror(errno));
            return rmErrors();
        }
    } else {
        return ret;
    }

    return SUCCESS;
}

ErrorCode ExecuteNMRequest(RequestType reqType, void* request, int sockfd) {
    switch (reqType) {
        case REQUEST_CREATE:
            return ExecuteCreate((CreateRequest*)request);
        case REQUEST_DELETE:
            return ExecuteDelete((DeleteRequest*)request);
        case REQUEST_COPY_READ:
            return ExecuteCopyRead(((SSCopyRequest*)request)->path, sockfd);
        case REQUEST_COPY_WRITE:
            return ExecuteCopyWrite(((SSCopyRequest*)request)->path, sockfd);
        default:
            break;
    }
    return FAILURE;
}


ErrorCode ExecuteCreate(CreateRequest* Req) {
    char path[MAX_PATH_SIZE];
    ErrorCode ret;
    strcpy(path, Req->path);
    strcat(path, "/");
    strcat(path, Req->newObject);
    if (Req->isDirectory) {
        lprintf("Creating Directory %s", path);
        if ((ret = createDirectory(path, 0777))) {
            return ret;
        }
        lprintf("Directory Created successfully");
    } else {
        lprintf("Creating file %s", path);
        if ((ret = createFile(path, 0777))) {
            return ret;
        }
        lprintf("File Created successfully");
    }
    return SUCCESS;
}
ErrorCode ExecuteDelete(DeleteRequest* Req) {
    ErrorCode ret = IsDirectory(Req->path);
    lprintf("Executing delete");
    if (ret == IS_DIRECTORY) {
        lprintf("Deleting directory %s", Req->path);
        if ((ret = deleteDirectory(Req->path))) {
            return ret;
        }
    } else if (ret == IS_FILE) {
        lprintf("Deleting path %s", Req->path);
        if ((ret = deleteFile(Req->path))) {
            return ret;
        }
    } else {
        return ret;
    }
    lprintf("Successfully deleted");
    return SUCCESS;
}