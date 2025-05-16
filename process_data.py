import sys
import zlib
import lzma
from cryptography.fernet import Fernet
import base64
import logging
import os

# Configure logging to write to archextract.log with a simple format
logging.basicConfig(filename='archextract.log', level=logging.INFO, format='%(message)s')

# Function to log a message to both the log file and console if verbose mode is on
def log_message(msg):
    logging.info(msg)  # Write message to log file
    if verbose >= 1:  # Check if verbose mode is enabled
        print(msg)    # Display message on console

# Function to log an error to both the log file and stderr
def log_error(msg):
    logging.error(f"ERROR: {msg}")  # Write error to log file
    print(f"ERROR: {msg}", file=sys.stderr)  # Display error on console

# Function to validate if a Fernet key is correctly formatted
def validate_fernet_key(key):
    try:
        decoded_key = base64.urlsafe_b64decode(key)  # Decode the key
        if len(decoded_key) not in [16, 24, 32]:  # Check key length
            return False
        return True
    except Exception:
        return False  # Return False if decoding fails

# Function to process data with no compression or encryption
def process_none(data, expected_size):
    if len(data) != expected_size:  # Check if data matches expected size
        log_error("Data size mismatch for no processing")
        return None
    return data  # Return original data if sizes match

# Function to decompress data using zlib
def process_zlib(data, expected_size):
    try:
        decompressed = zlib.decompress(data)  # Decompress the data
        if len(decompressed) != expected_size:  # Verify decompressed size
            log_error("Zlib decompressed size mismatch")
            return None
        return decompressed  # Return decompressed data
    except zlib.error as e:  # Handle decompression errors
        log_error(f"Zlib decompression failed: {e}")
        return None

# Function to decompress data using lzma
def process_lzma(data, expected_size):
    try:
        decompressed = lzma.decompress(data)  # Decompress the data
        if len(decompressed) != expected_size:  # Verify decompressed size
            log_error("LZMA decompressed size mismatch")
            return None
        return decompressed  # Return decompressed data
    except lzma.LZMAError as e:  # Handle decompression errors
        log_error(f"LZMA decompression failed: {e}")
        return None

# Function to decrypt data using Fernet
def process_fernet(data, expected_size):
    if len(data) < 44:  # Check if data contains a valid key (minimum 44 bytes)
        log_error("Fernet data too short for key")
        return None
    key = data[:44]  # Extract the key from the first 44 bytes
    if not validate_fernet_key(key):  # Validate the key format
        log_error("Invalid Fernet key")
        return None
    try:
        f = Fernet(key)  # Create Fernet object with the key
        decrypted = f.decrypt(data[44:])  # Decrypt the remaining data
        if len(decrypted) != expected_size:  # Verify decrypted size
            log_error("Fernet decrypted size mismatch")
            return None
        log_message(f"Decrypted file with key: {key.decode()}")  # Log decryption success
        return decrypted  # Return decrypted data
    except Exception as e:  # Handle decryption errors
        log_error(f"Fernet decryption failed: {e}")
        return None

if __name__ == "__main__":
    # Check if the correct number of arguments is provided
    if len(sys.argv) != 5:
        print("Usage: python3 process_data.py <method> <input_file> <output_file> <expected_size>", file=sys.stderr)
        sys.exit(1)

    # Extract command-line arguments
    method = int(sys.argv[1])  # Processing method (0: none, 1: zlib, 2: lzma, 3: fernet)
    input_file = sys.argv[2]   # Input file path
    output_file = sys.argv[3]  # Output file path
    expected_size = int(sys.argv[4])  # Expected size of processed data
    verbose = 1  # Set verbose mode to match C program's default

    # Read the input file as binary data
    with open(input_file, 'rb') as f:
        data = f.read()

    # Process the data based on the specified method
    result = None
    if method == 0:
        result = process_none(data, expected_size)
    elif method == 1:
        result = process_zlib(data, expected_size)
    elif method == 2:
        result = process_lzma(data, expected_size)
    elif method == 3:
        result = process_fernet(data, expected_size)
    else:
        log_error("Unknown processing method")  # Handle invalid method
        sys.exit(1)

    # Exit if processing failed
    if result is None:
        sys.exit(1)

    # Create output directory if it doesn’t exist
    os.makedirs(os.path.dirname(output_file), exist_ok=True)
    
    # Write the processed data to the output file
    try:
        with open(output_file, 'wb') as f:
            f.write(result)  # Save the processed data
    except Exception as e:  # Handle file writing errors
        log_error(f"Failed to write output file {output_file}: {e}")
        sys.exit(1)
