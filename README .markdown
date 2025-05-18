# ARCHEX - Archive Extractor CLI

## Overview
ARCHEX is a command-line interface (CLI) tool designed to extract and process archive files in `.hex` and `.txt` (xxd format) formats. It consists of two main components:

- **archex.sh**: A Bash script that provides an interactive CLI for managing archive extraction tasks.
- **archex.c**: A C program that performs the actual extraction and processing of archive files, supporting various compression and encryption methods.

This project was developed as part of a lab assignment to demonstrate file processing, CLI development, and system programming concepts.

## Features
- **Interactive CLI**: Use `archex.sh` to set parameters and run extraction tasks interactively.
- **File Discovery**: Search for `.hex` and `.txt` files in the current or specified directories.
- **Archive Extraction**: Supports archives with a custom `ARCH` magic number, handling different endianness and processing methods (e.g., ZLIB, LZMA, FERNET).
- **Logging**: Logs all operations to `archextract.log` in append mode, preserving previous logs.
- **Metadata Reporting**: Generates a `metadata.txt` file in the output directory with details of extracted files.
- **Command History**: Navigate previous commands using Page Up/Page Down or arrow keys.

## Prerequisites
- **Bash**: For running `archex.sh`.
- **GCC**: For compiling `archex.c` into the executable `archex`.
- **Python 3**: Required for `process_data.py` to process archive data with methods like ZLIB, LZMA, and FERNET.
- **Operating System**: Tested on Linux/Unix-like systems (e.g., Ubuntu, Kali Linux).

## Dependencies
### For C Program (`archex.c`)
- No additional libraries required (uses standard C libraries like `<stdio.h>`, `<stdlib.h>`, etc.).
- Compiler: GCC (install with: `sudo apt-get install build-essential`).
- Python 3 (required for `popen()` to call `process_data.py`).

### For Python Script (`process_data.py`)
- **Standard Libraries**: `zlib`, `lzma` (included with Python).
- **External Library**: `cryptography` (for FERNET encryption/decryption).
  - Install globally with: `pip3 install cryptography --break-system-packages`
  - Alternative (if avoiding virtual environment): `sudo apt install python3-cryptography`

### For Bash Script (`archex.sh`)
- Bash environment (available on Linux).
- Standard Unix utilities (e.g., `find`).

## Installation
1. **Clone or Download the Project**:
   - Ensure all files (`archex.sh`, `archex.c`, and `process_data.py`) are in the same directory.

2. **Install System Dependencies**:
   - Update system: `sudo apt-get update`
   - Install GCC: `sudo apt-get install build-essential`
   - Install Python 3: `sudo apt-get install python3`

3. **Install Python Dependency**:
   - Install `cryptography` globally:
     ```
     pip3 install cryptography --break-system-packages
     ```
   - Alternative (if avoiding virtual environment):
     ```
     sudo apt install python3-cryptography
     ```
   - Or, install from downloaded `.tar.gz` file (if offline):
     - Navigate to the directory containing the downloaded file (e.g., `~/Downloads`):
       ```
       cd ~/Downloads
       ```
     - Extract the `.tar.gz` file:
       ```
       tar -xzf cryptography-42.0.5.tar.gz
       ```
     - Enter the extracted directory:
       ```
       cd cryptography-42.0.5
       ```
     - Install the package:
       ```
       python3 setup.py install
       ```
     - Note: Ensure dependencies `build-essential`, `libssl-dev`, `libffi-dev`, and `python3-dev` are installed:
       ```
       sudo apt install -y build-essential libssl-dev libffi-dev python3-dev
       ```
   - Or, download and install using `wget` (if online):
     - Ensure `wget` is installed (install with `sudo apt install wget` if needed):
       ```
       wget --version
       ```
     - Download the latest `.tar.gz` file from PyPI (e.g., version 42.0.5):
       ```
       wget https://files.pythonhosted.org/packages/f6/47/92a8914716f2405f33f1814b97353e3cfa223cd94a77104075d42de3099e/cryptography-45.0.2.tar.gz
       ```
     - Extract the downloaded file:
       ```
       tar -xzf cryptography-42.0.5.tar.gz
       ```
     - Enter the extracted directory:
       ```
       cd cryptography-42.0.5
       ```
     - Install the package:
       ```
       python3 setup.py install
       ```
     - Note: Ensure dependencies are installed (see above).

4. **Compile the C Program**:
   ```
   gcc -o archex archex.c
   ```

5. **Make the Bash Script Executable**:
   ```
   chmod +x archex.sh
   ```

6. **Verify Python 3 Installation**:
   - Ensure `python3` is installed and `process_data.py` is available:
     ```
     python3 --version
     ```

## Usage
### Running the CLI (`archex.sh`)
Start the CLI:
```
./archex.sh
```
This will clear the screen, display the ARCHEX banner, and present the `archex>` prompt.

#### Available Commands:
- `help`: Display the help message with available commands.
- `find [/directory]`: List all `.hex` and `.txt` files in the current directory or a specified directory.
- `set fn <filename>`: Set the input archive file (e.g., `archive_be.hex`).
- `set o <directory>`: Set the output directory (e.g., `./big_hex`).
- `set vn <version>`: Set the version number in hex (e.g., `0x02`).
- `set v <mode>`: Set the verbose mode (0, 1, or 2).
- `show`: Display current parameter settings.
- `run`: Execute the extraction process with the current settings.
- `exit`: Exit the CLI.

#### Example Workflow:
```
archex> find
Available archive files (.hex and .txt) in .:
archive_be.hex
archive_le.hex

archex> set fn archive_be.hex
Input file set to: archive_be.hex

archex> set o big_hex
Output directory set to: big_hex

archex> set vn 0x02
Version number set to: 0x02

archex> set v 2
Verbose mode set to: 2

archex> run
Starting unarchive process...
Executing: ./archex -i archive_be.hex -o big_hex -v 2 -vn 0x02
Unarchive completed successfully. Check big_hex for extracted files.

archex> exit
Exiting archex CLI. Goodbye!
```

### Running the C Program Directly (`archex`)
You can also run the `archex` executable directly without the CLI for a single extraction task:
```
./archex -i <input_file> [-o <output_dir>] [-v [0|1|2]] [-vn <version>]
```
- `-i <input_file>`: Specify the input archive file (required).
- `-o <output_dir>`: Specify the output directory (default: `./extracted`).
- `-v [0|1|2]`: Set verbose mode (0: silent, 1: basic info, 2: detailed info; default: 0).
- `-vn <version>`: Specify the version number in hex (e.g., `0x02`; default: `0x01`).

#### Example:
```
./archex -i archive_be.hex -o big_hex -v 2 -vn 0x02
```

## Output Files
- **Extracted Files**: Extracted files are placed in the specified output directory.
- **Metadata Report**: A `metadata.txt` file is generated in the output directory, listing extracted files with their original size, processed size, and processing method.
- **Log File**: All operations and errors are logged to `archextract.log`.

## Included Files
- `archex.c`: C program for archive extraction.
- `archex.sh`: Bash script for interactive CLI.
- `process_data.py`: Python script for processing archive data.

## Notes
- **Log Preservation**: The `archextract.log` file retains all logs across multiple runs, as it is opened in append mode.
- **Command History**: Use Page Up/Page Down or arrow keys in `archex.sh` to navigate previous commands.
- **Error Handling**: Errors are logged to both `archextract.log` and displayed on the console, ensuring you can debug issues easily.
- **Directory Permissions**: Ensure the output directory (e.g., `big_hex`) is writable. If needed, create parent directories manually or use absolute paths (e.g., `/home/user/big_hex`).
- **Archive Files**: Ensure archive files (e.g., `archive_be.hex`) are available in the working directory, or specify their full path.
