/*
 * @brief Virtual Comm port call back routines
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
#include <lpcapi/cdc/cdc_usbd_cfg.h>
#include <lpcapi/cdc/cdc_vcom.h>
#include <lpcapi/romapi_18xx_43xx.h>
#include <string.h>

/*****************************************************************************
 * Private types/enumerations/variables
 ****************************************************************************/

/* Part of WORKAROUND for artf42016. */
static USB_EP_HANDLER_T g_defaultCdcHdlr;

/*****************************************************************************
 * Public types/enumerations/variables
 ****************************************************************************/

/**
 * Global variable to hold Virtual COM port control data.
 */
VCOM_DATA_T g_vCOM;

/*****************************************************************************
 * Private functions
 ****************************************************************************/

/* VCOM bulk EP_IN endpoint handler */
static ErrorCode_t VCOM_bulk_in_hdlr(USBD_HANDLE_T hUsb, void *data, uint32_t event)
{
	VCOM_DATA_T *pVcom = (VCOM_DATA_T *) data;

	if (event == USB_EVT_IN) {
		pVcom->tx_flags &= ~VCOM_TX_BUSY;
	}
	return LPC_OK;
}

/* VCOM bulk EP_OUT endpoint handler */
static ErrorCode_t VCOM_bulk_out_hdlr(USBD_HANDLE_T hUsb, void *data, uint32_t event)
{
	VCOM_DATA_T *pVcom = (VCOM_DATA_T *) data;

	switch (event) {
	case USB_EVT_OUT:
		pVcom->rx_count = USBD_API->hw->ReadEP(hUsb, USB_CDC_OUT_EP, pVcom->rx_buff);
		if (pVcom->rx_flags & VCOM_RX_BUF_QUEUED) {
			pVcom->rx_flags &= ~VCOM_RX_BUF_QUEUED;
			if (pVcom->rx_count != 0) {
				pVcom->rx_flags |= VCOM_RX_BUF_FULL;
			}

		}
		else if (pVcom->rx_flags & VCOM_RX_DB_QUEUED) {
			pVcom->rx_flags &= ~VCOM_RX_DB_QUEUED;
			pVcom->rx_flags |= VCOM_RX_DONE;
		}
		break;

	case USB_EVT_OUT_NAK:
		/* queue free buffer for RX */
		if ((pVcom->rx_flags & (VCOM_RX_BUF_FULL | VCOM_RX_BUF_QUEUED)) == 0) {
			USBD_API->hw->ReadReqEP(hUsb, USB_CDC_OUT_EP, pVcom->rx_buff, VCOM_RX_BUF_SZ);
			pVcom->rx_flags |= VCOM_RX_BUF_QUEUED;
		}
		break;

	default:
		break;
	}

	return LPC_OK;
}

/* Set line coding call back routine */
static ErrorCode_t VCOM_SetLineCode(USBD_HANDLE_T hCDC, CDC_LINE_CODING *line_coding)
{
	VCOM_DATA_T *pVcom = &g_vCOM;

	/* Called when baud rate is changed/set. Using it to know host connection state */
	pVcom->tx_flags = VCOM_TX_CONNECTED;	/* reset other flags */

	return LPC_OK;
}

/* CDC EP0_patch part of WORKAROUND for artf42016. */
static ErrorCode_t CDC_ep0_override_hdlr(USBD_HANDLE_T hUsb, void *data, uint32_t event)
{
	USB_CORE_CTRL_T *pCtrl = (USB_CORE_CTRL_T *) hUsb;
	USB_CDC_CTRL_T *pCdcCtrl = (USB_CDC_CTRL_T *) data;
	USB_CDC0_CTRL_T *pCdc0Ctrl = (USB_CDC0_CTRL_T *) data;
	uint8_t cif_num, dif_num;
	CIC_SetRequest_t setReq;
	ErrorCode_t ret = ERR_USBD_UNHANDLED;

	if ( (event == USB_EVT_OUT) &&
		 (pCtrl->SetupPacket.bmRequestType.BM.Type == REQUEST_CLASS) &&
		 (pCtrl->SetupPacket.bmRequestType.BM.Recipient == REQUEST_TO_INTERFACE) ) {

		/* Check which CDC control structure to use. If epin_num doesn't have BIT7 set then we are
		   at wrong offset so use the old CDC control structure. BIT7 is set for all EP_IN endpoints.

		 */
		if ((pCdcCtrl->epin_num & 0x80) == 0) {
			cif_num = pCdc0Ctrl->cif_num;
			dif_num = pCdc0Ctrl->dif_num;
			setReq = pCdc0Ctrl->CIC_SetRequest;
		}
		else {
			cif_num = pCdcCtrl->cif_num;
			dif_num = pCdcCtrl->dif_num;
			setReq = pCdcCtrl->CIC_SetRequest;
		}
		/* is the request target is our interfaces */
		if (((pCtrl->SetupPacket.wIndex.WB.L == cif_num)  ||
			 (pCtrl->SetupPacket.wIndex.WB.L == dif_num)) ) {

			pCtrl->EP0Data.pData -= pCtrl->SetupPacket.wLength;
			ret = setReq(pCdcCtrl, &pCtrl->SetupPacket, &pCtrl->EP0Data.pData,
						 pCtrl->SetupPacket.wLength);
			if ( ret == LPC_OK) {
				/* send Acknowledge */
				USBD_API->core->StatusInStage(pCtrl);
			}
		}

	}
	else {
		ret = g_defaultCdcHdlr(hUsb, data, event);
	}
	return ret;
}

/*****************************************************************************
 * Public functions
 ****************************************************************************/

/* Virtual com port init routine */
ErrorCode_t vcom_init(USBD_HANDLE_T hUsb, USB_CORE_DESCS_T *pDesc, USBD_API_INIT_PARAM_T *pUsbParam)
{
	USBD_CDC_INIT_PARAM_T cdc_param;
	ErrorCode_t ret = LPC_OK;
	uint32_t ep_indx;
	USB_CORE_CTRL_T *pCtrl = (USB_CORE_CTRL_T *) hUsb;

	g_vCOM.hUsb = hUsb;
	memset((void *) &cdc_param, 0, sizeof(USBD_CDC_INIT_PARAM_T));
	cdc_param.mem_base = pUsbParam->mem_base;
	cdc_param.mem_size = pUsbParam->mem_size;
	cdc_param.cif_intf_desc = (uint8_t *) find_IntfDesc(pDesc->high_speed_desc, CDC_COMMUNICATION_INTERFACE_CLASS);
	cdc_param.dif_intf_desc = (uint8_t *) find_IntfDesc(pDesc->high_speed_desc, CDC_DATA_INTERFACE_CLASS);
	cdc_param.SetLineCode = VCOM_SetLineCode;

	ret = USBD_API->cdc->init(hUsb, &cdc_param, &g_vCOM.hCdc);
	if (ret != LPC_OK) {
		return ret;
	}

	/*	WORKAROUND for artf42016 ROM driver BUG:
	    The default CDC class handler in initial ROM (REV A silicon) was not
	    sending proper handshake after processing SET_REQUEST messages targeted
	    to CDC interfaces. The workaround will send the proper handshake to host.
	    Due to this bug some terminal applications such as Putty have problem
	    establishing connection.
	 */
	g_defaultCdcHdlr = pCtrl->ep0_hdlr_cb[pCtrl->num_ep0_hdlrs - 1];
	/* store the default CDC handler and replace it with ours */
	pCtrl->ep0_hdlr_cb[pCtrl->num_ep0_hdlrs - 1] = CDC_ep0_override_hdlr;

	/* allocate transfer buffers */
	if (cdc_param.mem_size < VCOM_RX_BUF_SZ) {
		return ERR_FAILED;
	}
	g_vCOM.rx_buff = (uint8_t *) cdc_param.mem_base;
	cdc_param.mem_base += VCOM_RX_BUF_SZ;
	cdc_param.mem_size -= VCOM_RX_BUF_SZ;

	/* register endpoint interrupt handler */
	ep_indx = (((USB_CDC_IN_EP & 0x0F) << 1) + 1);
	ret = USBD_API->core->RegisterEpHandler(hUsb, ep_indx, VCOM_bulk_in_hdlr, &g_vCOM);
	if (ret != LPC_OK) {
		return ret;
	}

	/* register endpoint interrupt handler */
	ep_indx = ((USB_CDC_OUT_EP & 0x0F) << 1);
	ret = USBD_API->core->RegisterEpHandler(hUsb, ep_indx, VCOM_bulk_out_hdlr, &g_vCOM);
	if (ret != LPC_OK) {
		return ret;
	}

	/* update mem_base and size variables for cascading calls. */
	pUsbParam->mem_base = cdc_param.mem_base;
	pUsbParam->mem_size = cdc_param.mem_size;

	return ret;
}

/* Virtual com port buffered read routine */
uint32_t vcom_bread(uint8_t *pBuf, uint32_t buf_len)
{
	VCOM_DATA_T *pVcom = &g_vCOM;
	uint16_t cnt = 0;
	/* read from the default buffer if any data present */
	if (pVcom->rx_count) {
		cnt = (pVcom->rx_count < buf_len) ? pVcom->rx_count : buf_len;
		memcpy(pBuf, pVcom->rx_buff, cnt);
		pVcom->rx_rd_count += cnt;

		/* enter critical section */
	    nvic_disable_irq(NVIC_USB0_IRQ);
		if (pVcom->rx_rd_count >= pVcom->rx_count) {
			pVcom->rx_flags &= ~VCOM_RX_BUF_FULL;
			pVcom->rx_rd_count = pVcom->rx_count = 0;
		}
		/* exit critical section */
		nvic_enable_irq(NVIC_USB0_IRQ);
	}
	return cnt;
}

/* Virtual com port read routine */
ErrorCode_t vcom_read_req(uint8_t *pBuf, uint32_t len)
{
	VCOM_DATA_T *pVcom = &g_vCOM;

	/* check if we queued Rx buffer */
	if (pVcom->rx_flags & (VCOM_RX_BUF_QUEUED | VCOM_RX_DB_QUEUED)) {
		return ERR_BUSY;
	}
	/* enter critical section */
	nvic_disable_irq(NVIC_USB0_IRQ);
	/* if not queue the request and return 0 bytes */
	USBD_API->hw->ReadReqEP(pVcom->hUsb, USB_CDC_OUT_EP, pBuf, len);
	/* exit critical section */
	nvic_enable_irq(NVIC_USB0_IRQ);
	pVcom->rx_flags |= VCOM_RX_DB_QUEUED;

	return LPC_OK;
}

/* Gets current read count. */
uint32_t vcom_read_cnt(void)
{
	VCOM_DATA_T *pVcom = &g_vCOM;
	uint32_t ret = 0;

	if (pVcom->rx_flags & VCOM_RX_DONE) {
		ret = pVcom->rx_count;
		pVcom->rx_count = 0;
	}

	return ret;
}

/* Virtual com port write routine*/
uint32_t vcom_write(uint8_t *pBuf, uint32_t len)
{
	VCOM_DATA_T *pVcom = &g_vCOM;
	uint32_t ret = 0;

	if ( (pVcom->tx_flags & VCOM_TX_CONNECTED) && ((pVcom->tx_flags & VCOM_TX_BUSY) == 0) ) {
		pVcom->tx_flags |= VCOM_TX_BUSY;

		/* enter critical section */
	    nvic_disable_irq(NVIC_USB0_IRQ);
		ret = USBD_API->hw->WriteEP(pVcom->hUsb, USB_CDC_IN_EP, pBuf, len);
		/* exit critical section */
	    nvic_enable_irq(NVIC_USB0_IRQ);
	}

	return ret;
}
