

#ifndef FLASH_FUNCTIONS_H_
#define FLASH_FUNCTIONS_H_

extern void flash_info();
extern int Read_Page(uint32_t pageNum,uint8_t *buffer);
extern int Blankcheck_page(uint32_t pageNum,uint8_t *buffer);
extern int Blankcheck_device(uint8_t * buffer);
extern int check_badBlock_Info(uint8_t * buffer);
extern int Write_page(uint32_t pageNum,uint8_t * buffer,int BUF_SIZ);
extern int Erase_block(uint32_t blockNum);
extern void Check_ECC(uint32_t pageNum,uint8_t * buffer1,uint8_t * buffer2,uint32_t BUF_SIZ);
extern int copy_page(uint32_t dstPageNum,uint32_t srcPageNum);
extern int Mark_badBlock(uint32_t blockNum);
extern void print_help();
extern void Unknown_command();
extern void test_flash(int BUF_SIZ,uint8_t * buffer1,uint8_t * buffer2);
extern void NANDflashInit();


#endif /* FLASH_FUNCTIONS_H_ */
