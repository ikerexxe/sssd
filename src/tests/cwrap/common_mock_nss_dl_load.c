/*
    Authors:
        Iker Pedrosa <ipedrosa@redhat.com>

    Copyright (C) 2021 Red Hat

    SSSD tests: Fake nss dl load

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dlfcn.h>

#include "common_mock_nss_dl_load.h"

#define LIB_PATH    "libnss_wrapper.so"


static errno_t load_function(const char *fname, void **fptr)
{
    void *dl_handle = NULL;
    errno_t ret;

    dl_handle = dlopen(LIB_PATH, RTLD_NOW);
    if (dl_handle == NULL) {
        DEBUG(SSSDBG_FATAL_FAILURE, "Unable to load %s module, "
              "error: %s\n", LIB_PATH, dlerror());
        ret = ELIBACC;
        goto done;
    }

    *fptr = dlsym(dl_handle, fname);
    if (*fptr == NULL) {
        DEBUG(SSSDBG_FATAL_FAILURE, "Library '%s' did not provide "
              "mandatory symbol '%s', error: %s.\n",
              LIB_PATH, fname, dlerror());
        ret = ELIBBAD;
        goto done;
    }

    ret = EOK;

done:
    return ret;
}

static enum nss_status
mock_getpwnam_r(const char *name, struct passwd *result,
                char *buffer, size_t buflen, int *errnop)
{
    void *pwd_pointer = NULL;
    int rc;
    errno_t (*fptr)(const char *, struct passwd *, char *,
                    size_t, struct passwd **) = NULL;

    rc = load_function("getpwnam_r", (void *)&fptr);
    if (rc) {
        return NSS_STATUS_UNAVAIL;
    }

    rc = (*fptr)(name, result, buffer, buflen, (struct passwd **)&pwd_pointer);
    if (rc == 0 && pwd_pointer == result) {
        return NSS_STATUS_SUCCESS;
    } else if (rc == 0 && (pwd_pointer == result)) {
        return NSS_STATUS_NOTFOUND;
    } else {
        return NSS_STATUS_UNAVAIL;
    }
}

static enum nss_status
mock_getpwuid_r(uid_t uid, struct passwd *result,
                char *buffer, size_t buflen, int *errnop)
{
    void *pwd_pointer = NULL;
    int rc;
    errno_t (*fptr)(uid_t, struct passwd *, char *,
                    size_t, struct passwd **) = NULL;

    rc = load_function("getpwuid_r", (void *)&fptr);
    if (rc) {
        return NSS_STATUS_UNAVAIL;
    }

    rc = (*fptr)(uid, result, buffer, buflen, (struct passwd **)&pwd_pointer);
    if (rc == 0 && pwd_pointer == result) {
        return NSS_STATUS_SUCCESS;
    } else if (rc == 0 && (pwd_pointer == result)) {
        return NSS_STATUS_NOTFOUND;
    } else {
        return NSS_STATUS_UNAVAIL;
    }
}

errno_t mock_sss_load_nss_pw_symbols(struct sss_nss_ops *ops)
{
    ops->getpwnam_r = mock_getpwnam_r;
    ops->getpwuid_r = mock_getpwuid_r;

    return EOK;
}
