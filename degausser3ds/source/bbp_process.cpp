#include "comm.h"






int get_u32(u8 *buf,int index)
{

}
int set_u32(u8 *buf,int index,int value)
{

}
int get_u16(u8 *buf,int index)
{

}
int set_u16(u8 *buf,int index,int value)
{

}
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
	int init(u8 * buf,u32 size)
	{
		if(size >sizeof(raw))
			return -1;
		for(u32 i=0;i<size;i++)
			raw[i]=buf[i];
		this->size=size;
		return 0;
	}
	int raw_to_bbp()
	{
		int res;
		memcpy((u8*)&item,raw,sizeof(item));
		memcpy((u8*)&header,raw+sizeof(item)/*312*/,sizeof(header));
		testprintf("main start=%d ",header.main_start);
		if ((res=gz_decompress2(main,&size_main,raw+sizeof(item)+header.main_start,header.main_size))!=0)
		{
			testprintf("decompress main fail,res=%d",res);
			return -1;
		}
		if(header.has_vol==1)
		{
			if((res=gz_decompress2(vol,&size_vol,raw+sizeof(item)+header.vol_start,header.vol_size))!=0)
			{
				testprintf("decompress vol fail,res=%d",res);
				return -1;
			}
		}
		return 0;
	}
	int bbp_to_raw()
	{
		int res;
		memcpy(raw,(u8*)&item,sizeof(item));

		if((res=gz_compress(raw+header.main_start+sizeof(item),&header.main_size,main,size_main)!=0))
		{
			testprintf("gz_compress main fail,res=%d",res);
			return -1;
		}
		size=raw+header.main_start+sizeof(item)+header.main_size;

		if(header.has_vol==1)
		{
			if((res=gz_compress(raw+header.vol_start+sizeof(item),&header.vol_size,vol,size_vol)!=0))
			{
				testprintf("gz_compress vol fail,res=%d",res);
				return -1;
			}
			header.vol_start=header.main_start+header.main_size;
			size=raw+header.vol_start+sizeof(item)+header.vol_size;
		}
		memcpy(raw+sizeof(item)/*312*/,(u8*)&header,sizeof(header));

	}
	int raw_size()
	{
		return size;
	}
	u8* get_raw()
	{
		return this->raw;
	}
};

#ifdef TEST
u8 buffer[524288];
int main()
{
	printf("%d ",sizeof(_JbMgrItem));
	u8 buf[1024*1024];
	FILE *fp;
	int size;
	fp=fopen("./test.bbp","rb");
	printf("%d",fp);
	fflush(stdout);
    fseek (fp, 0 , SEEK_END);
    size= ftell (fp);
    rewind (fp);
    fread (buf,1,size,fp);
    Bbp bbp;
    printf("good so far");
    fflush(stdout);
    bbp.init(buf,size);
    printf("good so far2");
    fflush(stdout);
    bbp.raw_to_bbp();

}
#endif