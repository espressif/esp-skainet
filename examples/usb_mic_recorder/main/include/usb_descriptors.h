/* SPDX-FileCopyrightText: 2022-2023 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef _USB_DESCRIPTORS_H_
#define _USB_DESCRIPTORS_H_

//--------------------------------------------------------------------+
// Audio v2.0 Descriptor Templates
//--------------------------------------------------------------------+

/* Standard Interface Association Descriptor (IAD) */
#define TUD_AUDIO_DESC_IAD_LEN 8
#define TUD_AUDIO_DESC_IAD(_firstitfs, _nitfs, _stridx) \
  TUD_AUDIO_DESC_IAD_LEN, TUSB_DESC_INTERFACE_ASSOCIATION, _firstitfs, _nitfs, TUSB_CLASS_AUDIO, AUDIO_FUNCTION_SUBCLASS_UNDEFINED, AUDIO_FUNC_PROTOCOL_CODE_V2, _stridx

/* Standard AC Interface Descriptor(4.7.1) */
#define TUD_AUDIO_DESC_STD_AC_LEN 9
#define TUD_AUDIO_DESC_STD_AC(_itfnum, _nEPs, _stridx) /* _nEPs is 0 or 1 */\
  TUD_AUDIO_DESC_STD_AC_LEN, TUSB_DESC_INTERFACE, _itfnum, /* fixed to zero */ 0x00, _nEPs, TUSB_CLASS_AUDIO, AUDIO_SUBCLASS_CONTROL, AUDIO_INT_PROTOCOL_CODE_V2, _stridx

/* Class-Specific AC Interface Header Descriptor(4.7.2) */
#define TUD_AUDIO_DESC_CS_AC_LEN 9
#define TUD_AUDIO_DESC_CS_AC(_bcdADC, _category, _totallen, _ctrl) /* _bcdADC : Audio Device Class Specification Release Number in Binary-Coded Decimal, _category : see audio_function_t, _totallen : Total number of bytes returned for the class-specific AudioControl interface i.e. Clock Source, Unit and Terminal descriptors - Do not include TUD_AUDIO_DESC_CS_AC_LEN, we already do this here*/ \
  TUD_AUDIO_DESC_CS_AC_LEN, TUSB_DESC_CS_INTERFACE, AUDIO_CS_AC_INTERFACE_HEADER, U16_TO_U8S_LE(_bcdADC), _category, U16_TO_U8S_LE(_totallen + TUD_AUDIO_DESC_CS_AC_LEN), _ctrl

/* Clock Source Descriptor(4.7.2.1) */
#define TUD_AUDIO_DESC_CLK_SRC_LEN 8
#define TUD_AUDIO_DESC_CLK_SRC(_clkid, _attr, _ctrl, _assocTerm, _stridx) \
  TUD_AUDIO_DESC_CLK_SRC_LEN, TUSB_DESC_CS_INTERFACE, AUDIO_CS_AC_INTERFACE_CLOCK_SOURCE, _clkid, _attr, _ctrl, _assocTerm, _stridx

/* Input Terminal Descriptor(4.7.2.4) */
#define TUD_AUDIO_DESC_INPUT_TERM_LEN 17
#define TUD_AUDIO_DESC_INPUT_TERM(_termid, _termtype, _assocTerm, _clkid, _nchannelslogical, _channelcfg, _idxchannelnames, _ctrl, _stridx) \
  TUD_AUDIO_DESC_INPUT_TERM_LEN, TUSB_DESC_CS_INTERFACE, AUDIO_CS_AC_INTERFACE_INPUT_TERMINAL, _termid, U16_TO_U8S_LE(_termtype), _assocTerm, _clkid, _nchannelslogical, U32_TO_U8S_LE(_channelcfg), _idxchannelnames, U16_TO_U8S_LE(_ctrl), _stridx

/* Output Terminal Descriptor(4.7.2.5) */
#define TUD_AUDIO_DESC_OUTPUT_TERM_LEN 12
#define TUD_AUDIO_DESC_OUTPUT_TERM(_termid, _termtype, _assocTerm, _srcid, _clkid, _ctrl, _stridx) \
  TUD_AUDIO_DESC_OUTPUT_TERM_LEN, TUSB_DESC_CS_INTERFACE, AUDIO_CS_AC_INTERFACE_OUTPUT_TERMINAL, _termid, U16_TO_U8S_LE(_termtype), _assocTerm, _srcid, _clkid, U16_TO_U8S_LE(_ctrl), _stridx

/* Feature Unit Descriptor(4.7.2.8) */
// 1 - Channel
#define TUD_AUDIO_DESC_FEATURE_UNIT_ONE_CHANNEL_LEN 6+(1+1)*4
#define TUD_AUDIO_DESC_FEATURE_UNIT_ONE_CHANNEL(_unitid, _srcid, _ctrlch0master, _ctrlch1, _stridx) \
  TUD_AUDIO_DESC_FEATURE_UNIT_ONE_CHANNEL_LEN, TUSB_DESC_CS_INTERFACE, AUDIO_CS_AC_INTERFACE_FEATURE_UNIT, _unitid, _srcid, U32_TO_U8S_LE(_ctrlch0master), U32_TO_U8S_LE(_ctrlch1), _stridx

// 2 - Channels
#define TUD_AUDIO_DESC_FEATURE_UNIT_TWO_CHANNEL_LEN (6+(2+1)*4)
#define TUD_AUDIO_DESC_FEATURE_UNIT_TWO_CHANNEL(_unitid, _srcid, _ctrlch0master, _ctrlch1, _ctrlch2, _stridx) \
  TUD_AUDIO_DESC_FEATURE_UNIT_TWO_CHANNEL_LEN, TUSB_DESC_CS_INTERFACE, AUDIO_CS_AC_INTERFACE_FEATURE_UNIT, _unitid, _srcid, U32_TO_U8S_LE(_ctrlch0master), U32_TO_U8S_LE(_ctrlch1), U32_TO_U8S_LE(_ctrlch2), _stridx

// 3 - Channels
#define TUD_AUDIO_DESC_FEATURE_UNIT_THREE_CHANNEL_LEN (6+(3+1)*4)
#define TUD_AUDIO_DESC_FEATURE_UNIT_THREE_CHANNEL(_unitid, _srcid, _ctrlch0master, _ctrlch1, _ctrlch2, _ctrlch3, _stridx) \
  TUD_AUDIO_DESC_FEATURE_UNIT_THREE_CHANNEL_LEN, TUSB_DESC_CS_INTERFACE, AUDIO_CS_AC_INTERFACE_FEATURE_UNIT, _unitid, _srcid, U32_TO_U8S_LE(_ctrlch0master), U32_TO_U8S_LE(_ctrlch1), U32_TO_U8S_LE(_ctrlch2), U32_TO_U8S_LE(_ctrlch3), _stridx

// 4 - Channels
#define TUD_AUDIO_DESC_FEATURE_UNIT_FOUR_CHANNEL_LEN (6+(4+1)*4)
#define TUD_AUDIO_DESC_FEATURE_UNIT_FOUR_CHANNEL(_unitid, _srcid, _ctrlch0master, _ctrlch1, _ctrlch2, _ctrlch3, _ctrlch4, _stridx) \
  TUD_AUDIO_DESC_FEATURE_UNIT_FOUR_CHANNEL_LEN, TUSB_DESC_CS_INTERFACE, AUDIO_CS_AC_INTERFACE_FEATURE_UNIT, _unitid, _srcid, U32_TO_U8S_LE(_ctrlch0master), U32_TO_U8S_LE(_ctrlch1), U32_TO_U8S_LE(_ctrlch2), U32_TO_U8S_LE(_ctrlch3), U32_TO_U8S_LE(_ctrlch4), _stridx

// For more channels, add definitions here

/* Standard AS Interface Descriptor(4.9.1) */
#define TUD_AUDIO_DESC_STD_AS_INT_LEN 9
#define TUD_AUDIO_DESC_STD_AS_INT(_itfnum, _altset, _nEPs, _stridx) \
  TUD_AUDIO_DESC_STD_AS_INT_LEN, TUSB_DESC_INTERFACE, _itfnum, _altset, _nEPs, TUSB_CLASS_AUDIO, AUDIO_SUBCLASS_STREAMING, AUDIO_INT_PROTOCOL_CODE_V2, _stridx

/* Class-Specific AS Interface Descriptor(4.9.2) */
#define TUD_AUDIO_DESC_CS_AS_INT_LEN 16
#define TUD_AUDIO_DESC_CS_AS_INT(_termid, _ctrl, _formattype, _formats, _nchannelsphysical, _channelcfg, _stridx) \
  TUD_AUDIO_DESC_CS_AS_INT_LEN, TUSB_DESC_CS_INTERFACE, AUDIO_CS_AS_INTERFACE_AS_GENERAL, _termid, _ctrl, _formattype, U32_TO_U8S_LE(_formats), _nchannelsphysical, U32_TO_U8S_LE(_channelcfg), _stridx

/* Type I Format Type Descriptor(2.3.1.6 - Audio Formats) */
#define TUD_AUDIO_DESC_TYPE_I_FORMAT_LEN 6
#define TUD_AUDIO_DESC_TYPE_I_FORMAT(_subslotsize, _bitresolution) /* _subslotsize is number of bytes per sample (i.e. subslot) and can be 1,2,3, or 4 */\
  TUD_AUDIO_DESC_TYPE_I_FORMAT_LEN, TUSB_DESC_CS_INTERFACE, AUDIO_CS_AS_INTERFACE_FORMAT_TYPE, AUDIO_FORMAT_TYPE_I, _subslotsize, _bitresolution

/* Standard AS Isochronous Audio Data Endpoint Descriptor(4.10.1.1) */
#define TUD_AUDIO_DESC_STD_AS_ISO_EP_LEN 7
#define TUD_AUDIO_DESC_STD_AS_ISO_EP(_ep, _attr, _maxEPsize, _interval) \
  TUD_AUDIO_DESC_STD_AS_ISO_EP_LEN, TUSB_DESC_ENDPOINT, _ep, _attr, U16_TO_U8S_LE(_maxEPsize), _interval

/* Class-Specific AS Isochronous Audio Data Endpoint Descriptor(4.10.1.2) */
#define TUD_AUDIO_DESC_CS_AS_ISO_EP_LEN 8
#define TUD_AUDIO_DESC_CS_AS_ISO_EP(_attr, _ctrl, _lockdelayunit, _lockdelay) \
  TUD_AUDIO_DESC_CS_AS_ISO_EP_LEN, TUSB_DESC_CS_ENDPOINT, AUDIO_CS_EP_SUBTYPE_GENERAL, _attr, _ctrl, _lockdelayunit, U16_TO_U8S_LE(_lockdelay)

/* Standard AS Isochronous Feedback Endpoint Descriptor(4.10.2.1) */
#define TUD_AUDIO_DESC_STD_AS_ISO_FB_EP_LEN 7

// AUDIO simple descriptor (UAC2) for 1 microphone input
// - 2 Input Terminal, 1 Feature Unit (Mute and Volume Control), 1 Output Terminal, 1 Clock Source

#define TUD_AUDIO_MIC_TWO_CH_DESC_LEN (TUD_AUDIO_DESC_IAD_LEN\
  + TUD_AUDIO_DESC_STD_AC_LEN\
  + TUD_AUDIO_DESC_CS_AC_LEN\
  + TUD_AUDIO_DESC_CLK_SRC_LEN\
  + TUD_AUDIO_DESC_INPUT_TERM_LEN\
  + TUD_AUDIO_DESC_OUTPUT_TERM_LEN\
  + TUD_AUDIO_DESC_FEATURE_UNIT_TWO_CHANNEL_LEN\
  + TUD_AUDIO_DESC_STD_AS_INT_LEN\
  + TUD_AUDIO_DESC_STD_AS_INT_LEN\
  + TUD_AUDIO_DESC_CS_AS_INT_LEN\
  + TUD_AUDIO_DESC_TYPE_I_FORMAT_LEN\
  + TUD_AUDIO_DESC_STD_AS_ISO_EP_LEN\
  + TUD_AUDIO_DESC_CS_AS_ISO_EP_LEN)

#define TUD_AUDIO_MIC_TWO_CH_DESC_N_AS_INT 1    // Number of AS interfaces

#define TUD_AUDIO_MIC_TWO_CH_DESCRIPTOR(_itfnum, _stridx, _nBytesPerSample, _nBitsUsedPerSample, _epin, _epsize) \
  /* Standard Interface Association Descriptor (IAD) */\
  TUD_AUDIO_DESC_IAD(/*_firstitfs*/ _itfnum, /*_nitfs*/ 0x02, /*_stridx*/ 0x00),\
  /* Standard AC Interface Descriptor(4.7.1) */\
  TUD_AUDIO_DESC_STD_AC(/*_itfnum*/ _itfnum, /*_nEPs*/ 0x00, /*_stridx*/ _stridx),\
  /* Class-Specific AC Interface Header Descriptor(4.7.2) */\
  TUD_AUDIO_DESC_CS_AC(/*_bcdADC*/ 0x0200, /*_category*/ AUDIO_FUNC_MICROPHONE, /*_totallen*/ TUD_AUDIO_DESC_CLK_SRC_LEN+TUD_AUDIO_DESC_INPUT_TERM_LEN+TUD_AUDIO_DESC_OUTPUT_TERM_LEN+TUD_AUDIO_DESC_FEATURE_UNIT_TWO_CHANNEL_LEN, /*_ctrl*/ AUDIO_CS_AS_INTERFACE_CTRL_LATENCY_POS),\
  /* Clock Source Descriptor(4.7.2.1) */\
  TUD_AUDIO_DESC_CLK_SRC(/*_clkid*/ 0x04, /*_attr*/ AUDIO_CLOCK_SOURCE_ATT_INT_FIX_CLK, /*_ctrl*/ (AUDIO_CTRL_R << AUDIO_CLOCK_SOURCE_CTRL_CLK_FRQ_POS), /*_assocTerm*/ 0x01,  /*_stridx*/ 0x00),\
  /* Input Terminal Descriptor(4.7.2.4) */\
  TUD_AUDIO_DESC_INPUT_TERM(/*_termid*/ 0x01, /*_termtype*/ AUDIO_TERM_TYPE_IN_GENERIC_MIC, /*_assocTerm*/ 0x03, /*_clkid*/ 0x04, /*_nchannelslogical*/ 0x02, /*_channelcfg*/ AUDIO_CHANNEL_CONFIG_NON_PREDEFINED, /*_idxchannelnames*/ 0x00, /*_ctrl*/ AUDIO_CTRL_R << AUDIO_IN_TERM_CTRL_CONNECTOR_POS, /*_stridx*/ 0x00),\
  /* Output Terminal Descriptor(4.7.2.5) */\
  TUD_AUDIO_DESC_OUTPUT_TERM(/*_termid*/ 0x03, /*_termtype*/ AUDIO_TERM_TYPE_USB_STREAMING, /*_assocTerm*/ 0x01, /*_srcid*/ 0x02, /*_clkid*/ 0x04, /*_ctrl*/ 0x0000, /*_stridx*/ 0x00),\
  /* Feature Unit Descriptor(4.7.2.8) */\
  TUD_AUDIO_DESC_FEATURE_UNIT_TWO_CHANNEL(/*_unitid*/ 0x02, /*_srcid*/ 0x01, /*_ctrlch0master*/ AUDIO_CTRL_RW << AUDIO_FEATURE_UNIT_CTRL_MUTE_POS | AUDIO_CTRL_RW << AUDIO_FEATURE_UNIT_CTRL_VOLUME_POS, /*_ctrlch1*/ AUDIO_CTRL_RW << AUDIO_FEATURE_UNIT_CTRL_MUTE_POS | AUDIO_CTRL_RW << AUDIO_FEATURE_UNIT_CTRL_VOLUME_POS, /*_ctrlch2*/ AUDIO_CTRL_RW << AUDIO_FEATURE_UNIT_CTRL_MUTE_POS | AUDIO_CTRL_RW << AUDIO_FEATURE_UNIT_CTRL_VOLUME_POS, /*_stridx*/ 0x00),\
  /* Standard AS Interface Descriptor(4.9.1) */\
  /* Interface 1, Alternate 0 - default alternate setting with 0 bandwidth */\
  TUD_AUDIO_DESC_STD_AS_INT(/*_itfnum*/ (uint8_t)((_itfnum)+1), /*_altset*/ 0x00, /*_nEPs*/ 0x00, /*_stridx*/ 0x00),\
  /* Standard AS Interface Descriptor(4.9.1) */\
  /* Interface 1, Alternate 1 - alternate interface for data streaming */\
  TUD_AUDIO_DESC_STD_AS_INT(/*_itfnum*/ (uint8_t)((_itfnum)+1), /*_altset*/ 0x01, /*_nEPs*/ 0x01, /*_stridx*/ 0x00),\
  /* Class-Specific AS Interface Descriptor(4.9.2) */\
  TUD_AUDIO_DESC_CS_AS_INT(/*_termid*/ 0x03, /*_ctrl*/ AUDIO_CTRL_NONE, /*_formattype*/ AUDIO_FORMAT_TYPE_I, /*_formats*/ AUDIO_DATA_FORMAT_TYPE_I_PCM, /*_nchannelsphysical*/ 0x02, /*_channelcfg*/ AUDIO_CHANNEL_CONFIG_NON_PREDEFINED, /*_stridx*/ 0x00),\
  /* Type I Format Type Descriptor(2.3.1.6 - Audio Formats) */\
  TUD_AUDIO_DESC_TYPE_I_FORMAT(_nBytesPerSample, _nBitsUsedPerSample),\
  /* Standard AS Isochronous Audio Data Endpoint Descriptor(4.10.1.1) */\
  TUD_AUDIO_DESC_STD_AS_ISO_EP(/*_ep*/ _epin, /*_attr*/ (TUSB_XFER_ISOCHRONOUS | TUSB_ISO_EP_ATT_ASYNCHRONOUS | TUSB_ISO_EP_ATT_DATA), /*_maxEPsize*/ _epsize, /*_interval*/ TUD_OPT_HIGH_SPEED ? 0x04 : 0x01),\
  /* Class-Specific AS Isochronous Audio Data Endpoint Descriptor(4.10.1.2) */\
  TUD_AUDIO_DESC_CS_AS_ISO_EP(/*_attr*/ AUDIO_CS_AS_ISO_DATA_EP_ATT_NON_MAX_PACKETS_OK, /*_ctrl*/ AUDIO_CTRL_NONE, /*_lockdelayunit*/ AUDIO_CS_AS_ISO_DATA_EP_LOCK_DELAY_UNIT_UNDEFINED, /*_lockdelay*/ 0x0000)

// AUDIO simple descriptor (UAC2) for 3 microphone input
// - 1 Input Terminal, 1 Feature Unit (Mute and Volume Control), 1 Output Terminal, 1 Clock Source

#define TUD_AUDIO_MIC_THREE_CH_DESC_LEN (TUD_AUDIO_DESC_IAD_LEN\
  + TUD_AUDIO_DESC_STD_AC_LEN\
  + TUD_AUDIO_DESC_CS_AC_LEN\
  + TUD_AUDIO_DESC_CLK_SRC_LEN\
  + TUD_AUDIO_DESC_INPUT_TERM_LEN\
  + TUD_AUDIO_DESC_OUTPUT_TERM_LEN\
  + TUD_AUDIO_DESC_FEATURE_UNIT_THREE_CHANNEL_LEN\
  + TUD_AUDIO_DESC_STD_AS_INT_LEN\
  + TUD_AUDIO_DESC_STD_AS_INT_LEN\
  + TUD_AUDIO_DESC_CS_AS_INT_LEN\
  + TUD_AUDIO_DESC_TYPE_I_FORMAT_LEN\
  + TUD_AUDIO_DESC_STD_AS_ISO_EP_LEN\
  + TUD_AUDIO_DESC_CS_AS_ISO_EP_LEN)

#define TUD_AUDIO_MIC_THREE_CH_DESC_N_AS_INT 1  // Number of AS interfaces

#define TUD_AUDIO_MIC_THREE_CH_DESCRIPTOR(_itfnum, _stridx, _nBytesPerSample, _nBitsUsedPerSample, _epin, _epsize) \
  /* Standard Interface Association Descriptor (IAD) */\
  TUD_AUDIO_DESC_IAD(/*_firstitfs*/ _itfnum, /*_nitfs*/ 0x02, /*_stridx*/ 0x00),\
  /* Standard AC Interface Descriptor(4.7.1) */\
  TUD_AUDIO_DESC_STD_AC(/*_itfnum*/ _itfnum, /*_nEPs*/ 0x00, /*_stridx*/ _stridx),\
  /* Class-Specific AC Interface Header Descriptor(4.7.2) */\
  TUD_AUDIO_DESC_CS_AC(/*_bcdADC*/ 0x0200, /*_category*/ AUDIO_FUNC_MICROPHONE, /*_totallen*/ TUD_AUDIO_DESC_CLK_SRC_LEN+TUD_AUDIO_DESC_INPUT_TERM_LEN+TUD_AUDIO_DESC_OUTPUT_TERM_LEN+TUD_AUDIO_DESC_FEATURE_UNIT_THREE_CHANNEL_LEN, /*_ctrl*/ AUDIO_CS_AS_INTERFACE_CTRL_LATENCY_POS),\
  /* Clock Source Descriptor(4.7.2.1) */\
  TUD_AUDIO_DESC_CLK_SRC(/*_clkid*/ 0x04, /*_attr*/ AUDIO_CLOCK_SOURCE_ATT_INT_FIX_CLK, /*_ctrl*/ (AUDIO_CTRL_R << AUDIO_CLOCK_SOURCE_CTRL_CLK_FRQ_POS), /*_assocTerm*/ 0x01,  /*_stridx*/ 0x00),\
  /* Input Terminal Descriptor(4.7.2.4) */\
  TUD_AUDIO_DESC_INPUT_TERM(/*_termid*/ 0x01, /*_termtype*/ AUDIO_TERM_TYPE_IN_GENERIC_MIC, /*_assocTerm*/ 0x03, /*_clkid*/ 0x04, /*_nchannelslogical*/ 0x03, /*_channelcfg*/ AUDIO_CHANNEL_CONFIG_NON_PREDEFINED, /*_idxchannelnames*/ 0x00, /*_ctrl*/ AUDIO_CTRL_R << AUDIO_IN_TERM_CTRL_CONNECTOR_POS, /*_stridx*/ 0x00),\
  /* Output Terminal Descriptor(4.7.2.5) */\
  TUD_AUDIO_DESC_OUTPUT_TERM(/*_termid*/ 0x03, /*_termtype*/ AUDIO_TERM_TYPE_USB_STREAMING, /*_assocTerm*/ 0x01, /*_srcid*/ 0x02, /*_clkid*/ 0x04, /*_ctrl*/ 0x0000, /*_stridx*/ 0x00),\
  /* Feature Unit Descriptor(4.7.2.8) */\
  TUD_AUDIO_DESC_FEATURE_UNIT_THREE_CHANNEL(/*_unitid*/ 0x02, /*_srcid*/ 0x01, /*_ctrlch0master*/ AUDIO_CTRL_RW << AUDIO_FEATURE_UNIT_CTRL_MUTE_POS | AUDIO_CTRL_RW << AUDIO_FEATURE_UNIT_CTRL_VOLUME_POS, /*_ctrlch1*/ AUDIO_CTRL_RW << AUDIO_FEATURE_UNIT_CTRL_MUTE_POS | AUDIO_CTRL_RW << AUDIO_FEATURE_UNIT_CTRL_VOLUME_POS, /*_ctrlch2*/ AUDIO_CTRL_RW << AUDIO_FEATURE_UNIT_CTRL_MUTE_POS | AUDIO_CTRL_RW << AUDIO_FEATURE_UNIT_CTRL_VOLUME_POS, /*_ctrlch3*/ AUDIO_CTRL_RW << AUDIO_FEATURE_UNIT_CTRL_MUTE_POS | AUDIO_CTRL_RW << AUDIO_FEATURE_UNIT_CTRL_VOLUME_POS, /*_stridx*/ 0x00),\
  /* Standard AS Interface Descriptor(4.9.1) */\
  /* Interface 1, Alternate 0 - default alternate setting with 0 bandwidth */\
  TUD_AUDIO_DESC_STD_AS_INT(/*_itfnum*/ (uint8_t)((_itfnum)+1), /*_altset*/ 0x00, /*_nEPs*/ 0x00, /*_stridx*/ 0x00),\
  /* Standard AS Interface Descriptor(4.9.1) */\
  /* Interface 1, Alternate 1 - alternate interface for data streaming */\
  TUD_AUDIO_DESC_STD_AS_INT(/*_itfnum*/ (uint8_t)((_itfnum)+1), /*_altset*/ 0x01, /*_nEPs*/ 0x01, /*_stridx*/ 0x00),\
  /* Class-Specific AS Interface Descriptor(4.9.2) */\
  TUD_AUDIO_DESC_CS_AS_INT(/*_termid*/ 0x03, /*_ctrl*/ AUDIO_CTRL_NONE, /*_formattype*/ AUDIO_FORMAT_TYPE_I, /*_formats*/ AUDIO_DATA_FORMAT_TYPE_I_PCM, /*_nchannelsphysical*/ 0x03, /*_channelcfg*/ AUDIO_CHANNEL_CONFIG_NON_PREDEFINED, /*_stridx*/ 0x00),\
  /* Type I Format Type Descriptor(2.3.1.6 - Audio Formats) */\
  TUD_AUDIO_DESC_TYPE_I_FORMAT(_nBytesPerSample, _nBitsUsedPerSample),\
  /* Standard AS Isochronous Audio Data Endpoint Descriptor(4.10.1.1) */\
  TUD_AUDIO_DESC_STD_AS_ISO_EP(/*_ep*/ _epin, /*_attr*/ (TUSB_XFER_ISOCHRONOUS | TUSB_ISO_EP_ATT_ASYNCHRONOUS | TUSB_ISO_EP_ATT_DATA), /*_maxEPsize*/ _epsize, /*_interval*/ TUD_OPT_HIGH_SPEED ? 0x04 : 0x01),\
  /* Class-Specific AS Isochronous Audio Data Endpoint Descriptor(4.10.1.2) */\
  TUD_AUDIO_DESC_CS_AS_ISO_EP(/*_attr*/ AUDIO_CS_AS_ISO_DATA_EP_ATT_NON_MAX_PACKETS_OK, /*_ctrl*/ AUDIO_CTRL_NONE, /*_lockdelayunit*/ AUDIO_CS_AS_ISO_DATA_EP_LOCK_DELAY_UNIT_UNDEFINED, /*_lockdelay*/ 0x0000)

#endif