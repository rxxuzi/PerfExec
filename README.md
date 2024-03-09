# PerfExec

PerfExec is a tool designed for measuring the performance of executable files in a Windows environment. It evaluates the execution time, providing insights into the average, median, fastest, and slowest execution times. PerfExec is easy to use, offering a straightforward command-line interface for conducting performance tests.

## Features

- **Performance Benchmarking**: Assess the performance of executable files by measuring execution times.
- **Execution Count Customization**: Specify the number of times the executable file or command should be run.
- **Additional Command-Line Arguments**: Support for passing extra arguments to the executable file or command.
- **Result Output to File**: Option to save benchmark results to a specified file.

## Usage

Execute the program from the command line using the following syntax:

```shell
benchmark.exe <executable file path> [options]
```

### Options

- `-t <count>`: Specifies the number of times to execute the test. Defaults to 10 executions if not specified. Valid range is 1 to 100. Note: Cannot be used with the `-a` option.
- `-a <count>`: Executes the specified command a given number of times in parallel. Specifies the number of parallel executions. Note: Cannot be used with the `-t` option.
- `-o <filename>`: Specifies the filename where benchmark test results will be saved. Defaults to `'benchmark_result.txt'` if not specified.
- `-x "<args>"`: Passes additional command-line options to the executable file or command. The options should be enclosed in quotes.
- `-s <filename>`: Specifies that the executable is a system-registered executable. This option is used to run executables located in system paths.
- `-d`: Displays detailed results, providing more comprehensive information about the benchmark results.
- `-h`: Displays the help message.
- `-v`: Displays the version of the tool.

### Examples

#### Sequential Execution

To execute `example.exe` 10 times in sequential order and save the results to `results.txt`:

```bash
benchmark.exe example.exe -t 10 -o results.txt
```

#### Parallel Execution

To execute `example.exe` 30 times in parallel and save the results to `parallel_results.txt`:

```bash
benchmark.exe example.exe -a 30 -o parallel_results.txt
```

#### Using System-Registered Executable

To execute a system-registered Python script (`script.py`) located in system paths, passing `1000` as a command-line argument, and save the results to `python_results.txt`:

```bash
benchmark.exe python -s script.py -x "1000" -t 10 -o python_results.txt
```

In this example, the `-s` option is used to specify an executable registered with the system, in this case Python. The `-x` option is used to pass an additional argument (`-1000`) to the script, and the `-t` option specifies the number of executions.

## Building

To build this tool, you need a C compiler such as GCC (MinGW). Compile the source file (`benchmark.c`) using your compiler to generate the executable file.

```sh
gcc benchmark.c -o benchmark
```

## License

This project is released under the MIT License. For more details, see the [LICENSE](LICENSE).
