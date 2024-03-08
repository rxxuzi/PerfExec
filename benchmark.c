/*
benchmark.c
Version 2.2
Copyright 2024 rxxuzi
MIT License
This program measures the execution time of a given executable file and runs in a Windows environment.
*/
#include <windows.h>
#include <stdio.h>
#include <time.h>
#include <string.h>

#define DEFAULT_OUTPUT_FILE "benchmark_result.txt"
#define DEFAULT_COUNT 10
#define MAX_THREADS 16
#define MAX_PATH 260
#define VERSION "2.2"

char* get_time(); // 時刻を文字列で取得。動的にメモリを確保するのでfreeで解放する必要がある。
void help(char* exepath); // help用
void getFullPath(char *executableName, char *fullPath, size_t fullPathSize); //　システムに登録された実行ファイルのフルパスを取得する
int compare(const void *a, const void *b); // 比較関数

static BOOL async_flag = FALSE;
static BOOL output_flag = FALSE; // 出力ファイル用フラグ
static BOOL hasExtraArgs = FALSE; // -x によるコマンドライン引数のフラグ
static BOOL isSystemFile = FALSE; // -s による、フラグ

//結果を格納するための構造体 
typedef struct {
    double average;      // 平均値
    double median;       // 中央値
    int fastestIndex;    // 最速の実行時間のインデックス
    int slowestIndex;    // 最遅の実行時間のインデックス
    double fastest;      // 最速
    double slowest;      // 最遅
    double* times;       // 時間(配列)
    int size;            // 配列サイズ
} BenchmarkResults;

//スレッドの情報.
typedef struct {
    int index;
    double* times;
    char commandLine[2048];
} ThreadData;

// デバッグ情報
typedef struct {
    double elapsed_time;
    char *target_file;
    char *output_file;
    BenchmarkResults br;
    BOOL debug_flag;
} DebugData;

void calculateResults(int size, double *times, double sum, BenchmarkResults *results) {
    // 配列のサイズと本体を保存
    results->size = size;
    results->times = times;

    // ソートされた配列を作成
    double *sorted = (double*)malloc(size * sizeof(double));
    for (int i = 0; i < size; i++) {
        sorted[i] = times[i];
    }
    qsort(sorted, size, sizeof(double), compare);

    // 中央値を計算
    if (size % 2 == 0) {
        results->median = (sorted[(size / 2) - 1] + sorted[size / 2]) / 2.0;
    } else {
        results->median = sorted[size / 2];
    }

    // 平均値を計算
    results->average = sum / size;

    // 最速と最遅のインデックスを見つける
    int fastIndex = 0, slowIndex = 0;
    for (int i = 1; i < size; i++) {
        if (times[i] < times[fastIndex])fastIndex = i;
        if (times[i] > times[slowIndex]) slowIndex = i;
    }
    
    results->fastestIndex = fastIndex;
    results->slowestIndex = slowIndex;
    results->fastest = times[fastIndex];
    results->slowest = times[slowIndex];
    
    free(sorted);
}

// ファイル出力
int output(double *times, int size, char *file, char *exefile, double sum) {
    FILE *fp = fopen(file, "w");
    if (fp == NULL){
        // I/O Error
        return 1;
    }

    fprintf(fp, "Benchmark Test Results for %s \n", exefile);
    fprintf(fp, "Execution Type: %s\n", async_flag ? "Parallel Processing" : "Sequential Processing");

    char *date = get_time();
    fprintf(fp, "%s\n", date);

    BenchmarkResults br;
    calculateResults(size, times, sum , &br);

    double average = br.average;
    double median = br.median;
    int fast = br.fastestIndex;
    int slow = br.slowestIndex;

    fprintf(fp, "Total Runs: -> %d\n" , size);
    fprintf(fp, "Average: %lf ms \n", average);
    fprintf(fp, "Median : %lf ms \n", median);
    fprintf(fp, "Fastest: %lf ms -> Run(%03d) \n", times[fast] , fast);
    fprintf(fp, "Slowest: %lf ms -> Run(%03d) \n", times[slow] , slow);

    fprintf(fp, "\n---Details for Each Run---\n");
    for (size_t i = 0; i < size; i++) {
        fprintf(fp , "Run %03d: %f ms\n", i , times[i]);
    }

    free(date);
    return 0;    
}

DWORD WINAPI AsyncBenchmark(LPVOID lpParam) {
    ThreadData* td = (ThreadData*)lpParam;

    LARGE_INTEGER start, end, freq;
    QueryPerformanceCounter(&start);

    STARTUPINFO si;
    PROCESS_INFORMATION pi;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));

    if (!CreateProcess(NULL, td->commandLine, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
        printf("CreateProcess failed (%d).\n", GetLastError());
        return 1;
    }

    WaitForSingleObject(pi.hProcess, INFINITE);

    QueryPerformanceCounter(&end);
    QueryPerformanceFrequency(&freq);

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    double elapsed = (double)(end.QuadPart - start.QuadPart) / (double)freq.QuadPart;
    td->times[td->index] = elapsed * 1000.0;

    return 0;
}

void show(const DebugData* d) {
    puts("\nBenchmark: ");
    for (size_t i = 0; i < d->br.size; i ++) {
        printf("Run %03d: %f ms\n", i + 1, d->br.times[i]);
    }

    if (d->debug_flag) {
        printf("-----Detailed Results-----\n");
        printf("Average : %7f ms\n", d->br.average);
        printf("Median  : %7f ms\n", d->br.median);
        printf("Fastest : %7f ms Run(%03d)\n",d->br.fastest, d->br.fastestIndex + 1);
        printf("Slowest : %7f ms Run(%03d)\n",d->br.slowest, d->br.slowestIndex + 1);
        printf("Execution Type: %s\n", async_flag ? "(Parallel)" : "(Sequential)");
        printf("Target File   : %s\n" , d->target_file);
        printf("Total Runs    : %d\n" , d->br.size);
        printf("Elapsed Time  : %7f ms \n", d->elapsed_time);
    } else {
        printf("-----Summary Results-----\n");
        printf("Average: %lf ms\n", d->br.average);
        printf("Median : %lf ms\n", d->br.median);
    }

    if(output_flag) printf("Output -> %s", d->output_file);

}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Type for help : %s -h" , strrchr(argv[0], '\\') + 1);
        return 1;
    }

    if (strcmp(argv[1] , "-h") == 0 || strcmp(argv[1] , "--help") == 0 ) {
        help(argv[0]);
        return 0;
    } else if (strcmp(argv[1] , "-v") == 0 || strcmp(argv[1] , "--version") == 0) {
        printf("Version : %s" , VERSION);
        return 0;
    }

    DebugData dd;
    dd.debug_flag = FALSE;

    char *executable = argv[1]; //実行ファイル
    char *target = argv[1]; // 実行するターゲット

    char *output_file = DEFAULT_OUTPUT_FILE;
    int count = DEFAULT_COUNT;
    int num_of_threads = MAX_THREADS;

    char extraArgs[1024] = {0}; // 実行ファイルに渡す追加の引数を格納

    async_flag = FALSE; // 並列処理フラグ

    LARGE_INTEGER frequency;    // タイマーの周波数
    LARGE_INTEGER sd, ed;       // 開始時刻と終了時刻
    
    // オプションを確認
    for(int i = 2; i < argc; i++) {
        if (strcmp(argv[i], "-t") == 0 && i + 1 < argc) {
            // -tオプション
            async_flag = FALSE;
            count = atoi(argv[i + 1]);
            count = (count < 1) ? 1 : (count > 100 ? 100 : count); // 1 ~ 100
            i++; 
        } else if (strcmp(argv[i], "-o") == 0) {
            // -oオプション
            output_flag = TRUE;
            if(i + 1 < argc && argv[i+1][0] != '-') {
                output_file = argv[i + 1];
                i++; 
            }
        } else if (strcmp(argv[i], "-x") == 0 && i + 1 < argc) {
            // -xオプション
            char* x_arg = argv[i + 1];
            strncpy(extraArgs, x_arg, sizeof(extraArgs) - 1);
            hasExtraArgs = TRUE;
        } else if (strcmp(argv[i], "-s") == 0 && i + 1 < argc) {
            // -sオプション
            isSystemFile = TRUE;
            char fullPath[MAX_PATH];
            getFullPath(argv[1], fullPath, sizeof(fullPath));

            // fullPathから改行文字を削除する
            fullPath[strcspn(fullPath, "\n")] = 0;
            executable = fullPath; // システムに登録されたexeファイルのフルパスを取得
            if (i + 1 < argc && argv[i+1][0] != '-') {
                target = argv[i + 1]; // 実行ファイル名を更新
                i++; 
            }
        } else if (strcmp(argv[i], "-a") == 0){
            // -a オプション
            async_flag = TRUE;
            if(i + 1 < argc && argv[i+1][0] != '-') {
                num_of_threads =  atoi(argv[i + 1]);
                i++; 
            }
            count = num_of_threads;
        } else if (strcmp(argv[i], "-d") == 0){
            dd.debug_flag = TRUE;
        }
        
    }

    dd.output_file = output_file;

    // タイマーの周波数を取得
    QueryPerformanceFrequency(&frequency);
    QueryPerformanceCounter(&sd);

    // ファイルが存在するか確認。
    WIN32_FIND_DATA findFileData;
    HANDLE handle = FindFirstFile(target, &findFileData);
    if (handle == INVALID_HANDLE_VALUE) {
        printf("Error: '%s' does not exist.\n", target);
        return 1;
    }
    FindClose(handle);
    char commandLine[2048]; // コマンドライン全体を格納

    if (isSystemFile) {
        // `-s` オプションが指定された場合の処理
        if (hasExtraArgs) {
            snprintf(commandLine, sizeof(commandLine), "%s %s %s", executable, target, extraArgs);
        } else {
            snprintf(commandLine, sizeof(commandLine), "%s %s", executable, target);
        }
    } else {
        // `-s` オプションが指定されていない場合の処理
        if (hasExtraArgs) {
            snprintf(commandLine, sizeof(commandLine), "%s %s", target, extraArgs);
        } else {
            snprintf(commandLine, sizeof(commandLine), "%s", target);
        }
    }

    // 各実行時間を格納する配列
    double times[count];
    double total_time = 0.0;

    if (async_flag) {        
        /*並列処理*/
        int total_executed = 0; // 実行された回数

        while (total_executed < count) {
            int current_batch = min(MAX_THREADS, count - total_executed); // 現在のバッチで実行する回数
            HANDLE threads[current_batch]; // 現在のバッチのスレッドハンドル
            ThreadData td[current_batch]; // 現在のバッチのスレッドデータ

            for (int i = 0; i < current_batch; i++) {
                td[i].index = total_executed + i;
                td[i].times = times;
                strcpy(td[i].commandLine, commandLine);
                
                threads[i] = CreateThread(NULL, 0, AsyncBenchmark, &td[i], 0, NULL);
                if (threads[i] == NULL) {
                    printf("CreateThread failed (%d).\n", GetLastError());
                    return 1;
                }
            }
            // 現在のバッチのスレッドの終了を待つ
            WaitForMultipleObjects(current_batch, threads, TRUE, INFINITE);
            // スレッドハンドルを閉じる
            for (int i = 0; i < current_batch; i++) {
                CloseHandle(threads[i]);
            }
            total_executed += current_batch;
        }
        for (int i = 0; i < count; i++) {
            total_time += times[i];
        }
    } else {
        /*逐次処理*/
        for (int i = 0; i < count; i++) {
            // 開始時間
            LARGE_INTEGER start, end, freq;
            QueryPerformanceCounter(&start);

            // 実行ファイルの実行
            STARTUPINFO si;
            PROCESS_INFORMATION pi;
            ZeroMemory(&si, sizeof(si));
            si.cb = sizeof(si);
            ZeroMemory(&pi, sizeof(pi));

            if (!CreateProcess(
                NULL,           /* No module name (use command line) */
                commandLine,    /* Command line */
                NULL,           /* Process handle not inheritable */
                NULL,           /* Thread handle not inheritable */
                FALSE,          /* Set handle inheritance to FALSE */
                0,              /* No creation flags */
                NULL,           /* Use parent's environment block */
                NULL,           /* Use parent's starting directory */
                &si,            /* Pointer to STARTUPINFO structure */
                &pi)            /* Pointer to PROCESS_INFORMATION structure */
            ) {
                printf("CreateProcess failed (%d).\n", GetLastError());
                return 1;
            }
            // 子プロセスの終了を待つ。
            WaitForSingleObject(pi.hProcess, INFINITE);
            // 終了時間
            QueryPerformanceCounter(&end);
            QueryPerformanceFrequency(&freq);
            // プロセスとスレッドのハンドルを閉じる
            CloseHandle(pi.hProcess);
            CloseHandle(pi.hThread);
            // 実行時間の計算
            double elapsed = (double)(end.QuadPart - start.QuadPart) / (double)freq.QuadPart;
            times[i] = elapsed * 1000.0; 
            total_time += times[i];        
        }
    }

    // 関数実行後の時刻を取得
    QueryPerformanceCounter(&ed);    
    
    BenchmarkResults br;
    calculateResults(count, times, total_time, &br);
    dd.elapsed_time = (double)(ed.QuadPart - sd.QuadPart) * 1000.0 / frequency.QuadPart;
    dd.target_file = target;
    dd.br = br;

    show(&dd);// 結果を表示

    if (output_flag) {
        if(output(times, count, output_file, target, total_time)){
            printf("I/O Error");
            return 1;
        }
    }

    return 0;
}

void help(char *exepath) {
    char* exe = strrchr(exepath, '\\') + 1;
    printf("Usage: %s <exe filepath or command> [options]\n", exe);
    printf("Options:\n");
    printf("  -t <count>    : Executes the specified executable file or command the number of times specified. If not specified, it defaults to %d executions.\n", DEFAULT_COUNT);
    printf("                  You can specify a range from 1 to 100. Note: Cannot be used with -a option.\n");
    printf("  -a <count>    : Executes the specified executable file or command the number of times specified in parallel.\n");
    printf("                  Specifies the number of parallel executions. Note: Cannot be used with -t option.\n");
    printf("  -d            : Displays detailed results. This option provides more comprehensive information about the benchmark results.\n");
    printf("  -o <filename> : Saves the benchmark test results to the specified file. If no filename is specified,\n");
    printf("                  it will be saved to '%s'.\n", DEFAULT_OUTPUT_FILE);
    printf("  -x <\"args\">   : Passes additional command-line options to the executable file or command. The option should be enclosed in quotes.\n");
    printf("  -s <filename> : Specifies that the executable is a system-registered executable. Use this option to run executables located in system paths.\n");
    printf("  -h            : Displays this help message.\n");
    printf("  -v            : Display this version.\n");
    //　例 : 
    printf("Example:\n");
    
    printf("  %s python -s script.py -x \"1000\" -t 10 -o result.txt\n", exe);
    printf("    -> Executes 'python script.py' with the command-line argument '1000' 10 times and saves the results to 'result.txt'.\n");
    printf("       'python' is assumed to be a system-registered executable.\n");

    printf("  %s example.exe -a 30 -o result.txt\n", exe);
    printf("    -> Executes 'example.exe' 30 times in parallel and saves the results to 'result.txt'.\n");
    printf("\nNote: The -a and -t options are mutually exclusive and cannot be used together.\n");
}

//時刻を Y-M-D H:M:S形式で取得する。
char* get_time() {
    time_t t = time(NULL);
    struct tm *local = localtime(&t);
    char* buf = malloc(128); // メモリを動的に割り当てる
    if (buf != NULL) {
        strftime(buf, 128, "%Y-%m-%d %H:%M:%S %A", local);
    }
    return buf;
}

// システムに登録された***.exeのフルパスを取得する
void getFullPath(char *executableName, char *fullPath, size_t fullPathSize) {
    char command[512];
    snprintf(command, sizeof(command), "where %s", executableName);
    FILE *fp = _popen(command, "r");

    if (fp != NULL) {
        fgets(fullPath, fullPathSize, fp);
        _pclose(fp);
    }
}

// 比較関数の定義
int compare(const void *a, const void *b) {
    return (*(int*)a - *(int*)b);
}