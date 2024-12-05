/*
 *  Component: macos.c
 *  OS management - MacOS implementation
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

#if defined(OS_MACOS)

#include <sys/param.h>
#include <sys/sysctl.h>

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
 *  This function retrieves the number of cores on MacOS
 *
 * Parameters:
 *      None
 *
 * Return:
 *      None
 */
HIDDEN long retrieveNumberOfCores()
{
    int nm[2];
    size_t len = 4;
    uint32_t count;

    /* Retrieve number of cores using the proper MacOS API */
    nm[0] = CTL_HW;
    nm[1] = HW_AVAILCPU;
    sysctl(nm, 2, &count, &len, NULL, 0);

    if (count < 1) {
        nm[1] = HW_NCPU;
        sysctl(nm, 2, &count, &len, NULL, 0);
    }

    return (long)count;
}

#endif /* defined(OS_MACOS) */
