#include <fstream>
#include <iostream>
#include <ctime>
#include <cstring>
#include <conio.h>
using namespace std;
int MAX_USER = 20;
int UserCount = 0;
int TotalPBN = 0;
#define FILE_NAME_LENGTH 100
#define MAX_ITEM_IN_DIRECTORY 100
#define COMMAND_LENGTH 100
bool hasRoot = false;
//启动系统时，加载最基本的信息；最大的用户个数，已经注册的用户的数量，一共使用了多少块物理块
void Initialize()
{
    ifstream inFile("init", ifstream::binary);
    if (!inFile)
        return;
    inFile.read((char*)&MAX_USER, sizeof(int));
    inFile.read((char*)&UserCount, sizeof(int));
    inFile.read((char*)&TotalPBN, sizeof(int));
    inFile.read((char*)&hasRoot, sizeof(bool));
    inFile.close();
}
//系统关闭时，将这些信息写入文件中，为下次启动准备
void Exit()
{
    ofstream outFile("init", ofstream::binary);
    outFile.write((char*)&MAX_USER, sizeof(int));
    outFile.write((char*)&UserCount, sizeof(int));
    outFile.write((char*)&TotalPBN, sizeof(int));
    outFile.write((char*)&hasRoot, sizeof(bool));
    outFile.close();
}

//索引类
class Index
{
public:
    char FileName[FILE_NAME_LENGTH] = {0};
    char Extension[10] = { 0 };
    int Right = 000; //共享时，100,不可读不可写，101，不可读可写，110，可读不可写，111，可读可写；0XX，对于创建者，可读可写
    int PBN = 0; //Physic Block Number,物理块号
    char Creator[30] = { 0 };
    int ParentIndexBlock = -1;//如果该文件是目录文件的话，保存的就是父目录的物理块号
    //int Count = 0;//指向该索引的连接数，用于实现文件共享
    //int NextIndex = -1;//如果用户增添内容，分配新的物理盘，下一个盘的物理位置
    //int NextIndexNumber = -1;//同上
    int Status = 1;//1，表示不删除；0，表示删除，不在写入磁盘中
    int Type = -1;
    time_t ModifyTime;//最近被修改的时间
    time_t CreateTime;//创建的时间
public :
    //0,创建时，更新两个时间；1，修改时，只更新修改时间
    void UpdateTime(int option)
    {
        if (option == 0)
        {
            time(&ModifyTime);
            time(&CreateTime);
        }
        else if (option == 1)
            time(&this->ModifyTime);
    }
    //将索引文件从指定的物理块号中读取出来，并返回索引的个数
    //索引所在的物理块号，索引对象数组的大小，指针
    static int Read(int BlockNum, int IndexNum, Index*&tmp)
    {
        tmp = (Index*)malloc(IndexNum * sizeof(Index));
        char FileName[30];
        sprintf(FileName, "Block%d", BlockNum);
        ifstream inFile(FileName, ifstream::binary);
        if (!inFile)
            return 0;
        int i = 0;
        for (i = 0; i < IndexNum && !inFile.eof(); i++)
        {
            inFile.read((char*)(tmp + i), sizeof(Index));
        }
        inFile.close();
        return i - 1;
    }
    //将索引指针所指向的索引对象的内容全部写入磁盘中，其中指针所指向的对象数组的大小是固定的
    //将IndexNum个索引对象写入磁盘中
    static bool Write(int BlockNum, int IndexNum, Index * index)
    {
        char FileName[30];
        sprintf(FileName, "Block%d", BlockNum);
        ofstream outFile(FileName, ofstream::binary);
        for (int i = 0; i < IndexNum; i++)
        {
            if ((index + i)->Status == 1)
                outFile.write((char*)(index + i), sizeof(Index));
        }
        outFile.close();
        free(index);
        index = nullptr;
        return true;
    }
    //在索引对象数组中寻找指定创建者的对象，返回物理块号
    static int Find(Index * index, char name[], int IndexNum)
    {
        for (int i = 0; i < IndexNum; i++)
        {
            if (strcmp((index + i)->Creator, name) == 0)
                return (index + i)->PBN;
        }
        return -1;
    }
    //在所有对象数组中寻找指定文件名的所有者，返回物理块号
    static int Find(int IndexBlock, char FileName[], int type)
    {
        Index *tmpIndex = nullptr;
        int count = Read(IndexBlock, 100, tmpIndex);
        int ret = -1;
        for (int i = 0; i < count; i++)
        {
            if (strcmp(FileName, tmpIndex[i].FileName) == 0 && tmpIndex[i].Type == type)
            {
                ret = tmpIndex[i].PBN;
                break;
            }
        }
        return ret;
    }
    //根据文件名删除索引
    static bool Delete(int BlockNum, char FileName[], int type)
    {
        Index *tmpIndex = nullptr;
        int count = Read(BlockNum, 100, tmpIndex);
        for (int i = 0; i < count; i++)
        {
            if (strcmp(tmpIndex[i].FileName, FileName) == 0 && tmpIndex[i].Type == type)
                tmpIndex[i].Status = 0;
        }
        Write(BlockNum, count, tmpIndex);
        return true;
    }
    //根据索引表的序号删除，待定
    static bool Delete(int BlockNum, int IndexNum)
    {

    }
    //向指定的物理块号中添加一条新的索引值,用于实现创建文件操作
    //文件名，拓展，类型，创建者，索引所在的物理块号，索引指向的文件所在的物理块号,type=1,目录文件，
    static int Add(char FileName[], char Extension[], int type, char Creator[], int BlockNum, int PBN, int Right, int ParrentBlock)
    {
        Index *tmpIndex = nullptr;
        int count = Read(BlockNum, 100, tmpIndex);
        memcpy(tmpIndex[count].FileName, FileName, sizeof(char)*FILE_NAME_LENGTH);
        memcpy(tmpIndex[count].Creator, Creator, sizeof(char) * 30);
        if (type == 0)
        {
            memcpy(tmpIndex[count].Extension, Extension, sizeof(Extension));
        }
        tmpIndex[count].ParentIndexBlock = ParrentBlock;
        tmpIndex[count].PBN = PBN;
        tmpIndex[count].Right = Right;
        tmpIndex[count].Type = type;

        time(&tmpIndex[count].CreateTime);
        time(&tmpIndex[count].ModifyTime);

        tmpIndex[count].Status = 1;
        count += 1;


        Write(BlockNum, count, tmpIndex);
        return 0;
    }
    //创建一个新的索引文件
    static int Create()
    {
        char name[30];
        sprintf(name, "Block%d", TotalPBN);
        ofstream outFile(name);
        outFile.close();
        return TotalPBN++;
    }
};

//文件控制块类
class FCB//FileControlBlock
{
public:
    int Type = -1; //1，目录文件，0，数据文件
    char FileName[FILE_NAME_LENGTH];
    char Extension[10];
    int Index;//指向索引文件
    int IndexNumber;//索引文件表项的序号
    //比如Index=99,IndexNumber=100,那么索引的位置就在"Block99"中的第100项
    int Status = 1;//1,不变；0，被删除，写入磁盘的时候，被删除的表项不再写入磁盘中
public:
    //读入文件控制块，返回个数
    static int Read(int BlockNum, int FCBNum, FCB *&tmp)
    {
        //给定物理盘号读取相应的数据
        tmp = (FCB*)malloc(sizeof(FCB) * FCBNum);
        char FileName[30];
        sprintf(FileName, "Block%d", BlockNum);
        ifstream inFile(FileName, ifstream::binary);
        if (!inFile)
            return 0;
        int i = 0;
        for (i = 0; i < FCBNum && !inFile.eof(); i++)
        {
            inFile.read((char*)(tmp + i), sizeof(FCB));
        }
        inFile.close();
        return i - 1;
    }
    //将对象数组写入磁盘中，FCBNum为要写入的对象的个数
    static bool Write(int BlockNum, int FCBNum, FCB *fcb)
    {
        //给定物理盘号，将数据写入
        char FileName[30];
        sprintf(FileName, "Block%d", BlockNum);
        ofstream outFile(FileName, ofstream::binary);
        for (int i = 0; i < FCBNum; i++)
        {
            if ((fcb + i)->Status == 1)
                outFile.write((char*)(fcb + i), sizeof(FCB));
        }
        outFile.close();
        free(fcb);
        return true;
    }
    //根据文件名，寻找目的文件所在的物理块号
    static int  Find(int BlockNum, char FileName[], int type)
    {
        FCB *fcb = nullptr;
        int count = Read(BlockNum, 100, fcb);
        int i = 0;
        int ret = -1;
        for (i = 0; i < count; i++)
        {
            if (strcmp(fcb[i].FileName, FileName) == 0)
            {
                ret = Index::Find(fcb[i].Index,  FileName, type);
                Write(BlockNum, count, fcb);
                break;
            }
        }
        return ret;
    }
    static int Delete(int BlockNum, char FileName[], int type)
    {
        FCB *tmpFCB = nullptr;
        int count = Read(BlockNum, 100, tmpFCB);
        for (int i = 0; i < count; i++)
        {
            if (strcmp(tmpFCB[i].FileName, FileName) == 0 && tmpFCB[i].Type == type)
            {
                tmpFCB[i].Status = 0;
                Index::Delete(tmpFCB[i].Index, FileName, type);
                break;
            }
        }
        Write(BlockNum, count, tmpFCB);
        return 0;
    }
    //向文件控制块表中添加新的一项记录
    //0 数据文件 1 目录文件
    static int Add(int BlockNum, int type, char FileName[], char Extension[], char Creator[], int Right, int PBN, int ParrentDirectory)
    {
        FCB *fcb = nullptr;
        int count = Read(BlockNum, 100, fcb);
        int index;
        if (count == 0)
        {
            index = Index::Create();
        }
        else
        {
            index = fcb[0].Index;
        }
        fcb[count].Index = index;
        memcpy(fcb[count].FileName, FileName, sizeof(char)*FILE_NAME_LENGTH);
        if (type == 0)
        {
            memcpy(fcb[count].Extension, Extension, sizeof(Extension));
        }
        fcb[count].Type = type;
        Index::Add(FileName, Extension, type, Creator, index, PBN, Right, ParrentDirectory);

        fcb[count].Status = 1;
        count += 1;
        Write(BlockNum, count, fcb);
        return 0;
    }
    //创建目录文件，顺便创建索引文件，并关联目录文件到父节点
    static int Create(int ParrentDirectoryBlock)
    {
        char name[30];
        sprintf(name, "Block%d", TotalPBN);
        int PBN = TotalPBN++;
        ofstream outFile(name);
        outFile.close();
        FCB *tmpFCB = nullptr;
        class Index *tmpIndex = nullptr;
        int count = Read(TotalPBN, MAX_ITEM_IN_DIRECTORY, tmpFCB);
        int index = Index::Create();
        Index::Read(index, MAX_ITEM_IN_DIRECTORY, tmpIndex);

        tmpIndex[0].ParentIndexBlock = ParrentDirectoryBlock;
        tmpIndex[0].Status = 1;
        tmpFCB[0].Index = index;
        tmpFCB[0].Status = 1;
        Index::Write(index, 1, tmpIndex);
        FCB::Write(PBN, 1, tmpFCB);
        return PBN;
    }
};

//文件操作类
class File
{
public:
    char CurrentDirecotry[300];//当前目录的字符串,如c:\windows\..
    char CurrentName[FILE_NAME_LENGTH];//当前目录的名，如果现在位于c:\windows\visual ，那么该变量的值就为visual
    //int CurrentFileBlock = -1;
    int CurrentDirectoryBlock = -1;//当前目录文件的物理块地址
    int ParrentDirectoryBlock = -1;//记录父目录的物理块号，如果为根目录，那么值为-1
    int RootDirectoryBlock = -1;//记录根目录的物理块号
    int Status = 0;//是否有用户登陆
    char CurrentUser[30];//当前登陆的用户名
    bool isRoot = false;
public:
    bool Open(char name[40], ifstream &inFile)
    {

        return true;
    }
    //左边显示十六进制 右边显示char类型
    void Read(char FileName[FILE_NAME_LENGTH])
    {
        char name[30];
        int block = FCB::Find(CurrentDirectoryBlock, FileName, 0);
        if (block == -1)
        {
            puts("文件不存在");
            return;
        }
        sprintf(name, "Block%d", block);
        ifstream inFile(name);
        const int MaxCol = 10;//一行中显示多少列
        char buffer[30];
        while (!inFile.eof())
        {
            int i = 0;
            char ch;
            for (i = 0; i < MaxCol; i++)
            {
                inFile.read(&ch, sizeof(char));
                if (inFile.eof()) break;
                buffer[i] = ch;
                //putchar(ch);
            }
            int j;
            for (j = 0; j < MaxCol; j++)
            {
                printf("%2x ", buffer[j]);
            }
            for (; j < MaxCol; j++)
                printf("   ");
            printf("        ");
            for (int j = 0; j < i; j++)
            {
                if ('a' <= buffer[j] && buffer[j] <= 'z' || 'A' <= buffer[j] && buffer[j] <= 'Z')
                    putchar(buffer[j]);
                else
                    putchar('.');
            }
            puts("");
        }
        inFile.close();
    }
    void DeleteFile(char FileName[])
    {
        int b = FCB::Find(CurrentDirectoryBlock, FileName, 0);
        char fName[30];
        sprintf(fName, "Block%d", b);
        FCB::Delete(CurrentDirectoryBlock, FileName, 0);
        remove(fName);
    }
    //type 文件的类型
    void CreateFile(char FileName[], char Extension[])
    {
        FCB *tmpFCB = nullptr;
        int count = FCB::Read(CurrentDirectoryBlock, MAX_ITEM_IN_DIRECTORY, tmpFCB);
        for (int i = 0; i < count; i++)
        {
            if (strcmp(tmpFCB[i].FileName, FileName) == 0 && tmpFCB[i].Type == 0)
            {
                puts("文件已存在");
                return;
            }
        }
        free(tmpFCB);
        char name[30];
        sprintf(name, "Block%d", TotalPBN);
        int PBN = TotalPBN++;
        ofstream outFile(name);
        /*char str[] = "Fuck Asshole";
        outFile.write((char*)str, sizeof(str));*/
        outFile.close();
        FCB::Add(CurrentDirectoryBlock, 0, FileName, Extension, CurrentUser, 0, PBN, ParrentDirectoryBlock);
    }
    void CreateDirectory(char DirectoryName[])
    {
        FCB *tmpFCB = nullptr;
        int count = FCB::Read(CurrentDirectoryBlock, MAX_ITEM_IN_DIRECTORY, tmpFCB);
        for (int i = 0; i < count; i++)
        {
            if (strcmp(tmpFCB[i].FileName, DirectoryName) == 0 && tmpFCB[i].Type == 1)
            {
                puts("目录已存在");
                return;
            }
        }
        free(tmpFCB);
        char name[30];
        //为子目录创建文件控制块表
        int PBN = FCB::Create(CurrentDirectoryBlock);
        FCB::Add(CurrentDirectoryBlock, 1, DirectoryName, "", CurrentUser, 0, PBN, ParrentDirectoryBlock);
    }
    //删除目录，这个比较麻烦，需要递归，遍历文件树，递归删除所有文件
    void DelelteDirecotry(char DirectoryName[FILE_NAME_LENGTH])
    {
        printf("是否删除文件夹下的所有内容[y/n]");
        char op;
        cin >> op;
        if (op == 'n' || op == 'N')
            return;
        int b = FCB::Find(CurrentDirectoryBlock, DirectoryName, 1);
        FCB *tmpFCB = nullptr;//子目录文件控制块表
        int count = FCB::Read(b, MAX_ITEM_IN_DIRECTORY, tmpFCB);
        int index = tmpFCB[0].Index;
        int bkCurr = CurrentDirectoryBlock;
        int bkPar = ParrentDirectoryBlock;
        ChageToSubDirectory(DirectoryName, 0); //将工作目录，切换到子目录的文件控制块表上
        for (int i = 1; i < count; i++)
        {
            if (tmpFCB[i].Type == 0) //该文件为数据文件
            {
                DeleteFile(tmpFCB[i].FileName);
            }
            else if (tmpFCB[i].Type == 1)
            {
                DelelteDirecotry(tmpFCB[i].FileName);//递归删除
            }
        }
        char fName[30];
        sprintf(fName, "Block%d", b);
        remove(fName);
        sprintf(fName, "Block%d", index);
        remove(fName);
        ParrentDirectoryBlock = bkPar;
        CurrentDirectoryBlock = bkCurr;
        FCB::Delete(CurrentDirectoryBlock, DirectoryName, 1);
    }
    //显示当前目录下所有的文件，数据文件和目录文件
    void DisplayDirectory()
    {
        FCB *tmpFCB = nullptr;
        Index *tmpIndex = nullptr;
        FCB::Read(CurrentDirectoryBlock, MAX_ITEM_IN_DIRECTORY, tmpFCB);
        int count = Index::Read(tmpFCB[0].Index, 100, tmpIndex);
        tm *tmpTime;
        printf("%10s%10s%10s%20s\n", "Name", "Owner", "Type", "CreateTime");
        for (int i = 1; i < count; i++)
        {
            printf("%10s%10s", tmpIndex[i].FileName, tmpIndex[i].Creator);
            if (tmpIndex[i].Type == 0)
            {
                printf("%10s", "DATA");
            }
            else if (tmpIndex[i].Type == 1)
            {
                printf("%10s", "DIR");
            }
            tmpTime = localtime(&tmpIndex[i].CreateTime);
            printf("\t\t%4d/%2d/%2d  %2d:%2d\n", tmpTime->tm_year + 1900, tmpTime->tm_mon + 1, tmpTime->tm_mday, tmpTime->tm_hour, tmpTime->tm_min);
            tmpTime = localtime(&tmpIndex[i].ModifyTime);
            //printf("\t%4d/%2d/%2d  %2d:%2d   %2d\n", tmpTime->tm_year + 1900, tmpTime->tm_mon, tmpTime->tm_mday, tmpTime->tm_hour, tmpTime->tm_min,tmpTime->tm_sec);
        }
    }
    //切换到子目录下,option=0，类内函数调用，=1类外调用，需要改变路径
    bool ChageToSubDirectory(char SubDirectory[COMMAND_LENGTH], int option)
    {
        FCB *tmpFCB = nullptr;
        int count = FCB::Read(CurrentDirectoryBlock, MAX_ITEM_IN_DIRECTORY, tmpFCB);
        bool exist = false;
        for (int i = 0; i < count; i++) //不存在该名字的目录文件
        {
            if (strcmp(tmpFCB[i].FileName, SubDirectory) == 0 && tmpFCB[i].Type == 1)
                exist = true;
        }
        if (!exist)
        {
            puts("目录不存在");
            return false;
        }
        int b = FCB::Find(CurrentDirectoryBlock, SubDirectory, 1);
        if (b == -1)
        {
            puts("目录不存在");
            return false;
        }
        else
        {
            if (option == 1)
            {
                strcat(CurrentDirecotry, SubDirectory);
                strcat(CurrentDirecotry, "/");
            }
            ParrentDirectoryBlock = CurrentDirectoryBlock;
            CurrentDirectoryBlock = b;
            memcpy(CurrentName, SubDirectory, sizeof(char)*FILE_NAME_LENGTH);
            return true;
        }
    }
    //切换到上一级目录
    void ChageToParDirectory()
    {
        if (ParrentDirectoryBlock == 0 && isRoot == false)
            return;
        if (ParrentDirectoryBlock == -1)
            return;
        Index *tmpIndex = nullptr;
        FCB *tmpFCB = nullptr;
        FCB::Read(CurrentDirectoryBlock, MAX_ITEM_IN_DIRECTORY, tmpFCB);
        Index::Read(tmpFCB[0].Index, MAX_ITEM_IN_DIRECTORY, tmpIndex);
        CurrentDirectoryBlock = tmpIndex[0].ParentIndexBlock;
        FCB::Read(CurrentDirectoryBlock, MAX_ITEM_IN_DIRECTORY, tmpFCB);
        Index::Read(tmpFCB[0].Index, MAX_ITEM_IN_DIRECTORY, tmpIndex);
        ParrentDirectoryBlock = tmpIndex[0].ParentIndexBlock;
        int len = strlen(CurrentDirecotry);
        len -= 1;
        while (CurrentDirecotry[len - 1] != '/')
            len--;
        CurrentDirecotry[len] = 0;
        free(tmpFCB);
        free(tmpIndex);
    }
    //给出当前的路径
    void DisplayCurrentPath()
    {
        printf("%s:%s", CurrentUser, CurrentDirecotry);
        printf("$");
    }
    //给出绝对路径
    void ChageDirectory(char directory[])
    {
        char node[FILE_NAME_LENGTH];
        int len = strlen(directory);
        int backup = CurrentDirectoryBlock;
        int backup2 = ParrentDirectoryBlock;
        CurrentDirectoryBlock = RootDirectoryBlock;
        ParrentDirectoryBlock = -1;
        for (int i = 1; directory[i]; i++)
        {
            if (directory[i] == '/')
                continue;
            int j = 0;
            while (true)
            {
                node[j] = directory[i];
                if (directory[i + 1] == 0 || directory[i + 1] == '/')
                    break;
                i++;
                j++;
            }
            node[j + 1] = 0;
            if (!ChageToSubDirectory(node, 0))
            {
                puts("目录不存在");
                CurrentDirectoryBlock = backup;
                return;
            }
        }
        strcpy(CurrentDirecotry, directory);
        strcat(CurrentDirecotry, "/");
    }
    //设置当前用户的信息,0--登出时，1--登陆时
    void SetCurrentUserName(char name[], int option)
    {
        if (option == 0)
        {
            memset(CurrentUser, 0, sizeof(CurrentUser));
            memset(CurrentDirecotry, 0, sizeof(CurrentDirecotry));
            Status = 0;
        }
        else if (option == 1)
        {
            memcpy(CurrentUser, name, sizeof(char) * 30);
            memcpy(CurrentDirecotry, "/", sizeof(char)*strlen("/"));
            Status = 1;
        }
    }
    //设置当前用户所在的目录的物理块号
    void SetCurrentUserDirection(int block, int option)
    {
        if (option == 0)
        {
            CurrentDirectoryBlock = -1;
            ParrentDirectoryBlock = -1;
        }
        else if (option == 1)
        {
            CurrentDirectoryBlock = block;
            RootDirectoryBlock = block;
        }
    }
    //删除文件或者文件夹
    void Delete(char Name[FILE_NAME_LENGTH])
    {
        FCB *tmpFCB = nullptr;
        int count = FCB::Read(CurrentDirectoryBlock, MAX_ITEM_IN_DIRECTORY, tmpFCB);
        for (int i = 0; i < count; i++)
        {
            if (strcmp(Name, tmpFCB[i].FileName) == 0 && tmpFCB[i].Type == 0)
                DeleteFile(Name);
            if (strcmp(Name, tmpFCB[i].FileName) == 0 && tmpFCB[i].Type == 1)
                DelelteDirecotry(Name);
        }
        free(tmpFCB);
    }
    //修改数据文件,调用外部的编辑器
    void Edit(char FileName[FILE_NAME_LENGTH])
    {
        FCB *tmpFCB = nullptr;
        Index *tmpIndex = nullptr;
        int count = FCB::Read(CurrentDirectoryBlock, MAX_ITEM_IN_DIRECTORY, tmpFCB);
        bool exist = false;
        int index;
        for (int i = 0; i < count; i++)
        {
            if (strcmp(tmpFCB[i].FileName, FileName) == 0 && tmpFCB[i].Type == 0)
            {
                exist = true;
                index = tmpFCB[i].Index;
                break;
            }
        }
        free(tmpFCB);
        if (!exist)
        {
            puts("文件不存在");
            return;
        }
        //更新修改的时间
        count = Index::Read(index, MAX_ITEM_IN_DIRECTORY, tmpIndex);
        for (int i = 0; i < count; i++)
        {
            if (strcmp(tmpIndex[i].FileName, FileName) == 0 && tmpIndex[i].Type == 0)
            {
                tmpIndex[i].UpdateTime(1);
                break;
            }
        }
        free(tmpIndex);
        int block = FCB::Find(CurrentDirectoryBlock, FileName, 0);
        char name[30];
        sprintf(name, "Block%d", block);

        ifstream inFile(name, ifstream::binary);
        ofstream outFile("tmp");
        char ch;
        //将目标文件的内容提取到临时文件中
        while (!inFile.eof())
        {
            inFile.read(&ch, sizeof(char));
            outFile.write(&ch, sizeof(char));
        }
        inFile.close();
        outFile.close();
        system("vim tmp");

        inFile.open("tmp");
        outFile.open(name, ifstream::binary);
        while (!inFile.eof())
        {
            inFile.read(&ch, sizeof(char));
            outFile.write(&ch, sizeof(char));
        }
        remove("tmp");
    }
    //将文件设置为共享，添加到共享文件夹中
    void SetShared(char FileName[FILE_NAME_LENGTH])
    {

    }

    //解析命令
    void GetCommand()
    {
        /*mkdir cd rm create ls clear */
        while (true)
        {
            DisplayCurrentPath();
            char cmd[COMMAND_LENGTH];
            cin >> cmd;

            switch (cmd[0])
            {
            case 'c' | 'C':
            {
                switch (cmd[1])
                {
                case 'd' | 'D':
                {
                    char path[COMMAND_LENGTH];
                    cin >> path;
                    int j = 0;
                    bool absolute = false;
                    int len = strlen(path);
                    for (int i = 0; i < len; i++)
                    {
                        if (path[i] == '/')
                            absolute = true;
                        path[j] = path[i];
                        j++;
                    }
                    if (absolute)
                        ChageDirectory(path);
                    else if (path[0] == '.' && path[1] == '.')
                        ChageToParDirectory();
                    else
                        ChageToSubDirectory(path, 1);

                    break;
                }
                case 'r' | 'R':
                {
                    char name[COMMAND_LENGTH];
                    cin >> name;
                    CreateFile(name, "txt");
                    break;
                }
                case 'l'|'L':
                {
                    system("cls");
                    break;
                }
                case 'a'|'A':
                {
                    char name[30];
                    cin >> name;
                    Read(name);
                    break;
                }
                }
                break;
            }
            case 'r' | 'R':
            {
                char name[COMMAND_LENGTH];
                cin >> name;
                Delete(name);
                break;
            }
            case 'm' | 'M':
            {
                char name[COMMAND_LENGTH];
                cin >> name;
                CreateDirectory(name);
                break;
            }
            case 'l'|'L':
            {
                DisplayDirectory();
                break;
            }
            case 'e'|'E':
            {
                char name[30];
                cin >> name;
                Edit(name);
                break;
            }
            case 'q' | 'Q':
            {
                return;
                break;
            }
            }
            /*cout << CurrentDirectoryBlock << endl;
            cout << ParrentDirectoryBlock << endl;*/
        }
    }
};

File file;

class User
{
    char Password[30];
public:
    int FCBBlock = 0;
    char UserName[30];
public:
    static User*Load()
    {
        User *tmp = (User*)malloc(sizeof(User) * MAX_USER);
        ifstream inFile("UserInfo", ifstream::binary);
        for (int i = 0; i < MAX_USER && !inFile.eof(); i++)
        {
            inFile.read((char*)(tmp + i), sizeof(User));
        }
        inFile.close();
        return tmp;
    }
    static void Flush(User *user)
    {
        ofstream outFile("UserInfo", ofstream::binary);
        for (int i = 0; i < UserCount && !outFile.eof(); i++)
        {
            outFile.write((char*)(user + i), sizeof(User));
        }
        free(user);
        outFile.close();
    }
    static bool AddUser(char name[30], char pwd[30], User *users)
    {
        if (!hasRoot)
            return false;
        if (UserCount >= MAX_USER)
            return false;
        else
        {
            strcpy(users[UserCount].Password, pwd);
            strcpy(users[UserCount].UserName, name);
            int PBN = FCB::Create(0);
            users[UserCount].FCBBlock = PBN;
            FCB::Add(0, 1, name, "", name, 0, PBN, 0);
            UserCount++;
            return true;
        }
    }
    static void CreateRoot(char name[30], char pwd[30], User *users)
    {
        int PBN = FCB::Create(-1);
        strcpy(users[0].Password, pwd);
        strcpy(users[0].UserName, name);
        users[0].FCBBlock = PBN;
        UserCount = 1;
        hasRoot = true;
    }
    static bool Login(User*users, File &file)
    {
        bool exist = false;
        char name[30];
        char password[30];
        cout << "Account: ";
        cin >> name;
        char a;
        char b;
        int i = 0;
        cout << "Password: ";
        while ((a = getch()) != 13 && (b = getch()) != 13)
        {
            password[i] = a == 0 ? b : a;
            i++;
            putchar('*');
        }
        password[i] = 0;
        puts("");
        for (int i = 0; i < UserCount; i++)
        {
            if (strcmp(users[i].UserName, name) == 0)
            {
                exist = true;
                if (strcmp(users[i].Password, password) == 0)
                {
                    file.SetCurrentUserDirection(users[i].FCBBlock, 1);
                    file.SetCurrentUserName(users[i].UserName, 1);
                    if (strcmp(name, "root") == 0)
                    {
                        file.isRoot = true;
                    }
                    puts("登陆成功");
                    return true;
                }
                else
                {
                    puts("密码错误");
                    return false;
                }
            }
        }
        if (!exist)
            puts("用户不存在");
        return false;
    }
    static void Logout(File &file)
    {
        file.Status = 0;
        file.isRoot = false;
        file.SetCurrentUserName("", 0);
        file.SetCurrentUserDirection(-1, 0);
    }
    void SetPassWord(char pwd[30])
    {
        strcpy(Password, pwd);
    }
    static void ClearAll()
    {
        system("del init");
        system("del Block*");
        system("del UserInfo");
        UserCount = 0;
        TotalPBN = 0;
        hasRoot = false;
    }
};
//单元测试
void TestUser()
{
    //Initialize();
    User *users = User::Load();
    User::AddUser("admin", "123", users);
    //User::Flush(users);
    //Exit();
    //UserCount = 0;
    //Initialize();
    //cout << UserCount << endl;
    //users = User::Load();
    User::Login(users, file);
    file.GetCommand();
}

int main()
{
    Initialize();
    puts("。。。命令说明。。。");
    puts("new    ---  创建新的用户");
    puts("login  ---  用户登陆");
    puts("exit   ---  退出文件系统");
    puts("quit   ---  退出当前用户");
    puts("cd     ---  切换文件目录 cd .. 切换上一级目录 cd 支持绝对路径和相对路径");
    puts("rm     ---  删除文件或者目录，删除目录时递归删除子目录和子文件");
    puts("ls     ---  罗列当前目录下的信息");
    puts("cat    ---  以十六进制的形式打印数据文件");
    puts("edit   ---  以文本形式编辑数据文件");
    puts("mkdir  ---  创建目录");
    puts("create ---  创建数据文件");
    puts("root   ---  重置系统，创建超级管理员 能够查看所有的用户的目录");
    puts("Have Fun :)");
    system("pause");
    system("cls");
    User *users = User::Load();
    char cmd[40];
    if (!hasRoot)
    {
        puts("你必须创建一个root用户");
        cout << "password:";
        char pwd[100];
        cin >> pwd;
        User::CreateRoot("root", pwd, users);
    }
    bool exit = false;
    while (!exit)
    {
        cout << "FileSystem$";
        cin >> cmd;
        switch (cmd[0])
        {
        case 'n'|'N':
        {
            puts("创建用户");
            char name[40];
            char pwd[40];
            cout << "Account: ";
            cin >> name;
            cout << "Password:";
            cin >> pwd;
            if (User::AddUser(name, pwd, users))
                puts("添加用户成功");
            else
                puts("添加用户失败");
            system("cls");
            break;
        }
        case 'l'|'L':
        {
            if (User::Login(users, file))
            {
                system("cls");
                file.GetCommand();
            }
            User::Logout(file);
            break;
        }
        case 'r' | 'R':
        {
            User::ClearAll();
            char pwd[100];
            cout << "set password for root" << endl;
            cout << "password :";
            cin >> pwd;
            User::CreateRoot("root", pwd, users);
            system("cls");
            break;
        }
        case 'E'|'e':
        {
            exit = true;
            break;
        }
        case 'c'|'C':
        {
            system("cls");
            break;
        }
        }
    }
    User::Flush(users);
    Exit();
    return 0;
}
