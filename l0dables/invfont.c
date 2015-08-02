/* Font data for Invaders */

/* Space Invaders Pixel Art */

/* Bitmaps */
const uint8_t InvadersBitmaps[] = {
 /* Char 65 is 11px wide @ 0 */
  0x70,  /*  ***     */ 
  0x18,  /*    **    */ 
  0x7d,  /*  ***** * */ 
  0xb6,  /* * ** **  */ 
  0xbc,  /* * ****   */ 
  0x3c,  /*   ****   */ 
  0xbc,  /* * ****   */ 
  0xb6,  /* * ** **  */ 
  0x7d,  /*  ***** * */ 
  0x18,  /*    **    */ 
  0x70,  /*  ***     */ 


 /* Char 66 is 12px wide @ 11 */
  0x9c,  /* *  ***   */ 
  0x9e,  /* *  ****  */ 
  0x5e,  /*  * ****  */ 
  0x76,  /*  *** **  */ 
  0x37,  /*   ** *** */ 
  0x5f,  /*  * ***** */ 
  0x5f,  /*  * ***** */ 
  0x37,  /*   ** *** */ 
  0x76,  /*  *** **  */ 
  0x5e,  /*  * ****  */ 
  0x9e,  /* *  ****  */ 
  0x9c,  /* *  ***   */ 


 /* Char 67 is 8px wide @ 23 */
  0x58,  /*  * **    */ 
  0xbc,  /* * ****   */ 
  0x16,  /*    * **  */ 
  0x3f,  /*   ****** */ 
  0x3f,  /*   ****** */ 
  0x16,  /*    * **  */ 
  0xbc,  /* * ****   */ 
  0x58,  /*  * **    */ 


 /* Char 80 is 7px wide @ 31 */
  0xc0,  /* **       */ 
  0xec,  /* *** **   */ 
  0x7e,  /*  ******  */ 
  0x3f,  /*   ****** */ 
  0x7e,  /*  ******  */ 
  0xec,  /* *** **   */ 
  0xc0,  /* **       */ 


 /* Char 85 is 16px wide @ 38 */
  0x20,  /*   *     */ 
  0x30,  /*   **    */ 
  0x78,  /*  ****   */ 
  0xec,  /* *** **  */ 
  0x7c,  /*  *****  */ 
  0x3c,  /*   ****  */ 
  0x2e,  /*   * *** */ 
  0x7e,  /*  ****** */ 
  0x7e,  /*  ****** */ 
  0x2e,  /*   * *** */ 
  0x3c,  /*   ****  */ 
  0x7c,  /*  *****  */ 
  0xec,  /* *** **  */ 
  0x78,  /*  ****   */ 
  0x30,  /*   **    */ 
  0x20,  /*   *     */ 


 /* Char 97 is 11px wide @ 54 */
  0x9e,  /* *  ****  */ 
  0x38,  /*   ***    */ 
  0x7d,  /*  ***** * */ 
  0x36,  /*   ** **  */ 
  0x3c,  /*   ****   */ 
  0x3c,  /*   ****   */ 
  0x3c,  /*   ****   */ 
  0x36,  /*   ** **  */ 
  0x7d,  /*  ***** * */ 
  0x38,  /*   ***    */ 
  0x9e,  /* *  ****  */ 


 /* Char 98 is 12px wide @ 65 */
  0x1c,  /*    ***   */ 
  0x5e,  /*  * ****  */ 
  0xfe,  /* *******  */ 
  0xb6,  /* * ** **  */ 
  0x37,  /*   ** *** */ 
  0x5f,  /*  * ***** */ 
  0x5f,  /*  * ***** */ 
  0x37,  /*   ** *** */ 
  0xb6,  /* * ** **  */ 
  0xfe,  /* *******  */ 
  0x5e,  /*  * ****  */ 
  0x1c,  /*    ***   */ 


 /* Char 99 is 8px wide @ 77 */
  0x98,  /* *  **    */ 
  0x5c,  /*  * ***   */ 
  0xb6,  /* * ** **  */ 
  0x5f,  /*  * ***** */ 
  0x5f,  /*  * ***** */ 
  0xb6,  /* * ** **  */ 
  0x5c,  /*  * ***   */ 
  0x98,  /* *  **    */ 


};

/* Character descriptors */
const FONT_CHAR_INFO InvadersLengths[] = {
 {11}, /* A */
 {12}, /* B */
 { 8}, /* C */
 { 7}, /* P */
 {16}, /* U */
 {11}, /* a */
 {12}, /* b */
 { 8}, /* c */
};

const uint16_t InvadersExtra[] = {
80,85,97,98,99,65535
};

/* Font info */
const struct FONT_DEF Font_Invaders = {
	  0,   /* width (1 == compressed) */
	  8,   /* character height */
	 65,   /* first char */
	 67,   /* last char */
    InvadersBitmaps, InvadersLengths, InvadersExtra
};

/* Font metadata: 
 * Name:          Invaders
 * Height:        8 px (1 bytes)
 * Maximum width: 16 px
 * Storage size:  93 bytes (uncompressed)
 */
