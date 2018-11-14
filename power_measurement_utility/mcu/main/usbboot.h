#ifndef USBBOOT_H
#define USBBOOT_H

extern void USB_BootMode(struct RequestPacket *inBuffer);
extern void USB_FLASH_SAVE(struct FlashBootPackage *inBuffer);
extern void USB_BootReset(struct FlashResetPackage *inBuffer);
extern void USB_FlashErase(struct FlashErasePackage *inBuffer);

#endif
