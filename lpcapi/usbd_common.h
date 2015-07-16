#include <lpcapi/romapi_18xx_43xx.h>
#include <lpcapi/usbd.h>

#ifdef NOEXTERN
#define EXTERN
#else
#define EXTERN extern
#endif


/*****************************************************************************
 * Private types/enumerations/variables
 ****************************************************************************/
EXTERN USBD_HANDLE_T g_hUsb;

/* Endpoint 0 patch that prevents nested NAK event processing */
// uint32_t g_ep0RxBusy = 0;/* flag indicating whether EP0 OUT/RX buffer is busy. */
EXTERN USB_EP_HANDLER_T g_Ep0BaseHdlr;	/* variable to store the pointer to base EP0 handler */

/*****************************************************************************
 * Public types/enumerations/variables
 ****************************************************************************/
EXTERN const USBD_API_T *g_pUsbApi;

/*****************************************************************************
 * Private functions
 ****************************************************************************/

/* EP0_patch part of WORKAROUND for artf45032. */
ErrorCode_t EP0_patch(USBD_HANDLE_T hUsb, void *data, uint32_t event);

#undef EXTERN
