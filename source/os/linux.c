/*
 *  Component: linux.c
 *  OS management - Linux implementation
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

#include "os.h"

#if defined(OS_LINUX)

#include <unistd.h>

#include "../compiler/compiler.h"


/**
 * retrieveNumberOfCores
 *
 * Retrieve number of cores
 *
 * Input:
 *      None
 *
 * Output:
 *      None
 *
 * Description:
 *  This function retrieves the number of cores on Linux
 *
 * Parameters:
 *      None
 *
 * Return:
 *      None
 */
HIDDEN long retrieveNumberOfCores()
{
    long int count;

    /* Retrieve number of cores using the proper Linux API */
    count = sysconf(_SC_NPROCESSORS_ONLN);

    return (long)count;
}

#endif /* defined(OS_LINUX) */
