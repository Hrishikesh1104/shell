# Custom Shell Project

A feature-rich custom shell implementation written in C++ with a web-based interface. This project includes a fully functional shell with built-in commands, tab completion, command history, pipelines, and I/O redirection, accessible through both a terminal interface and a web browser.

## Table of Contents

- [Features](#features)
- [Project Structure](#project-structure)
- [Prerequisites](#prerequisites)
- [Building the Project](#building-the-project)
  - [Local Build](#local-build)
  - [Docker Build](#docker-build)
- [Usage](#usage)
  - [Terminal Usage](#terminal-usage)
  - [Web Interface](#web-interface)
- [Shell Features](#shell-features)
  - [Built-in Commands](#built-in-commands)
  - [Tab Completion](#tab-completion)
  - [Command History](#command-history)
  - [Pipelines](#pipelines)
  - [I/O Redirection](#io-redirection)
  - [Quote Handling](#quote-handling)
- [Configuration](#configuration)
- [Deployment](#deployment)
  - [Local Deployment](#local-deployment)
  - [AWS EC2 Deployment](#aws-ec2-deployment)

## Features

### Shell Features
- **Built-in Commands**: `echo`, `exit`, `type`, `pwd`, `cd`, `history`
- **Tab Auto-completion**: Intelligent completion for built-ins and PATH executables
- **Command History**: Persistent history with readline integration
- **Pipelines**: Support for multi-stage command pipelines (`cmd1 | cmd2 | cmd3`)
- **I/O Redirection**: Standard output/error redirection with append support
- **Quote Handling**: Proper parsing of single quotes, double quotes, and backslash escaping
- **PATH Resolution**: Automatic search for executables in PATH directories
- **External Command Execution**: Fork-exec model for running system commands

### Web Interface Features
- **WebSocket-based Terminal**: Real-time terminal emulation in the browser
- **Access Control**: Key-based authentication for security
- **Session Isolation**: Each connection gets an isolated temporary directory
- **Cross-platform**: Works on any system with Docker support

## Project Structure

shell-project/
  shell.cpp            # Main C++ shell implementation
  shell                # Compiled shell executable (generated)
  Makefile             # Build configuration for local development
  Dockerfile           # Container build configuration
  README.md            # This file
  server/              # Node.js web server
      package.json     # Node.js dependencies
      server.js        # Express + WebSocket server
      public/
          index.html   # Web frontend interface

## Prerequisites

### For Local Development
- **C++ Compiler**: `clang++` or `g++` with C++17 or later support
- **readline library**: GNU readline development headers
  - macOS: `brew install readline`
  - Ubuntu: `apt-get install libreadline-dev`
- **Node.js**: Version 20.x or later
- **npm**: Comes with Node.js

### For Docker
- **Docker**: Version 20.10 or later

## Building the Project

### Local Build

1. **Install readline** (if not already installed):
   ```bash
   # macOS
   brew install readline
   
   # Ubuntu/Debian
   sudo apt-get install libreadline-dev libncurses5-dev
   ```

2. **Update Makefile** (if needed):
   - The Makefile assumes readline is installed at `/opt/homebrew/opt/readline` (macOS Homebrew default)
   - For other systems, update the `READLINE_PREFIX` variable in the Makefile

3. **Build the shell**:
   ```bash
   make
   ```

4. **Test the shell**:
   ```bash
   ./shell
   ```

5. **Build the web server**:
   ```bash
   cd server
   npm install
   ```

### Docker Build

1. **Create a `.env` file** in the project root:
   ```bash
   ACCESS_KEY=your-secret-key-here
   ```

2. **Build the Docker image**:
   ```bash
   docker build -t shell-project .
   ```

3. **Run the container**:
   ```bash
   docker run -p 3000:3000 --env-file .env shell-project
   ```

The server will be accessible at `http://localhost:3000`.

## Usage

### Terminal Usage

Run the shell directly:
```bash
./shell
```

You'll see the prompt:
```
[MY CUSTOM SHELL IS RUNNING]
$ 
```

#### Example Commands

```bash
# Built-in commands
$ echo Hello World
Hello World

$ pwd
/Users/hrishi/Documents/C++/shell-project

$ cd ~/Documents
$ pwd
/Users/hrishi/Documents

# Command history
$ history 5
    10  echo Hello
    11  pwd
    12  cd ~/Documents
    13  pwd
    14  history 5

# Type checking
$ type echo
echo is a shell builtin

$ type ls
ls is /usr/bin/ls

# Pipelines
$ echo "hello world" | tr '[:lower:]' '[:upper:]'
HELLO WORLD

$ ls -la | grep shell
-rwxr-xr-x  1 user  staff  123456 shell

# I/O Redirection
$ echo "test" > output.txt
$ cat output.txt
test

$ echo "append" >> output.txt
$ cat output.txt
test
append

# Error redirection
$ ls nonexistent 2> error.log
$ cat error.log
ls: nonexistent: No such file or directory
```

### Web Interface

**Live Deployment:**
- **Website**: [http://35.154.38.68:3000/](http://35.154.38.68:3000/)
- **Access Key**: Please contact me for the access key

**Local Setup:**

1. **Start the server** (locally or via Docker):
   ```bash
   # Local
   cd server
   npm start
   
   # Docker
   docker run -p 3000:3000 --env-file .env shell-project
   ```

2. **Open your browser** and navigate to `http://localhost:3000`

3. **Enter the access key** when prompted (must match the `ACCESS_KEY` environment variable)

4. **Use the shell** in your browser - type commands and see output in real-time

## Shell Features

### Built-in Commands

#### `echo [arguments...]`
Prints its arguments to standard output.

```bash
$ echo Hello World
Hello World
$ echo "Hello    World"
Hello    World
```

#### `exit`
Terminates the shell. History is automatically saved to `HISTFILE` if set.

#### `type <command>`
Identifies whether a command is a builtin or an external executable.

```bash
$ type echo
echo is a shell builtin
$ type ls
ls is /usr/bin/ls
$ type nonexistent
nonexistent: not found
```

#### `pwd`
Prints the current working directory.

```bash
$ pwd
/Users/hrishi/Documents/C++/shell-project
```

#### `cd [directory]`
Changes the current working directory. Supports `~` for home directory.

```bash
$ cd ~/Documents
$ pwd
/Users/hrishi/Documents
$ cd /tmp
$ pwd
/tmp
```

#### `history [n]`
Displays command history. Without arguments, shows all history. With a number, shows the last `n` commands.

**Options:**
- `history -r <file>`: Read history from a file
- `history -w <file>`: Write history to a file (overwrite)
- `history -a <file>`: Append new history entries to a file

```bash
$ history 5
    10  echo Hello
    11  pwd
    12  cd ~/Documents
    13  pwd
    14  history 5
```

### Tab Completion

The shell provides intelligent tab completion:

1. **Single match**: Automatically completes the command
2. **Multiple matches**: First tab extends to the longest common prefix, second tab lists all matches
3. **No matches**: Produces a bell sound

**Completion sources:**
- Built-in commands: `echo`, `exit`, `history`
- Executables in PATH directories

**Example:**
```bash
$ ec<TAB>        # Completes to "echo "
$ hi<TAB><TAB>   # Lists: history
$ ls<TAB>        # Completes if unique, or lists matches
```

### Command History

The shell maintains a persistent command history using GNU readline:

- **Automatic saving**: History is saved to `HISTFILE` on exit
- **Automatic loading**: History is loaded from `HISTFILE` on startup
- **Navigation**: Use arrow keys (↑/↓) to navigate history
- **History file**: Set `HISTFILE` environment variable to specify the history file location

**Example:**
```bash
export HISTFILE=~/.myshell_history
./shell
# History will be saved/loaded from ~/.myshell_history
```

### Pipelines

The shell supports multi-stage pipelines using the `|` operator:

```bash
$ echo "hello world" | tr '[:lower:]' '[:upper:]'
HELLO WORLD

$ ls -la | grep shell | wc -l
1

$ cat file.txt | sort | uniq
```

**How it works:**
- Each command in the pipeline runs in a separate process
- Standard output of one command is connected to standard input of the next
- All commands run concurrently
- The shell waits for all processes to complete

### I/O Redirection

The shell supports standard output and error redirection:

**Standard Output:**
- `>` or `1>`: Redirect stdout to a file (overwrite)
- `>>` or `1>>`: Redirect stdout to a file (append)

**Standard Error:**
- `2>`: Redirect stderr to a file (overwrite)
- `2>>`: Redirect stderr to a file (append)

**Examples:**
```bash
$ echo "test" > output.txt
$ echo "more" >> output.txt
$ cat output.txt
test
more

$ ls nonexistent 2> error.log
$ cat error.log
ls: nonexistent: No such file or directory

$ ls > output.txt 2> error.log
```

### Quote Handling

The shell properly handles quotes and escaping:

- **Single quotes (`'`)**: Preserves all characters literally
- **Double quotes (`"`)**: Allows variable expansion and command substitution (if implemented)
- **Backslash (`\`)**: Escapes special characters

**Examples:**
```bash
$ echo 'Hello World'
Hello World

$ echo "Hello    World"
Hello    World

$ echo 'It'\''s a test'
It's a test

$ echo "He said \"Hello\""
He said "Hello"
```

## Configuration

### Environment Variables

- **`HISTFILE`**: Path to the history file (default: not set, history not persisted)
  ```bash
  export HISTFILE=~/.myshell_history
  ```

- **`PATH`**: Search path for executables (standard Unix PATH)
  ```bash
  export PATH=/usr/local/bin:/usr/bin:/bin
  ```

- **`HOME`**: Home directory (used by `cd ~`)
  ```bash
  export HOME=/Users/hrishi
  ```

### Server Configuration

Create a `.env` file in the project root:

```env
ACCESS_KEY=your-secret-key-here
PORT=3000
```

The `ACCESS_KEY` is required to access the web interface. The `PORT` is optional and defaults to 3000.

## Deployment

### Local Deployment

1. **Install dependencies**:
   ```bash
   # Install readline
   brew install readline  # macOS
   # or
   sudo apt-get install libreadline-dev  # Ubuntu
   
   # Install Node.js dependencies
   cd server
   npm install
   cd ..
   ```

2. **Build the shell**:
   ```bash
   make
   ```

3. **Set environment variables**:
   ```bash
   export ACCESS_KEY="your-secret-key"
   ```

4. **Start the server**:
   ```bash
   cd server
   npm start
   ```

5. **Access the shell**:
   - Web interface: `http://localhost:3000`
   - Terminal: `./shell`

### AWS EC2 Deployment

This project can be deployed on AWS EC2 using Docker. Follow these steps:

#### 1. Launch an EC2 Instance

- **AMI**: Ubuntu 22.04 LTS (or Amazon Linux 2)
- **Instance Type**: `t3.micro` (free tier eligible) or `t3.small` for better performance
- **Storage**: 20GB (default is sufficient)
- **Security Group**: Allow inbound traffic on:
  - Port 22 (SSH) - from your IP
  - Port 3000 (HTTP) - from 0.0.0.0/0 or your IP range
  - Port 443 (HTTPS) - if using SSL (recommended for production)

#### 2. Connect to Your EC2 Instance

```bash
# Using SSH (replace with your key and instance IP)
ssh -i your-key.pem ubuntu@<EC2-Public-IP>   # Ubuntu
```

#### 3. Install Docker and Docker Compose

**Ubuntu 22.04:**
```bash
sudo apt-get update
sudo apt-get install -y docker.io docker-compose-plugin
sudo usermod -aG docker $USER
newgrp docker
```

#### 4. Clone or Upload the Project

```bash
# Clone from GitHub (if available)
git clone https://github.com/your-username/shell-project.git
cd shell-project

# Or upload using SCP
scp -i your-key.pem -r ./shell-project ubuntu@<EC2-Public-IP>:~/
```

#### 5. Configure Environment Variables

```bash
cd shell-project

# Create .env file
cat > .env << EOF
ACCESS_KEY=$(openssl rand -base64 32)
PORT=3000
EOF

# View the generated key (save it securely)
cat .env
```

#### 6. Build and Run with Docker

```bash
# Build the Docker image
sudo docker build -t shell-project .

# Run the container
sudo docker run -d -p 3000:3000 --env-file .env --restart unless-stopped --name shell-server shell-project
```

#### 7. Verify the Deployment

```bash
# Check if the container is running
sudo docker ps

# View logs
sudo docker logs shell-server

# Test the server
curl http://localhost:3000
```

#### 8. Access Your Shell

- **Web Interface**: `http://<EC2-Public-IP>:3000`
- **Enter the Access Key**: Use the key from `.env` file
