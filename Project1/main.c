#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <io.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>
#include <pthread.h>
#define DEFAULT printf("%c[%dm", 0x1B, 0)
#define MAX_BUFFER 512
#define MAX_LENGTH 200
#define MAX_DIR 50
#define MAX_NAME 20


//User
typedef struct tagUserNode {
    char name[MAX_NAME];
    char dir[MAX_DIR];
    int UID;
    int GID;
    int year;
    int month;
    int wday;
    int day;
    int hour;
    int minute;
    int sec;
    struct tagUserNode* LinkNode;
}UserNode;

typedef struct tagUser {
    int topUID;
    int topGID;
    UserNode* head;
    UserNode* tail;
    UserNode* current;
}UserList;


//Directory
typedef struct tagTreeNode {
    char name[MAX_NAME];
    char type;
    int mode;
    int permission[9];
    int SIZE;
    int UID;
    int GID;
    int month;
    int day;
    int hour;
    int minute;
    struct tagTreeNode* Parent;
    struct tagTreeNode* LeftChild;
    struct tagTreeNode* RightSibling;
}TreeNode;

typedef struct tagDirectoryTree {
    TreeNode* root;
    TreeNode* current;
}DirectoryTree;


//stack using linked list
typedef struct tagStackNode {
    char name[MAX_NAME];
    struct tagStackNode* LinkNode;
}StackNode;


typedef struct tagStack {
    StackNode* TopNode;
}Stack;

time_t ltime;
struct tm* today;


int Mkdir(DirectoryTree* dirTree, char* cmd);
int rm(DirectoryTree* dirTree, char* cmd);
int cd(DirectoryTree* dirTree, char* cmd);
int pwd(DirectoryTree* dirTree, Stack* dirStack, char* cmd);
//int ls(DirectoryTree* dirTree, char* cmd);
//int cat(DirectoryTree* dirTree, char* cmd);


int chmod_(DirectoryTree* dirTree, char* cmd);
int chown_(DirectoryTree* dirTree, char* cmd);
int find_(DirectoryTree* dirTree, char* cmd);
void Instruction(DirectoryTree* dirTree, char* cmd);
void PrintStart();
void PrintHead(DirectoryTree* dirTree, Stack* dirStack);

//grep
void grep(char* searching_word, char* f_name);
void grep_n(char* searching_word, char* f_name);
//directory
//utility
int Mode2Permission(TreeNode* dirNode);
void PrintPermission(TreeNode* dirNode);
void DestroyNode(TreeNode* dirNode);
void DestroyDir(TreeNode* dirNode);
TreeNode* IsExistDir(DirectoryTree* dirTree, char* dirName, char type);
char* getDir(char* dirPath);

//save & load
void getPath(DirectoryTree* dirTree, TreeNode* dirNode, Stack* dirStack);
void WriteNode(DirectoryTree* dirTree, TreeNode* dirNode, Stack* dirStack);
void SaveDir(DirectoryTree* dirTree, Stack* dirStack);
int ReadNode(DirectoryTree* dirTree, char* tmp);
DirectoryTree* LoadDir();

//mycp
int mycp(DirectoryTree* dirTree, char* sName, char* oName);

//mkdir
DirectoryTree* InitializeTree();
int MakeDir(DirectoryTree* dirTree, char* dirName, char type);
//rm
int RemoveDir(DirectoryTree* dirTree, char* dirName);
//cd
int Movecurrent(DirectoryTree* dirTree, char* dirPath);
int MovePath(DirectoryTree* dirTree, char* dirPath);
//pwd
int PrintPath(DirectoryTree* dirTree, Stack* dirStack);
//ls
void ls(DirectoryTree* dirTree);
void ls_a(DirectoryTree* dirTree);
void ls_l(DirectoryTree* dirTree);
void ls_al(DirectoryTree* dirTree);

//cat
int Concatenate(DirectoryTree* dirTree, char* fName, int o);
//chmod
int ChangeMode(DirectoryTree* dirTree, int mode, char* dirName);
void ChangeModeAll(TreeNode* dirNode, int mode);
//chown
int ChangeOwner(DirectoryTree* dirTree, char* userName, char* dirName);
void ChangeOwnerAll(TreeNode* dirNode, char* userName);
//find
int ReadDir(DirectoryTree* dirTree, char* tmp, char* dirName, int o);
void FindDir(DirectoryTree* dirTree, char* dirName, int o);
//user

void WriteUser(UserList* userList, UserNode* userNode);
void SaveUserList(UserList* userList);
int ReadUser(UserList* userList, char* tmp);
UserList* LoadUserList();
UserNode* IsExistUser(UserList* userList, char* userName);

int HasPermission(TreeNode* dirNode, char o);
void Login(UserList* userList, DirectoryTree* dirTree);
//stack
int IsEmpty(Stack* dirStack);
Stack* InitializeStack();
int Push(Stack* dirStack, char* dirName);
char* Pop(Stack* dirStack);
//time
void GetMonth(int i);
void GetWeekday(int i);
//global variable
DirectoryTree* Linux;
Stack* dStack;
UserList* usrList;
FILE* Dir;
FILE* User;


int main()
{
    char cmd[50];
    Linux = LoadDir();
    usrList = LoadUserList();
    dStack = InitializeStack();

    Login(usrList, Linux);
    //PrintStart();
    SaveUserList(usrList);

    while (1) {
        PrintHead(Linux, dStack);
        fgets(cmd, sizeof(cmd), stdin);
        cmd[strlen(cmd) - 1] = '\0';
        Instruction(Linux, cmd);
    }
    return 0;
}
//directory
//utility
int Mode2Permission(TreeNode* dirNode)
{
    char buf[4];
    int tmp;
    int j;

    for (int i = 0; i < 9; i++)
        dirNode->permission[i] = 0;

    sprintf(buf, "%d", dirNode->mode);

    for (int i = 0; i < 3; i++) {
        tmp = buf[i] - '0';
        j = 2;

        while (tmp != 0) {
            dirNode->permission[3 * i + j] = tmp % 2;
            tmp /= 2;
            j--;
        }
    }
    return 0;
}

void PrintPermission(TreeNode* dirNode)
{
    char rwx[4] = "rwx";

    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            if (dirNode->permission[3 * i + j] == 1)
                printf("%c", rwx[j]);
            else
                printf("-");
        }
    }
}

void DestroyNode(TreeNode* dirNode)
{
    free(dirNode);
}

void DestroyDir(TreeNode* dirNode)
{
    if (dirNode->RightSibling != NULL) {
        DestroyDir(dirNode->RightSibling);
    }
    if (dirNode->LeftChild != NULL) {
        DestroyDir(dirNode->LeftChild);
    }

    dirNode->LeftChild = NULL;
    dirNode->RightSibling = NULL;
    DestroyNode(dirNode);
}

TreeNode* IsExistDir(DirectoryTree* dirTree, char* dirName, char type)
{
    //variables
    TreeNode* returnNode = NULL;

    returnNode = dirTree->current->LeftChild;

    while (returnNode != NULL) {
        if (strcmp(returnNode->name, dirName) == 0 && returnNode->type == type)
            break;
        returnNode = returnNode->RightSibling;
    }
    return returnNode;
}

char* getDir(char* dirPath)
{
    char* tmpPath = (char*)malloc(MAX_DIR);
    char* str = NULL;
    char tmp[MAX_DIR];
    char tmp2[MAX_DIR];

    strncpy(tmp, dirPath, MAX_DIR);
    str = strtok(dirPath, "/");
    while (str != NULL) {
        strncpy(tmp2, str, MAX_DIR);
        str = strtok(NULL, "/");
    }
    strncpy(tmpPath, tmp, strlen(tmp) - strlen(tmp2) - 1);
    tmpPath[strlen(tmp) - strlen(tmp2) - 1] = '\0';

    return tmpPath;
}
//mycp
int mycp(DirectoryTree* dirTree, char* sName, char* oName) {

    //printf("source : %s\n",sName);
    //printf("object : %s\n\n",oName);

    char buf[1024];
    int in, out;
    int nread;

    if (access(sName, F_OK) != 0) {
        printf("원본 파일이 존재하지 않습니다.\n");
        return -1;
    }
    if (strcmp(sName, oName) == 0) {
        printf("원본과 대상이 같습니다.\n");
        return -1;
    }

    in = open(sName, O_RDONLY); //원본파일
    out = open(oName, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR);//만들파일
    nread = read(in, buf, sizeof(buf)); //읽은만큼 nread가 올라가고
    write(out, buf, nread);          //read만큼 쓴다.

    MakeDir(dirTree, oName, 'f');

    return 0;
}

//save & load
void getPath(DirectoryTree* dirTree, TreeNode* dirNode, Stack* dirStack)
{
    TreeNode* tmpNode = NULL;
    char tmp[MAX_DIR] = "";

    tmpNode = dirNode->Parent;

    if (tmpNode == dirTree->root) {
        strcpy(tmp, "/");
    }
    else {
        while (tmpNode->Parent != NULL) {
            Push(dirStack, tmpNode->name);
            tmpNode = tmpNode->Parent;
        }
        while (IsEmpty(dirStack) == 0) {
            strcat(tmp, "/");
            strcat(tmp, Pop(dirStack));
        }
    }
    fprintf(Dir, " %s\n", tmp);
}

void WriteNode(DirectoryTree* dirTree, TreeNode* dirNode, Stack* dirStack)
{
    fprintf(Dir, "%s %c %d ", dirNode->name, dirNode->type, dirNode->mode);
    fprintf(Dir, "%d %d %d %d %d %d %d", dirNode->SIZE, dirNode->UID, dirNode->GID, dirNode->month, dirNode->day, dirNode->hour, dirNode->minute);

    if (dirNode == dirTree->root)
        fprintf(Dir, "\n");
    else
        getPath(dirTree, dirNode, dirStack);

    if (dirNode->RightSibling != NULL) {
        WriteNode(dirTree, dirNode->RightSibling, dirStack);
    }
    if (dirNode->LeftChild != NULL) {
        WriteNode(dirTree, dirNode->LeftChild, dirStack);
    }
}

void SaveDir(DirectoryTree* dirTree, Stack* dirStack)
{
    Dir = fopen("Directory.txt", "w");
    WriteNode(dirTree, dirTree->root, dirStack);
    fclose(Dir);
}

int ReadNode(DirectoryTree* dirTree, char* tmp)
{
    TreeNode* NewNode = (TreeNode*)malloc(sizeof(TreeNode));
    TreeNode* tmpNode = NULL;
    char* str;

    NewNode->LeftChild = NULL;
    NewNode->RightSibling = NULL;
    NewNode->Parent = NULL;

    str = strtok(tmp, " ");
    strncpy(NewNode->name, str, MAX_NAME);
    str = strtok(NULL, " ");
    NewNode->type = str[0];
    str = strtok(NULL, " ");
    NewNode->mode = atoi(str);
    Mode2Permission(NewNode);
    str = strtok(NULL, " ");
    NewNode->SIZE = atoi(str);
    str = strtok(NULL, " ");
    NewNode->UID = atoi(str);
    str = strtok(NULL, " ");
    NewNode->GID = atoi(str);
    str = strtok(NULL, " ");
    NewNode->month = atoi(str);
    str = strtok(NULL, " ");
    NewNode->day = atoi(str);
    str = strtok(NULL, " ");
    NewNode->hour = atoi(str);
    str = strtok(NULL, " ");
    NewNode->minute = atoi(str);

    str = strtok(NULL, " ");
    if (str != NULL) {
        str[strlen(str) - 1] = '\0';
        MovePath(dirTree, str);
        NewNode->Parent = dirTree->current;

        if (dirTree->current->LeftChild == NULL) {
            dirTree->current->LeftChild = NewNode;
        }
        else {
            tmpNode = dirTree->current->LeftChild;
            while (tmpNode->RightSibling != NULL)
                tmpNode = tmpNode->RightSibling;
            tmpNode->RightSibling = NewNode;
        }
    }
    else {
        dirTree->root = NewNode;
        dirTree->current = dirTree->root;
    }
    return 0;
}

DirectoryTree* LoadDir()
{
    DirectoryTree* dirTree = (DirectoryTree*)malloc(sizeof(DirectoryTree));
    char tmp[MAX_LENGTH];
    Dir = fopen("Directory.txt", "r");

    while (fgets(tmp, MAX_LENGTH, Dir) != NULL) {
        ReadNode(dirTree, tmp);
    }

    fclose(Dir);
    dirTree->current = dirTree->root;
    return dirTree;
}

//mkdir
DirectoryTree* InitializeTree()
{
    DirectoryTree* dirTree = (DirectoryTree*)malloc(sizeof(DirectoryTree));
    TreeNode* NewNode = (TreeNode*)malloc(sizeof(TreeNode));

    time(&ltime);
    today = localtime(&ltime);
    strncpy(NewNode->name, "/", MAX_NAME);

    NewNode->type = 'd';
    NewNode->mode = 755;
    Mode2Permission(NewNode);
    NewNode->UID = usrList->head->UID;
    NewNode->GID = usrList->head->GID;
    NewNode->SIZE = 4096;
    NewNode->month = today->tm_mon + 1;
    NewNode->day = today->tm_mday;
    NewNode->hour = today->tm_hour;
    NewNode->minute = today->tm_min;
    NewNode->Parent = NULL;
    NewNode->LeftChild = NULL;
    NewNode->RightSibling = NULL;

    dirTree->root = NewNode;
    dirTree->current = dirTree->root;

    return dirTree;
}

int MakeDir(DirectoryTree* dirTree, char* dirName, char type)
{
    TreeNode* NewNode = (TreeNode*)malloc(sizeof(TreeNode));
    TreeNode* tmpNode = NULL;

    if (HasPermission(dirTree->current, 'w') != 0) {
        printf("mkdir: '%s' 디렉터리를 만들 수 없습니다: 허가 거부\n", dirName);
        free(NewNode);
        return -1;
    }
    if (strcmp(dirName, ".") == 0 || strcmp(dirName, "..") == 0) {
        printf("mkdir: '%s' 디렉터리를 만들 수 없습니다\n", dirName);
        free(NewNode);
        return -1;
    }
    tmpNode = IsExistDir(dirTree, dirName, type);
    if (tmpNode != NULL && tmpNode->type == 'd') {
        printf("mkdir: '%s' 디렉터리를 만들 수 없습니다: 파일이 존재합니다\n", dirName);
        free(NewNode);
        return -1;
    }
    time(&ltime);
    today = localtime(&ltime);

    NewNode->LeftChild = NULL;
    NewNode->RightSibling = NULL;

    strncpy(NewNode->name, dirName, MAX_NAME);
    if (dirName[0] == '.') {
        NewNode->type = 'd';
        //rwx------
        NewNode->mode = 700;
        NewNode->SIZE = 4096;
    }
    else if (type == 'd') {
        NewNode->type = 'd';
        //rwxr-xr-x
        NewNode->mode = 755;
        NewNode->SIZE = 4096;
    }
    else {
        NewNode->type = 'f';
        //rw-r--r--
        NewNode->mode = 644;
        NewNode->SIZE = 0;
    }
    Mode2Permission(NewNode);
    NewNode->UID = usrList->current->UID;
    NewNode->GID = usrList->current->GID;
    NewNode->month = today->tm_mon + 1;
    NewNode->day = today->tm_mday;
    NewNode->hour = today->tm_hour;
    NewNode->minute = today->tm_min;
    NewNode->Parent = dirTree->current;

    if (dirTree->current->LeftChild == NULL) {
        dirTree->current->LeftChild = NewNode;
    }
    else {
        tmpNode = dirTree->current->LeftChild;

        while (tmpNode->RightSibling != NULL) {
            tmpNode = tmpNode->RightSibling;
        }
        tmpNode->RightSibling = NewNode;
    }

    return 0;
}

//rm
int RemoveDir(DirectoryTree* dirTree, char* dirName)
{
    TreeNode* DelNode = NULL;
    TreeNode* tmpNode = NULL;
    TreeNode* prevNode = NULL;

    tmpNode = dirTree->current->LeftChild;

    if (tmpNode == NULL) {
        printf("rm: '%s'를 지울 수 없음: 그런 파일이나 디렉터리가 없습니다\n", dirName);
        return -1;
    }

    if (strcmp(tmpNode->name, dirName) == 0) {
        dirTree->current->LeftChild = tmpNode->RightSibling;
        DelNode = tmpNode;
        if (DelNode->LeftChild != NULL)
            DestroyDir(DelNode->LeftChild);
        DestroyNode(DelNode);
    }
    else {
        while (tmpNode != NULL) {
            if (strcmp(tmpNode->name, dirName) == 0) {
                DelNode = tmpNode;
                break;
            }
            prevNode = tmpNode;
            tmpNode = tmpNode->RightSibling;
        }
        if (DelNode != NULL) {
            prevNode->RightSibling = DelNode->RightSibling;

            if (DelNode->LeftChild != NULL)
                DestroyDir(DelNode->LeftChild);
            DestroyNode(DelNode);
        }
        else {
            printf("rm: '%s'를 지울 수 없음: 그런 파일이나 디렉터리가 없습니다\n", dirName);
            return -1;
        }
    }
    return 0;
}

//cd
int Movecurrent(DirectoryTree* dirTree, char* dirPath)
{
    TreeNode* tmpNode = NULL;
    if (strcmp(dirPath, ".") == 0) {
    }
    else if (strcmp(dirPath, "..") == 0) {
        if (dirTree->current != dirTree->root) {
            dirTree->current = dirTree->current->Parent;
        }
    }
    else {

        tmpNode = IsExistDir(dirTree, dirPath, 'd');
        if (tmpNode != NULL) {
            dirTree->current = tmpNode;
        }
        else
            return -1;
    }
    return 0;
}

int MovePath(DirectoryTree* dirTree, char* dirPath)
{
    TreeNode* tmpNode = NULL;
    char tmpPath[MAX_DIR];
    char* str = NULL;
    int val = 0;

    strncpy(tmpPath, dirPath, MAX_DIR);
    tmpNode = dirTree->current;
    //if input is root
    if (strcmp(dirPath, "/") == 0) {
        dirTree->current = dirTree->root;
    }
    else {
        //if input is absolute path
        if (strncmp(dirPath, "/", 1) == 0) {
            if (strtok(dirPath, "/") == NULL) {
                return -1;
            }
            dirTree->current = dirTree->root;
        }
        //if input is relative path
        str = strtok(tmpPath, "/");
        while (str != NULL) {
            val = Movecurrent(dirTree, str);
            //if input path doesn't exist
            if (val != 0) {
                dirTree->current = tmpNode;
                return -1;
            }
            str = strtok(NULL, "/");
        }
    }
    return 0;
}

//pwd
int PrintPath(DirectoryTree* dirTree, Stack* dirStack)
{

    TreeNode* tmpNode = NULL;
    tmpNode = dirTree->current;
    //if current directory is root
    if (tmpNode == dirTree->root) {
        printf("/");
    }
    else {
        //until current directory is root, repeat Push
        while (tmpNode->Parent != NULL) {
            Push(dirStack, tmpNode->name);
            tmpNode = tmpNode->Parent;
        }
        //until stack is empty, repeat Pop
        while (IsEmpty(dirStack) == 0) {
            printf("/");
            printf("%s", Pop(dirStack));
        }
    }
    printf("\n");
    return 0;
}

//cat
int Concatenate(DirectoryTree* dirTree, char* fName, int o)
{
    UserNode* tmpUser = NULL;
    TreeNode* tmpNode = NULL;
    FILE* fp;
    char buf[MAX_BUFFER];
    char tmpName[MAX_NAME];
    char* str;
    int tmpSIZE = 0;
    int cnt = 1;

    //file read
    if (o != 0) {
        if (o == 4) {
            tmpUser = usrList->head;
            while (tmpUser != NULL) {
                printf("%s:x:%d:%d:%s:%s\n", tmpUser->name, tmpUser->UID, tmpUser->GID, tmpUser->name, tmpUser->dir);
                tmpUser = tmpUser->LinkNode;
            }
            return 0;
        }
        tmpNode = IsExistDir(dirTree, fName, 'f');

        if (tmpNode == NULL) {
            return -1;
        }
        fp = fopen(fName, "r");

        while (feof(fp) == 0) {
            fgets(buf, sizeof(buf), fp);
            if (feof(fp) != 0) {
                break;
            }
            if (o == 2) {
                if (buf[strlen(buf) - 1] == '\n') {
                    printf("     %d ", cnt);
                    cnt++;
                }
            }
            else if (o == 3) {
                if (buf[strlen(buf) - 1] == '\n' && buf[0] != '\n') {
                    printf("     %d ", cnt);
                    cnt++;
                }
            }
            fputs(buf, stdout);
        }
        fclose(fp);
    }
    else {
        fp = fopen(fName, "w");
        while (fgets(buf, sizeof(buf), stdin)) {
            fputs(buf, fp);
            tmpSIZE += strlen(buf) - 1;
        }
        fclose(fp);
        tmpNode = IsExistDir(dirTree, fName, 'f');
        if (tmpNode != NULL) {
            time(&ltime);
            today = localtime(&ltime);
            tmpNode->month = today->tm_mon + 1;
            tmpNode->day = today->tm_mday;
            tmpNode->hour = today->tm_hour;
            tmpNode->minute = today->tm_min;
        }
        else {
            MakeDir(dirTree, fName, 'f');
        }
        tmpNode = IsExistDir(dirTree, fName, 'f');
        tmpNode->SIZE = tmpSIZE;
    }
    return 0;
}
//chmod
int ChangeMode(DirectoryTree* dirTree, int mode, char* dirName)
{
    TreeNode* tmpNode = NULL;
    TreeNode* tmpNode2 = NULL;
    tmpNode = IsExistDir(dirTree, dirName, 'd');
    tmpNode2 = IsExistDir(dirTree, dirName, 'f');

    if (tmpNode != NULL) {
        if (HasPermission(tmpNode, 'w') != 0) {
            printf("chmod: '%s'파일을 수정할 수 없음: 허가거부\n", dirName);
            return -1;
        }
        tmpNode->mode = mode;
        Mode2Permission(tmpNode);
    }
    else if (tmpNode2 != NULL) {
        if (HasPermission(tmpNode2, 'w') != 0) {
            printf("chmod: '%s'파일을 수정할 수 없음: 허가거부\n", dirName);
            return -1;
        }
        tmpNode2->mode = mode;
        Mode2Permission(tmpNode2);
    }
    else {
        printf("chmod: '%s에 접근할 수 없습니다: 그런 파일이나 디렉터리가 없습니다\n", dirName);
        return -1;
    }
    return 0;
}

void ChangeModeAll(TreeNode* dirNode, int mode)
{
    if (dirNode->RightSibling != NULL) {
        ChangeModeAll(dirNode->RightSibling, mode);
    }
    if (dirNode->LeftChild != NULL) {
        ChangeModeAll(dirNode->LeftChild, mode);
    }
    dirNode->mode = mode;
    Mode2Permission(dirNode);
}
//chown
int ChangeOwner(DirectoryTree* dirTree, char* userName, char* dirName)
{
    TreeNode* tmpNode = NULL;
    TreeNode* tmpNode2 = NULL;
    UserNode* tmpUser = NULL;
    tmpNode = IsExistDir(dirTree, dirName, 'd');
    tmpNode2 = IsExistDir(dirTree, dirName, 'f');

    if (tmpNode != NULL) {
        if (HasPermission(tmpNode, 'w') != 0) {
            printf("chown: '%s'파일을 수정할 수 없음: 허가거부\n", dirName);
            return -1;
        }
        tmpUser = IsExistUser(usrList, userName);
        if (tmpUser != NULL) {
            tmpNode->UID = tmpUser->UID;
            tmpNode->GID = tmpUser->GID;
        }
        else {
            printf("chown: 잘못된 사용자: '%s'\n", userName);
            printf("Try 'chown --help' for more information.\n");
            return -1;
        }
    }
    else if (tmpNode2 != NULL) {
        if (HasPermission(tmpNode2, 'w') != 0) {
            printf("chown: '%s'파일을 수정할 수 없음: 허가거부\n", dirName);
            return -1;
        }
        tmpUser = IsExistUser(usrList, userName);
        if (tmpUser != NULL) {
            tmpNode2->UID = tmpUser->UID;
            tmpNode2->GID = tmpUser->GID;
        }
        else {
            printf("chown: 잘못된 사용자: '%s'\n", userName);
            printf("Try 'chown --help' for more information.\n");
            return -1;
        }
    }
    else {
        printf("chown: '%s'에 접근할 수 없습니다: 그런 파일이나 디렉터리가 없습니다\n", dirName);
        return -1;
    }
    return 0;
}

void ChangeOwnerAll(TreeNode* dirNode, char* userName)
{
    UserNode* tmpUser = NULL;
    tmpUser = IsExistUser(usrList, userName);

    if (dirNode->RightSibling != NULL) {
        ChangeOwnerAll(dirNode->RightSibling, userName);
    }
    if (dirNode->LeftChild != NULL) {
        ChangeOwnerAll(dirNode->LeftChild, userName);
    }
    dirNode->UID = tmpUser->UID;
    dirNode->GID = tmpUser->GID;
}
//find
int ReadDir(DirectoryTree* dirTree, char* tmp, char* dirName, int o)
{
    char* str;
    char str2[MAX_NAME];
    if (o == 0) {
        str = strtok(tmp, " ");
        strcpy(str2, str);
        for (int i = 0; i < 10; i++) {
            str = strtok(NULL, " ");
        }
        if (str != NULL) {
            if (strstr(str2, dirName) != NULL) {
                str[strlen(str) - 1] = '\0';
                if (strcmp(str, "/") == 0)
                    printf("/%s\n", str2);
                else
                    printf("%s/%s\n", str, str2);
            }
        }
    }
    else {
        str = strtok(tmp, " ");
        strcpy(str2, str);
        for (int i = 0; i < 10; i++) {
            str = strtok(NULL, " ");
        }
        if (str != NULL) {
            if (strstr(str, dirName) != NULL) {
                str[strlen(str) - 1] = '\0';
                if (strcmp(str, "/") == 0)
                    printf("/%s\n", str2);
                else
                    printf("%s/%s\n", str, str2);
            }
        }
    }
    return 0;
}

void FindDir(DirectoryTree* dirTree, char* dirName, int o)
{
    char tmp[MAX_LENGTH];
    Dir = fopen("Directory.txt", "r");

    while (fgets(tmp, MAX_LENGTH, Dir) != NULL) {
        ReadDir(dirTree, tmp, dirName, o);
    }
    fclose(Dir);
}
//user
void WriteUser(UserList* userList, UserNode* userNode)
{
    time(&ltime);
    today = localtime(&ltime);

    userList->current->year = today->tm_year + 1900;
    userList->current->month = today->tm_mon + 1;
    userList->current->wday = today->tm_wday;
    userList->current->day = today->tm_mday;
    userList->current->hour = today->tm_hour;
    userList->current->minute = today->tm_min;
    userList->current->sec = today->tm_sec;

    fprintf(User, "%s %d %d %d %d %d %d %d %d %d %s\n", userNode->name, userNode->UID, userNode->GID, userNode->year, userNode->month, userNode->wday, userNode->day, userNode->hour, userNode->minute, userNode->sec, userNode->dir);

    if (userNode->LinkNode != NULL) {
        WriteUser(userList, userNode->LinkNode);
    }

}

void SaveUserList(UserList* userList)
{
    User = fopen("User.txt", "w");
    WriteUser(userList, userList->head);
    fclose(Dir);
}

int ReadUser(UserList* userList, char* tmp)
{
    UserNode* NewNode = (UserNode*)malloc(sizeof(UserNode));
    char* str;

    NewNode->LinkNode = NULL;

    str = strtok(tmp, " ");
    strncpy(NewNode->name, str, MAX_NAME);
    str = strtok(NULL, " ");
    NewNode->UID = atoi(str);
    str = strtok(NULL, " ");
    NewNode->GID = atoi(str);
    str = strtok(NULL, " ");
    NewNode->year = atoi(str);
    str = strtok(NULL, " ");
    NewNode->month = atoi(str);
    str = strtok(NULL, " ");
    NewNode->wday = atoi(str);
    str = strtok(NULL, " ");
    NewNode->day = atoi(str);
    str = strtok(NULL, " ");
    NewNode->hour = atoi(str);
    str = strtok(NULL, " ");
    NewNode->minute = atoi(str);
    str = strtok(NULL, " ");
    NewNode->sec = atoi(str);
    str = strtok(NULL, " ");
    str[strlen(str) - 1] = '\0';
    strncpy(NewNode->dir, str, MAX_DIR);

    if (strcmp(NewNode->name, "root") == 0) {
        userList->head = NewNode;
        userList->tail = NewNode;
    }
    else {
        userList->tail->LinkNode = NewNode;
        userList->tail = NewNode;
    }
    return 0;
}

UserList* LoadUserList()
{
    UserList* userList = (UserList*)malloc(sizeof(UserList));
    char tmp[MAX_LENGTH];
    User = fopen("User.txt", "r");

    while (fgets(tmp, MAX_LENGTH, User) != NULL) {
        ReadUser(userList, tmp);
    }

    fclose(User);
    userList->current = NULL;
    return userList;
}

UserNode* IsExistUser(UserList* userList, char* userName)
{
    UserNode* returnUser = NULL;
    returnUser = userList->head;

    while (returnUser != NULL) {
        if (strcmp(returnUser->name, userName) == 0)
            break;
        returnUser = returnUser->LinkNode;
    }
    return returnUser;
}

int HasPermission(TreeNode* dirNode, char o)
{
    if (usrList->current->UID == 0)
        return 0;

    if (usrList->current->UID == dirNode->UID) {
        if (o == 'r') {
            if (dirNode->permission[0] == 0)
                return -1;
            else
                return 0;
        }
        if (o == 'w') {
            if (dirNode->permission[1] == 0)
                return -1;
            else
                return 0;
        }
        if (o == 'x') {
            if (dirNode->permission[2] == 0)
                return -1;
            else
                return 0;
        }
    }
    else if (usrList->current->GID == dirNode->GID) {
        if (o == 'r') {
            if (dirNode->permission[3] == 0)
                return -1;
            else
                return 0;
        }
        if (o == 'w') {
            if (dirNode->permission[4] == 0)
                return -1;
            else
                return 0;
        }
        if (o == 'x') {
            if (dirNode->permission[5] == 0)
                return -1;
            else
                return 0;
        }
    }
    else {
        if (o == 'r') {
            if (dirNode->permission[6] == 0)
                return -1;
            else
                return 0;
        }
        if (o == 'w') {
            if (dirNode->permission[7] == 0)
                return -1;
            else
                return 0;
        }
        if (o == 'x') {
            if (dirNode->permission[8] == 0)
                return -1;
            else
                return 0;
        }
    }
    return -1;
}

void Login(UserList* userList, DirectoryTree* dirTree)
{
    UserNode* tmpUser = NULL;
    char userName[MAX_NAME];
    char tmp[MAX_DIR];

    tmpUser = userList->head;

    printf("Users: ");
    while (tmpUser != NULL) {
        printf("%s ", tmpUser->name);
        tmpUser = tmpUser->LinkNode;
    }
    printf("\n");

    while (1) {
        printf("Login as: ");
        fgets(userName, sizeof(userName), stdin);
        userName[strlen(userName) - 1] = '\0';
        if (strcmp(userName, "exit") == 0) {
            exit(0);
        }
        tmpUser = IsExistUser(userList, userName);
        if (tmpUser != NULL) {
            userList->current = tmpUser;
            break;
        }
        printf("'%s' 유저가 존재하지 않습니다\n", userName);
    }
    strcpy(tmp, userList->current->dir);
    MovePath(dirTree, tmp);
}
//stack
//stack function
int IsEmpty(Stack* dirStack)
{
    if (dirStack->TopNode == NULL) {
        return -1;
    }
    return 0;
}

Stack* InitializeStack()
{
    Stack* returnStack = (Stack*)malloc(sizeof(Stack));

    if (returnStack == NULL) {
        printf("error occurred, returnStack.\n");
        return NULL;
    }
    //initialize Stack
    returnStack->TopNode = NULL;
    return returnStack;
}

int Push(Stack* dirStack, char* dirName)
{
    StackNode* dirNode = (StackNode*)malloc(sizeof(StackNode));

    if (dirStack == NULL) {
        printf("error occurred, dirStack.\n");
        return -1;
    }
    if (dirNode == NULL) {
        printf("error occurred, dirNode.\n");
        return -1;
    }
    //set dirNode
    strncpy(dirNode->name, dirName, MAX_NAME);
    dirNode->LinkNode = dirStack->TopNode;
    //set dirStack
    dirStack->TopNode = dirNode;
    return 0;
}

char* Pop(Stack* dirStack)
{
    StackNode* returnNode = NULL;
    if (dirStack == NULL) {
        printf("error occurred, dirStack.\n");
        return NULL;
    }
    if (IsEmpty(dirStack) == -1) {
        printf("Stack Empty.\n");
        return NULL;
    }
    returnNode = dirStack->TopNode;
    dirStack->TopNode = returnNode->LinkNode;

    return returnNode->name;
}

//instruction command
int Mkdir(DirectoryTree* dirTree, char* cmd)
{
    TreeNode* tmpNode = NULL;
    char* str;
    char tmp[MAX_DIR];
    char tmp2[MAX_DIR];
    char tmp3[MAX_DIR];
    int val;
    int tmpMode;
    if (cmd == NULL) {
        printf("mkdir: 잘못된 연산자\n");
        printf("Try 'mkdir --help' for more information.\n");
        return -1;
    }

    tmpNode = dirTree->current;
    if (cmd[0] == '-') {
        if (strcmp(cmd, "-p") == 0) {
            str = strtok(NULL, " ");
            if (str == NULL) {
                printf("mkdir: 잘못된 연산자\n");
                printf("Try 'mkdir --help' for more information.\n");
                return -1;
            }
            if (strncmp(str, "/", 1) == 0) {
                dirTree->current = dirTree->root;
            }
            str = strtok(str, "/");
            while (str != NULL) {
                val = Movecurrent(dirTree, str);
                if (val != 0) {
                    MakeDir(dirTree, str, 'd');
                    Movecurrent(dirTree, str);
                }
                str = strtok(NULL, "/");
            }
            dirTree->current = tmpNode;
        }
        else if (strcmp(cmd, "-m") == 0) {
            str = strtok(NULL, " ");
            if (str == NULL) {
                printf("mkdir: 잘못된 연산자\n");
                printf("Try 'mkdir --help' for more information.\n");
                return -1;
            }
            if (str[0] - '0' < 8 && str[1] - '0' < 8 && str[2] - '0' < 8 && strlen(str) == 3) {
                tmpMode = atoi(str);
                str = strtok(NULL, " ");
                if (str == NULL) {
                    printf("mkdir: 잘못된 연산자\n");
                    printf("Try 'mkdir --help' for more information.\n");
                    return -1;
                }
                val = MakeDir(dirTree, str, 'd');
                if (val == 0) {
                    tmpNode = IsExistDir(dirTree, str, 'd');
                    tmpNode->mode = tmpMode;
                    Mode2Permission(tmpNode);
                }
            }
            else {
                printf("mkdir: 잘못된 모드: '%s'\n", str);
                printf("Try 'mkdir --help' for more information.\n");
                return -1;
            }
        }
        else {
            str = strtok(cmd, "-");
            if (str == NULL) {
                printf("mkdir: 잘못된 연산자\n");
                printf("Try 'mkdir --help' for more information.\n");
                return -1;
            }
            else {
                printf("mkdir: 부적절한 옵션 -- '%s'\n", str);
                printf("Try 'mkdir --help' for more information.\n");
                return -1;
            }
        }
    }
    else {
        str = strtok(NULL, " ");
        if (str == NULL) {
            strncpy(tmp, cmd, MAX_DIR);
            if (strstr(cmd, "/") == NULL) {
                MakeDir(dirTree, cmd, 'd');
                return 0;
            }
            else {
                strncpy(tmp2, getDir(cmd), MAX_DIR);
                val = MovePath(dirTree, tmp2);
                if (val != 0) {
                    printf("mkdir: '%s': 그런 파일이나 디렉터리가 없습니다\n", tmp2);
                    return -1;
                }
                str = strtok(tmp, "/");
                while (str != NULL) {
                    strncpy(tmp3, str, MAX_NAME);
                    str = strtok(NULL, "/");
                }
                MakeDir(dirTree, tmp3, 'd');
                dirTree->current = tmpNode;
            }
        }
    }
    return 0;
}

int rm(DirectoryTree* dirTree, char* cmd)
{
    TreeNode* currentNode = NULL;
    TreeNode* tmpNode = NULL;
    TreeNode* tmpNode2 = NULL;
    char* str;
    char tmp[MAX_DIR];
    char tmp2[MAX_DIR];
    char tmp3[MAX_DIR];
    int val;

    if (cmd == NULL) {
        printf("rm: 잘못된 연산자\n");
        printf("Try 'rm --help' for more information.\n");
        return -1;
    }
    currentNode = dirTree->current;
    if (cmd[0] == '-') {
        if (strcmp(cmd, "-r") == 0) {
            str = strtok(NULL, " ");
            if (str == NULL) {
                printf("rm: 잘못된 연산자\n");
                printf("Try 'rm --help' for more information.\n");

                return -1;
            }
            strncpy(tmp, str, MAX_DIR);
            if (strstr(str, "/") == NULL) {
                tmpNode = IsExistDir(dirTree, str, 'd');
                if (tmpNode == NULL) {
                    printf("rm: '%s'를 지울 수 없음: 그런 파일이나 디렉터리가 없습니다\n", str);
                    return -1;
                }
                else {
                    if (HasPermission(dirTree->current, 'w') != 0 || HasPermission(tmpNode, 'w') != 0) {
                        printf("rm: '%s'디렉터리 또는 파일을 지울 수 없습니다: 허가거부\n", str);
                        return -1;
                    }
                    RemoveDir(dirTree, str);
                }
            }
            else {
                strncpy(tmp2, getDir(str), MAX_DIR);
                val = MovePath(dirTree, tmp2);
                if (val != 0) {
                    printf("rm: '%s': 그런 파일이나 디렉터리가 없습니다\n", tmp2);
                    return -1;
                }
                str = strtok(tmp, "/");
                while (str != NULL) {
                    strncpy(tmp3, str, MAX_NAME);
                    str = strtok(NULL, "/");
                }
                tmpNode = IsExistDir(dirTree, tmp3, 'd');
                if (tmpNode == NULL) {
                    printf("rm: '%s'를 지울 수 없음: 그런 파일이나 디렉터리가 없습니다\n", tmp3);
                    dirTree->current = currentNode;
                    return -1;
                }
                else {
                    if (HasPermission(dirTree->current, 'w') != 0 || HasPermission(tmpNode, 'w') != 0) {
                        printf("rm: '%s'디렉터리 또는 파일을 지울 수 없습니다: 허가거부\n", tmp3);
                        dirTree->current = currentNode;
                        return -1;
                    }
                    RemoveDir(dirTree, tmp3);
                }
                dirTree->current = currentNode;
            }
        }
        else if (strcmp(cmd, "-f") == 0) {
            str = strtok(NULL, " ");
            if (str == NULL) {
                return -1;
            }
            strncpy(tmp, str, MAX_DIR);
            if (strstr(str, "/") == NULL) {
                tmpNode = IsExistDir(dirTree, str, 'f');
                tmpNode2 = IsExistDir(dirTree, str, 'd');

                if (tmpNode2 != NULL) {
                    return -1;
                }
                if (tmpNode == NULL) {
                    return -1;
                }
                else {
                    if (HasPermission(dirTree->current, 'w') != 0 || HasPermission(tmpNode, 'w') != 0) {
                        return -1;
                    }
                    RemoveDir(dirTree, str);
                }
            }
            else {
                strncpy(tmp2, getDir(str), MAX_DIR);
                val = MovePath(dirTree, tmp2);
                if (val != 0) {
                    return -1;
                }
                str = strtok(tmp, "/");
                while (str != NULL) {
                    strncpy(tmp3, str, MAX_NAME);
                    str = strtok(NULL, "/");
                }
                tmpNode = IsExistDir(dirTree, tmp3, 'f');
                tmpNode2 = IsExistDir(dirTree, tmp3, 'd');

                if (tmpNode2 != NULL) {
                    dirTree->current = currentNode;
                    return -1;
                }
                if (tmpNode == NULL) {
                    dirTree->current = currentNode;
                    return -1;
                }
                else {
                    if (HasPermission(dirTree->current, 'w') != 0 || HasPermission(tmpNode, 'w') != 0) {
                        dirTree->current = currentNode;
                        return -1;
                    }
                    RemoveDir(dirTree, tmp3);
                }
                dirTree->current = currentNode;
            }
        }
        else if (strcmp(cmd, "-rf") == 0) {
            str = strtok(NULL, " ");
            if (str == NULL) {
                return -1;
            }
            strncpy(tmp, str, MAX_DIR);
            if (strstr(str, "/") == NULL) {
                tmpNode = IsExistDir(dirTree, str, 'd');
                if (tmpNode == NULL) {
                    return -1;
                }
                else {
                    if (HasPermission(dirTree->current, 'w') != 0 || HasPermission(tmpNode, 'w') != 0) {
                        return -1;
                    }
                    RemoveDir(dirTree, str);
                }
            }
            else {
                strncpy(tmp2, getDir(str), MAX_DIR);
                val = MovePath(dirTree, tmp2);
                if (val != 0) {
                    return -1;
                }
                str = strtok(tmp, "/");
                while (str != NULL) {
                    strncpy(tmp3, str, MAX_NAME);
                    str = strtok(NULL, "/");
                }
                tmpNode = IsExistDir(dirTree, tmp3, 'd');
                if (tmpNode == NULL) {
                    dirTree->current = currentNode;
                    return -1;
                }
                else {
                    if (HasPermission(dirTree->current, 'w') != 0 || HasPermission(tmpNode, 'w') != 0) {
                        dirTree->current = currentNode;
                        return -1;
                    }
                    RemoveDir(dirTree, tmp3);
                }
                dirTree->current = currentNode;
            }
        }
        else if (strcmp(cmd, "--help") == 0) {
            printf("사용법: rm [<옵션>]... [<파일>]...\n");
            printf("  Remove (unlink) the FILE(s).\n\n");
            printf("  Options:\n");
            printf("    -f, --force    \t ignore nonexistent files and arguments, never prompt\n");
            printf("    -r, --recursive\t remove directories and their contents recursively\n");
            printf("        --help\t 이 도움말을 표시하고 끝냅니다\n");
            return -1;
        }
        else {
            str = strtok(cmd, "-");
            if (str == NULL) {
                printf("rm: 잘못된 연산자\n");
                printf("Try 'rm --help' for more information.\n");
                return -1;
            }
            else {
                printf("rm: 부적절한 옵션 -- '%s'\n", str);
                printf("Try 'rm --help' for more information.\n");
                return -1;
            }
        }
    }
    else {
        strncpy(tmp, cmd, MAX_DIR);
        if (strstr(cmd, "/") == NULL) {
            tmpNode = IsExistDir(dirTree, cmd, 'f');
            tmpNode2 = IsExistDir(dirTree, cmd, 'd');

            if (tmpNode2 != NULL) {
                printf("rm:'%s'를 지울 수 없음: 디렉터리입니다\n", cmd);
                return -1;
            }
            if (tmpNode == NULL) {
                printf("rm: '%s'를 지울 수 없음: 그런 파일이나 디렉터리가 없습니다\n", cmd);
                return -1;
            }
            else {
                if (HasPermission(dirTree->current, 'w') != 0 || HasPermission(tmpNode, 'w') != 0) {
                    printf("rm: '%s'디렉터리 또는 파일을 지울 수 없습니다: 허가거부\n", cmd);
                    return -1;
                }
                RemoveDir(dirTree, cmd);
            }
        }
        else {
            strncpy(tmp2, getDir(cmd), MAX_DIR);
            val = MovePath(dirTree, tmp2);
            if (val != 0) {
                printf("rm: '%s': 그런 파일이나 디렉터리가 없습니다\n", tmp2);
                return -1;
            }
            str = strtok(tmp, "/");
            while (str != NULL) {
                strncpy(tmp3, str, MAX_NAME);
                str = strtok(NULL, "/");
            }
            tmpNode = IsExistDir(dirTree, tmp3, 'f');
            tmpNode2 = IsExistDir(dirTree, tmp3, 'd');

            if (tmpNode2 != NULL) {
                printf("rm:'%s'를 지울 수 없음: 디렉터리입니다\n", tmp3);
                dirTree->current = currentNode;
                return -1;
            }
            if (tmpNode == NULL) {
                printf("rm: '%s'를 지울 수 없음: 그런 파일이나 디렉터리가 없습니다\n", tmp3);
                dirTree->current = currentNode;
                return -1;
            }
            else {
                if (HasPermission(dirTree->current, 'w') != 0 || HasPermission(tmpNode, 'w') != 0) {
                    printf("rm: '%s'디렉터리 또는 파일을 지울 수 없습니다: 허가거부\n", tmp3);
                    dirTree->current = currentNode;
                    return -1;
                }
                RemoveDir(dirTree, tmp3);
            }
            dirTree->current = currentNode;
        }
    }
    return 0;
}

int cd(DirectoryTree* dirTree, char* cmd)
{
    TreeNode* tmpNode = NULL;
    char* str = NULL;
    char tmp[MAX_DIR];
    int val;

    if (cmd == NULL) {
        strcpy(tmp, usrList->current->dir);
        MovePath(dirTree, tmp);
    }
    else if (cmd[0] == '-') {
        if (strcmp(cmd, "--help") == 0) {
            printf("사용법: cd 디렉터리...\n");
            printf("  Change the shell working directory.\n\n");
            printf("  Options:\n");
            printf("        --help\t 이 도움말을 표시하고 끝냅니다\n");
            return -1;
        }
        else {
            str = strtok(cmd, "-");
            if (str == NULL) {
                printf("cd: 잘못된 연산자\n");
                printf("Try 'cd --help' for more information.\n");
                return -1;
            }
            else {
                printf("cd: 부적절한 옵션 -- '%s'\n", str);
                printf("Try 'cd --help' for more information.\n");

                return -1;
            }
        }
    }
    else {
        tmpNode = IsExistDir(dirTree, cmd, 'd');
        if (tmpNode != NULL) {
            if (HasPermission(tmpNode, 'r') != 0) {
                printf("-bash: cd: '%s': 허가거부\n", cmd);
                return -1;
            }
        }
        tmpNode = IsExistDir(dirTree, cmd, 'f');
        if (tmpNode != NULL) {
            printf("-bash: cd: '%s': 디렉터리가 아닙니다\n", cmd);
            return -1;
        }
        val = MovePath(dirTree, cmd);
        if (val != 0)
            printf("-bash: cd: '%s': 그런 파일이나 디렉터리가 없습니다\n", cmd);
    }
    return 0;
}

int pwd(DirectoryTree* dirTree, Stack* dirStack, char* cmd)
{
    char* str = NULL;
    if (cmd == NULL) {
        PrintPath(dirTree, dirStack);
    }
    else if (cmd[0] == '-') {
        if (strcmp(cmd, "--help") == 0) {
            printf("사용법: pwd\n");
            printf("  Print the name of the current working directory.\n\n");
            printf("  Options:\n");
            printf("        --help\t 이 도움말을 표시하고 끝냅니다\n");
            return -1;
        }
        else {
            str = strtok(cmd, "-");
            if (str == NULL) {
                printf("pwd: 잘못된 연산자\n");
                printf("Try 'pwd --help' for more information.\n");
                return -1;
            }
            else {
                printf("pwd: 부적절한 옵션 -- '%s'\n", str);
                printf("Try 'pwd --help' for more information.\n");
                return -1;
            }
        }
    }

    return 0;
}

void ls(DirectoryTree* dirTree) {
    int count = 1;
    TreeNode* tmpNode = dirTree->current;
    if (tmpNode->LeftChild == NULL)
        printf("directory empt\n");
    else {
        tmpNode = tmpNode->LeftChild;
        while (tmpNode->RightSibling != NULL) {
            if (strlen(tmpNode->name) < 8)
                printf("%s\t\t", tmpNode->name);
            else
                printf("%s\t", tmpNode->name);
            tmpNode = tmpNode->RightSibling;
            if (count % 5 == 0)
                printf("\n");
            count++;
        }
        printf("%s\t\n", tmpNode->name);
    }
}
void ls_a(DirectoryTree* dirTree) {
    int count = 1;
    TreeNode* tmpNode = dirTree->current;
    if (tmpNode->LeftChild == NULL) {
        printf(".\t\t..\n");
    }
    else {
        printf(".\t\t..\t\t");
        count = count + 2;
        tmpNode = tmpNode->LeftChild;
        while (tmpNode->RightSibling != NULL) {
            if (strlen(tmpNode->name) < 8)
                printf("%s\t\t", tmpNode->name);
            else
                printf("%s\t", tmpNode->name);
            tmpNode = tmpNode->RightSibling;
            if (count % 5 == 0)
                printf("\n");
            count++;
        }
        printf("%s\t\n", tmpNode->name);
    }
}
void ls_l(DirectoryTree* dirTree) {
    time_t timer;
    TreeNode* tmpNode = dirTree->current;
    timer = time(NULL);
    if (tmpNode->LeftChild == NULL)
        printf("directory empty\n");
    else {
        tmpNode = tmpNode->LeftChild;
        while (tmpNode->RightSibling != NULL) {
            printf("%c", tmpNode->type);
            PrintPermission(tmpNode);
            printf(" %d %d %d", tmpNode->SIZE, tmpNode->UID, tmpNode->GID);
            printf(" %d(month) %d(day) %d(hour) %d(min) ", tmpNode->month, tmpNode->day, tmpNode->hour, tmpNode->minute);
            printf("%s\n", tmpNode->name);
            tmpNode = tmpNode->RightSibling;
        }
        printf("%c", tmpNode->type);
        PrintPermission(tmpNode);
        printf(" %d %d %d", tmpNode->SIZE, tmpNode->UID, tmpNode->GID);
        printf(" %d(month) %d(day) %d(hour) %d(min) ", tmpNode->month, tmpNode->day, tmpNode->hour, tmpNode->minute);
        printf("%s\n", tmpNode->name);
    }
}
void ls_al(DirectoryTree* dirTree) {
    time_t timer;
    TreeNode* tmpNode = dirTree->current;
    timer = time(NULL);
    if (tmpNode->LeftChild == NULL) {
        //.
        printf("%c", dirTree->current->type);
        PrintPermission(dirTree->current);
        printf(" %d %d %d", dirTree->current->SIZE, dirTree->current->UID, dirTree->current->GID);
        printf(" %d(month) %d(day) %d(hour) %d(min) ", dirTree->current->month, dirTree->current->day, dirTree->current->hour, dirTree->current->minute);
        printf(".\n");
        //..
        if (strcmp(dirTree->current->name, "/") == 0) {
            printf("%c", dirTree->current->type);
            PrintPermission(dirTree->current);
            printf(" %d %d %d", dirTree->current->SIZE, dirTree->current->UID, dirTree->current->GID);
            printf(" %d(month) %d(day) %d(hour) %d(min) ", dirTree->current->month, dirTree->current->day, dirTree->current->hour, dirTree->current->minute);
            printf("..\n");
        }
        else {
            printf("%c", tmpNode->Parent->type);
            PrintPermission(tmpNode->Parent);
            printf(" %d %d %d", tmpNode->Parent->SIZE, tmpNode->Parent->UID, tmpNode->Parent->GID);
            printf(" %d(month) %d(day) %d(hour) %d(min) ", tmpNode->Parent->month, tmpNode->Parent->day, tmpNode->Parent->hour, tmpNode->Parent->minute);
            printf("..\n");
        }
    }
    else {
        //.
        printf("%c", dirTree->current->type);
        PrintPermission(dirTree->current);
        printf(" %d %d %d", dirTree->current->SIZE, dirTree->current->UID, dirTree->current->GID);
        printf(" %d(month) %d(day) %d(hour) %d(min) ", dirTree->current->month, dirTree->current->day, dirTree->current->hour, dirTree->current->minute);
        printf(".\n");
        //..
        if (strcmp(dirTree->current->name, "/") == 0) {
            printf("%c", dirTree->current->type);
            PrintPermission(dirTree->current);
            printf(" %d %d %d", dirTree->current->SIZE, dirTree->current->UID, dirTree->current->GID);
            printf(" %d(month) %d(day) %d(hour) %d(min) ", dirTree->current->month, dirTree->current->day, dirTree->current->hour, dirTree->current->minute);
            printf("..\n");
        }
        else {
            printf("%c", tmpNode->Parent->type);
            PrintPermission(tmpNode->Parent);
            printf(" %d %d %d", tmpNode->Parent->SIZE, tmpNode->Parent->UID, tmpNode->Parent->GID);
            printf(" %d(month) %d(day) %d(hour) %d(min) ", tmpNode->Parent->month, tmpNode->Parent->day, tmpNode->Parent->hour, tmpNode->Parent->minute);
            printf("..\n");
        }


        tmpNode = tmpNode->LeftChild;
        while (tmpNode->RightSibling != NULL) {
            printf("%c", tmpNode->type);
            PrintPermission(tmpNode);
            printf(" %d %d %d", tmpNode->SIZE, tmpNode->UID, tmpNode->GID);
            printf(" %d(month) %d(day) %d(hour) %d(min) ", tmpNode->month, tmpNode->day, tmpNode->hour, tmpNode->minute);
            printf("%s\n", tmpNode->name);
            tmpNode = tmpNode->RightSibling;
        }
        printf("%c", tmpNode->type);
        PrintPermission(tmpNode);
        printf(" %d %d %d", tmpNode->SIZE, tmpNode->UID, tmpNode->GID);
        printf(" %d(month) %d(day) %d(hour) %d(min) ", tmpNode->month, tmpNode->day, tmpNode->hour, tmpNode->minute);
        printf("%s\n", tmpNode->name);
    }
}
int cat(DirectoryTree* dirTree, char* cmd)
{
    TreeNode* currentNode = NULL;
    TreeNode* tmpNode = NULL;
    TreeNode* tmpNode2 = NULL;
    char* str;
    char tmp[MAX_DIR];
    char tmp2[MAX_DIR];
    char tmp3[MAX_DIR];
    int val;
    /*
        cat0: write, EOF to save
        cat1: read
        cat2: read w/ line number
    */
    if (cmd == NULL) {
        printf("cat: 잘못된 연산자\n");
        return -1;
    }
    currentNode = dirTree->current;

    if (strcmp(cmd, ">") == 0) {  // > 옵션 사용 했을 때
        str = strtok(NULL, " ");
        if (str == NULL) {
            printf("cat: 잘못된 연산자\n");
            printf("Try 'cat --help' for more information.\n");
            return -1;
        }
        strncpy(tmp, str, MAX_DIR);
        if (strstr(str, "/") == NULL) {
            if (HasPermission(dirTree->current, 'w') != 0) { // 쓰기권환인지 확인
                printf("cat: '%s'파일을 만들 수 없음: 권한없음\n", dirTree->current->name);
                return -1;
            }
            tmpNode = IsExistDir(dirTree, str, 'd'); //쓰기권환이 있으면 IsExistDIR실행
            if (tmpNode != NULL) {
                printf("cat: '%s': 디렉터리입니다\n", str);
                return -1;
            }
            else {
                Concatenate(dirTree, str, 0);
            }
        }
        else {
            strncpy(tmp2, getDir(str), MAX_DIR);
            val = MovePath(dirTree, tmp2);
            if (val != 0) {
                printf("cat: '%s': 그런 파일이나 디렉터리가 없습니다\n", tmp2);
                return -1;
            }
            str = strtok(tmp, "/");
            while (str != NULL) {
                strncpy(tmp3, str, MAX_NAME);
                str = strtok(NULL, "/");
            }
            if (HasPermission(dirTree->current, 'w') != 0) {
                printf("cat: '%s'파일을 만들 수 없음: 허가거부\n", dirTree->current->name);
                dirTree->current = currentNode;
                return -1;
            }
            tmpNode = IsExistDir(dirTree, tmp3, 'd');
            if (tmpNode != NULL) {
                printf("cat: '%s': 디렉터리입니다\n", tmp3);
                dirTree->current = currentNode;
                return -1;
            }
            else {
                Concatenate(dirTree, tmp3, 0);
            }
            dirTree->current = currentNode;
        }
        return 0;
    }
    else if (cmd[0] == '-') {
        if (strcmp(cmd, "-n") == 0) {
            str = strtok(NULL, " ");
            strncpy(tmp, str, MAX_DIR);
            if (strstr(str, "/") == NULL) {
                if (HasPermission(dirTree->current, 'w') != 0) {
                    printf("cat: '%s'파일을 만들 수 없음: 허가거부\n", dirTree->current->name);
                    return -1;
                }
                tmpNode = IsExistDir(dirTree, str, 'd');
                tmpNode2 = IsExistDir(dirTree, str, 'f');

                if (tmpNode == NULL && tmpNode2 == NULL) {
                    printf("cat: '%s': 그런 파일이나 디렉터리가 없습니다\n", str);
                    return -1;
                }
                else if (tmpNode != NULL && tmpNode2 == NULL) {
                    printf("cat: '%s': 디렉터리입니다\n", str);
                    return -1;
                }
                else if (tmpNode2 != NULL && HasPermission(tmpNode2, 'r') != 0) {
                    printf("cat: '%s'파일을 열 수 없음: 허가거부\n", tmpNode2->name);
                    return -1;
                }
                else {
                    Concatenate(dirTree, str, 2);
                }
            }
            else {
                strncpy(tmp2, getDir(str), MAX_DIR);
                val = MovePath(dirTree, tmp2);
                if (val != 0) {
                    printf("cat: '%s': 그런 파일이나 디렉터리가 없습니다\n", tmp2);
                    return -1;
                }
                str = strtok(tmp, "/");
                while (str != NULL) {
                    strncpy(tmp3, str, MAX_NAME);
                    str = strtok(NULL, "/");
                }
                tmpNode = IsExistDir(dirTree, tmp3, 'd');
                tmpNode2 = IsExistDir(dirTree, tmp3, 'f');

                if (tmpNode == NULL && tmpNode2 == NULL) {
                    printf("cat: '%s': 그런 파일이나 디렉터리가 없습니다\n", tmp3);
                    dirTree->current = currentNode;
                    return -1;
                }
                else if (tmpNode != NULL && tmpNode2 == NULL) {
                    printf("cat: '%s': 디렉터리입니다\n", tmp3);
                    dirTree->current = currentNode;
                    return -1;
                }
                else if (tmpNode2 != NULL && HasPermission(tmpNode2, 'r') != 0) {
                    printf("cat: '%s'파일을 열 수 없음: 허가거부\n", tmpNode2->name);
                    dirTree->current = currentNode;
                    return -1;
                }
                else {
                    Concatenate(dirTree, tmp3, 2);
                }
                dirTree->current = currentNode;
            }
        }
        else if (strcmp(cmd, "-b") == 0) {
            str = strtok(NULL, " ");
            strncpy(tmp, str, MAX_DIR);
            if (strstr(str, "/") == NULL) {
                if (HasPermission(dirTree->current, 'w') != 0) {
                    printf("cat: '%s'파일을 만들 수 없음: 허가거부\n", dirTree->current->name);
                    return -1;
                }
                tmpNode = IsExistDir(dirTree, str, 'd');
                tmpNode2 = IsExistDir(dirTree, str, 'f');
                if (tmpNode == NULL && tmpNode2 == NULL) {
                    printf("cat: '%s': 그런 파일이나 디렉터리가 없습니다\n", str);
                    return -1;
                }
                else if (tmpNode != NULL && tmpNode2 == NULL) {
                    printf("cat: '%s': 디렉터리입니다\n", str);
                    return -1;
                }
                else if (tmpNode2 != NULL && HasPermission(tmpNode2, 'r') != 0) {
                    printf("cat: '%s'파일을 열 수 없음: 허가거부\n", tmpNode2->name);
                    return -1;
                }
                else {
                    Concatenate(dirTree, str, 3);
                }
            }
            else {
                strncpy(tmp2, getDir(str), MAX_DIR);
                val = MovePath(dirTree, tmp2);
                if (val != 0) {
                    printf("cat: '%s': 그런 파일이나 디렉터리가 없습니다\n", tmp2);
                    return -1;
                }
                str = strtok(tmp, "/");
                while (str != NULL) {
                    strncpy(tmp3, str, MAX_NAME);
                    str = strtok(NULL, "/");
                }
                tmpNode = IsExistDir(dirTree, tmp3, 'd');
                tmpNode2 = IsExistDir(dirTree, tmp3, 'f');
                if (tmpNode == NULL && tmpNode2 == NULL) {
                    printf("cat: '%s': 그런 파일이나 디렉터리가 없습니다\n", tmp3);
                    dirTree->current = currentNode;
                    return -1;
                }
                else if (tmpNode != NULL && tmpNode2 == NULL) {
                    printf("cat: '%s': 디렉터리입니다\n", tmp3);
                    dirTree->current = currentNode;
                    return -1;
                }
                else if (tmpNode2 != NULL && HasPermission(tmpNode2, 'r') != 0) {
                    printf("cat: '%s'파일을 열 수 없음: 허가거부\n", tmpNode2->name);
                    dirTree->current = currentNode;
                    return -1;
                }
                else {
                    Concatenate(dirTree, tmp3, 3);
                }
                dirTree->current = currentNode;
            }
        }
        else if (strcmp(cmd, "--help") == 0) {
            printf("사용법: cat [<옵션>]... [<파일>]...\n");
            printf("  FILE(들)을 합쳐서 표준 출력으로 보낸다.\n\n");
            printf("  Options:\n");
            printf("    -n, --number         \t number all output line\n");
            printf("    -b, --number-nonblank\t number nonempty output line\n");
            printf("        --help\t 이 도움말을 표시하고 끝냅니다\n");
            return -1;
        }
        else {
            str = strtok(cmd, "-");
            if (str == NULL) {
                printf("cat: 잘못된 연산자\n");
                printf("Try 'cat --help' for more information.\n");
                return -1;
            }
            else {
                printf("cat: 부적절한 옵션 -- '%s'\n", str);
                printf("Try 'cat --help' for more information.\n");
                return -1;
            }
        }
    }
    else {
        if (strcmp(cmd, "/etc/passwd") == 0) {
            Concatenate(dirTree, cmd, 4);
            return 0;
        }

        strncpy(tmp, cmd, MAX_DIR);
        if (strstr(cmd, "/") == NULL) {
            if (HasPermission(dirTree->current, 'w') != 0) {
                printf("cat: '%s'파일을 만들 수 없음: 허가거부\n", dirTree->current->name);
                return -1;
            }
            tmpNode = IsExistDir(dirTree, cmd, 'd');
            tmpNode2 = IsExistDir(dirTree, cmd, 'f');
            if (tmpNode == NULL && tmpNode2 == NULL) {
                printf("cat: '%s': 그런 파일이나 디렉터리가 없습니다\n", cmd);
                return -1;
            }
            else if (tmpNode != NULL && tmpNode2 == NULL) {
                printf("cat: '%s': 디렉터리입니다\n", cmd);
                return -1;
            }
            else if (tmpNode2 != NULL && HasPermission(tmpNode2, 'r') != 0) {
                printf("cat: '%s'파일을 열 수 없음: 허가거부\n", tmpNode2->name);
                return -1;
            }
            else {
                Concatenate(dirTree, cmd, 1);
            }

        }
        else {
            strncpy(tmp2, getDir(cmd), MAX_DIR);
            val = MovePath(dirTree, tmp2);
            if (val != 0) {
                printf("cat: '%s': 그런 파일이나 디렉터리가 없습니다\n", tmp2);
                return -1;
            }
            str = strtok(tmp, "/");
            while (str != NULL) {
                strncpy(tmp3, str, MAX_NAME);
                str = strtok(NULL, "/");
            }
            tmpNode = IsExistDir(dirTree, tmp3, 'd');
            tmpNode2 = IsExistDir(dirTree, tmp3, 'f');
            if (tmpNode == NULL && tmpNode2 == NULL) {
                printf("cat: '%s': 그런 파일이나 디렉터리가 없습니다\n", tmp3);
                dirTree->current = currentNode;
                return -1;
            }
            else if (tmpNode != NULL && tmpNode2 == NULL) {
                printf("cat: '%s': 디렉터리입니다\n", tmp3);
                dirTree->current = currentNode;
                return -1;
            }
            else if (tmpNode2 != NULL && HasPermission(tmpNode2, 'r') != 0) {
                printf("cat: '%s'파일을 열 수 없음: 허가거부\n", tmpNode2->name);
                dirTree->current = currentNode;
                return -1;
            }
            else {
                Concatenate(dirTree, tmp3, 1);
            }
            dirTree->current = currentNode;
        }
    }
    return 1;
}

void grep(char* searching_word, char* f_name) {
    int i = 1;
    char output_line[MAX_LENGTH];
    FILE* fp = fopen(f_name, "rt");
    if (fp == NULL)
        printf("cannot read the file\n");
    while (1) {
        if (feof(fp))
            break;
        else
            fgets(output_line, sizeof(output_line), fp);
        i++;
    }
    FILE* fp2 = NULL;
    fp2 = fopen(f_name, "rt");
    for (int j = 1; j < i - 1; j++) {
        fgets(output_line, sizeof(output_line), fp2);
        if (strstr(output_line, searching_word) != NULL)
            printf("%s", output_line);
    }
    fclose(fp);
}

void grep_n(char* searching_word, char* f_name) {
    int i = 1;
    char output_line[MAX_LENGTH];
    FILE* fp = fopen(f_name, "rt");
    if (fp == NULL)
        printf("cannot read the file\n");
    while (1) {
        if (feof(fp))
            break;
        else
            fgets(output_line, sizeof(output_line), fp);
        i++;
    }
    FILE* fp2 = NULL;
    fp2 = fopen(f_name, "rt");
    for (int j = 1; j < i - 1; j++) {
        fgets(output_line, sizeof(output_line), fp2);
        if (strstr(output_line, searching_word) != NULL)
            printf("%d:%s", j, output_line);
    }
    fclose(fp);
}

int chmod_(DirectoryTree* dirTree, char* cmd)
{
    TreeNode* tmpNode = NULL;
    char* str;
    int tmp;

    if (cmd == NULL) {
        printf("chmod: 잘못된 연산자\n");
        printf("Try 'chmod --help' for more information.\n");
        return -1;
    }
    if (cmd[0] == '-') {
        if (strcmp(cmd, "-R") == 0) {
            str = strtok(NULL, " ");
            if (str == NULL) {
                printf("chmod: 잘못된 연산자\n");
                printf("Try 'chmod --help' for more information.\n");
                return -1;
            }
            if (str[0] - '0' < 8 && str[1] - '0' < 8 && str[2] - '0' < 8 && strlen(str) == 3) {
                tmp = atoi(str);
                str = strtok(NULL, " ");
                if (str == NULL) {
                    printf("chmod: 잘못된 연산자\n");
                    printf("Try 'chmod --help' for more information.\n");
                    return -1;
                }
                tmpNode = IsExistDir(dirTree, str, 'd');
                if (tmpNode != NULL) {
                    if (tmpNode->LeftChild == NULL)
                        ChangeMode(dirTree, tmp, str);
                    else {
                        ChangeMode(dirTree, tmp, str);
                        ChangeModeAll(tmpNode->LeftChild, tmp);
                    }
                }
                else {
                    printf("chmod: '%s': 그런 파일이나 디렉터리가 없습니다\n", str);
                    return -1;
                }
            }
            else {
                printf("chmod: 잘못된 모드: '%s'\n", str);
                printf("Try 'chmod --help' for more information.\n");
                return -1;
            }
        }
        else if (strcmp(cmd, "--help") == 0) {
            printf("사용법: chmod [옵션]... 8진수-MODE... 디렉터리...\n");
            printf("  Change the mode of each FILE to MODE.\n\n");
            printf("  Options:\n");
            printf("    -R, --recursive\t change files and directories recursively\n");
            printf("        --help\t 이 도움말을 표시하고 끝냅니다\n");
            return -1;
        }
        else {
            str = strtok(cmd, "-");
            if (str == NULL) {
                printf("chmod: 잘못된 연산자\n");
                printf("Try 'chmod --help' for more information.\n");
                return -1;
            }
            else {
                printf("chmod: 부적절한 옵션 -- '%s'\n", str);
                printf("Try 'chmod --help' for more information.\n");
                return -1;
            }
        }
    }
    else {
        if (cmd[0] - '0' < 8 && cmd[1] - '0' < 8 && cmd[2] - '0' < 8 && strlen(cmd) == 3) {
            tmp = atoi(cmd);
            str = strtok(NULL, " ");
            if (str == NULL) {
                printf("chmod: 잘못된 연산자\n");
                printf("Try 'chmod --help' for more information.\n");
                return -1;
            }
            ChangeMode(dirTree, tmp, str);
        }
        else {
            printf("chmod: 잘못된 모드: '%s'\n", cmd);
            printf("Try 'chmod --help' for more information.\n");
            return -1;
        }
    }
    return 0;
}

int chown_(DirectoryTree* dirTree, char* cmd)
{
    TreeNode* tmpNode = NULL;
    UserNode* tmpUser = NULL;
    char* str;
    char tmp[MAX_NAME];

    if (cmd == NULL) {
        printf("chown: 잘못된 연산자\n");
        printf("Try 'chown --help' for more information.\n");
        return -1;
    }
    if (cmd[0] == '-') {
        if (strcmp(cmd, "-R") == 0) {
            str = strtok(NULL, " ");
            if (str == NULL) {
                printf("chown: 잘못된 연산자\n");
                printf("Try 'chown --help' for more information.\n");
                return -1;
            }
            tmpUser = IsExistUser(usrList, str);
            if (tmpUser != NULL) {
                strncpy(tmp, str, MAX_NAME);
            }
            else {
                printf("chown: 잘못된 사용자: '%s'\n", str);
                printf("Try 'chown --help' for more information.\n");
                return -1;
            }
            str = strtok(NULL, " ");
            if (str == NULL) {
                printf("chown: 잘못된 연산자\n");
                printf("Try 'chown --help' for more information.\n");
                return -1;
            }
            tmpNode = IsExistDir(dirTree, str, 'd');
            if (tmpNode != NULL) {
                if (tmpNode->LeftChild == NULL)
                    ChangeOwner(dirTree, tmp, str);
                else {
                    ChangeOwner(dirTree, tmp, str);
                    ChangeOwnerAll(tmpNode->LeftChild, tmp);
                }
            }
            else {
                printf("chown: '%s': 그런 파일이나 디렉터리가 없습니다\n", str);
                return -1;
            }
        }
        else if (strcmp(cmd, "--help") == 0) {
            printf("사용법: chown [옵션]... [소유자]... 파일...\n");
            printf("  Change the owner and/or group of each FILE to OWNER and/or GROUP.\n\n");
            printf("  Options:\n");
            printf("    -R, --recursive\t change files and directories recursively\n");
            printf("        --help\t 이 도움말을 표시하고 끝냅니다\n");
            return -1;
        }
        else {
            str = strtok(cmd, "-");
            if (str == NULL) {
                printf("chown: 잘못된 연산자\n");
                printf("Try 'chown --help' for more information.\n");
                return -1;
            }
            else {
                printf("chown: 부적절한 옵션 -- '%s'\n", str);
                printf("Try 'chown --help' for more information.\n");
                return -1;
            }
        }
    }
    else {
        strncpy(tmp, cmd, MAX_NAME);
        str = strtok(NULL, " ");
        if (str == NULL) {
            printf("chown: 잘못된 연산자\n");
            printf("Try 'chown --help' for more information.\n");
            return -1;
        }
        else {
            ChangeOwner(dirTree, tmp, str);
        }
    }
    return 0;
}

int find_(DirectoryTree* dirTree, char* cmd)
{
    char* str;
    if (cmd == NULL) {
        FindDir(dirTree, dirTree->current->name, 1);
        return 0;
    }
    else if (cmd[0] == '-') {
        if (strcmp(cmd, "-name") == 0) {
            str = strtok(NULL, " ");
            if (str == NULL) {
                printf("find: 잘못된 연산자\n");
                printf("Try 'find --help' for more information.\n");
                return -1;
            }
            FindDir(dirTree, str, 0);
        }
        else if (strcmp(cmd, "--help") == 0) {
            printf("사용법: find [<옵션>]... [<파일>]...\n");
            printf("\n");
            printf("  Options:\n");
            printf("    -name\t finds file by name\n");
            printf("        --help\t 이 도움말을 표시하고 끝냅니다\n");
            return -1;
        }
        else {
            str = strtok(cmd, "-");
            if (str == NULL) {
                printf("find: 잘못된 연산자\n");
                printf("Try 'find --help' for more information.\n");
                return -1;
            }
            else {
                printf("find: 부적절한 옵션 -- '%s'\n", str);
                printf("Try 'find --help' for more information.\n");
                return -1;
            }
        }
    }
    else {
        FindDir(dirTree, cmd, 1);
    }

    return 0;
}

void Instruction(DirectoryTree* dirTree, char* cmd)
{
    char* str;
    char* str1;
    char* str2;
    int val;
    if (strcmp(cmd, "") == 0 || cmd[0] == ' ') {
        return;
    }
    str = strtok(cmd, " ");

    if (strcmp(str, "mkdir") == 0) {
        str = strtok(NULL, " ");
        val = Mkdir(dirTree, str);
        if (val == 0) {
            SaveDir(dirTree, dStack);
        }
    }
    else if (strcmp(str, "cp") == 0) {
        str = strtok(NULL, " ");
        str1 = strtok(NULL, " ");
        val = mycp(dirTree, str, str1);
        if (val == 0) {
            SaveDir(dirTree, dStack);
        }
    }
    else if (strcmp(str, "rm") == 0) {
        str = strtok(NULL, " ");
        val = rm(dirTree, str);
        if (val == 0) {
            SaveDir(dirTree, dStack);
        }
    }
    else if (strcmp(str, "cd") == 0) {
        str = strtok(NULL, " ");
        cd(dirTree, str);
    }
    else if (strcmp(str, "pwd") == 0) {
        str = strtok(NULL, " ");
        pwd(dirTree, dStack, str);
    }
    else if (strcmp(str, "ls") == 0) {
        str = strtok(NULL, " ");
        if (str == NULL)
            ls(dirTree);
        else if (strcmp(str, "-a") == 0)
            ls_a(dirTree);
        else if (strcmp(str, "-l") == 0)
            ls_l(dirTree);
        else
            ls_al(dirTree);
    }
    else if (strcmp(str, "cat") == 0) {
        str = strtok(NULL, " ");
        val = cat(dirTree, str);
        if (val == 0) {
            SaveDir(dirTree, dStack);
        }
    }
    else if (strcmp(str, "chmod") == 0) {
        str = strtok(NULL, " ");
        val = chmod_(dirTree, str);
        if (val == 0) {
            SaveDir(dirTree, dStack);
        }
    }
    else if (strcmp(str, "chown") == 0) {
        str = strtok(NULL, " ");
        val = chown_(dirTree, str);
        if (val == 0) {
            SaveDir(dirTree, dStack);
        }
    }
    else if (strcmp(str, "find") == 0) {
        str = strtok(NULL, " ");
        find_(dirTree, str);
    }
    else if (strcmp(cmd, "exit") == 0) {
        printf("로그아웃\n");
        exit(0);
    }
    else if (strcmp(str, "grep") == 0) {
        str = strtok(NULL, " ");
        str1 = strtok(NULL, " ");
        str2 = strtok(NULL, " ");
        if (strcmp(str, "-n") == 0)
            grep_n(str1, str2);
        else
            grep(str, str1);
    }
    else {
        printf("'%s': 명령을 찾을 수 없습니다\n", cmd);
    }
    return;
}

void PrintHead(DirectoryTree* dirTree, Stack* dirStack)
{
    TreeNode* tmpNode = NULL;
    char tmp[MAX_DIR] = "";
    char tmp2[MAX_DIR] = "";
    char usr;

    if (usrList->current == usrList->head)
        usr = '#';
    else
        usr = '$';

    printf("%s@os-Virtualbox", usrList->current->name);
    DEFAULT;
    printf(":");
    tmpNode = dirTree->current;

    if (tmpNode == dirTree->root) {
        strcpy(tmp, "/");
    }
    else {
        while (tmpNode->Parent != NULL) {
            Push(dirStack, tmpNode->name);
            tmpNode = tmpNode->Parent;
        }
        while (IsEmpty(dirStack) == 0) {
            strcat(tmp, "/");
            strcat(tmp, Pop(dirStack));
        }
    }

    strncpy(tmp2, tmp, strlen(usrList->current->dir));

    if (usrList->current == usrList->head) {
        printf("%s", tmp);
    }
    else if (strcmp(usrList->current->dir, tmp2) != 0) {
        printf("%s", tmp);
    }
    else {
        tmpNode = dirTree->current;
        while (tmpNode->Parent != NULL) {
            Push(dirStack, tmpNode->name);
            tmpNode = tmpNode->Parent;
        }
        Pop(dirStack);
        Pop(dirStack);
        printf("~");
        while (IsEmpty(dirStack) == 0) {
            printf("/");
            printf("%s", Pop(dirStack));
        }
    }
    DEFAULT;
    printf("%c ", usr);
}
//time
void GetMonth(int i)
{
    switch (i) {
    case 1:
        printf("Jan ");
        break;
    case 2:
        printf("Feb ");
        break;
    case 3:
        printf("Mar ");
        break;
    case 4:
        printf("Apr ");
        break;
    case 5:
        printf("May ");
        break;
    case 6:
        printf("Jun ");
        break;
    case 7:
        printf("Jul ");
        break;
    case 8:
        printf("Aug ");
        break;
    case 9:
        printf("Sep ");
        break;
    case 10:
        printf("Oct ");
        break;
    case 11:
        printf("Nov ");
        break;
    case 12:
        printf("Dec ");
        break;
    default:
        break;
    }
}

void GetWeekday(int i)
{
    switch (i) {
    case 0:
        printf("Sun ");
        break;
    case 1:
        printf("Mon ");
        break;
    case 2:
        printf("Tue ");
        break;
    case 3:
        printf("Wed ");
        break;
    case 4:
        printf("Thu ");
        break;
    case 5:
        printf("Fri ");
        break;
    case 6:
        printf("Sat ");
        break;
    default:
        break;
    }
}
