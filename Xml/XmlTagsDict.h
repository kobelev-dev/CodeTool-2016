//
//      Kirill Kobelev, Moscow-Paris-Sammamish.
//  -------------------------------------------------
//   All rights reserved. Commercial use without written permission prohibited.
//
//   Simple XML reader/writer.
//

#ifndef	Common_XmlTagsDict_H
#define	Common_XmlTagsDict_H

//
//  Possible types of records in the dictionary of the XML tags.
//
enum TXmlDictRecordType : unsigned char
{
	xdrt_none	= 0,			// End of the tags dictionary. Every tags dictionary should be terminated with this record.
							// All other fields in the record with this type are not important.
	xdrt_attr		= 1,			// Attribute in the tag header. This tag can be picked up and written only as a tag attribute.
	xdrt_field		= 2,			// Leaf tag that cannot have subtags. This tag can be picked up and written to only as a leaf tag.
	xdrt_cnr		= 4,			// Container with tag attrs, tags and subcontainers. When an XML file is scanned and it contains
							// xlt_leaf_tag lexema, that corresponds to this dict record, this situation is treated as syntax error.
	xdrt_atrfl		= 3,			// When the XML file is scanned, reader will accept both the tag attr and the leaf tag. When the XML
							// file is being written, this record type is equivalent to xdrt_attr.
	xdrt_eof		= 0,			// Alias of the xdrt_none.
};

//
//  Mandatory/Optional and SingleInst/MultipleInst properties of the tag.
//
enum TXmlItemReqDupMode : unsigned char
{
	xrdm_opt_si,				// The item is optional, not more than one instance.
	xrdm_opt_mi,				// The item is optional, duplications are allowed.
	xrdm_req_si,				// The item is mandatory, not more than one instance.
	xrdm_req_mi,				// The item is mandatory, duplications are allowed.
};

//
//  Type of the body of the tag attribute or the body of the leaf tag.
//
enum TXmlItemBodyType : unsigned char
{
	xtbt_none,				// This value cannot be used in the dict records other than eof and container.
	xtbt_bool,				// Bool value: true, false, yes, no, 0, 1, etc.
	xtbt_number,				// 64 bit signed decimal number.
	xtbt_string,				// String of text.
	xtbt_guid,				// Guid as a string of hex digits.
	xtbt_color,				// Color in the RGB representation, like: "232,16,48" or the keyword "Transp".
};

//
//  This definition should be used for initing the TXmlDictRecord structures with constants.
//
#define DefTag(s)	(short)(sizeof(s)/sizeof(wchar_t)-1), (s)

//
//  For now the ext flags field is a placeholder for future extensions.
//
enum TXmlItemExtFlags : unsigned short
{
	xtef_none,
};

//
//  Major structure in this file. Note that the name of the tag is always a simple explicit string.
//  Wildcards, tag groups, etc are not supported in this implementation.
//
struct TXmlDictRecord
{
	TXmlDictRecordType			m_rec_type;
	TXmlItemReqDupMode			m_req_dup_mode;
	TXmlItemBodyType			m_body_type;

	BYTE						m_tag_name_len;			// Length of the name cannot be ZERO except for the EOF records.
	wchar_t						*m_tag_name;				// The tag name field cannot be NULL except for the EOF records. Length
															// of the name is present here for perf reasons. Nevertheless all tag names
															// in the dictionary should be NULL terminated.
	WORD						m_tag_ident;				// The tag id cannot be zero. This is checked in the code with assert().

	TXmlItemExtFlags				m_ext_flags;

	const TXmlDictRecord			*m_inner_schema;			// Innner schema is required only for container records.
};

#endif	// Common_XmlTagsDict_H


