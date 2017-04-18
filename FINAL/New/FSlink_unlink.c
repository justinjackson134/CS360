/*  link_unlikc.c file implements link and unlink. link(old_file, new_file) creates a hard
link from new_file to old_file. Hard links can only be to regular files, not DIRs, because
linking to DIRs may create loops in the file system name space. Hard link files share the 
same inode. Therefore, they must be on the same device. */