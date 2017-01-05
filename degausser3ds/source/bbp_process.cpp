#include "comm.h"
#include "bbp_process.h"

u32 align_to_4(u32 a)
{
	return (a+3)&(~3);
}



int get_u32(u8 *buf,int index)
{

}
int set_u32(u8 *buf,int index,u32 value)
{
	*((u32*)(buf+index))=value;
	return 0;
}
int get_u16(u8 *buf,int index)
{

}
int set_u16(u8 *buf,int index,int value)
{

}


	int Bbp:: init(u8 * buf,u32 size)
	{
		if(size >sizeof(raw))
			return -1;
		for(u32 i=0;i<size;i++)
			raw[i]=buf[i];
		this->size=size;
		return 0;
	}
	int Bbp:: set_id(u32 id)
	{
		item.ID=id;
		set_u32(main,4,id);
		return 0;
	}
	int Bbp:: raw_to_bbp()
	{
		int res;
		memcpy((u8*)&item,raw,sizeof(item));
		memcpy((u8*)&header,raw+sizeof(item)/*312*/,sizeof(header));
		debugprintf("\nmain start=%d\n ",header.main_start);
		if ((res=gz_decompress2(main,&size_main,raw+sizeof(item)+header.main_start,header.main_size))!=0)
		{
			debugprintf("\ndecompress main fail,res=%d\n",res);
			return -1;
		}
		if(header.has_vol==1)
		{
			if((res=gz_decompress2(vol,&size_vol,raw+sizeof(item)+header.vol_start,header.vol_size))!=0)
			{
				debugprintf("\ndecompress vol fail,res=%d\n",res);
				return -1;
			}
		}
		return 0;
	}
	int Bbp:: bbp_to_raw()
	{
		int res;
		memset(raw,0,size);
		memcpy(raw,(u8*)&item,sizeof(item));

		if((res=gz_compress(raw+header.main_start+sizeof(item),&header.main_size,main,size_main)!=0))
		{
			debugprintf("\ngz_compress main fail,res=%d\n",res);
			return -1;
		}
		size=header.main_start+sizeof(item)+header.main_size;

		if(header.has_vol==1)
		{
			header.vol_start=align_to_4(header.main_start+header.main_size);
			if((res=gz_compress(raw+header.vol_start+sizeof(item),&header.vol_size,vol,size_vol)!=0))
			{
				debugprintf("\ngz_compress vol fail,res=%d\n",res);
				return -1;
			}
			size=header.vol_start+sizeof(item)+header.vol_size;
		}
		memcpy(raw+sizeof(item)/*312*/,(u8*)&header,sizeof(header));

	}
	int Bbp:: raw_size()
	{
		return size;
	}
	u8* Bbp:: get_raw()
	{
		return this->raw;
	}

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
    printf("good so far @%d",__LINE__);
    fflush(stdout);
    bbp.init(buf,size);
    printf("good so far @%d",__LINE__);
    fflush(stdout);
    bbp.raw_to_bbp();
    bbp.bbp_to_raw();
    FILE *fp2=fopen("output.bbp","wb");
    fwrite(bbp.get_raw(),1,bbp.raw_size(),fp2);
    fclose(fp2);


    Bbp bbp2;
    bbp2.init(bbp.get_raw(),bbp.raw_size());
    bbp2.raw_to_bbp();
    bbp2.bbp_to_raw();
    FILE *fp3=fopen("output2.bbp","wb");
    fwrite(bbp2.get_raw(),1,bbp2.raw_size(),fp3);
    fclose(fp3);

}
#endif
