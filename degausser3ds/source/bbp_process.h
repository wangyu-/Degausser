struct Header
{
	u32 unused[3];
	u32 main_start;//3
	u32 main_size;//4
	u32 has_vol;//5
	u32 unused2;//vocal id?
	u32 vol_start;//7
	u32 vol_size;//8
	u32 unused3[8];
};
struct Bbp
{
	u8 raw[1024*1024];
	u32 size;
	_JbMgrItem item;
	Header header;
	u8 main[1024*1024];
	u32 size_main;
	u8 vol[1024*1024];
	u32 size_vol;
	int init(u8 * buf,u32 size);
	int set_id(u32 id);
	int raw_to_bbp();
	int bbp_to_raw();
	int raw_size();
	u8* get_raw();
};
