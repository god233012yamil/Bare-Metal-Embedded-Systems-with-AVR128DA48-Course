#include "utfs.h"

#include <stdio.h>
#include <string.h>

#define UTFS_IDENTIFIER 0x1984u
#define UTFS_VERSION_V1 1u
#define UTFS_HEADER_SIZE 24u

typedef struct {
    uint16_t identifier;
    uint8_t version;
    uint8_t flags;
    uint16_t signature;
    uint16_t reserved;
    uint32_t size;
    char filename[UTFS_MAX_FILENAME + 1u];
} utfs_header_t;

static utfs_file_t *file_list[UTFS_MAX_FILES];
static bool structure_saved;
static bool utfs_verbose;
static uint32_t base_address;

uint32_t sys_write(uint32_t address, void *ptr, uint32_t length);
uint32_t sys_read(uint32_t address, void *ptr, uint32_t length);

static bool header_is_valid(const utfs_header_t *header);
static void header_from_file(utfs_header_t *header, const utfs_file_t *file);
static int16_t find_registered_file_by_name(const char *filename);
static int16_t find_registered_file_pointer(const utfs_file_t *file);
static void log_header(const utfs_header_t *header);

/**
 * Initializes the UTFS runtime state.
 *
 * Args:
 *     verbose: Enables printf-based diagnostic output when true.
 *
 * Returns:
 *     RES_OK when the internal file list is cleared successfully.
 */
utfs_result_e utfs_init(bool verbose)
{
    memset(file_list, 0, sizeof(file_list));
    structure_saved = false;
    utfs_verbose = verbose;
    base_address = 0u;
    return RES_OK;
}

/**
 * Sets the start address of the UTFS storage region.
 *
 * Args:
 *     baseaddr: First byte address used by UTFS in the backing store.
 *
 * Returns:
 *     RES_OK after updating the base address.
 */
utfs_result_e utfs_baseaddress_set(uint32_t baseaddr)
{
    base_address = baseaddr;
    return RES_OK;
}

/**
 * Registers a caller-owned RAM object as a UTFS file.
 *
 * Args:
 *     file: Pointer to the UTFS file descriptor.
 *     flags: File flags stored in the on-medium header.
 *     options: Registration options. UTFS_OPT_REPLACE replaces a duplicate name.
 *
 * Returns:
 *     RES_OK on success, or an error code when the file cannot be registered.
 */
utfs_result_e utfs_register(utfs_file_t *file, utfs_flags_e flags, utfs_options_e options)
{
    if (file == NULL) {
        return RES_PARAM_ERROR;
    }

    file->flags = (uint16_t)flags;

    for (uint8_t index = 0u; index < UTFS_MAX_FILES; index++) {
        if (file_list[index] == NULL) {
            continue;
        }

        if (strncmp(file_list[index]->filename, file->filename, UTFS_MAX_FILENAME + 1u) == 0) {
            if ((options & UTFS_OPT_REPLACE) != 0u) {
                file_list[index] = file;
                return RES_OK;
            }

            return RES_FILENAME_EXISTS;
        }
    }

    for (uint8_t index = 0u; index < UTFS_MAX_FILES; index++) {
        if (file_list[index] == NULL) {
            file_list[index] = file;
            return RES_OK;
        }
    }

    return RES_FILESYSTEM_FULL;
}

/**
 * Removes a registered file descriptor from the UTFS runtime list.
 *
 * Args:
 *     file: File descriptor to remove.
 *
 * Returns:
 *     RES_OK when removed, or RES_FILE_NOT_FOUND when not registered.
 */
utfs_result_e utfs_unregister(utfs_file_t *file)
{
    int16_t index = find_registered_file_pointer(file);

    if (file == NULL) {
        return RES_PARAM_ERROR;
    }

    if (index < 0) {
        return RES_FILE_NOT_FOUND;
    }

    file_list[index] = NULL;
    return RES_OK;
}

/**
 * Loads all registered files from the backing store.
 *
 * Returns:
 *     RES_OK when a valid UTFS structure is found. RES_INVALID_FS is returned
 *     when no valid UTFS headers exist at the configured base address.
 */
utfs_result_e utfs_load(void)
{
    uint32_t position = base_address;
    uint8_t valid_headers = 0u;

    for (uint8_t scan = 0u; scan < UTFS_MAX_FILES; scan++) {
        utfs_header_t header;

        if (sys_read(position, &header, UTFS_HEADER_SIZE) != UTFS_HEADER_SIZE) {
            return RES_READ_ERROR;
        }

        if (!header_is_valid(&header)) {
            break;
        }

        valid_headers++;
        position += UTFS_HEADER_SIZE;

        int16_t file_index = find_registered_file_by_name(header.filename);
        if (file_index >= 0) {
            utfs_file_t *file = file_list[file_index];
            uint32_t read_size = header.size;

            file->signature = header.signature;
            file->flags = (file->flags & 0xFF00u) | header.flags;

            if ((file->data != NULL) && (file->size > 0u)) {
                if (read_size > file->size) {
                    read_size = file->size;
                }

                if (sys_read(position, file->data, read_size) != read_size) {
                    return RES_READ_ERROR;
                }

                file->size_loaded = read_size;
            }
        }

        position += header.size;
    }

    if (valid_headers == 0u) {
        return RES_INVALID_FS;
    }

    structure_saved = true;
    return RES_OK;
}

/**
 * Saves all registered files to the backing store.
 *
 * Returns:
 *     RES_OK when every header and data block is written successfully.
 */
utfs_result_e utfs_save(void)
{
    uint32_t position = base_address;

    for (uint8_t index = 0u; index < UTFS_MAX_FILES; index++) {
        if (file_list[index] == NULL) {
            continue;
        }

        utfs_header_t header;
        header_from_file(&header, file_list[index]);

        if (sys_write(position, &header, UTFS_HEADER_SIZE) != UTFS_HEADER_SIZE) {
            return RES_FILESYSTEM_FULL;
        }
        position += UTFS_HEADER_SIZE;

        if (file_list[index]->size > 0u) {
            if (sys_write(position, file_list[index]->data, file_list[index]->size) != file_list[index]->size) {
                return RES_FILESYSTEM_FULL;
            }
        }
        position += file_list[index]->size;
    }

    structure_saved = true;
    return RES_OK;
}

/**
 * Saves all files, matching the reference UTFS flush API.
 *
 * Returns:
 *     RES_OK when all registered files are written.
 */
utfs_result_e utfs_save_flush(void)
{
    return utfs_save();
}

/**
 * Loads one registered file from the backing store.
 *
 * Args:
 *     file: Registered file descriptor to load.
 *
 * Returns:
 *     RES_OK when loaded, or RES_FILE_NOT_FOUND when not present.
 */
utfs_result_e utfs_load_file(utfs_file_t *file)
{
    if (file == NULL) {
        return RES_PARAM_ERROR;
    }

    uint32_t position = base_address;

    for (uint8_t scan = 0u; scan < UTFS_MAX_FILES; scan++) {
        utfs_header_t header;

        if (sys_read(position, &header, UTFS_HEADER_SIZE) != UTFS_HEADER_SIZE) {
            return RES_READ_ERROR;
        }

        if (!header_is_valid(&header)) {
            break;
        }

        position += UTFS_HEADER_SIZE;

        if (strncmp(header.filename, file->filename, UTFS_MAX_FILENAME + 1u) == 0) {
            uint32_t read_size = header.size;

            if (read_size > file->size) {
                read_size = file->size;
            }

            if ((file->data != NULL) && (read_size > 0u)) {
                if (sys_read(position, file->data, read_size) != read_size) {
                    return RES_READ_ERROR;
                }
            }

            file->signature = header.signature;
            file->flags = (file->flags & 0xFF00u) | header.flags;
            file->size_loaded = read_size;
            return RES_OK;
        }

        position += header.size;
    }

    return RES_FILE_NOT_FOUND;
}

/**
 * Saves a single registered file. The full structure is created first if needed.
 *
 * Args:
 *     file: Registered file descriptor to save.
 *
 * Returns:
 *     RES_OK on success, or an error code on failure.
 */
utfs_result_e utfs_save_file(utfs_file_t *file)
{
    if (file == NULL) {
        return RES_PARAM_ERROR;
    }

    if (!structure_saved) {
        return utfs_save();
    }

    uint32_t position = base_address;

    for (uint8_t index = 0u; index < UTFS_MAX_FILES; index++) {
        if (file_list[index] == NULL) {
            continue;
        }

        if (file_list[index] == file) {
            utfs_header_t header;
            header_from_file(&header, file);

            if (sys_write(position, &header, UTFS_HEADER_SIZE) != UTFS_HEADER_SIZE) {
                return RES_WRITE_ERROR;
            }

            position += UTFS_HEADER_SIZE;
            if (sys_write(position, file->data, file->size) != file->size) {
                return RES_WRITE_ERROR;
            }

            return RES_OK;
        }

        position += UTFS_HEADER_SIZE + file_list[index]->size;
    }

    return RES_FILE_NOT_FOUND;
}

/**
 * Configures a file descriptor with a name, RAM buffer, and size.
 *
 * Args:
 *     file: File descriptor to configure.
 *     name: Null-terminated file name. Maximum usable length is 11 characters.
 *     data: RAM buffer associated with the file.
 *     size: Size of the RAM buffer in bytes.
 *
 * Returns:
 *     RES_OK on success, or RES_PARAM_ERROR for invalid pointers.
 */
utfs_result_e utfs_set(utfs_file_t *file, char *name, void *data, uint32_t size)
{
    if ((file == NULL) || (name == NULL)) {
        return RES_PARAM_ERROR;
    }

    memset(file, 0, sizeof(*file));
    strncpy(file->filename, name, UTFS_MAX_FILENAME);
    file->filename[UTFS_MAX_FILENAME] = '\0';
    file->data = data;
    file->size = size;
    file->size_loaded = 0u;
    return RES_OK;
}

/**
 * Updates the file name stored in a descriptor.
 *
 * Args:
 *     file: File descriptor to modify.
 *     name: New file name.
 *
 * Returns:
 *     RES_OK on success, or RES_PARAM_ERROR for invalid pointers.
 */
utfs_result_e utfs_set_filename(utfs_file_t *file, char *name)
{
    if ((file == NULL) || (name == NULL)) {
        return RES_PARAM_ERROR;
    }

    memset(file->filename, 0, sizeof(file->filename));
    strncpy(file->filename, name, UTFS_MAX_FILENAME);
    file->filename[UTFS_MAX_FILENAME] = '\0';
    return RES_OK;
}

/**
 * Updates the RAM buffer associated with a file descriptor.
 *
 * Args:
 *     file: File descriptor to modify.
 *     data: New RAM buffer.
 *     size: Size of the RAM buffer in bytes.
 *
 * Returns:
 *     RES_OK on success, or RES_PARAM_ERROR for invalid pointers.
 */
utfs_result_e utfs_set_data(utfs_file_t *file, void *data, uint32_t size)
{
    if (file == NULL) {
        return RES_PARAM_ERROR;
    }

    file->data = data;
    file->size = size;
    return RES_OK;
}

/**
 * Reads the application signature stored in a file descriptor.
 *
 * Args:
 *     file: File descriptor to inspect.
 *
 * Returns:
 *     The signature value, or zero when file is NULL.
 */
uint16_t utfs_file_signature(utfs_file_t *file)
{
    if (file == NULL) {
        return 0u;
    }

    return file->signature;
}

/**
 * Sets the application signature stored in a file descriptor.
 *
 * Args:
 *     file: File descriptor to modify.
 *     signature: Application-defined signature value.
 *
 * Returns:
 *     RES_OK on success, or RES_PARAM_ERROR when file is NULL.
 */
utfs_result_e utfs_file_signature_set(utfs_file_t *file, uint16_t signature)
{
    if (file == NULL) {
        return RES_PARAM_ERROR;
    }

    file->signature = signature;
    return RES_OK;
}

/**
 * Converts a UTFS result code into a readable string.
 *
 * Args:
 *     result: UTFS result code.
 *
 * Returns:
 *     Pointer to a constant string describing the result.
 */
const char *utfs_result_str(utfs_result_e result)
{
    switch (result) {
        case RES_OK:
            return "RES_OK";
        case RES_FILE_NOT_FOUND:
            return "RES_FILE_NOT_FOUND";
        case RES_READ_ERROR:
            return "RES_READ_ERROR";
        case RES_WRITE_ERROR:
            return "RES_WRITE_ERROR";
        case RES_PARAM_ERROR:
            return "RES_PARAM_ERROR";
        case RES_FILENAME_EXISTS:
            return "RES_FILENAME_EXISTS";
        case RES_FILESYSTEM_FULL:
            return "RES_FILESYSTEM_FULL";
        case RES_INVALID_FS:
            return "RES_INVALID_FS";
        default:
            return "RES_UNKNOWN";
    }
}

/**
 * Prints the registered UTFS runtime table through printf.
 *
 * Returns:
 *     RES_OK after printing the current status.
 */
utfs_result_e utfs_status(void)
{
    for (uint8_t index = 0u; index < UTFS_MAX_FILES; index++) {
        if (file_list[index] != NULL) {
            printf("%u: %s, size=%lu, loaded=%lu, sig=0x%04X\r\n",
                   index,
                   file_list[index]->filename,
                   (unsigned long)file_list[index]->size,
                   (unsigned long)file_list[index]->size_loaded,
                   file_list[index]->signature);
        }
    }

    return RES_OK;
}

static bool header_is_valid(const utfs_header_t *header)
{
    if (header == NULL) {
        return false;
    }

    if (header->identifier != UTFS_IDENTIFIER) {
        return false;
    }

    if (header->version != UTFS_VERSION_V1) {
        return false;
    }

    if (header->filename[0] == '\0') {
        return false;
    }

    if (utfs_verbose) {
        log_header(header);
    }

    return true;
}

static void header_from_file(utfs_header_t *header, const utfs_file_t *file)
{
    memset(header, 0, sizeof(*header));
    header->identifier = UTFS_IDENTIFIER;
    header->version = UTFS_VERSION_V1;
    header->flags = (uint8_t)(file->flags & 0x00FFu);
    header->signature = file->signature;
    header->reserved = 0u;
    header->size = file->size;
    strncpy(header->filename, file->filename, UTFS_MAX_FILENAME);
    header->filename[UTFS_MAX_FILENAME] = '\0';
}

static int16_t find_registered_file_by_name(const char *filename)
{
    for (uint8_t index = 0u; index < UTFS_MAX_FILES; index++) {
        if (file_list[index] == NULL) {
            continue;
        }

        if (strncmp(file_list[index]->filename, filename, UTFS_MAX_FILENAME + 1u) == 0) {
            return (int16_t)index;
        }
    }

    return -1;
}

static int16_t find_registered_file_pointer(const utfs_file_t *file)
{
    for (uint8_t index = 0u; index < UTFS_MAX_FILES; index++) {
        if (file_list[index] == file) {
            return (int16_t)index;
        }
    }

    return -1;
}

static void log_header(const utfs_header_t *header)
{
    printf("UTFS header: file=%s size=%lu sig=0x%04X\r\n",
           header->filename,
           (unsigned long)header->size,
           header->signature);
}
