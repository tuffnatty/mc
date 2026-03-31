/*
   lib/vfs - test vfs_clone_file() functionality

   Copyright (C) 2026
   Free Software Foundation, Inc.

   Written by:
   Phil Krylov <phil@krylov.eu>, 2026

   This file is part of the Midnight Commander.

   The Midnight Commander is free software: you can redistribute it
   and/or modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation, either version 3 of the License,
   or (at your option) any later version.

   The Midnight Commander is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#define TEST_SUITE_NAME "/lib/vfs"

#include "tests/mctest.h"

#include <stdarg.h>
#include <stdlib.h>

#ifdef __linux__
#include <linux/fs.h>   // FICLONE
#include <sys/ioctl.h>  // ioctl()
#endif

#include "lib/strutil.h"
#include "src/vfs/local/local.c"

/* --------------------------------------------------------------------------------------------- */

static int clone_syscall__call_count = 0;
static gboolean clone_syscall__call_arguments_are_proper = FALSE;

#ifdef __FreeBSD__
/* @Mock */
ssize_t
copy_file_range (int infd, off_t *inoffp, int outfd, off_t *outoffp, size_t len, unsigned int flags)
{
    (void) infd;
    (void) inoffp;
    (void) outfd;
    (void) outoffp;
    (void) len;

    clone_syscall__call_count++;
    clone_syscall__call_arguments_are_proper = (flags == COPY_FILE_RANGE_CLONE);

    return -1;
}
#endif

#ifdef __linux__
#ifdef __GLIBC__
int
ioctl (int fd, unsigned long request, ...)
#else  // POSIX, musl
int
ioctl (int fd, int request, ...)
#endif
{
    (void) fd;

    clone_syscall__call_count++;
    clone_syscall__call_arguments_are_proper = (request == FICLONE);
    return -1;
}
#endif

/* --------------------------------------------------------------------------------------------- */

/* @Before */
static void
setup (void)
{
    str_init_strings (NULL);

    vfs_init ();
    vfs_init_localfs ();
    vfs_setup_work_dir ();
}

/* --------------------------------------------------------------------------------------------- */

/* @After */
static void
teardown (void)
{
    vfs_shut ();
    str_uninit_strings ();
}

/* --------------------------------------------------------------------------------------------- */

#if defined(__linux__) || defined(__FreeBSD__)
/* @Test */
START_TEST (test_vfs_clone_file)
{
    int fdin;
    int fdout;
    static const char test_file1[] = "mctestclone1.tst";
    static const char test_file2[] = "mctestclone2.tst";
    vfs_path_t *vpath1 = vfs_path_from_str (test_file1);
    vfs_path_t *vpath2 = vfs_path_from_str (test_file2);

    // given
    clone_syscall__call_count = 0;
    clone_syscall__call_arguments_are_proper = FALSE;
    g_file_set_contents (test_file1, "test", sizeof ("test") - 1, NULL);
    fdin = mc_open (vpath1, O_RDONLY | O_BINARY);
    fdout = mc_open (vpath2, O_CREAT | O_WRONLY | O_TRUNC | O_BINARY);

    // when
    vfs_clone_file (fdout, fdin);
    mc_close (fdout);
    mc_close (fdin);
    unlink (test_file1);
    unlink (test_file2);
    vfs_path_free (vpath1, TRUE);
    vfs_path_free (vpath2, TRUE);

    // then
    ck_assert (clone_syscall__call_count > 0 && clone_syscall__call_arguments_are_proper);
}
END_TEST
#endif

/* --------------------------------------------------------------------------------------------- */

int
main (void)
{
    TCase *tc_core;

    tc_core = tcase_create ("Core");

    tcase_add_checked_fixture (tc_core, setup, teardown);

    // Add new tests here: ***************
#if defined(__linux__) || defined(__FreeBSD__)
    tcase_add_test (tc_core, test_vfs_clone_file);
#else
    (void) clone_syscall__call_count;
    (void) clone_syscall__call_arguments_are_proper;
#endif
    // ***********************************

    return mctest_run_all (tc_core);
}
