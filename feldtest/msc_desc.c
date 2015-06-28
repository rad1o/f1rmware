/*
 * @brief USB descriptors for the Mass Storage Class (MSC) example
 *
 * @note
 * Copyright(C) NXP Semiconductors, 2013
 * All rights reserved.
 *
 * @par
 * Software that is described herein is for illustrative purposes only
 * which provides customers with programming information regarding the
 * LPC products.  This software is supplied "AS IS" without any warranties of
 * any kind, and NXP Semiconductors and its licensor disclaim any and
 * all warranties, express or implied, including all implied warranties of
 * merchantability, fitness for a particular purpose and non-infringement of
 * intellectual property rights.  NXP Semiconductors assumes no responsibility
 * or liability for the use of the software, conveys no license or rights under any
 * patent, copyright, mask work right, or any other intellectual property rights in
 * or to any products. NXP Semiconductors reserves the right to make changes
 * in the software without notification. NXP Semiconductors also makes no
 * representation or warranty that such application will be suitable for the
 * specified use without further testing or modification.
 *
 * @par
 * Permission to use, copy, modify, and distribute this software and its
 * documentation is hereby granted, under NXP Semiconductors' and its
 * licensor's relevant copyrights in the software, without fee, provided that it
 * is used in conjunction with NXP Semiconductors microcontrollers.  This
 * copyright, permission, and disclaimer notice must appear in all copies of
 * this code.
 */

#include "app_usbd_cfg.h"

/*****************************************************************************
 * Private types/enumerations/variables
 ****************************************************************************/

/*****************************************************************************
 * Public types/enumerations/variables
 ****************************************************************************/

/**
 * USB Standard Device Descriptor
 */
ALIGNED(4) const uint8_t USB_DeviceDescriptor[] = {
	USB_DEVICE_DESC_SIZE,				/* bLength */
	USB_DEVICE_DESCRIPTOR_TYPE,			/* bDescriptorType */
	WBVAL(0x0200),						/* bcdUSB: 2.00 */
	0x00,								/* bDeviceClass */
	0x00,								/* bDeviceSubClass */
	0x00,								/* bDeviceProtocol */
	USB_MAX_PACKET0,					/* bMaxPacketSize0 */
	WBVAL(0x1FC9),						/* idVendor */
	WBVAL(0x0082),						/* idProduct */
	WBVAL(0x0100),						/* bcdDevice: 1.00 */
	0x01,								/* iManufacturer */
	0x02,								/* iProduct */
	0x03,								/* iSerialNumber */
	0x01								/* bNumConfigurations */
};

/**
 * USB Device Qualifier
 */
ALIGNED(4) const uint8_t USB_DeviceQualifier[] = {
	USB_DEVICE_QUALI_SIZE,					/* bLength */
	USB_DEVICE_QUALIFIER_DESCRIPTOR_TYPE,	/* bDescriptorType */
	WBVAL(0x0200),							/* bcdUSB: 2.00 */
	0x00,									/* bDeviceClass */
	0x00,									/* bDeviceSubClass */
	0x00,									/* bDeviceProtocol */
	USB_MAX_PACKET0,						/* bMaxPacketSize0 */
	0x01,									/* bNumOtherSpeedConfigurations */
	0x00									/* bReserved */
};

/**
 * USB FSConfiguration Descriptor
 * All Descriptors (Configuration, Interface, Endpoint, Class, Vendor)
 */
ALIGNED(4) uint8_t USB_FsConfigDescriptor[] = {
	/* Configuration 1 */
	USB_CONFIGURATION_DESC_SIZE,			/* bLength */
	USB_CONFIGURATION_DESCRIPTOR_TYPE,		/* bDescriptorType */
	WBVAL(									/* wTotalLength */
		USB_CONFIGURATION_DESC_SIZE         +
		USB_INTERFACE_DESC_SIZE             +	/* MSC interface */
		2 * USB_ENDPOINT_DESC_SIZE          +	/* bulk endpoints */
		0
		),
	0x01,									/* bNumInterfaces */
	0x01,									/* bConfigurationValue */
	0x00,									/* iConfiguration */
	USB_CONFIG_SELF_POWERED,				/* bmAttributes  */
	USB_CONFIG_POWER_MA(2),					/* bMaxPower */

	/* Interface 0, Alternate Setting 0, MSC class interface descriptor */
	USB_INTERFACE_DESC_SIZE,			/* bLength */
	USB_INTERFACE_DESCRIPTOR_TYPE,		/* bDescriptorType */
	USB_MSC_IF_NUM,						/* bInterfaceNumber: Number of Interface */
	0x00,								/* bAlternateSetting: Alternate setting */
	0x02,								/* bNumEndpoints: One endpoint used */
	USB_DEVICE_CLASS_STORAGE,			/* bInterfaceClass: Communication Interface Class */
	MSC_SUBCLASS_SCSI,					/* bInterfaceSubClass: Abstract Control Model */
	MSC_PROTOCOL_BULK_ONLY,				/* bInterfaceProtocol: no protocol used */
	0x04,								/* iInterface: */

	/* Endpoint, EP Bulk Out */
	USB_ENDPOINT_DESC_SIZE,				/* bLength */
	USB_ENDPOINT_DESCRIPTOR_TYPE,		/* bDescriptorType */
	USB_MSC_OUT_EP,						/* bEndpointAddress */
	USB_ENDPOINT_TYPE_BULK,				/* bmAttributes */
	WBVAL(USB_FS_MAX_BULK_PACKET),		/* wMaxPacketSize */
	0x00,								/* bInterval: ignore for Bulk transfer */
	/* Endpoint, EP Bulk In */
	USB_ENDPOINT_DESC_SIZE,				/* bLength */
	USB_ENDPOINT_DESCRIPTOR_TYPE,		/* bDescriptorType */
	USB_MSC_IN_EP,						/* bEndpointAddress */
	USB_ENDPOINT_TYPE_BULK,				/* bmAttributes */
	WBVAL(USB_FS_MAX_BULK_PACKET),		/* wMaxPacketSize */
	0x00,								/* bInterval: ignore for Bulk transfer */
	/* Terminator */
	0									/* bLength */
};

/**
 * USB HSConfiguration Descriptor
 * All Descriptors (Configuration, Interface, Endpoint, Class, Vendor)
 */
ALIGNED(4) uint8_t USB_HsConfigDescriptor[] = {
	/* Configuration 1 */
	USB_CONFIGURATION_DESC_SIZE,			/* bLength */
	USB_CONFIGURATION_DESCRIPTOR_TYPE,		/* bDescriptorType */
	WBVAL(									/* wTotalLength */
		USB_CONFIGURATION_DESC_SIZE         +
		USB_INTERFACE_DESC_SIZE             +	/* MSC interface */
		2 * USB_ENDPOINT_DESC_SIZE          +	/* bulk endpoints */
		0
		),
	0x01,									/* bNumInterfaces */
	0x01,									/* bConfigurationValue */
	0x00,									/* iConfiguration */
	USB_CONFIG_SELF_POWERED,				/* bmAttributes  */
	USB_CONFIG_POWER_MA(2),					/* bMaxPower */

	/* Interface 0, Alternate Setting 0, MSC class interface descriptor */
	USB_INTERFACE_DESC_SIZE,			/* bLength */
	USB_INTERFACE_DESCRIPTOR_TYPE,		/* bDescriptorType */
	USB_MSC_IF_NUM,						/* bInterfaceNumber: Number of Interface */
	0x00,								/* bAlternateSetting: Alternate setting */
	0x02,								/* bNumEndpoints: One endpoint used */
	USB_DEVICE_CLASS_STORAGE,			/* bInterfaceClass: Communication Interface Class */
	MSC_SUBCLASS_SCSI,					/* bInterfaceSubClass: Abstract Control Model */
	MSC_PROTOCOL_BULK_ONLY,				/* bInterfaceProtocol: no protocol used */
	0x04,								/* iInterface: */

	/* Endpoint, EP Bulk Out */
	USB_ENDPOINT_DESC_SIZE,				/* bLength */
	USB_ENDPOINT_DESCRIPTOR_TYPE,		/* bDescriptorType */
	USB_MSC_OUT_EP,						/* bEndpointAddress */
	USB_ENDPOINT_TYPE_BULK,				/* bmAttributes */
	WBVAL(USB_HS_MAX_BULK_PACKET),		/* wMaxPacketSize */
	0x00,								/* bInterval: ignore for Bulk transfer */
	/* Endpoint, EP Bulk In */
	USB_ENDPOINT_DESC_SIZE,				/* bLength */
	USB_ENDPOINT_DESCRIPTOR_TYPE,		/* bDescriptorType */
	USB_MSC_IN_EP,						/* bEndpointAddress */
	USB_ENDPOINT_TYPE_BULK,				/* bmAttributes */
	WBVAL(USB_HS_MAX_BULK_PACKET),		/* wMaxPacketSize */
	0x00,								/* bInterval: ignore for Bulk transfer */
	/* Terminator */
	0									/* bLength */
};

/**
 * USB String Descriptor (optional)
 */
ALIGNED(4) const uint8_t USB_StringDescriptor[] = {
	/* Index 0x00: LANGID Codes */
	0x04,								/* bLength */
	USB_STRING_DESCRIPTOR_TYPE,			/* bDescriptorType */
	WBVAL(0x0409),						/* wLANGID  0x0409 = US English*/
	/* Index 0x01: Manufacturer */
	(3 * 2 + 2),						/* bLength (3 Char + Type + length) */
	USB_STRING_DESCRIPTOR_TYPE,			/* bDescriptorType */
	'N', 0,
	'X', 0,
	'P', 0,
	/* Index 0x02: Product */
	(8 * 2 + 2),						/* bLength */
	USB_STRING_DESCRIPTOR_TYPE,			/* bDescriptorType */
	'L', 0,
	'P', 0,
	'C', 0,
	' ', 0,
	'D', 0,
	'i', 0,
	's', 0,
	'k', 0,
	/* Index 0x03: Serial Number */
	(15 * 2 + 2),						/* bLength  */
	USB_STRING_DESCRIPTOR_TYPE,			/* bDescriptorType */
	'1', 0,
	'2', 0,
	'3', 0,
	'4', 0,
	'5', 0,
	'6', 0,
	'7', 0,
	'8', 0,
	'9', 0,
	'A', 0,
	'B', 0,
	'C', 0,
	'D', 0,
	'E', 0,
	'F', 0,
	/* Index 0x04: Interface 1, Alternate Setting 0 */
	(8 * 2 + 2),						/* bLength  */
	USB_STRING_DESCRIPTOR_TYPE,			/* bDescriptorType */
	'L', 0,
	'P', 0,
	'C', 0,
	' ', 0,
	'D', 0,
	'i', 0,
	's', 0,
	'k', 0,
};
