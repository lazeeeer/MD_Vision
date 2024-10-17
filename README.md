# MD_Vision Project

School: University Of Western Ontario

Project Contributors:
- Lazar Krstic
- Steven Lim
- Luka Labus

---
This is a repo for our 2024 Electrical and Computer Engineering capstone project: MD_Vision

This is the simplest buildable example. The example is used by command `idf.py create-project`
that copies the project to user specified path and set it's name. For more information follow the [docs page](https://docs.espressif.com/projects/esp-idf/en/latest/api-guides/build-system.html#start-a-new-project)

## How to build the project:
ESP-IDF projects are built using CMake. The project build configuration is contained in `CMakeLists.txt`
files that provide set of directives and instructions describing the project's source files and targets
(executable, library, or both). 

## Required Libraries and IDF-Components
- u8g2 monochrome graphics library - https://github.com/olikraus/u8g2
- esp32-camera - https://github.com/espressif/esp32-camera

Please clone the following libraries into the components folder using the commands:
git clone https://github.com/espressif/esp32-camera ./components/esp32-camera
git clone https://github.com/olikraus/u8g2 ./components/u8g2


## Project Structure / Organization
```
├── CMakeLists.txt
├── main
│   ├── CMakeLists.txt
│   └── main.c
└── README.md                  This is the file you are currently reading
```
