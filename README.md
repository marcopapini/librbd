# librbd
## Reliability Block Diagrams (RBD) evaluation library

librbd is a high-performance RBD evaluation library that exploits both Symmetric Multi-Processing (SMP) and Single Instruction-Multiple Data (SIMD).

## Building from source

### Make tool

#### Configure makefile

Edit `make/tools.mk` file by setting the proper tools variables `CC`, `LD` and `AR`. The default variables are:

```sh
CC := gcc
LD := gcc
AR := ar
```

Edit `make/options.mk` file by setting the proper options variables `C_FLAGS` and `C_FLAGS_SHARED`.
These variables are used to provide options and defines to C Compiler, the former one providing common options and the latter one providing options needed to generate Shared Objects only.

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

## Test application

### Make tool

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

## License

This program is free software: you can redistribute it and/or modify it under the terms of the GNU Affero General Public License as published by the Free Software Foundation, either version 3 of the License, or any later version.

This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Affero General Public License for more details.

You should have received a copy of the GNU Affero General Public License along with this program.  If not, see <https://www.gnu.org/licenses/>.

