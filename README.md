# bldr_s
boot loader for booting by PXE, writed in SmallerC (model huge/unreal)
https://github.com/alexfru/SmallerC

To clean: 

    make clean

To compile: 

    make 

To run with qemu booting from a PXE server loading via tftp the file bldr_s.0:

    make bootpxe
    
In the makefile change 1/0 in the line 22 for set model unreal/huge.    

Screenshots: 

    https://github.com/so1h/bldr_s/blob/master/make.png

    https://github.com/so1h/bldr_s/blob/master/make%20bootpxe.png
    
