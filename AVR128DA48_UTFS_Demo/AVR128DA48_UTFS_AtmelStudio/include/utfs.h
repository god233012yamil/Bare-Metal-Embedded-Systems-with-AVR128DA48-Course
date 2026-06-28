#ifndef UTFS_H_
#define UTFS_H_

#include <stdbool.h>
#include <stdint.h>

#ifndef UTFS_MAX_FILES
#define UTFS_MAX_FILES 5u
#endif

#ifndef UTFS_MAX_FILENAME
#define UTFS_MAX_FILENAME 11u
#endif

typedef enum {
    RES_OK = 0,
    RES_FILE_NOT_FOUND,
    RES_READ_ERROR,
    RES_WRITE_ERROR,
    RES_PARAM_ERROR,
    RES_FILENAME_EXISTS,
    RES_FILESYSTEM_FULL,
    RES_INVALID_FS
} utfs_result_e;

typedef struct {
    char filename[UTFS_MAX_FILENAME + 1u];
    uint16_t signature;
    uint16_t flags;
    uint32_t size;
    uint32_t size_loaded;
    void *data;
} utfs_file_t;

typedef enum {
    UTFS_NOFLAGS = 0
} utfs_flags_e;

typedef enum {
    UTFS_NOOPT = 0,
    UTFS_OPT_REPLACE = 0x01
} utfs_options_e;

#ifdef __cplusplus
extern "C" {
#endif

utfs_result_e utfs_init(bool verbose);
utfs_result_e utfs_baseaddress_set(uint32_t baseaddr);
utfs_result_e utfs_register(utfs_file_t *file, utfs_flags_e flags, utfs_options_e options);
utfs_result_e utfs_unregister(utfs_file_t *file);
utfs_result_e utfs_load(void);
utfs_result_e utfs_save(void);
utfs_result_e utfs_save_flush(void);
utfs_result_e utfs_load_file(utfs_file_t *file);
utfs_result_e utfs_save_file(utfs_file_t *file);
utfs_result_e utfs_set(utfs_file_t *file, char *name, void *data, uint32_t size);
utfs_result_e utfs_set_filename(utfs_file_t *file, char *name);
utfs_result_e utfs_set_data(utfs_file_t *file, void *data, uint32_t size);
uint16_t utfs_file_signature(utfs_file_t *file);
utfs_result_e utfs_file_signature_set(utfs_file_t *file, uint16_t signature);
const char *utfs_result_str(utfs_result_e result);
utfs_result_e utfs_status(void);

#ifdef __cplusplus
}
#endif

#endif
