# librbd
## Reliability Block Diagrams (RBD) evaluation library

librbd is a high-performance RBD evaluation library that exploits both Symmetric Multi-Processing (SMP) and Single Instruction-Multiple Data (SIMD).

## Building from source

### Preprocessor symbols

To compile librbd with SMP and/or SIMD support, perform the following:
* SMP: define `CPU_SMP` preprocessor symbol with a value different from `0`
* SIMD: define `CPU_SIMD` preprocessor symbol with a value different from `0`

> **_NOTE:_** if `CPU_SMP` or `CPU_SIMD` preprocessor symbol is undefined, the corresponding optimization will be disabled by default

### Make tool

librbd can be compiled using `make` tool through the provided makefile toolchain.

#### Configure makefile

Edit `make/tools.mk` file by setting the proper tools variables `CC`, `LD` and `AR`. The default variables are:

```sh
CC := gcc
LD := gcc
AR := ar
```

Edit `make/options.mk` file by setting the proper options variables `C_FLAGS` and `C_FLAGS_SHARED`.
These variables are used to provide options and defines to C Compiler, the former one providing common options and the latter one providing options needed to generate Shared Objects only.

> **_NOTE:_** the `CPU_SMP` and `CPU_SIMD` preprocessor symbols should be defined under the `C_FLAGS` options variable.

#### Build librbd

To build the librbd as static library (archive):

```sh
cd make
make clean-archive
make archive
```

To build the librbd as dynamic library (shared object):

```sh
cd make
make clean-shared
make shared
```

To build both librbd artifacts:

```sh
cd make
make cleanall
make all
```

### Visual Studio

librbd can be compiled using Microsoft Visual Studio through the provided project (created with VS 2022).

#### Configure VS Solution

Select the proper target, e.g., Release - x64.

> **_NOTE:_** Visual Studio project currently supports only i386 (x86) and amd64 (x64) target CPUs.

##### Configure Static Library

Open Properties window of `Static` project and open the `Configuration Properties -> C/C++ -> Preprocessor` panel.
Ensure that the `Preprocessor Definitions` fields includes the `COMPILE_LIB` define and that `CPU_SMP` and `CPU_SIMD` preprocessors symbols are set as required.

##### Configure Dynamic Library

Open Properties window of `Dynamic` project and open the `Configuration Properties -> C/C++ -> Preprocessor` panel.
Ensure that the `Preprocessor Definitions` fields includes the `COMPILE_DLL` define and that `CPU_SMP` and `CPU_SIMD` preprocessors symbols are set as required.

#### Build librbd

To build the librbd as static library (archive), right-click on the `Static` project and select Rebuild.
To build the librbd as dynamic library (dynamic-link library), right-click on the `Dynamic` project and select Rebuild.

## Test application

Test application executes RBD analysis on several pre-defined Reliability Block Diagrams.
For each analyzed RBD, two text files are produced, one providing the reliability curves of the input components and the other one providing the reliability curve of the analyzed RBD.
Furthermore, the execution of each RBD is repeated several times to produce and log the statistics on its corresponding execution time.
> **_NOTE:_** librbd test application is linked to librbd static library

### Make tool

librbd test application can be compiled using `make` tool through the provided makefile toolchain.

#### Configure makefile

Edit `make/tools.mk` file by setting the proper tools variables `CC`, `LD` and `AR`. The default variables are:

```sh
CC := gcc
LD := gcc
AR := ar
```

Edit `make/options.mk` file by setting the proper options variable `C_FLAGS`.
When linking against librbd built with CPU_SMP, then `C_FLAGS` shall provide option `-pthread`.

#### Build test application

Before building the test application, ensure that the librbd is built as static library (archive).

To build the test application:

```sh
cd test/make
make clean
make all
```

#### Run test application

To run the test application:

```sh
cd test/make
./test
```

Test application executes RBD analysis on several pre-defined Reliability Block Diagrams.
For each analyzed RBD, two text files are produced, one providing the reliability curves of the input components and the other one providing the reliability curve of the analyzed RBD.
Furthermore, the execution of each RBD is repeated several times to produce and log the statistics on its corresponding execution time.

### Visual Studio

librbd test application can be compiled using Microsoft Visual Studio through the provided project (created with VS 2022).

#### Configure VS Solution

Select the proper target, e.g., Release - x64.

> **_NOTE:_** Visual Studio project currently supports only i386 (x86) and amd64 (x64) target CPUs.

##### Configure test application

Open Properties window of `Test` project and open the `Configuration Properties -> C/C++ -> Preprocessor` panel.
Ensure that the `Preprocessor Definitions` fields includes the `LINK_TO_LIB` define.

> **_NOTE:_** The `LINK_TO_LIB` define is mandatory to compile any application to the librbd static archive using Visual Studio.

#### Build test application

To build the librbd as static library (archive), right-click on the `Test` project and select Rebuild. This operation automatically rebuilds also the librbd static library (archive).

#### Run test application

To run the test application:

```sh
cd vs\<ARCH>\<BUILD>
Test.exe
```

Where:
* `<ARCH>` is the target architecture, i.e., `Win32` or `x64`.
* `<BUILD>` is the selected build type, i.e., `Debug` or `Release`.

## License

This program is free software: you can redistribute it and/or modify it under the terms of the GNU Affero General Public License as published by the Free Software Foundation, either version 3 of the License, or any later version.

This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Affero General Public License for more details.

You should have received a copy of the GNU Affero General Public License along with this program.  If not, see <https://www.gnu.org/licenses/>.

