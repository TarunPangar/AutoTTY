# AutoTTY - U-Boot & Linux Boot Automation Tool

**AutoTTY** is a lightweight C-based automation tool that interacts with embedded Linux systems over UART/TTY. It is designed to automatically send bootloader commands, handle Linux login sequences, and execute shell commands during embedded board bring-up or testing.

## Features

- Automates U-Boot and Linux console interaction via serial port
- Configurable boot/login command sequence via a `commands.cfg` file
- Synchronized UART read/write using POSIX threads and mutexes
- Logs serial output to terminal in real-time
- Built entirely in C using low-level Linux system calls

## Requirements

- Linux-based host system
- POSIX-compatible C compiler (e.g., gcc)
- Access to a UART device

## Installation and Compilation
```sh
# Clone the repository
git clone https://github.com/TarunPangar/AutoTTY.git
cd AutoTTY

# Compile the program
make
```

## Project Structure
```
AutoTTY/
├── main.c            # Main execution loop, handles file open, serial device open, thread initializtion.
├── serial.c/h        # Low-level serial communication functions
├── parser.c/h        # Parses command configuration file
├── common.h          # Shared macros, configuration settings
├── commands.cfg      # Config file with boot and shell commands
├── Makefile          # Build automation script
├── README.md         # Documentation (this file)
└── LICENSE           # License information
```

## Usage
### Running the Application
#### 1. Prepare `commands.cfg` file
`commands.cfg` file should include all serial commands (e.g., bootloader commands, Linux shell commands), each on a new line. Below is a sample structure:
```bash
[u-boot]
> sf probe
> mtd list
> boot

[linux]
> echo Hello
> ifconfig
> ./run_script.sh
```
- The [u-boot] section is for bootloader commands.
- The [linux] section is for Linux shell commands.

#### 2. Configure the serial terminal and debug print statements
In `common.h`, 
- Edit TTY_PATH with the appropriate TTY device name.
- Enable ENABLE_STDOUT_PRINTS if debug print statements are required to print on stdout.
For example,
```bash
#define TTY_PATH "/dev/ttyUSB2"
#define ENABLE_STDOUT_PRINTS (1U)
```

#### 3. Run the tool
```bash
./autotty
```
This will print all the messages coming from serial device to a file `log_file.txt`.

### Clean
To remove compiled files and reset the environment:
```bash
make clean
```

## Use Cases
- Board bring-up and firmware verification
- U-Boot automation and kernel boot testing
- CI pipelines for embedded systems

## Contributing
1. Fork the repository
2. Create a new branch (`git checkout -b feature-branch`)
3. Commit your changes (`git commit -m "Add new feature"`)
4. Push to the branch (`git push origin feature-branch`)
5. Open a Pull Request

## License

This project is licensed under the [MIT License](LICENSE).
