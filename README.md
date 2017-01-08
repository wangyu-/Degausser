# Degausser

original descriptions:

Save editor for Daigasso! Band Brothers P. There are currently two flavours:

1. In the `src` folder is the C# code for _Degausser_, intended to run on Windows. This version is able to import, export, convert .bdx and .bin files, as well as play the songs. This has to be used in conjunction with an extdata dumping/restoring tool.
2. In the `degausser3ds` folder is C code for the homebrew _degausser3ds_. This version is only able to import all songs in a fixed directory, or export all songs to a fixed directory. This is able to import/export directly from/to extdata.

In the future we might use this space to discuss some of the file formats, but otherwise feel free to check out the Releases.


new features in this repo(degausser3ds only):

1.import songs as custom (so that it will be edit-able in game)

2.dump specific songs(instead of all)

3.delete specific songs

4.dump a songlist(do not dump BBP files)

how to use degausser3ds(for cfw user):

1.get hblauncher_loader.cia and install https://github.com/yellows8/hblauncher_loader/releases

2.put degausser3ds.3dsx degausser3ds.smdh into SDCARD:\3DS\degausser3ds\

 then follower the instructions
