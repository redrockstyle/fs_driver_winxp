/****************************************************************************

	Файл ioctl.h

    Заголовочный файл. Содержит определние ioctl-кодов.

	Маткин Илья Александрович               13.05.2009

****************************************************************************/

#ifndef _IOCTL_H_
#define _IOCTL_H_

//----------------------------------------

#define READ_IOCTL  CTL_CODE(FILE_DEVICE_UNKNOWN,0x801,METHOD_BUFFERED,FILE_READ_ACCESS | FILE_WRITE_ACCESS)

#define WRITE_IOCTL  CTL_CODE(FILE_DEVICE_UNKNOWN,0x802,METHOD_BUFFERED,FILE_READ_ACCESS | FILE_WRITE_ACCESS)

#define TEST_IOCTL  CTL_CODE(FILE_DEVICE_UNKNOWN,0x803,METHOD_NEITHER,FILE_ANY_ACCESS)

//----------------------------------------

#endif  // _IOCTL_H_