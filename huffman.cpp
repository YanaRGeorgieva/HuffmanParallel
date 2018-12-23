#include "ThreadPool.h"
#include <cstring>
#include <ctime>
#include <fstream>
#include <map>
#include <mutex>
#include <queue>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <thread>
#include <vector>
/*
    Yana Georgieva
    Fac.Num: 81281
    Huffman coding
*/

const unsigned int MAX_SYMBOLS = 256;
const unsigned int MAX_NUMBER_OF_THREADS = 64;
const unsigned int MAX_FILE_NAME_LENGTH = 257;

unsigned int numberOfThreads = 1;
unsigned granularity = 1;
bool isStatic = true;
bool isQuiet = false;

std::map<char, std::string> symbolVsHuffmanCode;

std::string fileName;
int fileLength;

std::string strEncoded;

unsigned frequencyTableOfInputSymbols[MAX_SYMBOLS];
unsigned frequencyTableOfInputSymbolsThreadI[MAX_NUMBER_OF_THREADS][MAX_SYMBOLS];

std::mutex myMutex;

struct Node {
    Node(const char& symbol = '\0', const int& frequencyOftheSymbol = 0)
    {
        left = right = nullptr;
        this->symbol = symbol;
        this->frequencyOftheSymbol = frequencyOftheSymbol;
    }

    char symbol;
    int frequencyOftheSymbol;
    Node *left, *right;
};

auto comp = [](const Node* a, const Node* b) { return a->frequencyOftheSymbol > b->frequencyOftheSymbol; };
std::priority_queue<Node*, std::vector<Node*>, decltype(comp)> huffmanTreeHeap(comp);

void distributeTasksCalculateTheFrequencyTable();
void encodedString(const bool&);
void encodedString();
void printTheCodes();
void constructTheMappingOfASCIIcodeToHuffmanTable(Node*, std::string);
void mainFunctionBuildTheTree();
void calculateFrequencyTableThreadI(const int&, const int&, const int&);
void decodedString();

void distributeTasksCalculateTheFrequencyTable()
{
    memset(frequencyTableOfInputSymbols, 0, sizeof(frequencyTableOfInputSymbols));
    memset(frequencyTableOfInputSymbolsThreadI, 0, sizeof(frequencyTableOfInputSymbolsThreadI));
    std::ifstream fin;
    fin.open(fileName.c_str());

    fin.seekg(0, fin.end);
    fileLength = fin.tellg();
    fin.close();
    if (fileLength < 0) {
        printf("Length is negative -> check the file name.\n");
        exit(1);
    }
    int particles = numberOfThreads * granularity;
    int partLength = fileLength / particles;
    if (!isQuiet) {
        printf("File length is: %d bytes.\n", fileLength);
        printf("Part of the file for each thread is about: %d bytes length.\n", partLength);
        printf("Granularity is %d.\nDivided into %d tasks.\n", granularity, particles);
    }
    if (isStatic) {
        std::thread t[numberOfThreads];

        printf("Calculation of the frequency table has began.\nUsing static distribution.\n");
        auto timeStart = std::chrono::high_resolution_clock::now();

        int it = 0;
        for (; it < particles - 1; it++) {
            if (it % numberOfThreads == 0 && it > 0) {
                for (int i = 0; i < numberOfThreads; i++)
                    t[i].join();
            }
            t[it % numberOfThreads] = std::thread(calculateFrequencyTableThreadI, it * partLength, partLength, it % numberOfThreads);
        }
        if (numberOfThreads == 1 && granularity > 1) {
            t[0].join();
        }
        t[it % numberOfThreads] = std::thread(calculateFrequencyTableThreadI, it * partLength, partLength + fileLength % numberOfThreads,
            it % numberOfThreads);

        for (int i = 0; i < numberOfThreads; i++)
            t[i].join();

        for (int i = 0; i < numberOfThreads; i++)
            for (int j = 0; j < MAX_SYMBOLS; j++)
                if (frequencyTableOfInputSymbolsThreadI[i][j] > 0)
                    frequencyTableOfInputSymbols[j] += frequencyTableOfInputSymbolsThreadI[i][j];

        auto timeEnd = std::chrono::high_resolution_clock::now();
        printf("Calculation of the frequency table has ended.\nIts execution time was %f millis.\n",
            (std::chrono::duration<double, std::milli>(timeEnd - timeStart).count()));
    } else {
        ThreadPool thread_pool(numberOfThreads);
        std::vector<std::future<void> > thread_futures;

        printf("Calculation of the frequency table has began.\nUsing pool of threads.\n");
        auto timeStart = std::chrono::high_resolution_clock::now();

        int it = 0;
        for (it = 0; it < particles; ++it) {
            if (it == particles - 1) {
                thread_futures.emplace_back(thread_pool.enqueue(calculateFrequencyTableThreadI, it * partLength,
                    partLength + fileLength % numberOfThreads, it % numberOfThreads));
                break;
            }
            thread_futures.emplace_back(thread_pool.enqueue(calculateFrequencyTableThreadI, it * partLength, partLength,
                it % numberOfThreads));
        }
        for (auto& it : thread_futures) {
            it.get();
        }
        for (int i = 0; i < numberOfThreads; i++)
            for (int j = 0; j < MAX_SYMBOLS; j++)
                if (frequencyTableOfInputSymbolsThreadI[i][j] > 0)
                    frequencyTableOfInputSymbols[j] += frequencyTableOfInputSymbolsThreadI[i][j];

        auto timeEnd = std::chrono::high_resolution_clock::now();
        printf("Calculation of the frequency table has ended.\nIts execution time was %f millis.\n",
            (std::chrono::duration<double, std::milli>(timeEnd - timeStart).count()));
    }
}

void mainFunctionBuildTheTree()
{
    struct Node *left, *right, *newNode;

    for (int i = 0; i < MAX_SYMBOLS; i++)
        if (frequencyTableOfInputSymbols[i] > 0)
            huffmanTreeHeap.push(new Node((char)i, frequencyTableOfInputSymbols[i]));

    while (huffmanTreeHeap.size() != 1) {
        left = huffmanTreeHeap.top();
        huffmanTreeHeap.pop();
        right = huffmanTreeHeap.top();
        huffmanTreeHeap.pop();
        newNode = new Node('\0', left->frequencyOftheSymbol + right->frequencyOftheSymbol);
        newNode->left = left;
        newNode->right = right;
        huffmanTreeHeap.push(newNode);
    }
    constructTheMappingOfASCIIcodeToHuffmanTable(huffmanTreeHeap.top(), "");
}

void calculateFrequencyTableThreadI(const int& currStart, const int& length, const int& threadNumber)
{
    if (!isQuiet) {
        myMutex.lock();
        printf("Thread %d started.\n", threadNumber);
        myMutex.unlock();
    }

    auto timeStart = std::chrono::high_resolution_clock::now();

    std::ifstream fin;
    fin.open(fileName, std::ios::in);
    fin.seekg(currStart, std::ios::beg);

    char symbol;
    int limit = currStart + length;
    for (int i = currStart; i < limit; i++) {
        fin.get(symbol);
        frequencyTableOfInputSymbolsThreadI[threadNumber][symbol]++;
    }

    auto timeEnd = std::chrono::high_resolution_clock::now();
    if (!isQuiet) {
        myMutex.lock();
        printf("Thread %d ended.\n", threadNumber);
        myMutex.unlock();

        myMutex.lock();
        printf("Thread %d execution time was %f millis.\n", threadNumber,
            (std::chrono::duration<double, std::milli>(timeEnd - timeStart).count()));
        myMutex.unlock();
    }
}

void constructTheMappingOfASCIIcodeToHuffmanTable(Node* root, std::string str)
{
    if (!root)
        return;
    if (root->symbol != '\0')
        symbolVsHuffmanCode[root->symbol] = str;
    constructTheMappingOfASCIIcodeToHuffmanTable(root->left, str + "0");
    constructTheMappingOfASCIIcodeToHuffmanTable(root->right, str + "1");
}

void printTheCodes()
{
    printf("Frequency Table:\n");
    for (std::map<char, std::string>::iterator it = symbolVsHuffmanCode.begin(); it != symbolVsHuffmanCode.end(); it++)
        printf("%c %s\n", it->first, it->second.c_str());
}

void decodedString()
{
    if (strEncoded.size() <= 0)
        encodedString(false);
    std::string result = "";
    struct Node* curr = huffmanTreeHeap.top();
    int strEncodedLength = strEncoded.size();
    for (int i = 0; i < strEncodedLength; i++) {
        if (strEncoded[i] == '0')
            curr = curr->left;
        else
            curr = curr->right;

        if (curr->left == nullptr && curr->right == nullptr) {
            result += curr->symbol;
            curr = huffmanTreeHeap.top();
        }
    }
    result += '\0';
    printf("Decoded :%s\n", result.c_str());
}

void encodedString()
{
    encodedString(true);
}

void encodedString(const bool& sayIt)
{
    if (strEncoded.size() == 0) {
        std::ifstream fin;
        fin.open(fileName);
        char symbol;
        int i = 0;
        while (i < fileLength) {
            fin.get(symbol);
            strEncoded += symbolVsHuffmanCode[symbol];
            i++;
        }
        fin.close();
        if (sayIt)
            printf("Encoded :%s\n", strEncoded.c_str());
    } else if (sayIt)
        printf("Encoded :%s\n", strEncoded.c_str());
}

int main(int argc, char* argv[])
{
    auto timeStart = std::chrono::high_resolution_clock::now();

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-q") == 0 || strcmp(argv[i], "-quiet") == 0)
            isQuiet = true;
        else if (strcmp(argv[i], "-f") == 0) {
            fileName = argv[i + 1];
            i++;
        } else if (strcmp(argv[i], "-t") == 0 || strcmp(argv[i], "-tasks") == 0) {
            numberOfThreads = atoi(argv[i + 1]);
            i++;
        } else if (strcmp(argv[i], "-g") == 0 || strcmp(argv[i], "-granularity") == 0) {
            granularity = atoi(argv[i + 1]);
            i++;
        } else if (strcmp(argv[i], "-b") == 0 || strcmp(argv[i], "-balancing") == 0) {
            if (strcmp(argv[i + 1], "static") == 0)
                isStatic = true;
            else if (strcmp(argv[i + 1], "dynamic") == 0)
                isStatic = false;
            i++;
        }
    }

    if (numberOfThreads > MAX_NUMBER_OF_THREADS) {
        printf("Too many threads.\nTry a smaller number.\n");
        exit(1);
    }

    if (!isQuiet) {
        printf("Main thread started.\nNumber of threads to be started: %d.\n", numberOfThreads);
    }

    distributeTasksCalculateTheFrequencyTable();
    mainFunctionBuildTheTree();

    auto timeEnd = std::chrono::high_resolution_clock::now();
    if (!isQuiet) {
        printf("Main thread ended.\nTotal execution time was %f millis.\n",
            std::chrono::duration<double, std::milli>(timeEnd - timeStart).count());
    }
    /*
    printTheCodes();
    encodedString();
    decodedString();*/
}
