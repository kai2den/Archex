#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>
#include <ctype.h>
#include <stdarg.h> // For variadic functions like log_error

// Define constants for maximum path length, line length, and magic number
#define MAX_PATH 256
#define MAX_LINE 1024
#define MAGIC_NUMBER 0x41524348 // "ARCH" in hex
#define LOG_FILE "archextract.log" // File for logging operations
#define REPORT_FILE "metadata.txt" // File for metadata output

// Enum for processing methods (compression/encryption types)
typedef enum { NO_PROCESSING = 0x00, ZLIB = 0x01, LZMA = 0x02, FERNET = 0x03 } Method;
// Enum for endianness (byte order)
typedef enum { ENDIAN_LITTLE, ENDIAN_BIG } Endianness;

// Global file pointers for logging and reporting, and verbose mode flag
FILE *log_fp = NULL; // File pointer for log file
FILE *report_fp = NULL; // File pointer for metadata file
int verbose = 0; // Verbose mode (0: off, 1: basic, 2: detailed)

// Function to log a message to the log file and console (if verbose mode is on)
void log_message(const char *msg) {
    if (log_fp) {
        fprintf(log_fp, "%s\n", msg);
        fflush(log_fp); // Ensure the message is written immediately
    }
    if (verbose >= 1) {
        printf("%s\n", msg);
        fflush(stdout); // Ensure console output is immediate
    }
}

// Function to log an error to both log file and stderr
void log_error(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt); // Start variadic argument processing
    if (log_fp) {
        fprintf(log_fp, "ERROR: ");
        vfprintf(log_fp, fmt, args);
        fprintf(log_fp, "\n");
        fflush(log_fp); // Flush to ensure immediate write
    }
    fprintf(stderr, "ERROR: ");
    vfprintf(stderr, fmt, args);
    fprintf(stderr, "\n");
    fflush(stderr); // Flush stderr output
    va_end(args); // End variadic argument processing
}

// Function to read a 32-bit unsigned integer from a buffer with specified endianness
uint32_t read_uint32(const uint8_t *buf, Endianness endian) {
    if (endian == ENDIAN_LITTLE)
        return buf[0] | (buf[1] << 8) | (buf[2] << 16) | (buf[3] << 24); // Little-endian order
    return buf[3] | (buf[2] << 8) | (buf[1] << 16) | (buf[0] << 24); // Big-endian order
}

// Function to read a 64-bit unsigned integer from a buffer with specified endianness
uint64_t read_uint64(const uint8_t *buf, Endianness endian) {
    uint64_t result = 0;
    if (endian == ENDIAN_LITTLE) {
        for (int i = 0; i < 8; i++) result |= ((uint64_t)buf[i]) << (i * 8); // Little-endian
    } else {
        for (int i = 0; i < 8; i++) result |= ((uint64_t)buf[7 - i]) << (i * 8); // Big-endian
    }
    return result;
}

// Function to check if a file has a ".hex" extension
int is_hex_file(const char *filename) {
    return strstr(filename, ".hex") != NULL;
}

// Function to check if a file has a ".txt" extension (xxd format)
int is_xxd_file(const char *filename) {
    return strstr(filename, ".txt") != NULL;
}

// Function to read a line of hex data from a file and convert it to binary
int read_hex_line(FILE *fp, uint8_t *buf, size_t *buf_len, int is_xxd) {
    char line[MAX_LINE];
    if (!fgets(line, MAX_LINE, fp)) return 0; // Read a line; return 0 if EOF

    if (is_xxd) {
        // For xxd format, find the hex data after the address
        char *hex_start = strchr(line, ':');
        if (!hex_start) return 0;
        hex_start++;
        while (*hex_start == ' ') hex_start++; // Skip spaces
        *buf_len = 0;
        // Convert hex pairs to bytes
        while (isxdigit(hex_start[0]) && isxdigit(hex_start[1])) {
            sscanf(hex_start, "%2hhx", &buf[*buf_len]);
            (*buf_len)++;
            hex_start += 2;
            if (*hex_start == ' ') hex_start++;
        }
    } else {
        // For raw hex format, convert the entire line
        size_t len = strlen(line);
        if (len > 0 && line[len - 1] == '\n') len--; // Remove newline
        if (len % 2 != 0) {
            log_error("Invalid hex line length");
            return 0;
        }
        *buf_len = len / 2; // Each byte is 2 hex chars
        for (size_t i = 0; i < *buf_len; i++) {
            sscanf(&line[i * 2], "%2hhx", &buf[i]); // Convert hex to byte
        }
    }
    return 1; // Success
}

// Function to create directories in a path recursively
int create_directories(const char *path) {
    char tmp[MAX_PATH];
    strncpy(tmp, path, MAX_PATH); // Copy path to temporary buffer
    for (char *p = tmp + 1; *p; p++) {
        if (*p == '/') { // For each directory separator
            *p = '\0'; // Temporarily end the string
            if (mkdir(tmp, 0755) && errno != EEXIST) { // Create directory
                log_error("Failed to create directory");
                return 0;
            }
            *p = '/'; // Restore separator
        }
    }
    return 1; // Success
}

// Function to process a single file entry in the archive
int process_file_entry(uint8_t *data, size_t *offset, size_t data_len, const char *output_dir, Endianness endian) {
    // Check if there’s enough data for the header
    if (*offset + 13 > data_len) {
        log_error("Incomplete file entry header");
        return 0;
    }

    // Read the length of the filename
    uint32_t name_len = read_uint32(&data[*offset], endian);
    *offset += 4;
    if (*offset + name_len + 17 > data_len) {
        log_error("Incomplete file entry");
        return 0;
    }

    // Read the filename
    char filename[MAX_PATH];
    strncpy(filename, (char *)&data[*offset], name_len);
    filename[name_len] = '\0'; // Null-terminate the string
    *offset += name_len;

    // Read original and processed sizes
    uint64_t orig_size = read_uint64(&data[*offset], endian);
    *offset += 8;
    uint64_t proc_size = read_uint64(&data[*offset], endian);
    *offset += 8;
    Method method = data[*offset]; // Read the processing method
    *offset += 1;

    // Check if there’s enough data for the file content
    if (*offset + proc_size > data_len) {
        log_error("Processed data exceeds archive size");
        return 0;
    }

    // Convert method to a string for reporting
    char method_str[16];
    switch (method) {
        case NO_PROCESSING: strcpy(method_str, "none"); break;
        case ZLIB: strcpy(method_str, "zlib"); break;
        case LZMA: strcpy(method_str, "lzma"); break;
        case FERNET: strcpy(method_str, "fernet"); break;
        default: log_error("Unknown processing method"); return 0;
    }

    // Write file details to the metadata report
    fprintf(report_fp, "%s\t%llu\t%llu\t%s\n", filename, orig_size, proc_size, method_str);
    if (verbose >= 1) {
        char msg[512];
        snprintf(msg, 512, "Processing %s: method=%s, orig_size=%llu, proc_size=%llu", filename, method_str, orig_size, proc_size);
        log_message(msg); // Log processing details
    }

    // Create the full output path and ensure directories exist
    char output_path[MAX_PATH];
    snprintf(output_path, MAX_PATH, "%s/%s", output_dir, filename);
    create_directories(output_path);

    // Write file data to a temporary file for processing
    FILE *temp_fp = fopen("temp.bin", "wb");
    if (!temp_fp) {
        log_error("Failed to create temp file");
        return 0;
    }
    if (method == FERNET) {
        // For FERNET, write the key and data separately
        fwrite(&data[*offset], 1, 44, temp_fp);
        fwrite(&data[*offset + 44], 1, proc_size - 44, temp_fp);
    } else {
        // For other methods, write the data as-is
        fwrite(&data[*offset], 1, proc_size, temp_fp);
    }
    fclose(temp_fp);
    *offset += proc_size;

    // Run the Python script to process the temporary file
    char cmd[512];
    snprintf(cmd, 512, "python3 process_data.py %d temp.bin %s %llu 2>&1", method, output_path, orig_size);
    FILE *pipe = popen(cmd, "r"); // Use popen to capture script output
    if (!pipe) {
        log_error("Failed to execute Python script");
        unlink("temp.bin");
        return 0;
    }

    // Read and log the Python script’s output
    char line[MAX_LINE];
    while (fgets(line, MAX_LINE, pipe)) {
        line[strcspn(line, "\n")] = 0; // Remove newline
        log_message(line); // Log each line of output
    }

    // Check the Python script’s exit status
    int ret = pclose(pipe);
    if (ret != 0) {
        log_error("Python processing failed with exit code %d", ret);
        unlink("temp.bin");
        return 0;
    }

    unlink("temp.bin"); // Remove temporary file
    return 1; // Success
}

// Main function to parse arguments and process the archive
int main(int argc, char *argv[]) {
    // Initialize default parameters
    char *input_file = NULL;
    char *output_dir = "./extracted";
    verbose = 0;

    // Parse command-line arguments
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-i") == 0 && i + 1 < argc) input_file = argv[++i];
        else if (strcmp(argv[i], "-o") == 0 && i + 1 < argc) output_dir = argv[++i];
        else if (strcmp(argv[i], "-v") == 0) verbose = (i + 1 < argc && isdigit(argv[i + 1][0])) ? atoi(argv[++i]) : 1;
    }

    // Check if input file is provided
    if (!input_file) {
        fprintf(stderr, "Usage: %s -i <input_file> [-o <output_dir>] [-v [0|1|2]]\n", argv[0]);
        return 1;
    }

    // Open log file in append mode
    log_fp = fopen(LOG_FILE, "a");
    if (!log_fp) {
        fprintf(stderr, "Failed to open log file\n");
        return 1;
    }

    // Create output directory if it doesn’t exist
    if (mkdir(output_dir, 0755) && errno != EEXIST) {
        log_error("Failed to create output directory");
        fclose(log_fp);
        return 1;
    }

    // Open metadata report file
    char report_path[MAX_PATH];
    snprintf(report_path, MAX_PATH, "%s/%s", output_dir, REPORT_FILE);
    report_fp = fopen(report_path, "w");
    if (!report_fp) {
        log_error("Failed to open report file");
        fclose(log_fp);
        return 1;
    }

    // Open the input archive file
    FILE *fp = fopen(input_file, "r");
    if (!fp) {
        log_error("Failed to open input file");
        fclose(log_fp);
        fclose(report_fp);
        return 1;
    }

    // Check if the file format is supported
    int is_hex = is_hex_file(input_file);
    int is_xxd = is_xxd_file(input_file);
    if (!is_hex && !is_xxd) {
        log_error("Unsupported file format");
        fclose(fp);
        fclose(log_fp);
        fclose(report_fp);
        return 1;
    }

    // Allocate memory to store the archive data
    uint8_t *data = NULL;
    size_t data_len = 0, data_capacity = 1024;
    data = malloc(data_capacity);
    if (!data) {
        log_error("Memory allocation failed");
        fclose(fp);
        fclose(log_fp);
        fclose(report_fp);
        return 1;
    }

    // Read the archive data into memory
    uint8_t buf[256];
    size_t buf_len;
    while (read_hex_line(fp, buf, &buf_len, is_xxd)) {
        if (data_len + buf_len > data_capacity) {
            data_capacity *= 2; // Double the capacity if needed
            uint8_t *new_data = realloc(data, data_capacity);
            if (!new_data) {
                log_error("Memory reallocation failed");
                free(data);
                fclose(fp);
                fclose(log_fp);
                fclose(report_fp);
                return 1;
            }
            data = new_data;
        }
        memcpy(&data[data_len], buf, buf_len);
        data_len += buf_len;
    }
    fclose(fp);

    // Check if the archive is large enough to contain a header
    if (data_len < 5) {
        log_error("Archive too small");
        free(data);
        fclose(log_fp);
        fclose(report_fp);
        return 1;
    }

    // Verify the magic number and determine endianness
    uint32_t magic = read_uint32(data, ENDIAN_BIG);
    Endianness endian = ENDIAN_BIG;
    if (magic != MAGIC_NUMBER) {
        magic = read_uint32(data, ENDIAN_LITTLE);
        if (magic != MAGIC_NUMBER) {
            log_error("Invalid magic number");
            free(data);
            fclose(log_fp);
            fclose(report_fp);
            return 1;
        }
        endian = ENDIAN_LITTLE;
    }

    // Read the version number directly from the archive (at offset 0x04)
    uint8_t version = data[4];
    char version_msg[64];
    snprintf(version_msg, 64, "Read version 0x%02x from archive", version);
    log_message(version_msg); // Log the version read from the file

    // Process each file entry in the archive
    size_t offset = 5; // Start after magic number and version
    while (offset < data_len) {
        if (!process_file_entry(data, &offset, data_len, output_dir, endian)) {
            log_message("Continuing after error in file entry");
        }
    }

    // Clean up resources
    free(data);
    fclose(log_fp);
    fclose(report_fp);
    return 0; // Success
}
