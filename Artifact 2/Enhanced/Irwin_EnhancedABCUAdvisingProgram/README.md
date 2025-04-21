# Enhanced ABCU Advising Program

## Summary

This project enhancement implements a course advising system for ABCU that uses an **AVL Binary Search Tree (BST)** to store and retrieve course data efficiently. The original version of the program used a standard BST, which could become unbalanced in real-world scenarios, leading to degraded performance. This enhancement introduces AVL self-balancing, duplicate detection, smart pointers for memory safety, and structured exception handling to improve reliability and maintainability.

The application supports CSV file input, in-order course traversal, and course search functionality. It also includes a command-line interface for loading and exploring course data interactively.

## Setup Instructions

### Prerequisites

#### Software:
- C++17 Compatible Compiler (e.g., `g++`, `clang++`)
- Make or build system (optional)
- Terminal or Command Prompt

This program was made in `Visual Studio 2022` and includes a solution file if that's your preferred method of editing and compiling this program.

#### Files:
- `EnhancedABCUAdvisingProgram.cpp`
- A valid CSV file (default: `ABCU_Advising_Program_Input_Extended.csv`) structured as:
```<courseId>,<courseName>,<prerequisite1>,<prerequisite2>,...```

## Usage

1. **Compile the Program**

 ```
 g++ -std=c++17 -o coursePlanner EnhancedABCUAdvisingProgram.cpp
```

2. **Run the Program**

 ```
./coursePlanner
```

