/* packet-cisco-marker.c
 * Routines for CISCO's ERSPAN3 Marker Packet
 * See: http://www.cisco.com/c/en/us/products/collateral/switches/nexus-9000-series-switches/white-paper-c11-733921.html#_Toc413144488
 * See: https://www.cisco.com/c/en/us/td/docs/switches/datacenter/nexus9000/sw/93x/system-management/b-cisco-nexus-9000-series-nx-os-system-management-configuration-guide-93x/b-cisco-nexus-9000-series-nx-os-system-management-configuration-guide-93x_chapter_011110.html
 * Copyright 2015, Peter Membrey
 *
 * Wireshark - Network traffic analyzer
 * By Gerald Combs <gerald@wireshark.org>
 * Copyright 1998 Gerald Combs
 *
 * Copied from packet-time.c
 * Fixed with additional documentation from Cisco and real-life observations
 * by Stéphane Lapie <stephane.lapie@darkbsd.org>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#define NEW_PROTO_TREE_API

#include "config.h"

#include <epan/packet.h>

void proto_register_erspan_marker(void);
void proto_reg_handoff_erspan_marker(void);


static dissector_handle_t marker_handle;

static header_field_info *hfi_marker = NULL;


#define CISCO_ERSPAN_MARKER_HFI_INIT HFI_INIT(proto_marker)

static header_field_info cisco_erspan_prop_header CISCO_ERSPAN_MARKER_HFI_INIT =
{ "Proprietary CISCO Header", "cisco_erspan_marker.prop_header",
  FT_BYTES, BASE_NONE, NULL, 0x0,
  NULL, HFILL };

static header_field_info cisco_erspan_info CISCO_ERSPAN_MARKER_HFI_INIT =
{ "Header", "cisco_erspan_marker.header",
  FT_BOOLEAN, 8, NULL, 0x0,
  NULL, HFILL };

static header_field_info cisco_erspan_version CISCO_ERSPAN_MARKER_HFI_INIT =
{ "Version", "cisco_erspan_marker.version",
  FT_UINT16, BASE_DEC, NULL, 0x0f00,
  NULL, HFILL };

static header_field_info cisco_erspan_type CISCO_ERSPAN_MARKER_HFI_INIT =
{ "Type", "cisco_erspan_marker.type",
  FT_UINT16, BASE_DEC, NULL, 0xf000,
  NULL, HFILL };

static header_field_info cisco_erspan_ssid CISCO_ERSPAN_MARKER_HFI_INIT =
{ "SSID", "cisco_erspan_marker.ssid",
  FT_UINT16, BASE_DEC, NULL, 0x00ff,
  NULL, HFILL };

static header_field_info cisco_erspan_granularity CISCO_ERSPAN_MARKER_HFI_INIT =
{ "Granularity", "cisco_erspan_marker.granularity",
  FT_UINT16, BASE_DEC, NULL, 0xff00,
  NULL, HFILL };

static header_field_info cisco_erspan_utcoffset CISCO_ERSPAN_MARKER_HFI_INIT =
{ "UTC Offset", "cisco_erspan_marker.utc_offset",
  FT_UINT16, BASE_DEC, NULL, 0x00ff,
  NULL, HFILL };

/* Timestamp is actually a 48-bit value, packed across 2 32-bit integers
 * Timestamp_hi : 0000 ffff (high 16-bits)
 * Timestamp_lo : ffff ffff (low 32-bits) */
static header_field_info cisco_erspan_timestamp CISCO_ERSPAN_MARKER_HFI_INIT =
{ "ASIC 48-bit Timestamp", "cisco_erspan_marker.timestamp",
  FT_UINT48, BASE_DEC, NULL, 0xffffffffffff,
  NULL, HFILL };

/* Comparison between the actual packet arrival time and this field
 * indicated that the Ethernet packet's arrival time was behind
 * the below field value by the value of the  UTC offset
 * (37 seconds as of Nov 2021) */
static header_field_info cisco_erspan_utc_sec CISCO_ERSPAN_MARKER_HFI_INIT =
{ "TAI Seconds", "cisco_erspan_marker.utc_sec",
  FT_UINT32, BASE_DEC, NULL, 0xffffffff,
  NULL, HFILL };

static header_field_info cisco_erspan_utc_usec CISCO_ERSPAN_MARKER_HFI_INIT =
{ "TAI Microseconds", "cisco_erspan_marker.utc_usec",
  FT_UINT32, BASE_DEC, NULL, 0xffffffff,
  NULL, HFILL };

static header_field_info cisco_erspan_sequence_number CISCO_ERSPAN_MARKER_HFI_INIT =
{ "Sequence Number", "cisco_erspan_marker.sequence_number",
  FT_UINT32, BASE_DEC, NULL, 0xffffffff,
  NULL, HFILL };

static header_field_info cisco_erspan_reserved CISCO_ERSPAN_MARKER_HFI_INIT =
{ "Reserved", "cisco_erspan_marker.reserved",
  FT_UINT32, BASE_DEC, NULL, 0xffffffff,
  NULL, HFILL };

/* The 32-bit signature is expected to be 0xA5A5A5A5,
 * and while the Cisco documentation does not mention packing details,
 * it does mention padding values to enforce alignment */
static header_field_info cisco_erspan_tail CISCO_ERSPAN_MARKER_HFI_INIT =
{ "TAIL", "cisco_erspan_marker.tail",
  FT_UINT64, BASE_HEX, NULL, 0x00000000ffffffff,
  NULL, HFILL };





static gint ett_marker = -1;


static int
dissect_marker(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, void* data _U_)
{
  proto_tree    *marker_tree;
  proto_item    *ti;

  col_set_str(pinfo->cinfo, COL_PROTOCOL, "CISCO ERSPAN3 MARKER");


  if (tree) {

    /* Skip the proprietary CISCO header - no docs have been released for this */
    guint32 offset = 20;

    ti = proto_tree_add_item(tree, hfi_marker, tvb, 0, -1, ENC_NA);
    marker_tree = proto_item_add_subtree(ti, ett_marker);

    proto_tree_add_item(marker_tree, &cisco_erspan_prop_header, tvb, 0, 20, ENC_LITTLE_ENDIAN);
    proto_tree_add_item(marker_tree, &cisco_erspan_info, tvb, offset, 2, ENC_LITTLE_ENDIAN);
    offset += 2;

    proto_tree_add_item(marker_tree, &cisco_erspan_version, tvb, offset, 2, ENC_BIG_ENDIAN);
    proto_tree_add_item(marker_tree, &cisco_erspan_type, tvb, offset, 2, ENC_BIG_ENDIAN);
    proto_tree_add_item(marker_tree, &cisco_erspan_ssid, tvb, offset, 2, ENC_BIG_ENDIAN);
    offset += 2;

    proto_tree_add_item(marker_tree, &cisco_erspan_granularity, tvb, offset, 2, ENC_BIG_ENDIAN);
    proto_tree_add_item(marker_tree, &cisco_erspan_utcoffset, tvb, offset, 2, ENC_BIG_ENDIAN);
    offset+= 2;

    proto_tree_add_item(marker_tree, &cisco_erspan_timestamp, tvb, offset, 8, ENC_BIG_ENDIAN);
    offset+=8;

    proto_tree_add_item(marker_tree, &cisco_erspan_utc_sec, tvb, offset, 4, ENC_BIG_ENDIAN);
    offset+=4;

    proto_tree_add_item(marker_tree, &cisco_erspan_utc_usec, tvb, offset, 4, ENC_BIG_ENDIAN);
    offset+=4;

    proto_tree_add_item(marker_tree, &cisco_erspan_sequence_number, tvb, offset, 4, ENC_LITTLE_ENDIAN);
    offset+=4;

    proto_tree_add_item(marker_tree, &cisco_erspan_reserved, tvb, offset, 4, ENC_LITTLE_ENDIAN);
    offset+=4;

    proto_tree_add_item(marker_tree, &cisco_erspan_tail, tvb, offset, 8, ENC_BIG_ENDIAN);
  }
  return tvb_captured_length(tvb);
}


void
proto_register_erspan_marker(void)
{


#ifndef HAVE_HFI_SECTION_INIT
  static header_field_info *hfi[] = {
    &cisco_erspan_prop_header,
    &cisco_erspan_info,
    &cisco_erspan_version,
    &cisco_erspan_type,
    &cisco_erspan_ssid,
    &cisco_erspan_granularity,
    &cisco_erspan_utcoffset,
    &cisco_erspan_timestamp,
    &cisco_erspan_utc_sec,
    &cisco_erspan_utc_usec,
    &cisco_erspan_sequence_number,
    &cisco_erspan_reserved,
    &cisco_erspan_tail
  };
#endif

  static gint *ett[] = {
    &ett_marker,
  };

  int proto_marker;



  proto_marker = proto_register_protocol("CISCO ERSPAN3 Marker Packet", "CISCO3 ERSPAN MARKER", "cisco-erspan3-marker");

  hfi_marker = proto_registrar_get_nth(proto_marker);

  proto_register_fields(proto_marker, hfi, array_length(hfi));
  proto_register_subtree_array(ett, array_length(ett));

  marker_handle = create_dissector_handle(dissect_marker, proto_marker);
}

void
proto_reg_handoff_erspan_marker(void)
{
  dissector_add_for_decode_as_with_preference("udp.port", marker_handle);
}

/*
 * Editor modelines  -  https://www.wireshark.org/tools/modelines.html
 *
 * Local Variables:
 * c-basic-offset: 2
 * tab-width: 8
 * indent-tabs-mode: nil
 * End:
 *
 * ex: set shiftwidth=2 tabstop=8 expandtab:
 * :indentSize=2:tabSize=8:noTabs=true:
 */
