#include <iostream>
#include <vector>
#include <cstring>

using namespace std;


extern "C"
{
void print_in_red(char *str, int length);   //打印
void print_in_default(char *str, int length); //打印
}

#pragma pack(1) // 修改对齐方式

/*
void print_in_red(char *str, int length) {
    char temp;
    cout << "\033[31m";
    for (int i = 0; i < length; ++i) {
        temp = *(str + i);
        if (temp == '\0') {
            break;
        }
        cout << temp;
    }
    cout << "\033[0m";
}

void print_in_default(char *str, int length) {
    char temp;
    for (int i = 0; i < length; ++i) {
        temp = *(str + i);
        if (temp == '\0') {
            break;
        }
        cout << temp;
    }
}
*/

/*DEFINES*/

typedef unsigned char b1;   //1字节
typedef unsigned short b2; //2字节
typedef unsigned int b4;   //4字节

/*STRUCTS*/

struct BPB {
    b2 BPB_BytsPerSec; // 每扇区字节数
    b1 BPB_SecPerClus;  // 每簇扇区数
    b2 BPB_RsvdSecCnt; // Boot记录占用的扇区数
    b1 BPB_NumFATs;     // FAT表个数
    b2 BPB_RootEntCnt; // 根目录最大文件数
    b2 BPB_TotSec16;   // 总扇区数
    b1 BPB_Media;   // 扇区描述符
    b2 BPB_FATSz16;   // FAT占用的扇区数
    b2 BPB_SecPerTrk;   // 每磁道扇区数
    b2 BPB_NumHeads;    // 磁头数
    b4 BPB_HiddSec;     // 隐藏扇区数
    b4 BPB_TotSec32;    //如果BPB_TotSec16为0，由这个值记录扇区数量
};

struct DirEntry {
    char DIR_Name[11];
    b1 DIR_Attr;
//    b1 DIR_NTRes;
//    b1 DIR_CrtTimeTenth;
//    b2 DIR_CrtTime;
//    b2 DIR_CrtDate;
//    b2 DIR_LstAccDate;
//    b2 DIR_FstClusHI;
    char REMAINED_Bits[10];
    b2 DIR_WrtTime;
    b2 DIR_WrtDate;
    b2 DIR_FstClusLO; // 需要-2，然后就是相对于数据开始扇区号的相对偏移量了, 但这里直接对应的fat1中的簇的相对于fat1开始扇区的偏移量
    b4 DIR_FileSize;
};

/*CONSTANTS*/

const static char imgPath[] = "./a.img"; //后期path要修改 因为程序在cmake-build-debug里面

/*POINTERS*/

FILE *fat12;
BPB *bpb;

/*PARAMETERS*/

// Fat1区的起始扇区号
int FatStartSector;
// 根目录区的起始扇区号
int RootDirStartSector;
// 根目录区的扇区数量，根目录区从19扇区开始，每个扇区512个字节
int RootDirSectors;
// 数据区的起始扇区号
int DataStartSector;

/*FLAGS*/
// 判断是否img文件读取失败
bool wrongImgPath = false;

/*FUNCTION DECLARATIONS*/

// 初始化全局指针
void init();

// 打印字符串，调用asm中的方法
void print(string str);

// 读取img文件
void readImg(const char *filename);

// 读取fat12文件中的内容到target内
void readFILE(int offset, int length, void *target);

// 读取BPB到定义的结构体
void readBPB();

// 设置参数
void setFatStartSector();

void setRootDirStartSector();

void setRootDirSectors();

void setDataStartSector();

// 获取用户命令
void getCommands();

// 将字符串变为大写
void upperStr(string &path);

// 根据FatEntry号读取对应的12位内容
int getFatValue(int FatEntryNum);

// 将dirEntry中的Dir_Name格式化到dirName中，如果有非法字符返回false
bool formatDirName(DirEntry *dirEntry, char *dirName);

// list根目录
void listRootDir(bool inDetail);

// list子文件夹，path为该文件夹的path
void listSubDir(char *path, DirEntry *dirEntry, bool inDetail);

// 得到根目录的详细信息
string getRootDetail();

// 得到子文件夹的详细信息
string getSubDirDetail(DirEntry *dirEntry);

// 根据path找到对应文件/文件夹
DirEntry *findFile(string &path);

// 根据name，在dirEntry对应的子文件夹下找到对应的文件/文件夹
DirEntry *findFileByName(string &targetName, DirEntry *dirEntry);

// 打印文件内容
void catFile(DirEntry *dirEntry);

// 将string分割
vector<string> strSplit(const string &str, const string &delim);

/*MAIN*/
int main() {
    init();
    readImg(imgPath);
    if (wrongImgPath) {
        print("WRONG IMAGE PATH OR FILE!\n");
        return 0;
    }
    getCommands();
}

/*FUNCTION DEFINITIONS*/
void init() {
    bpb = new BPB();
}

void print(string str) {
    print_in_default(&str.at(0), str.length());
}

void readFILE(int offset, int length, void *target) {
    fseek(fat12, offset, SEEK_SET);
    fread(target, 1, length, fat12);
}

void readImg(const char *filename) {
    fat12 = fopen(imgPath, "rb");
    if (fat12 == nullptr) {
        //printf("fail to open. errno = %d reason = %s \n", errno, strerror(errno));
        wrongImgPath = true;
        return;
    }
    readBPB();
    setFatStartSector();
    setRootDirStartSector();
    setRootDirSectors();
    setDataStartSector();
}

void readBPB() {
    readFILE(11, 25, bpb);
}

void setFatStartSector() {
    FatStartSector = bpb->BPB_RsvdSecCnt;
}

void setRootDirStartSector() {
    RootDirStartSector = bpb->BPB_RsvdSecCnt + bpb->BPB_NumFATs * bpb->BPB_FATSz16;
}

void setRootDirSectors() {
    RootDirSectors = ((bpb->BPB_RootEntCnt * 32) + (bpb->BPB_BytsPerSec - 1)) / bpb->BPB_BytsPerSec;
}

void setDataStartSector() {
    DataStartSector = RootDirStartSector + RootDirSectors;
}

void getCommands() {
    while (true) {
        print(">");
        string command;
        getline(cin, command);
        if (command == "exit") {
            break;
        }
        vector<string> splitCommand = strSplit(command, " ");
        if (splitCommand[0] == "ls") {
            bool hasL = false;
            bool hasFilePath = false;
            bool wrongCommand = false;
            string filePath;
            for (int i = 0; i < splitCommand.size(); ++i) {
                if (splitCommand[i] == "ls") {
                    continue;
                } else if (splitCommand[i] == "-l" || splitCommand[i] == "-ll") {
                    hasL = true;
                } else if(splitCommand[i][0] == '-'){
                    print("ls: illegal option "+splitCommand[i]+'\n');
                    wrongCommand = true;
                    break;
                } else {
                    if (!hasFilePath) {
                        hasFilePath = true;
                        filePath = splitCommand[i];
                    } else {
                        print("ls: too many arguments\n");
                        wrongCommand = true;
                        break;
                    }
                }
            }
            if (wrongCommand) {
                continue;
            }
            if (!hasFilePath) {
                listRootDir(hasL);
                continue;
            } else {
                DirEntry *dirEntry = findFile(filePath);
                if (dirEntry == nullptr){
                    continue;
                }
                listSubDir(&filePath[0], dirEntry, hasL);
            }
        } else if (splitCommand[0] == "cat") {
            DirEntry *dirEntry = findFile(splitCommand[1]);
            if (dirEntry == nullptr){
                continue;
            }
            if (dirEntry->DIR_Attr == 0x10){
                print(splitCommand[1]+"： Is a directory\n");
            } else{
                catFile(dirEntry);
            }
        } else {
            print("command not found: " + splitCommand[0] + "\n");
        }
    }
}

vector<string> strSplit(const string &str, const string &delim) {
    vector<string> res;
    if ("" == str) return res;
    //先将要切割的字符串从string类型转换为char*类型
    char *strs = new char[str.length() + 1]; //不要忘了
    strcpy(strs, str.c_str());

    char *d = new char[delim.length() + 1];
    strcpy(d, delim.c_str());

    char *p = strtok(strs, d);
    while (p) {
        string s = p; //分割得到的字符串转换为string类型
        res.push_back(s); //存入结果数组
        p = strtok(nullptr, d);
    }
    return res;
}

int getFatValue(int FatEntryNum) {
    //FAT项的偏移字节
    int fatPos = FatStartSector * bpb->BPB_BytsPerSec + FatEntryNum * 3 / 2;
    // 先读出FAT项开始的16位，2*8
    b2 bytes;
    b2 *bytes_ptr = &bytes;
    readFILE(fatPos, 2, bytes_ptr);
    return (FatEntryNum & 1) ? (bytes >> 4) : (bytes & ((1 << 12) - 1));
}

void upperStr(string &path) {
    for (int i = 0; i < path.size(); i++) {
        if (path[i] >= 'a' && path[i] <= 'z') {
            path[i] -= 32;
        }
    }

}

bool formatDirName(DirEntry *dirEntry, char *dirName) {
    int i;
    bool wrongName = false;
    for (i = 0; i < 11; i++) {
        if (!(((dirEntry->DIR_Name[i] >= 48) && (dirEntry->DIR_Name[i] <= 57)) ||
              ((dirEntry->DIR_Name[i] >= 65) && (dirEntry->DIR_Name[i] <= 90)) ||
              ((dirEntry->DIR_Name[i] >= 97) && (dirEntry->DIR_Name[i] <= 122)) ||
              (dirEntry->DIR_Name[i] == 32) || dirEntry->DIR_Name[i] == 46)) {
            wrongName = true;    //非英文、数字、空格、点
            break;
        }
    }
    if (wrongName) {
        return false;
    }
    int tempIndex = 0;
    for (i = 0; i < 11; i++) {
        if (dirEntry->DIR_Name[i] != ' ') {
            dirName[tempIndex] = dirEntry->DIR_Name[i];
            tempIndex++;
        } else {
            if (dirEntry->DIR_Attr == 0x10) {
                break;
            }
            dirName[tempIndex] = '.';
            tempIndex++;
            while (dirEntry->DIR_Name[i] == ' ') i++;
            i--;
        }
    }
    dirName[tempIndex] = '\0';
    return true;
}

DirEntry *findFile(string &path) {
    DirEntry *dirEntry = new DirEntry();
    vector<string> splitPath = strSplit(path, "/");
    string name = splitPath.at(0);
    bool fileExist = false;
    // 直接先遍历根目录
    for (int i = 0; i < bpb->BPB_RootEntCnt; ++i) {
        char fileName[128];
        readFILE(RootDirStartSector * bpb->BPB_BytsPerSec * bpb->BPB_SecPerClus + i * 32, 32, dirEntry);
        bool nameCorrect = formatDirName(dirEntry, fileName);
        if (!nameCorrect) {
            continue;
        }
        if (fileName == name) {
            fileExist = true;
            break;
        }
    }
    // 第一层就没有在根目录中找到
    if (!fileExist) {
        print(path + ": No such file or directory\n");
        return nullptr;
    }
    // 迭代找到目标文件/文件夹，中途有一个nullptr
    for (int i = 1; i < splitPath.size(); ++i) {
        dirEntry = findFileByName(splitPath[i], dirEntry);
    }
    if (dirEntry == nullptr) {
        print(path + ": No such file or directory\n");
        return nullptr;
    }
    return dirEntry;
}

DirEntry *findFileByName(string &targetName, DirEntry *dirEntry) {
    if (dirEntry == nullptr) {
        return nullptr;
    }
    int currentFatEntryNum = dirEntry->DIR_FstClusLO;
    int FatValue = 0;
    while (FatValue < 0xFF8) {
        FatValue = getFatValue(currentFatEntryNum);
        if (FatValue == 0xFF7) {
            print("Bad FATEntry!\n");
            break;
        }
        // 对应的数据区的扇区号
        int startSector = DataStartSector + (currentFatEntryNum - 2);
        int count = bpb->BPB_SecPerClus * bpb->BPB_BytsPerSec;
        int loop = 0;
        DirEntry *subDirEntry = new DirEntry();
        while (loop < count) {
            readFILE(startSector * count + loop, 32, subDirEntry);
            if (subDirEntry->DIR_Name[0] == '\0') {
                break;
            }
            char subFileName[12];
            bool nameCorrect = formatDirName(subDirEntry, subFileName);
            if (!nameCorrect) {
                loop += 32;
                continue;
            }
            loop += 32;
            if (subFileName == targetName) {
                return subDirEntry;
            }
        }
        currentFatEntryNum = FatValue;
    }
    // while结束没有找到，那肯定是不存在
    return nullptr;
}

void listRootDir(bool inDetail) {
    // 先打印根目录
    print("/");
    if (inDetail) {
        print(" " + getRootDetail());
    }
    print(":\n");
    // 直接先遍历根目录
    vector<DirEntry *> subDirs;
    for (int i = 0; i < bpb->BPB_RootEntCnt; ++i) {
        char fileName[12];
        // 指向新的堆空间
        DirEntry *dirEntry = new DirEntry();
        readFILE(RootDirStartSector * bpb->BPB_BytsPerSec * bpb->BPB_SecPerClus + i * 32, 32, dirEntry);
        bool nameCorrect = formatDirName(dirEntry, fileName);
        if (!nameCorrect) {
            delete dirEntry;
            continue;
        }
        if (dirEntry->DIR_Attr == 0x10) {
            subDirs.push_back(dirEntry);
            print_in_red(fileName, strlen(fileName));
            print(" ");
            if (inDetail) {
                print(getSubDirDetail(dirEntry) + "\n");
            }
        } else if (dirEntry->DIR_Attr == 0x20||dirEntry->DIR_Attr == 0x00) {
            print_in_default(fileName, strlen(fileName));
            print(" ");
            if (inDetail) {
                print(to_string(dirEntry->DIR_FileSize) + "\n");
            }
            delete dirEntry;
        } else {
            delete dirEntry;
        }
    }
    print("\n");
    char dirPath[128] = "/";
    for (int i = 0; i < subDirs.size(); ++i) {
        formatDirName(subDirs.at(i), dirPath + 1);
        listSubDir(dirPath, subDirs.at(i), inDetail);
        // 重新赋值
        strcpy(dirPath, "/");
    }
}

void listSubDir(char *path, DirEntry *dirEntry, bool inDetail) {
    // 如果需要ls不是文件夹，直接打印他的path
    if (dirEntry->DIR_Attr != 0x10) {
        print_in_default(path, strlen(path));
        print("\n");
        return;
    }
    // 先打印这个文件夹的path
    char dirPath[128];
    int tempLength = strlen(path);
    strcpy(dirPath, path);
    dirPath[tempLength] = '/';
    tempLength++;
    dirPath[tempLength] = '\0';
    print_in_default(dirPath, tempLength);
    if (inDetail) {
        print(" " + getSubDirDetail(dirEntry));
    }
    print(":\n");
    // 用来存放文件夹中的子文件夹
    vector<DirEntry *> subDirs;
    // 遍历文件夹
    int currentFatEntryNum = dirEntry->DIR_FstClusLO;
    int FatValue = 0;
    // 记录访问这个文件夹已经访问了多少fatEntry
    int visitedFatEntries = 0;
    while (FatValue < 0xFF8) {
        visitedFatEntries++;
        FatValue = getFatValue(currentFatEntryNum);
        if (FatValue == 0xFF7) {
            print("Bad FATEntry!\n");
            break;
        }
        // 对应的数据区的扇区号
        int startSector = DataStartSector + (currentFatEntryNum - 2);
        int count = bpb->BPB_SecPerClus * bpb->BPB_BytsPerSec;
        int loop = 0;
        while (loop < count) {
            DirEntry *subDirEntry = new DirEntry();
            // 存放子文件的文件名
            char subFileName[12];
            // 只有每个文件夹下面的第一个文件前方不用打印空格，visitedFatEntries是针对一个文件夹的fatEntry占据多个
            readFILE(startSector * count + loop, 32, subDirEntry);
            if (subDirEntry->DIR_Name[0] == '\0') {
                break;
            }
            bool nameCorrect = formatDirName(subDirEntry, subFileName);
            if (!nameCorrect) {
                delete subDirEntry;
                loop += 32;
                continue;
            }
            // 如果是文件夹，在遍历结束后将递归对他们进行遍历，保存他们的名字和DirEntry
            if (subDirEntry->DIR_Attr == 0x10 && subFileName[0] != '.') {
                subDirs.push_back(subDirEntry);
                print_in_red(subFileName, strlen(subFileName));
                print(" ");
                if (inDetail) {
                    print(getSubDirDetail(subDirEntry) + "\n");
                }
            } else if (subDirEntry->DIR_Attr == 0x10 && subFileName[0] == '.') {
                print_in_red(subFileName, strlen(subFileName));
                print(" ");
                if (inDetail) {
                    print("\n");
                }
                delete subDirEntry;
            } else if (subDirEntry->DIR_Attr == 0x20 || subDirEntry->DIR_Attr == 0x00) {
                print_in_default(subFileName, strlen(subFileName));
                print(" ");
                if (inDetail) {
                    print(to_string(subDirEntry->DIR_FileSize) + "\n");
                }
                delete subDirEntry;
            } else {
                delete subDirEntry;
            }
            loop += 32;
        }
        // 指向下一个扇区的号码
        currentFatEntryNum = FatValue;
    }
    // 打印一个空行，开始遍历子文件夹
    print("\n");
    for (int i = 0; i < subDirs.size(); ++i) {
        char subDirPath[128];
        strcpy(subDirPath, dirPath);
        char subDirName[12];
        formatDirName(subDirs[i], subDirName);
        // 子文件夹的父路径+子文件夹的名字就是子文件夹的路径
        strcat(subDirPath, subDirName);
        // 开始list子文件夹
        listSubDir(subDirPath, subDirs[i], inDetail);
        // 遍历完删除堆上的subDirEntry
        delete subDirs[i];
    }
    // 清空这个扇区的子文件夹信息
    subDirs.clear();
}

string getRootDetail() {
    int subArchives = 0;
    int subDirs = 0;
    for (int i = 0; i < bpb->BPB_RootEntCnt; ++i) {
        char fileName[12];
        // 指向新的堆空间
        DirEntry *dirEntry = new DirEntry();
        readFILE(RootDirStartSector * bpb->BPB_BytsPerSec * bpb->BPB_SecPerClus + i * 32, 32, dirEntry);
        bool nameCorrect = formatDirName(dirEntry, fileName);
        if (!nameCorrect) {
            delete dirEntry;
            continue;
        }
        if (dirEntry->DIR_Attr == 0x10) {
            subDirs++;
        }
        if (dirEntry->DIR_Attr == 0x20||dirEntry->DIR_Attr == 0x00) {
            subArchives++;
        }
        delete dirEntry;
    }
    string res = to_string(subDirs) + " " + to_string(subArchives);
    return res;
}

string getSubDirDetail(DirEntry *dirEntry) {
    int subArchives = 0;
    int subDirs = 0;
    int currentFatEntryNum = dirEntry->DIR_FstClusLO;
    int FatValue = 0;
    int visitedFatEntries = 0;
    while (FatValue < 0xFF8) {
        visitedFatEntries++;
        FatValue = getFatValue(currentFatEntryNum);
        if (FatValue == 0xFF7) {
            print("Bad FATEntry!\n");
            break;
        }
        // 对应的数据区的扇区号
        int startSector = DataStartSector + (currentFatEntryNum - 2);
        int count = bpb->BPB_SecPerClus * bpb->BPB_BytsPerSec;
        int loop = 0;
        while (loop < count) {
            DirEntry *subDirEntry = new DirEntry();
            char subFileName[12];
            readFILE(startSector * count + loop, 32, subDirEntry);
            if (subDirEntry->DIR_Name[0] == '\0') {
                break;
            }
            // 不规范名称的文件夹不计数
            bool nameCorrect = formatDirName(subDirEntry, subFileName);
            if (!nameCorrect) {
                delete subDirEntry;
                loop += 32;
                continue;
            }
            if (subDirEntry->DIR_Attr == 0x10 && subFileName[0] != '.') {
                subDirs++;
            }
            if (subDirEntry->DIR_Attr == 0x20 || subDirEntry->DIR_Attr == 0x00) {
                subArchives++;
            }
            delete subDirEntry;
            loop += 32;
        }
        // 指向下一个扇区的号码
        currentFatEntryNum = FatValue;
    }
    string res = to_string(subDirs) + " " + to_string(subArchives);
    return res;
}

void catFile(DirEntry *dirEntry) {
    if (dirEntry == nullptr) {
        return;
    }
    int currentFatEntryNum = dirEntry->DIR_FstClusLO;
    int FatValue = 0;
    int sectorCount = bpb->BPB_SecPerClus * bpb->BPB_BytsPerSec;
    char content[sectorCount];
    while (FatValue < 0xFF8) {
        FatValue = getFatValue(currentFatEntryNum);
        if (FatValue == 0xFF7) {
            print("Bad FATEntry!\n");
            break;
        }
        int startSector = DataStartSector + (currentFatEntryNum - 2);
        readFILE(startSector * sectorCount, sectorCount, content);
        print_in_default(content, strlen(content));
        currentFatEntryNum = FatValue;
    }
}
