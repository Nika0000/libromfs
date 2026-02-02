# libromfs

![CI Tests](https://github.com/Nika0000/libromfs/workflows/CI%20Tests/badge.svg)
[![CI Build](https://github.com/Nika0000/libromfs/actions/workflows/build.yml/badge.svg)](https://github.com/Nika0000/libromfs/actions/workflows/build.yml)

libromfs is an easy way to bundle files, resources and entire directory structures directly into any C++ application and access them through a simple interface. 

The main advantage of this library is that you no longer need to ship your application with a resources folder that contains data that is not intended to be modified anyways. Simply bundle it all into a single, standalone executable.

## Usage

libromfs depends on CMake for code generation.

```cmake
# Set the project name this romfs is being used for
set(LIBROMFS_PROJECT_NAME ${PROJECT_NAME})

# Set the location of our romfs folder
set(LIBROMFS_RESOURCE_LOCATION "${CMAKE_SOURCE_DIR}/romfs")

# Optional: Enable zlib compression (requires zlib, see COMPRESSION.md)
# set(LIBROMFS_COMPRESS_RESOURCES ON)

# Include libromfs
add_subdirectory(libromfs)

# Link libromfs into our application
target_link_libraries(my_application PUBLIC ${LIBROMFS_LIBRARY})
```

Now, you can place all your files and folders into the `./romfs` folder next to your `CMakeLists.txt` file. 
All files in this folder will now be packed into the `.rodata` section of a static library which then can be linked to your main application.

### Excluding Files with .romfsignore

You can optionally create a `.romfsignore` file in your resource folder to exclude specific files and directories from being embedded. The syntax is similar to `.gitignore`:

```sh
# .romfsignore - Exclude files from ROM filesystem
# Lines starting with # are comments
# Blank lines are ignored

# Exclude all Python files
**/*.py

# Exclude markdown documentation  
**/*.md

# Exclude entire folders
docs/**
.git/**
```

### Accessing Files

To access the files in the `./romfs` directory structure now, simply use `romfs::get` to get back an object containing functions to access the file's data.
The interface is comparable to regular C File I/O, `romfs::get()` is the counterpart to `fopen()` and the data access functions inside of the object are counterparts to `fread()`.

```cpp
/* Include libromfs header */
#include <romfs/romfs.hpp>

#include <cstdio>

int main() {
  /* Get file relative to the romfs folder */
  auto my_file = romfs::get("path/to/my/file.txt"); 
  
  /* Print size and content of file */
  std::printf("File Size: %zu bytes\n", my_file.size());
  std::printf("File content: %s\n", my_file.data());
}
```
