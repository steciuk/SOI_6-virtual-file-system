#include<stdlib.h>
#include<stdio.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<fcntl.h>
#include<string.h>
#include <unistd.h>

#define BUF_SIZE        2048
#define FILES_NUM       10
#define MAX_FILE_NAME   128

char* vFileName;
unsigned long vFileSize = 0, vFileStart = 0, vFileFree = 0;
int vFile_ID;
int inodeFree = FILES_NUM;
typedef struct vfsinode
{
  char fileName[MAX_FILE_NAME];
  unsigned short fileSize;
  unsigned short fileBegin; 
  unsigned char exist;
} inode;

typedef struct node
{
  inode data;
  struct node* next;
} listNode;

listNode* listHead = NULL;
listNode* actINode = NULL;
unsigned long actPos = 0;

int getLine (char *linia, int max_dl);
long isEnoughSpace(long size);
void diskStat(void);
listNode* prevInode(long end);
int writeToDisk(int src, char*dest, long size);
void readFromDisk(void);
listNode* getListNode(char* name);
void clearDisk(void);
int closeDisk(void);
int openDisk(void);
void deleteInode(char* del);
void addInode(listNode* new);
int clearInodeList(void);
void deleteInodeList(void);
int setInodeList(void);
long usedDisk(void);
void readInodeList(void);
int openVFS(void);
int isUnique(char* name);
void copyToDisk(void);
void help(void);
int createDisk(void);
int userMenu(void);
int test01(void);
int test02(void);
int copyToDiskTest(int i, int testNum);
char* concat(const char *s1, const char *s2);


int main(int argc, char** argv)
{
  if(argc != 3) 
  {
    printf("Incorrect arguments!");
    return 0;
  }

  vFileName = argv[1];
  vFileSize = atoi(argv[2]);

  if(vFileSize != 0)
  {   
    createDisk();
  }
  else openVFS(); 

  if(strcmp(vFileName, "testDisk1") == 0)
  {
    test01();
  }
  else if(strcmp(vFileName, "testDisk2") == 0)
  {
    test02();
  }
  else
  {
    userMenu();
  }
  
  return 0;
}

int test01(void)
{
  int iter = 0;
  while(copyToDiskTest(iter, 1) != 1)
  {
    iter++;
  }
  diskStat();
  userMenu();
}

int test02(void)
{
  int iter = 0;
  while(copyToDiskTest(iter, 2) != 1)
  {
    iter++;
  }
  diskStat();
  userMenu();
}

char* concat(const char *s1, const char *s2)
{
    char *result = malloc(strlen(s1) + strlen(s2) + 1);
    strcpy(result, s1);
    strcat(result, s2);
    return result;
}

int copyToDiskTest(int i, int testNum) 
{
  unsigned long size;
  int fileID;
  struct stat stbuf;
  char str[12];
  char str2[12];
  char* name;
  char* remote;
  
  if(inodeFree == 0)
  {
    fprintf(stderr, "copyToDisk(): Lack of inode's.\n");
    return 1;
  }
  sprintf(str2, "%d", testNum);
  remote = concat("test", str2);
  sprintf(str, "%d", i);
  name = concat("test", str);

  if(!isUnique(name))
  {
    fprintf(stderr, "copyToDisk(): Name taken!\n");
    return 1;
  }
  stat(remote, &stbuf);
  if((fileID = open(remote, O_RDONLY)) < 0 || (stbuf.st_mode & S_IFDIR)) 
  {
    fprintf(stderr, "copyToDisk(): No such file!\n");
    return 1;
  }
  size = stbuf.st_size;
  if(writeToDisk(fileID, name, size) == 1)
  {
    return 1;
  }
}

int userMenu(void)
{
  char   c,d;
  int i;
  
  printf("\nh - help\n");
  while(1) 
  {
    printf("> ");
    fflush(stdout);
    for(i=0, c='\0'; (d=getchar())!='\n'; i++, c=d)
      if (i>1) continue;
      switch(c)
      {
        case 'c':
          copyToDisk();
          break;
        case 'v':
          readFromDisk();
          break;
        case 'q':
          closeDisk();
          return 0;
        case 's':
          diskStat();
          break;
        case 'h':
          help();
          break;
        default:
          printf("h - help\n");
          break;
      }
  }
  return 1;
}

void help(void)
{
  printf("  c - copy to disk\n");
  printf("  v - copy from disk\n");
  printf("  s - disk stat\n");
  printf("  h - help\n");
  printf("  q - quit\n");
}

int getLine (char *linia, int max_dl)  
{
  char c;
  int i;
  
  for (i=0; (c=getchar())!=EOF && c!='\n' &&  i<=max_dl; i++) *linia++=c;
  *linia='\0';
  return i;
}

long isEnoughSpace(long size)
{
  listNode* temp1;

  if(vFileFree < size)
  {
    return -1;
  }

  if(listHead == NULL)
  {
    if((vFileSize - vFileStart) > size) return vFileStart;
    else return -1;
  }
  if((listHead->data.fileBegin - vFileStart) >= size)
  {
    return vFileStart;
  }
  
  for(temp1 = listHead; temp1->next != NULL; temp1 = temp1->next)
    if((temp1->next->data.fileBegin - (temp1->data.fileBegin + temp1->data.fileSize)) >= size)
    {
      return (temp1->data.fileBegin + temp1->data.fileSize);
    } 
      
  if((vFileSize - temp1->data.fileBegin + temp1->data.fileSize) >= size)
  {
    return (temp1->data.fileBegin + temp1->data.fileSize);
  }

  return -1;
  
}

void diskStat(void)
{
  listNode* temp1;
  if(listHead == NULL) printf("Disk empty!\n");
  temp1 = listHead; 

   printf("\nFree space: %ld\nDisk space: %ld\n", vFileFree, vFileSize);

  while(temp1 != NULL)
  { 
    printf("\nname: %s, size: %d, begin: %d, end: %d", temp1->data.fileName, temp1->data.fileSize, temp1->data.fileBegin, (temp1->data.fileBegin + temp1->data.fileSize));
    temp1 = temp1->next;
  }
}

listNode* prevInode(long end)
{
  listNode* temp1;
  
  if((listHead != NULL)&&(listHead->next == NULL)) return listHead;
  for (temp1 = listHead; temp1 != NULL; temp1 = temp1->next) if((temp1->data.fileBegin + temp1->data.fileSize) == end) return temp1;
  return NULL;
}


int writeToDisk(int src, char*dest, long size)
{
  char buf[BUF_SIZE];
  long begin;
  listNode* new, *prev;
  int count;
  
  begin = isEnoughSpace(size);
  if(begin < 0)
  {
    printf("\nwrite_to_vfile(): Not enough space!\n");
    return 1;
  }
  new =(listNode*)malloc(sizeof(listNode));
  strncpy(new->data.fileName, dest, MAX_FILE_NAME-1);
  new->data.fileSize  = size;
  new->data.fileBegin = begin;
  new->data.exist = 1;
  prev = prevInode(begin);
  if(prev == NULL)
  {
    if(listHead == NULL)
    {
      listHead = new;
      new->next = NULL;
    }
    else
    {
      new->next = listHead;
      listHead = new;
    }
  }
  else 
  {
    new->next = prev->next;
    prev->next = new;
  }
  lseek(vFile_ID, new->data.fileBegin, 0);
  count=BUF_SIZE;
  while (size > 0)
  {
    if (size < count) count = size;
    read(src, buf, count);
    write(vFile_ID, buf, count);
    size -= count;
  }
  inodeFree--;
  vFileFree -= new->data.fileSize;
  return 0;
}

void readFromDisk(void)
{
  listNode* temp1;
  int fileID;
  int count;
  long size;
  char buf[BUF_SIZE];
  char name[MAX_FILE_NAME];
  char remote[256];
  
  printf("Copy from: ");
  getLine(name, MAX_FILE_NAME);
  printf("Copy to: ");
  getLine(remote, 10000);
  if((temp1 = getListNode(name)) == NULL)
  {
    fprintf(stderr, "readFromDisk(): File doesnt exist!\n");
    return;
  }
  if((fileID = creat(remote, 0666)) < 0)
  {
    fprintf(stderr, "readFromDisk(): Unable to create!\n");
    return;
  } 
  lseek(vFile_ID, temp1->data.fileBegin, 0);
  count=BUF_SIZE;
  size = temp1->data.fileSize;
  while(size > 0) 
  {
    if(size < count) count = size;
    read(vFile_ID, buf, count); 
    write(fileID, buf, count);
    size -= count;
  }
  close(fileID);
  return;
}

listNode* getListNode(char* name)
{
  listNode* temp1;
  for(temp1 = listHead; temp1 != NULL; temp1 = temp1->next) if(strcmp(name, temp1->data.fileName) == 0) break;
  return temp1;
}

void clearDisk(void)
{
  unsigned int i;
  int buf[BUF_SIZE];
  int count;
  
  for(i = 0;i < BUF_SIZE;i++) buf[i] = '\0';
  i = vFileSize;
  count = BUF_SIZE;
  while(i > 0)
  {
    if(i < count) count = i;
    write(vFile_ID,buf,count);
    i -= count;
  }
}

int createDisk(void)
{
  if((sizeof(inode)*FILES_NUM) > vFileSize)
  {
    fprintf(stderr,"Disk to small!");
    exit(0);
  }
  
  vFile_ID = creat(vFileName,0666);
  close(vFileName);
  if((vFile_ID = open(vFileName,O_RDWR)) < 0)
  {
    fprintf(stderr,"Cant open disk file!");
    exit(0);
  }
  clearDisk();
  vFileStart = sizeof(inode)*FILES_NUM;
  vFileFree = vFileSize - vFileStart;
  return 1;
  
}

int closeDisk(void)
{
  setInodeList();
  close(vFile_ID);
}

int openDisk(void)
{
  struct stat vFileStat;
  if((vFile_ID = open(vFileName,O_RDWR)) < 0)
  {
    fprintf(stderr,"openVFS(): Cant open disk file!\n");
    exit(0);
  }
  stat(vFileName,&vFileStat);
  vFileSize = vFileStat.st_size;
  return 1;
}

void deleteInode(char* del)
{
  listNode* temp1, *temp2;
  if(listHead == NULL) return;
  for(temp1 = listHead; temp1 != NULL; temp2 = temp1, temp1 = temp1->next) if(strcmp(temp1->data.fileName, del) == 0) break;
  if(listHead == temp1) listHead = temp1->next;
  else temp2->next = temp1->next;
  vFileFree += temp1->data.fileSize;
  inodeFree++;
  free(temp1);
}

void addInode(listNode* new)
{
  listNode* temp;
  new->next = NULL;
  if(listHead == NULL) listHead = new;
  else
  {
    for(temp = listHead; temp->next != NULL;temp = temp->next);
    temp->next = new;
  }
  inodeFree--;
}

int clearInodeList(void)
{
  unsigned long count;
  char* temp;
  int i, retVal = 1;
  
  count = sizeof(inode)*FILES_NUM;
  temp = (char*)malloc(count);
  for(i = 0; i < count; i++) temp[i] = '\0';
  lseek(vFile_ID, 0, 0);
  if(write(vFile_ID, temp, count) < 0)
  {
    fprintf(stderr,"clearInodeList(): Error!");
    retVal = 0;
  }
  free(temp);
  return retVal;
}

void deleteInodeList(void)
{
  listNode* temp1;
  while(listHead != NULL)
  {
    temp1 =listHead;
    listHead = listHead->next;
    free(temp1);
  } 
}

int setInodeList(void)
{
  char* temp1;
  listNode* temp2;
  
  if(!clearInodeList()) return 0;
  lseek(vFile_ID, 0, 0);
  temp2 = listHead;
  while(temp2 != NULL)
  {
    temp1 = (char*)&(temp2->data);
    if(write(vFile_ID, temp1, sizeof(inode)) < 0) return 0;
    temp2 = temp2->next;
  }
  return 1;
}

long usedDisk(void)
{
  listNode* temp1;
  long sizeAll = 0;
  
  for(temp1 = listHead; temp1 != NULL; temp1 = temp1->next) sizeAll += temp1->data.fileSize;
  return sizeAll;
}

void readInodeList(void)
{
  char* temp1;
  inode* temp2;
  listNode* temp3;
  int i;
  
  temp1 = (char*)malloc(sizeof(inode));
  lseek(vFile_ID, 0, 0);
  for(i = 0; i < FILES_NUM; i++)
  {
    if(read(vFile_ID, temp1, sizeof(inode)) < 0) 
    {
      fprintf(stderr,"readInodeList(): Cant read form file!");
      free(temp1);
      deleteInodeList();
      exit(0);
    }
    temp2 = (inode*)temp1;
    if(temp2->exist) 
    {
      temp3 = (listNode*)malloc(sizeof(listNode));
      strncpy(temp3->data.fileName, temp2->fileName, MAX_FILE_NAME);
      temp3->data.fileSize = temp2->fileSize;
      temp3->data.fileBegin = temp2->fileBegin;
      temp3->data.exist = temp2->exist;
      addInode(temp3);
    }
  }
  vFileStart = sizeof(inode)*FILES_NUM;
  vFileFree = vFileSize - vFileStart - usedDisk();
  free(temp1);
}

int openVFS(void)
{
  int errFlag = 0;
  if(errFlag = openDisk()) readInodeList();
  return errFlag;
}

int isUnique(char* name) 
{
  listNode* temp1;
  
  for(temp1 = listHead; temp1 != NULL; temp1 = temp1->next) if (strcmp(temp1->data.fileName, name) == 0) return 0;
  return 1;
}

void copyToDisk(void) 
{
  char remote[100];
  char name[MAX_FILE_NAME];
  unsigned long size;
  int fileID;
  struct stat stbuf;
  
  if(inodeFree == 0)
  {
    fprintf(stderr, "copyToDisk(): Lack of inode's.\n");
    return;
  }
  printf("Copy from:\t");
  getLine(remote, 100);
  printf("Copy to:\t");
  getLine(name, MAX_FILE_NAME);
  if(!isUnique(name))
  {
    fprintf(stderr, "copyToDisk(): Name already taken!\n");
    return;
  }
  stat(remote, &stbuf);
  if((fileID = open(remote, O_RDONLY)) < 0 || (stbuf.st_mode & S_IFDIR)) 
  {
    fprintf(stderr, "copyToDisk(): File doesnt exist!\n");
    return;
  }
  size = stbuf.st_size;
  writeToDisk(fileID, name, size);
}

