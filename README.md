# bldr_s
Boot loader for booting by PXE, writed in SmallerC (model huge/unreal)
https://github.com/alexfru/SmallerC

To clean: 

    make clean

To compile: 

    make 

To run with qemu booting from a PXE server loading via tftp the file bldr_s.0:

    make bootpxe
    
In the line 11 of MAKEFILE set 1/0 for change model unreal/huge.    

Screenshots: 

    https://github.com/so1h/bldr_s/blob/master/make.png

    https://github.com/so1h/bldr_s/blob/master/make%20bootpxe.png
    
