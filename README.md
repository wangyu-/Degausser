# Degausser original descriptions:



Save editor for Daigasso! Band Brothers P. There are currently two flavours:

1. In the `src` folder is the C# code for _Degausser_, intended to run on Windows. This version is able to import, export, convert .bdx and .bin files, as well as play the songs. This has to be used in conjunction with an extdata dumping/restoring tool.
2. In the `degausser3ds` folder is C code for the homebrew _degausser3ds_. This version is only able to import all songs in a fixed directory, or export all songs to a fixed directory. This is able to import/export directly from/to extdata.

In the future we might use this space to discuss some of the file formats, but otherwise feel free to check out the Releases.


# new features in this repo(degausser3ds only):

1. import songs as custom (so that it will be edit-able in game) from SD:\bbpimportc\

2. dump a songlist(will not dump BBP files)  to "SD:\bbpdump\songlist.txt"

3. delete specific songs by "SD:\bbpdelete\songlist.txt"

4. dump specific songs by "SD:\bbpdump\songlist.txt".

 "songlist.txt" should be in utf8 format (with or without BOM). one ID per line(a hex number of 8 digits),the rest characters of each line will be ignored. see degausser3ds/examples/ for more info. you can just copy the "songlist.txt" created by "dump songlist" feature and edit it.

 (Q:how do i know the ID of a song?A:by the "dump songlist" feature)


# how to use degausser3ds(for cfw user):

1. get hblauncher_loader.cia and install (from https://github.com/yellows8/hblauncher_loader/releases)

2. put degausser3ds.3dsx degausser3ds.smdh into SDCARD:\3DS\degausser3ds\

3. run it via hblauncher, then follow the instructions

 then follow the instructions

