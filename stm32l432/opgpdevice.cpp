/*
 Copyright 2019 SoloKeys Developers

 Licensed under the Apache License, Version 2.0, <LICENSE-APACHE or
 http://apache.org/licenses/LICENSE-2.0> or the MIT license <LICENSE-MIT or
 http://opensource.org/licenses/MIT>, at your option. This file may not be
 copied, modified, or distributed except according to those terms.
 */

#include "opgpdevice.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "flash.h"
#include "memory_layout.h"

#include <spiffs.h>
static spiffs fs;

#define LOG_PAGE_SIZE 64
// 2048 for this CPU
#define BLOCK_SIZE PAGE_SIZE
#define TOTAL_FS_SIZE OPENPGP_NUM_PAGES*BLOCK_SIZE

int sprintfs();

static u8_t spiffs_work_buf[LOG_PAGE_SIZE * 2];
static u8_t spiffs_fds[32 * 4];
static u8_t spiffs_cache_buf[(LOG_PAGE_SIZE + 32) * 4];

static s32_t hw_spiffs_read(u32_t addr, u32_t size, u8_t *dst) {
	//memcpy(dst, fsbuf + addr, size);
    //   int page_offset = (sizeof(CTAP_residentKey) * index) / PAGE_SIZE;
    //   uint32_t addr = flash_addr(page_offset + RK_START_PAGE) + ((sizeof(CTAP_residentKey)*index) % PAGE_SIZE);
    //    uint32_t * ptr = (uint32_t *)addr;
    //    memmove((uint8_t*)rk,ptr,sizeof(CTAP_residentKey));

	return SPIFFS_OK;
}

static s32_t hw_spiffs_write(u32_t addr, u32_t size, u8_t *src) {
	//memcpy(fsbuf + addr, src, size);
    //flash_write(addr, (uint8_t*)rk, sizeof(CTAP_residentKey));
	return SPIFFS_OK;
}

static s32_t hw_spiffs_erase(u32_t addr, u32_t size) {
	//memset(fsbuf + addr, 0xff, size);
    //flash_erase_page(page);
	return SPIFFS_OK;
}

void hw_spiffs_mount() {
	spiffs_config cfg;
	cfg.phys_size = TOTAL_FS_SIZE;           // use all spi flash
	cfg.phys_addr = 0;                       // start spiffs at start of spi flash
	cfg.phys_erase_block = BLOCK_SIZE;       // according to the datasheet
	cfg.log_block_size = BLOCK_SIZE;         // let us not complicate things
	cfg.log_page_size = LOG_PAGE_SIZE;       // as we said

	cfg.hal_read_f = hw_spiffs_read;
	cfg.hal_write_f = hw_spiffs_write;
	cfg.hal_erase_f = hw_spiffs_erase;

	int res = SPIFFS_mount(&fs,
		&cfg,
		spiffs_work_buf,
		spiffs_fds,
		sizeof(spiffs_fds),
		spiffs_cache_buf,
		sizeof(spiffs_cache_buf),
		0);
	printf_device("mount res: %i\n", res);

	if (res || !SPIFFS_mounted(&fs)) {
		res = SPIFFS_format(&fs);
		printf_device("format res: %i\n", res);
	}

	u32_t total = 0;
	u32_t used = 0;
	SPIFFS_info(&fs, &total, &used);
	printf_device("Mounted OK. Memory total: %d used: %d\n", total, used);
	sprintfs();
}

int hwinit() {
	hw_spiffs_mount();

	return 0;
}

bool fileexist(char* name) {
	spiffs_DIR d;
	struct spiffs_dirent e;
	struct spiffs_dirent *pe = &e;

	SPIFFS_opendir(&fs, "/", &d);
	while ((pe = SPIFFS_readdir(&d, pe))) {
		if (0 == strcmp(name, (char *)pe->name)) {
			return true;
		}
	}
	return false;
}

int readfile(char* name, uint8_t * buf, size_t max_size, size_t *size) {
	*size = 0;

	spiffs_file fd = SPIFFS_open(&fs, name, SPIFFS_RDWR, 0);
	if (fd < 0)
		return fd;

	int res = SPIFFS_read(&fs, fd, buf, max_size);

	*size = res;
	int cres = SPIFFS_close(&fs, fd) < 0;
	if (cres < 0)
		return cres;

	return (res >= 0) ? 0 : res;
}

int writefile(char* name, uint8_t * buf, size_t size) {
	spiffs_file fd = SPIFFS_open(&fs, name, SPIFFS_CREAT | SPIFFS_TRUNC | SPIFFS_RDWR, 0);
	if (fd < 0)
		return fd;

	int res = SPIFFS_write(&fs, fd, buf, size);

	int cres = SPIFFS_close(&fs, fd) < 0;
	if (cres < 0)
		return cres;

	return (res >= 0) ? 0 : res;
}

int deletefile(char* name) {
	return SPIFFS_remove(&fs, name);
}

int sprintfs() {
	spiffs_DIR d;
	struct spiffs_dirent e;
	struct spiffs_dirent *pe = &e;

	u32_t total = 0;
	u32_t used = 0;
	SPIFFS_info(&fs, &total, &used);
	printf_device("Memory total: %d used: %d\n", total, used);

	SPIFFS_opendir(&fs, "/", &d);
	while ((pe = SPIFFS_readdir(&d, pe))) {
		printf_device("  [%4d] %s\n", pe->size, pe->name);
	}
	SPIFFS_closedir(&d);
	return 0;
}

bool fnmatch(char *pattern, char*name){
    if (strcmp(pattern, name) == 0)
        return true;

    if (strcmp(pattern, "*") == 0)
        return true;
        
    return false;
}

int deletefiles(char* name) {
	spiffs_DIR d;
	struct spiffs_dirent e;
	struct spiffs_dirent *pe = &e;
	int res;

	SPIFFS_opendir(&fs, "/", &d);
	while ((pe = SPIFFS_readdir(&d, pe))) {
		if (fnmatch(name, (char *)pe->name)) {
			spiffs_file fd = SPIFFS_open_by_dirent(&fs, pe, SPIFFS_RDWR, 0);
			if (fd < 0)
				return SPIFFS_errno(&fs);
			res = SPIFFS_fremove(&fs, fd);
			if (res < 0)
				return SPIFFS_errno(&fs);
		}
	}
	SPIFFS_closedir(&d);
	return 0;
}

int hwreboot() {

	return 0;
}
