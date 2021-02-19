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
//����ϵͳʱ���������������Ϣ�������û��������Ѿ�ע����û���������һ��ʹ���˶��ٿ������
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
//ϵͳ�ر�ʱ������Щ��Ϣд���ļ��У�Ϊ�´�����׼��
void Exit()
{
    ofstream outFile("init", ofstream::binary);
    outFile.write((char*)&MAX_USER, sizeof(int));
    outFile.write((char*)&UserCount, sizeof(int));
    outFile.write((char*)&TotalPBN, sizeof(int));
    outFile.write((char*)&hasRoot, sizeof(bool));
    outFile.close();
}

//������
class Index
{
public:
    char FileName[FILE_NAME_LENGTH] = {0};
    char Extension[10] = { 0 };
    int Right = 000; //����ʱ��100,���ɶ�����д��101�����ɶ���д��110���ɶ�����д��111���ɶ���д��0XX�����ڴ����ߣ��ɶ���д
    int PBN = 0; //Physic Block Number,������
    char Creator[30] = { 0 };
    int ParentIndexBlock = -1;//������ļ���Ŀ¼�ļ��Ļ�������ľ��Ǹ�Ŀ¼��������
    //int Count = 0;//ָ���������������������ʵ���ļ�����
    //int NextIndex = -1;//����û��������ݣ������µ������̣���һ���̵�����λ��
    //int NextIndexNumber = -1;//ͬ��
    int Status = 1;//1����ʾ��ɾ����0����ʾɾ��������д�������
    int Type = -1;
    time_t ModifyTime;//������޸ĵ�ʱ��
    time_t CreateTime;//������ʱ��
public :
    //0,����ʱ����������ʱ�䣻1���޸�ʱ��ֻ�����޸�ʱ��
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
    //�������ļ���ָ�����������ж�ȡ�����������������ĸ���
    //�������ڵ������ţ�������������Ĵ�С��ָ��
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
    //������ָ����ָ����������������ȫ��д������У�����ָ����ָ��Ķ�������Ĵ�С�ǹ̶���
    //��IndexNum����������д�������
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
    //����������������Ѱ��ָ�������ߵĶ��󣬷���������
    static int Find(Index * index, char name[], int IndexNum)
    {
        for (int i = 0; i < IndexNum; i++)
        {
            if (strcmp((index + i)->Creator, name) == 0)
                return (index + i)->PBN;
        }
        return -1;
    }
    //�����ж���������Ѱ��ָ���ļ����������ߣ�����������
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
    //�����ļ���ɾ������
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
    //��������������ɾ��������
    static bool Delete(int BlockNum, int IndexNum)
    {

    }
    //��ָ���������������һ���µ�����ֵ,����ʵ�ִ����ļ�����
    //�ļ�������չ�����ͣ������ߣ��������ڵ������ţ�����ָ����ļ����ڵ�������,type=1,Ŀ¼�ļ���
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
    //����һ���µ������ļ�
    static int Create()
    {
        char name[30];
        sprintf(name, "Block%d", TotalPBN);
        ofstream outFile(name);
        outFile.close();
        return TotalPBN++;
    }
};

//�ļ����ƿ���
class FCB//FileControlBlock
{
public:
    int Type = -1; //1��Ŀ¼�ļ���0�������ļ�
    char FileName[FILE_NAME_LENGTH];
    char Extension[10];
    int Index;//ָ�������ļ�
    int IndexNumber;//�����ļ���������
    //����Index=99,IndexNumber=100,��ô������λ�þ���"Block99"�еĵ�100��
    int Status = 1;//1,���䣻0����ɾ����д����̵�ʱ�򣬱�ɾ���ı����д�������
public:
    //�����ļ����ƿ飬���ظ���
    static int Read(int BlockNum, int FCBNum, FCB *&tmp)
    {
        //���������̺Ŷ�ȡ��Ӧ������
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
    //����������д������У�FCBNumΪҪд��Ķ���ĸ���
    static bool Write(int BlockNum, int FCBNum, FCB *fcb)
    {
        //���������̺ţ�������д��
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
    //�����ļ�����Ѱ��Ŀ���ļ����ڵ�������
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
    //���ļ����ƿ��������µ�һ���¼
    //0 �����ļ� 1 Ŀ¼�ļ�
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
    //����Ŀ¼�ļ���˳�㴴�������ļ���������Ŀ¼�ļ������ڵ�
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

//�ļ�������
class File
{
public:
    char CurrentDirecotry[300];//��ǰĿ¼���ַ���,��c:\windows\..
    char CurrentName[FILE_NAME_LENGTH];//��ǰĿ¼�������������λ��c:\windows\visual ����ô�ñ�����ֵ��Ϊvisual
    //int CurrentFileBlock = -1;
    int CurrentDirectoryBlock = -1;//��ǰĿ¼�ļ���������ַ
    int ParrentDirectoryBlock = -1;//��¼��Ŀ¼�������ţ����Ϊ��Ŀ¼����ôֵΪ-1
    int RootDirectoryBlock = -1;//��¼��Ŀ¼��������
    int Status = 0;//�Ƿ����û���½
    char CurrentUser[30];//��ǰ��½���û���
    bool isRoot = false;
public:
    bool Open(char name[40], ifstream &inFile)
    {

        return true;
    }
    //�����ʾʮ������ �ұ���ʾchar����
    void Read(char FileName[FILE_NAME_LENGTH])
    {
        char name[30];
        int block = FCB::Find(CurrentDirectoryBlock, FileName, 0);
        if (block == -1)
        {
            puts("�ļ�������");
            return;
        }
        sprintf(name, "Block%d", block);
        ifstream inFile(name);
        const int MaxCol = 10;//һ������ʾ������
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
    //type �ļ�������
    void CreateFile(char FileName[], char Extension[])
    {
        FCB *tmpFCB = nullptr;
        int count = FCB::Read(CurrentDirectoryBlock, MAX_ITEM_IN_DIRECTORY, tmpFCB);
        for (int i = 0; i < count; i++)
        {
            if (strcmp(tmpFCB[i].FileName, FileName) == 0 && tmpFCB[i].Type == 0)
            {
                puts("�ļ��Ѵ���");
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
                puts("Ŀ¼�Ѵ���");
                return;
            }
        }
        free(tmpFCB);
        char name[30];
        //Ϊ��Ŀ¼�����ļ����ƿ��
        int PBN = FCB::Create(CurrentDirectoryBlock);
        FCB::Add(CurrentDirectoryBlock, 1, DirectoryName, "", CurrentUser, 0, PBN, ParrentDirectoryBlock);
    }
    //ɾ��Ŀ¼������Ƚ��鷳����Ҫ�ݹ飬�����ļ������ݹ�ɾ�������ļ�
    void DelelteDirecotry(char DirectoryName[FILE_NAME_LENGTH])
    {
        printf("�Ƿ�ɾ���ļ����µ���������[y/n]");
        char op;
        cin >> op;
        if (op == 'n' || op == 'N')
            return;
        int b = FCB::Find(CurrentDirectoryBlock, DirectoryName, 1);
        FCB *tmpFCB = nullptr;//��Ŀ¼�ļ����ƿ��
        int count = FCB::Read(b, MAX_ITEM_IN_DIRECTORY, tmpFCB);
        int index = tmpFCB[0].Index;
        int bkCurr = CurrentDirectoryBlock;
        int bkPar = ParrentDirectoryBlock;
        ChageToSubDirectory(DirectoryName, 0); //������Ŀ¼���л�����Ŀ¼���ļ����ƿ����
        for (int i = 1; i < count; i++)
        {
            if (tmpFCB[i].Type == 0) //���ļ�Ϊ�����ļ�
            {
                DeleteFile(tmpFCB[i].FileName);
            }
            else if (tmpFCB[i].Type == 1)
            {
                DelelteDirecotry(tmpFCB[i].FileName);//�ݹ�ɾ��
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
    //��ʾ��ǰĿ¼�����е��ļ��������ļ���Ŀ¼�ļ�
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
    //�л�����Ŀ¼��,option=0�����ں������ã�=1������ã���Ҫ�ı�·��
    bool ChageToSubDirectory(char SubDirectory[COMMAND_LENGTH], int option)
    {
        FCB *tmpFCB = nullptr;
        int count = FCB::Read(CurrentDirectoryBlock, MAX_ITEM_IN_DIRECTORY, tmpFCB);
        bool exist = false;
        for (int i = 0; i < count; i++) //�����ڸ����ֵ�Ŀ¼�ļ�
        {
            if (strcmp(tmpFCB[i].FileName, SubDirectory) == 0 && tmpFCB[i].Type == 1)
                exist = true;
        }
        if (!exist)
        {
            puts("Ŀ¼������");
            return false;
        }
        int b = FCB::Find(CurrentDirectoryBlock, SubDirectory, 1);
        if (b == -1)
        {
            puts("Ŀ¼������");
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
    //�л�����һ��Ŀ¼
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
    //������ǰ��·��
    void DisplayCurrentPath()
    {
        printf("%s:%s", CurrentUser, CurrentDirecotry);
        printf("$");
    }
    //��������·��
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
                puts("Ŀ¼������");
                CurrentDirectoryBlock = backup;
                return;
            }
        }
        strcpy(CurrentDirecotry, directory);
        strcat(CurrentDirecotry, "/");
    }
    //���õ�ǰ�û�����Ϣ,0--�ǳ�ʱ��1--��½ʱ
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
    //���õ�ǰ�û����ڵ�Ŀ¼��������
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
    //ɾ���ļ������ļ���
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
    //�޸������ļ�,�����ⲿ�ı༭��
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
            puts("�ļ�������");
            return;
        }
        //�����޸ĵ�ʱ��
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
        //��Ŀ���ļ���������ȡ����ʱ�ļ���
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
    //���ļ�����Ϊ������ӵ������ļ�����
    void SetShared(char FileName[FILE_NAME_LENGTH])
    {

    }

    //��������
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
                    puts("��½�ɹ�");
                    return true;
                }
                else
                {
                    puts("�������");
                    return false;
                }
            }
        }
        if (!exist)
            puts("�û�������");
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
//��Ԫ����
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
    puts("����������˵��������");
    puts("new    ---  �����µ��û�");
    puts("login  ---  �û���½");
    puts("exit   ---  �˳��ļ�ϵͳ");
    puts("quit   ---  �˳���ǰ�û�");
    puts("cd     ---  �л��ļ�Ŀ¼ cd .. �л���һ��Ŀ¼ cd ֧�־���·�������·��");
    puts("rm     ---  ɾ���ļ�����Ŀ¼��ɾ��Ŀ¼ʱ�ݹ�ɾ����Ŀ¼�����ļ�");
    puts("ls     ---  ���е�ǰĿ¼�µ���Ϣ");
    puts("cat    ---  ��ʮ�����Ƶ���ʽ��ӡ�����ļ�");
    puts("edit   ---  ���ı���ʽ�༭�����ļ�");
    puts("mkdir  ---  ����Ŀ¼");
    puts("create ---  ���������ļ�");
    puts("root   ---  ����ϵͳ��������������Ա �ܹ��鿴���е��û���Ŀ¼");
    puts("Have Fun :)");
    system("pause");
    system("cls");
    User *users = User::Load();
    char cmd[40];
    if (!hasRoot)
    {
        puts("����봴��һ��root�û�");
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
            puts("�����û�");
            char name[40];
            char pwd[40];
            cout << "Account: ";
            cin >> name;
            cout << "Password:";
            cin >> pwd;
            if (User::AddUser(name, pwd, users))
                puts("����û��ɹ�");
            else
                puts("����û�ʧ��");
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
