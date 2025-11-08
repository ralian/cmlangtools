# cmLangTools: Proof of concept CMake Linter

- Right now it uses the headers and sources straight from CMake responsible for CML parsing
- Tested on Win32 but should work on non-Win32 with minor modifications
- It might not be terribly hard to use the bison grammar from the CMake repo, but ditch the CMake sources for the sake of decoupling

## Build & Run

```
cmake --preset debug .
cmake --build --preset debug
./build/debug/cmlangtools.exe test1.cmake
```

Right now this just outputs the following:

```
Command: cmake_minimum_required
  Argument: VERSION
  Argument: 3.24
Command: project
  Argument: testdummy
  Argument: LANGUAGES
  Argument: CXX
Command: message
  Argument: TEST
```
