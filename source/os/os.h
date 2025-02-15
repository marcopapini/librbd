/*
 *  Component: os.h
 *  Retrieval of target OS
 *
 *  librbd - Reliability Block Diagrams evaluation library
 *  Copyright (C) 2020-2024 by Marco Papini <papini.m@gmail.com>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Affero General Public License as published
 *  by the Free Software Foundation, either version 3 of the License, or
 *  any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Affero General Public License for more details.
 *
 *  You should have received a copy of the GNU Affero General Public License
 *  along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef OS_H_
#define OS_H_

#if     defined(_WIN32)     || defined(_WIN64)      || defined(__CYGWIN__)
/* Windows OS */
#define OS_WINDOWS
#elif   defined(__APPLE__)  || defined(__MACH__)
/* MacOS OS */
#define OS_MACOS
#elif   defined(__linux__)
/* Linux OS */
#define OS_LINUX
#else
/* Unknown OS */
#define OS_UNKNOWN
#endif


long retrieveNumberOfCores();


#endif /* OS_H_ */
