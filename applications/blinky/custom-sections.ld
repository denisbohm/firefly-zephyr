SECTION_PROLOGUE(executable_metadata_section_name,0x80180,)
{
	KEEP(*(".executable_metadata_section"));
} GROUP_ROM_LINK_IN(RAMABLE_REGION, ROMABLE_REGION)
