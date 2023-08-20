#include "iostream"
#include "string"
#include "vector"
#include <cstring>

#define ERR_NO_FILE "This File is not Exist!\n"
#define CANNOT_OPEN "Cannot Open This File!\n"
#define PARAM_WRONG "Parameter Cannot Be Passed!\n"
#define COMMAND_ERR "This Command is not Found!\n"
#define MAXN 10005
using namespace std;

typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;

int BytsPerSec;				//每扇区字节数
int SecPerClus;				//每簇扇区数
int RsvdSecCnt;				//Boot记录占用的扇区数
int NumFATs;				//FAT表个数
int RootEntCnt;				//根目录最大文件数
int FATSz;					//FAT扇区数
//asm
extern "C" {
void my_print(const char *, const int);
}
void myPrint(const char *s) {
    my_print(s, strlen(s));
}

//FAT1的偏移字节
int fatBase;
int fileRootBase;
int dataBase;
int BytsPerClus;
#pragma pack(1) 

class Node {
    string name;
    vector<Node*> next;
    string path;
    uint32_t FileSize;
    bool isFile = false;
    bool isVal = true;
    int dir_count = 0;
    int file_count = 0;
    char *content = new char[MAXN];
public:
    Node() = default;

    Node(string name, bool isVal){
        this->name = name;
        this->isVal = isVal;
    }

    Node(string name, string path){
        this->name = name;
        this->path = path;
    }

    Node(string name, uint32_t fileSize, bool isFile, string path){
        this->name = name;
        this->FileSize = fileSize;
        this->isFile = isFile;
        this->path = path;
    }

    void setPath(string p) {
        this->path = p;
    }

    void setName(string n) {
        this->name = n;
    }

    void addChild(Node *child){
        this->next.push_back(child);
    }

    void addFileChild(Node *child){
        this->next.push_back(child);
        this->file_count++;
    }

    void addDirChild(Node *child){
        child->addChild(new Node(".", false));
        child->addChild(new Node("..", false));
        this->next.push_back(child);
        this->dir_count++;
    }

    string getName() { return name; }

    string getPath() { return this->path; }

    char* getContent() { return content; }

    bool getIsFile() { return isFile;}

    vector<Node*> getNext() { return next; }

    bool getIsVal() { return isVal; }

    uint32_t getFileSize() { return FileSize; }
};
//BIOS Parameter Block
class BPB
{
    uint16_t BPB_BytsPerSec; //每扇区字节数
    uint8_t BPB_SecPerClus;  //每簇扇区数
    uint16_t BPB_RsvdSecCnt; //Boot记录占用的扇区数
    uint8_t BPB_NumFATs;		//FAT表个数
    uint16_t BPB_RootEntCnt; //根目录最大文件数
    uint16_t BPB_TotSec16;
    uint8_t BPB_Media;
    uint16_t BPB_FATSz16; //FAT扇区数
    uint16_t BPB_SecPerTrk;
    uint16_t BPB_NumHeads;
    uint32_t BPB_HiddSec;
    uint32_t BPB_TotSec32; //如果BPB_FATSz16为0，该值为FAT扇区数
public:
    BPB() {};

    void init(FILE *fat12){
        fseek(fat12, 11, SEEK_SET);   //BPB从偏移11个字节处开始
        fread(this, 1, 25, fat12); //BPB长度为25字节

        BytsPerSec = this->BPB_BytsPerSec; //初始化各个全局变量
        SecPerClus = this->BPB_SecPerClus;
        RsvdSecCnt = this->BPB_RsvdSecCnt;
        NumFATs = this->BPB_NumFATs;
        RootEntCnt = this->BPB_RootEntCnt;

        if (this->BPB_FATSz16 != 0){
            FATSz = this->BPB_FATSz16;
        }
        else{
            FATSz = this->BPB_TotSec32;
        }
        fatBase = RsvdSecCnt * BytsPerSec;
        fileRootBase = (RsvdSecCnt + NumFATs * FATSz) * BytsPerSec; //根目录首字节的偏移数=boot+fat1&2的总字节数
        dataBase = BytsPerSec * (RsvdSecCnt + FATSz * NumFATs + (RootEntCnt * 32 + BytsPerSec - 1) / BytsPerSec);
        BytsPerClus = SecPerClus * BytsPerSec; //每簇的字节数
    }
};
//BPB至此结束，长度25字节
//根目录条目
class RootEntry
{
public:
    char DIR_Name[11];
    uint8_t DIR_Attr; //文件属性
    char reserved[10];
    uint16_t DIR_WrtTime;
    uint16_t DIR_WrtDate;
    uint16_t DIR_FstClus; //开始簇号
    uint32_t DIR_FileSize;
    RootEntry() {};

    bool isValidNameAt(int j){
        return ((this->DIR_Name[j] >= 'a') && (this->DIR_Name[j] <= 'z'))
            ||((this->DIR_Name[j] >= 'A') && (this->DIR_Name[j] <= 'Z'))
            ||((this->DIR_Name[j] >= '0') && (this->DIR_Name[j] <= '9'))
            ||((this->DIR_Name[j] == ' '));
    }

    bool isEmptyName(){
        return this->DIR_Name[0] == '\0';
    }

    bool isInvalidName(){
        int invalid = false;
        for (int k = 0; k < 11; ++k) {
            if (!this->isValidNameAt(k)) {
                invalid = true;
                break;
            }
        }
        return invalid;
    }

    bool isFile(){
        return (this->DIR_Attr & 0x10) == 0;
    }

    void generateFileName(char name[12]){
        int tmp = -1;
        for (int j = 0; j < 11; ++j) {
            if (this->DIR_Name[j] != ' ') {
                name[++tmp] = this->DIR_Name[j];
            } else {
                name[++tmp] = '.';
                while (this->DIR_Name[j] == ' ') j++;
                j--;
            }
        }
        ++tmp;
        name[tmp] = '\0';
    }

    void generateDirName(char name[12]){
        int tmp = -1;
        for (int k = 0; k < 11; ++k) {
            if (this->DIR_Name[k] != ' ') {
                name[++tmp] = this->DIR_Name[k];
            } else {
                tmp++;
                name[tmp] = '\0';
                break;
            }
        }
    }

    uint32_t getFileSize(){ return DIR_FileSize; }

    uint16_t getFstClus() { return DIR_FstClus; }
};


vector<string> split(const string& str, const string& delim) {
    vector <string> res;
    if ("" == str) return res;
    
    char *strs = new char[str.length() + 1]; 
    strcpy(strs, str.c_str());

    char *d = new char[delim.length() + 1];
    strcpy(d, delim.c_str());

    char *p = strtok(strs, d);
    while (p) {
        string s = p; 
        res.push_back(s); 
        p = strtok(NULL, d);
    }

    return res;
}
int getFATValue(FILE *fat12, int num) {
    int base = RsvdSecCnt * BytsPerSec;
    int pos = base + num * 3 / 2;
    int type = num % 2;

    uint16_t bytes;
    uint16_t *bytesPtr = &bytes;
    fseek(fat12, pos, SEEK_SET);
    fread(bytesPtr, 1, 2, fat12);

    if (type == 0) {
        bytes = bytes << 4;
    }
    return bytes >> 4;
}
void RetrieveContent(FILE *fat12, int startClus, Node *child) {
    int base = BytsPerSec * (RsvdSecCnt + FATSz * NumFATs + (RootEntCnt * 32 + BytsPerSec - 1) / BytsPerSec);
    int currentClus = startClus;
    int value = 0;
    char *content = child->getContent();

    if (startClus == 0) return;

    while (value < 0xFF8) {
        value = getFATValue(fat12, currentClus);
        if (value == 0xFF7) {
            myPrint("Bad Cluster\n");
            break;
        }

        int size = SecPerClus * BytsPerSec;
        char *cluster = (char*)malloc(size);// new 512 space
        int startByte = base + (currentClus - 2)*SecPerClus*BytsPerSec;

        fseek(fat12, startByte, SEEK_SET);
        fread(cluster, 1, size, fat12);

        for (int i = 0; i < size; ++i) {
            *content = cluster[i];
            content++;//put into node.content
        }
        free(cluster);
        currentClus = value;
    }
}


void readChildren(FILE *fat12, int startClus, Node *root) {

    int base = BytsPerSec * (RsvdSecCnt + FATSz * NumFATs + (RootEntCnt * 32 + BytsPerSec - 1) / BytsPerSec);

    int currentClus = startClus;
    int value = 0;
    while (value < 0xFF8) {
        value = getFATValue(fat12, currentClus);
        if (value == 0xFF7) {
            myPrint("Bad Cluster\n");
            break;
        }

        int startByte = base + (currentClus - 2) * SecPerClus * BytsPerSec;

        int size = SecPerClus * BytsPerSec;
        int loop = 0;
        while (loop < size) {
            RootEntry *rootEntry = new RootEntry();
            fseek(fat12, startByte + loop, SEEK_SET);
            fread(rootEntry, 1, 32, fat12);

            loop += 32;

            if (rootEntry->isEmptyName() || rootEntry->isInvalidName()) {
                continue;
            }

            char tmpName[12];
            if ((rootEntry->isFile())) {
                rootEntry->generateFileName(tmpName);
                Node *child = new Node(tmpName, rootEntry->getFileSize(), true, root->getPath());
                root->addFileChild(child);
                RetrieveContent(fat12, rootEntry->getFstClus(), child);
            } else {
                rootEntry->generateDirName(tmpName);
                Node *child = new Node();
                child->setName(tmpName);
                child->setPath(root->getPath() + tmpName + "/");
                root->addDirChild(child);
                readChildren(fat12, rootEntry->getFstClus(), child);
            }
        }
    }
}


void initRootEntry(FILE *fat12, Node *root,RootEntry *RootEntry) {
    int base = fileRootBase;
    char realName[12];

    for (int i = 0; i < RootEntCnt; ++i) {
        fseek(fat12, base, SEEK_SET);
        fread(RootEntry, 1, 32, fat12);

        base += 32;

        if (RootEntry->isEmptyName() ||RootEntry->isInvalidName()) continue;

        if (RootEntry->isFile()) {
            RootEntry->generateFileName(realName);
            Node *child = new Node(realName, RootEntry->DIR_FileSize, true, root->getPath());
            root->addFileChild(child);
            RetrieveContent(fat12, RootEntry->DIR_FstClus, child);
        } else {
            RootEntry->generateDirName(realName);
            Node *child = new Node();
            child->setName(realName);
            child->setPath(root->getPath() + realName + "/");
            root->addDirChild(child);
            readChildren(fat12, RootEntry->getFstClus(), child);
        }
    }
}

void formatPath(string &s) {
    if (s[0] != '/') {
        s = "/" + s;
    }
}
string PathCorrection_test(string &s){
    string result;
    vector<string> tmp = split(s, "/");
    vector<string> valid;
    for(string str : tmp){
        if(str==".."){
            valid.pop_back();
        }else if(str=="."){
            continue;
        }else{
            valid.push_back(str);
        }
    }
    if(valid.size()==0){
        result = s;
    }else{
        for(string temp : valid){
            result = result+"/"+temp;
        }
    }
    return result;
}

void printCat(Node *root, string p, int &exist) {
    formatPath(p);
    if (p == root->getPath() + root->getName()) {
        if (root->getIsFile()) {//file
            exist = 1;
            if (root->getContent()[0] != 0) {
                myPrint(root->getContent());
                myPrint("\n");
            }
        } 
        else {
            exist = 2;
        }
        return;
    }
    if (p.length() <= root->getPath().length()) {
        return;
    }
    string tmp = p.substr(0, root->getPath().length());
    if (tmp == root->getPath()) {//folder recursion
        for (Node *q : root->getNext()) {
            printCat(q, p, exist);
        }
    }
}
void printLS(Node *r) {
    string str;
    Node *p = r;
    if (p->getIsFile()) {
        return;
    }
    else {
        str = p->getPath() + ":\n";
        const char *strPtr = str.c_str();
        myPrint(strPtr);
        str.clear();
        //打印每个next
        Node *q;
        int len = p->getNext().size();
        for (int i = 0; i < len; i++) {
            q = p->getNext()[i];
            if (!q->getIsFile()) {
                myPrint("\033[31m");
                myPrint(q->getName().c_str());
                myPrint("\033[0m");
                myPrint("  ");
            } 
            else {
                myPrint(q->getName().c_str());
                myPrint(" ");
            }
        }
        myPrint("\n");
        for (int i = 0; i < len; ++i) {
            if (p->getNext()[i]->getIsVal()) {
                printLS(p->getNext()[i]);
            }
        }
    }
}

void printLSL(Node *r) {
    string str;
    Node *p = r;
    if (p->getIsFile()) {
        return;
    }
    else {
        int fileNum = 0;
        int dirNum = 0;
        for (int j = 0; j < p->getNext().size(); ++j) {
            if (p->getNext()[j]->getName() == "." || p->getNext()[j]->getName() == "..") {
                continue;
            }
            if (p->getNext()[j]->getIsFile()) {
                fileNum++;
            } else {
                dirNum++;
            }
        }
        myPrint(p->getPath().c_str());
        myPrint(" ");
        myPrint(to_string(dirNum).c_str());
        myPrint(" ");
        myPrint(to_string(fileNum).c_str());
        myPrint("\n");
        str.clear();
        
        Node *q;
        int len = p->getNext().size();
        for (int i = 0; i < len; i++) {
            q = p->getNext()[i];
            if (!q->getIsFile()) {
                if (q->getName() == "." || q->getName() == "..") {
                        myPrint("\033[31m");
                        myPrint(q->getName().c_str());
                        myPrint("\033[0m");
                        myPrint(" ");
                        myPrint("\n");
                } else {
                    fileNum = 0;
                    dirNum = 0;
                    for (int j = 2; j < q->getNext().size(); ++j) {
                        if (q->getNext()[j]->getIsFile()) {
                            fileNum++;
                        } else {
                            dirNum++;
                        }
                    }
                    myPrint("\033[31m");
                    myPrint(q->getName().c_str());
                    myPrint("\033[0m");
                    myPrint(" ");
                    myPrint(to_string(dirNum).c_str());
                    myPrint(" ");
                    myPrint(to_string(fileNum).c_str());
                    myPrint("\n");
                }
            } else {
                myPrint(q->getName().c_str());
                myPrint(" ");
                myPrint(to_string(q->getFileSize()).c_str());
                myPrint("\n");
            }
        }
        myPrint("\n");
        for (int i = 0; i < len; ++i) {
            if (p->getNext()[i]->getIsVal()) {
                printLSL(p->getNext()[i]);
            }
        }
    }
}

bool isL(string &s) {
    if (s[0] != '-') {
        return false;
    }
    else {
        for (int i=1; i<s.size(); i++)
            if (s[i] != 'l') return false;
    }
    return true;
}
Node* findByName(Node *root, vector<string> dirs) {
    if (dirs.empty()) return root;
    string name = dirs[dirs.size()-1];
    for (int i=0; i<root->getNext().size(); i++) {
        if (name == root->getNext()[i]->getName()) {
            dirs.pop_back();
            return findByName(root->getNext()[i], dirs);
        }
    }
    return nullptr;
}

Node* isDir(string &s, Node *root) {
    s = PathCorrection_test(s);
    if(s.substr(0,3)=="/-l"){s = s.substr(1);}
    vector<string> tmp = split(s, "/");
    vector<string> dirs;
    while (!tmp.empty()) {
        dirs.push_back(tmp[tmp.size()-1]);
        tmp.pop_back();
    }
    return findByName(root, dirs);
}

void LS(vector<string> commands, Node* root) {
    if (commands.size() == 1) {
        printLS(root);
    } else {
        //ls -l
        bool hasL = false;
        bool hasDir = false;
        Node *toFind = root;
        for (int i=1; i<commands.size(); i++) {
            Node* newRoot = isDir(commands[i], root);
            if (isL(commands[i])) {
                hasL = true;
            } else if (!hasDir && newRoot != nullptr) {
                hasDir = true;
                toFind = newRoot;
            } else{
                myPrint(PARAM_WRONG);
                return;
            }
        }
        if (hasL) {
            printLSL(toFind);
        }else{
            printLS(toFind);
        }
    }
}
void CAT(vector<string> commands, Node* root) {
    if (commands.size() == 2 && commands[1][0] != '-') {
        int exist = 0;
        string str = PathCorrection_test(commands[1]);
        printCat(root, str, exist);
        if (exist == 0) {
            myPrint(ERR_NO_FILE);
        } else if (exist == 2) {
            myPrint(CANNOT_OPEN);
        }
    } 
    else {
        myPrint(PARAM_WRONG);
    }
}
int main()
{
    FILE *fat12;
    fat12 = fopen("./a.img", "rb"); 
    
    BPB *bpb = new BPB();
    bpb->init(fat12);
    Node *root = new Node();
    root->setName("");
    root->setPath("/"); 
    RootEntry *rootEntry = new RootEntry();
    initRootEntry(fat12, root,rootEntry);

    while (true) {
        myPrint("> ");
        string input;
        getline(cin, input);
        vector<string> commands = split(input, " ");
        if (commands[0] == "exit") {
            myPrint("EXIT\n");
            fclose(fat12);
            break;
        }  
        else if (commands[0] == "ls") {
            LS(commands, root);
        } 
        else if (commands[0] == "cat") {
            CAT(commands, root);
        } 
        else {
            myPrint(COMMAND_ERR);
        }
    }

}
