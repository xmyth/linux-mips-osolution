//header: syscall-exam.h
#ifndef _XGI_MODE_H
#define _XGI_MODE_H
#include <linux/ioctl.h>
typedef struct mychrdev_info {
        unsigned int  opmode;
        unsigned int  baseaddr;
        unsigned int  index;
        unsigned int  value;
} mydev_info_t;

#define MYCHRDEV_IOCTL_GET_INFO _IOR('m',0x01,mydev_info_t)
#define MYCHRDEV_IOCTL_SET_INFO 1

//for display switch
#define myiobase 0xffffffffbfd00000
#define MYIOREG(a) *((volatile unsigned char*)(myiobase+a))
#define UCHAR unsigned char
#define USHORT unsigned short
#define MYCHRDEV_OP_FLAG	0xa55a

#define XGI_P3D4 0x4C54
#define XGI_P3C4 0x4C44

#define inXGIREG(base)      MYIOREG(base)
#define outXGIREG(base,val) MYIOREG(base)=val;
#define orXGIREG(base,val)  do { \
                                unsigned char temp = MYIOREG(base); \
                                outXGIREG(base, temp | (val)); \
                            } while (0)

#define andXGIREG(base,val) do { \
                                unsigned char temp = MYIOREG(base); \
                                outXGIREG(base, temp & (val)); \
                            } while (0)

#define inXGIIDXREG(base,idx,var)\
                    do { \
                        MYIOREG(base)=idx; var=MYIOREG((base)+1); \
                    } while (0)

#define outXGIIDXREG(base,idx,val)\
                    do { \
                      outXGIREG(base,idx); outXGIREG((base)+1,val); \
                    } while (0)

#define orXGIIDXREG(base,idx,val)\
                    do { \
                        unsigned char temp; \
                        outXGIREG(base,idx);    \
                        temp = inXGIREG((base)+1)|(val); \
                        outXGIIDXREG(base,idx,temp); \
                    } while (0)

#define andXGIIDXREG(base,idx,and)\
                    do { \
                        unsigned char temp; \
                        outXGIREG(base,idx);    \
                        temp = inXGIREG((base)+1)&(and); \
                        outXGIIDXREG(base,idx,temp); \
                    } while (0)

#define setXGIIDXREG(base,idx,and,or)\
                    do { \
                        unsigned char tmp; \
                        outXGIREG(base,idx);    \
                        tmp = (inXGIREG((base)+1)&(and))|(or); \
                        outXGIIDXREG(base,idx,tmp); \
                    } while (0)

#endif
