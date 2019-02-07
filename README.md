# bldr_s
boot loader for booting by PXE, writed in SmallerC (model huge/unreal)

to compile: 

    make 

to run with qemu booting from a PXE server loading via tftp the file bldr_s.0:

    make bootpxe
    
In the makefile change 1/0 in the line 22 for set model unreal/huge.    
