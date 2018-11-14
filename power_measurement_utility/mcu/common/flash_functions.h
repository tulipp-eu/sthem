

#ifndef FLASH_FUNCTIONS_H_
#define FLASH_FUNCTIONS_H_

#include "usbprotocol.h"

struct __attribute__((__packed__)) Flash_Struct {

  uint16_t version;
  uint16_t Reserve0;
  uint32_t Datalength;
  uint32_t DataCRC;
  uint32_t Reserve1;
  uint32_t Reserve2;
  uint32_t HeaderCRC;
};
#define Flash_offset1 0x200000l
#define Flash_offset2 0x400000l

#define Flash_end1 0x3fffffl
#define Flash_end2 0x5fffffl

#define Flash_Header 512

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

void Fill_boot_reply_package( struct BootReplyPackage  * boot_reply_package,uint8_t *inBuffer );
uint32_t CRC32(uint32_t CRC , uint32_t *data , int length );
int check_flash_for_Framework(uint32_t Flash_ofset ,  uint8_t *inBuffer );

#endif /* FLASH_FUNCTIONS_H_ */
