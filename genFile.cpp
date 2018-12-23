#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <fstream>

const int MAX_SYMBOLS = 95;
int main(int argc, char* argv[])
{
    srand((unsigned)time(0));
    int fileLength = atoi(argv[2]);
    printf("File creation initiated.\n");
    auto timeStart = std::chrono::high_resolution_clock::now();

    std::ofstream fout;
    fout.open(argv[1], std::ios::out | std::ios::trunc);

    for (int i = 0; i < fileLength; i++) {
        int charNum = rand() % MAX_SYMBOLS + 33;
        fout.put((char)charNum);
    }
    fout.close();

    auto timeEnd = std::chrono::high_resolution_clock::now();
    printf("File creation complete for %f millis.\n", (std::chrono::duration<double, std::milli>(timeEnd - timeStart).count()));

    return 0;
}
