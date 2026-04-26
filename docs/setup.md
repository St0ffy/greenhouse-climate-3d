# Setup Guide

## What Must Be Installed

For this project you need three tools:

- `g++` or another C++17 compiler.
- `CMake` for normal project builds.
- `Git` for version control and GitHub upload.

On this machine, `g++` is available, but `cmake` and `git` are currently not found in `PATH`.

## Windows Setup

### 1. Install Git

Download Git for Windows:

```text
https://git-scm.com/download/win
```

During installation, keep the default option that adds Git to `PATH`.

After installation, restart PowerShell and check:

```powershell
git --version
```

### 2. Install CMake

Download CMake:

```text
https://cmake.org/download/
```

During installation, enable adding CMake to the system `PATH`.

After installation, restart PowerShell and check:

```powershell
cmake --version
```

### 3. Check Compiler

This project already found `g++`.

Check again:

```powershell
g++ --version
```

## First Git Commands

After Git is installed, run these commands from the project folder:

```powershell
git init
git add .
git commit -m "Initial greenhouse climate project scaffold"
```

## GitHub Connection

Create an empty repository on GitHub, for example:

```text
greenhouse-climate-3d
```

Then connect the local repository:

```powershell
git remote add origin https://github.com/YOUR_USERNAME/greenhouse-climate-3d.git
git branch -M main
git push -u origin main
```

If GitHub asks for a password, use a Personal Access Token instead of your account password.

## PuTTY / Linux Server Workflow

PuTTY is used to connect to a Linux server by SSH. The project itself should be stored on GitHub, then cloned on the server.

On the server:

```bash
git clone https://github.com/YOUR_USERNAME/greenhouse-climate-3d.git
cd greenhouse-climate-3d
cmake -S . -B build
cmake --build build
./build/greenhouse3d simulate configs/default_config.json
```

If the server does not have build tools:

```bash
sudo apt update
sudo apt install git cmake g++ build-essential
```

## Temporary Build Without CMake

Until CMake is installed, the current scaffold can be checked with:

```powershell
g++ -std=c++17 -I include src/main.cpp src/Config.cpp -o greenhouse3d.exe
.\greenhouse3d.exe simulate configs\default_config.json
```

This direct command is only for a quick check. The planned project build system is CMake.

