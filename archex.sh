#!/bin/bash

# Define default values for the script
INPUT_FILE=""
OUTPUT_DIR="./extracted"
VERBOSE_MODE="0"

# Set up history file for command recall
HISTORY_FILE="$HOME/.archex_history"
touch "$HISTORY_FILE"  # Create history file if it doesn't exist
history -r "$HISTORY_FILE"  # Load previous history from file

# Function to display the ASCII art banner
show_banner() {
    echo ""
    echo "          █████╗ ██████╗  ██████╗██╗  ██╗███████╗██╗  ██╗"
    echo "         ██╔══██╗██╔══██╗██╔════╝██║  ██║██╔════╝╚██╗██╔╝"
    echo "         ███████║██████╔╝██║     ███████║█████╗   ╚███╔╝ "
    echo "         ██╔══██║██╔══██╗██║     ██╔══██║██╔══╝   ██╔██╗ "
    echo "         ██║  ██║██║  ██║╚██████╗██║  ██║███████╗██╔╝ ██╗"
    echo "         ╚═╝  ╚═╝╚═╝  ╚═╝ ╚═════╝╚═╝  ╚═╝╚══════╝╚═╝  ╚═╝"
    echo ""
    echo "Archive Extractor CLI (archex)"
    echo "=============================="
}

# Function to display help message with available commands
show_help() {
    echo "Available commands:"
    echo "  help                - Show this help message"
    echo "  find [/directory]   - List all .hex and .txt files in the current directory or specified directory"
    echo "  set fn <filename>   - Set the input archive file (e.g., archive_be.hex)"
    echo "  set o <directory>   - Set the output directory (e.g., ./big_hex)"
    echo "  set v <mode>        - Set the verbose mode (0, 1, or 2)"
    echo "  show                - Show current parameter settings"
    echo "  run                 - Run the unarchive process with current settings"
    echo "  exit                - Exit the CLI"
    echo ""
    echo "Usage example:"
    echo "  find /path/to/dir"
    echo "  set fn archive_be.hex"
    echo "  set o big_hex"
    echo "  set v 2"
    echo "  run"
}

# Function to display current parameter settings
show_params() {
    echo "Current Parameters:"
    echo "  Input File      : $INPUT_FILE"
    echo "  Output Directory: $OUTPUT_DIR"
    echo "  Verbose Mode    : $VERBOSE_MODE"
}

# Function to list .hex and .txt files in a specified or current directory
show_files() {
    local search_dir="$1"  # Directory to search, if provided
    # Use current directory if no directory is specified
    if [ -z "$search_dir" ]; then
        search_dir="."
    fi
    # Check if the directory exists
    if [ ! -d "$search_dir" ]; then
        echo "Error: Directory '$search_dir' does not exist."
        return
    fi
    echo "Available archive files (.hex and .txt) in $search_dir:"
    # Find files with .hex or .txt extensions in the directory
    files=$(find "$search_dir" -maxdepth 1 -type f \( -name "*.hex" -o -name "*.txt" \) | sort)
    if [ -z "$files" ]; then
        echo "No .hex or .txt files found in $search_dir."
    else
        # Display only filenames without full path
        echo "$files" | while read -r file; do
            basename "$file"
        done
    fi
}

# Clear screen and display banner when script starts
clear
show_banner
echo "Type 'help' for available commands"

# Set the command prompt
PROMPT="archex> "

while true; do
    # Read user input with readline support and preserve spaces
    IFS='' read -e -p "$PROMPT" -r input
    # Add the command to history
    history -s "$input"
    history -w "$HISTORY_FILE"  # Save history to file

    # Split input into command and arguments
    read -r cmd arg1 arg2 <<< "$input"

    case $cmd in
        help)
            show_help  # Display help message
            ;;
        find)
            show_files "$arg1"  # List files in specified directory
            ;;
        set)
            case $arg1 in
                fn)
                    if [ -z "$arg2" ]; then
                        echo "Error: Please specify a filename (e.g., set fn archive_be.hex)"
                    else
                        INPUT_FILE="$arg2"  # Set input file
                        echo "Input file set to: $INPUT_FILE"
                    fi
                    ;;
                o)
                    if [ -z "$arg2" ]; then
                        echo "Error: Please specify an output directory (e.g., set o big_hex)"
                    else
                        OUTPUT_DIR="$arg2"  # Set output directory
                        echo "Output directory set to: $OUTPUT_DIR"
                    fi
                    ;;
                v)
                    if [ -z "$arg2" ] || ! [[ "$arg2" =~ ^[0-2]$ ]]; then
                        echo "Error: Please specify a verbose mode (0, 1, or 2) (e.g., set v 2)"
                    else
                        VERBOSE_MODE="$arg2"  # Set verbose mode
                        echo "Verbose mode set to: $VERBOSE_MODE"
                    fi
                    ;;
                *)
                    echo "Unknown parameter. Use 'help' to see available commands."
                    ;;
            esac
            ;;
        show)
            show_params  # Display current settings
            ;;
        run)
            if [ -z "$INPUT_FILE" ]; then
                echo "Error: Input file not set. Use 'set fn <filename>' to set it."
                continue
            fi
            if [ -z "$OUTPUT_DIR" ]; then
                echo "Error: Output directory not set. Use 'set o <directory>' to set it."
                continue
            fi
            echo "Starting unarchive process..."
            # Build and execute the archex command (without -vn)
            command="./archex -i $INPUT_FILE -o $OUTPUT_DIR -v $VERBOSE_MODE"
            echo "Executing: $command"
            $command
            if [ $? -eq 0 ]; then
                echo "Unarchive completed successfully. Check $OUTPUT_DIR for extracted files."
            else
                echo "Unarchive failed. Check archextract.log for details."
            fi
            ;;
        exit)
            echo "Exiting archex CLI. Goodbye!"  # Exit the script
            break
            ;;
        *)
            if [ -n "$cmd" ]; then
                echo "Unknown command. Type 'help' for available commands."
            fi
            ;;
    esac
done
