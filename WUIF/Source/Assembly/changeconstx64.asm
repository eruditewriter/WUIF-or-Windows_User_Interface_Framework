PUBLIC	?nCmdShow@App@WUIF@@3HB					  ; WUIF::App::nCmdShow
PUBLIC	?lpCmdLine@App@WUIF@@3QEA_WEA			  ; WUIF::App::lpCmdLine
PUBLIC	?winversion@App@WUIF@@3W4OSVersion@2@B	  ; WUIF::App::winversion
PUBLIC	?hInstance@App@WUIF@@3QEAUHINSTANCE__@@EA ; WUIF::App::hInstance
ifndef DEBUG ; WUIF::App::GFXflags
PUBLIC	?GFXflags@App@WUIF@@3EB
else
PUBLIC	?GFXflags@App@WUIF@@3V?$checked_bit_field@$1?ui_GFX_Flags@FLAGS@WUIF@@3Ubitfield_unique_id@@AE@@B
endif


.data
?nCmdShow@App@WUIF@@3HB					  DD 00H
?lpCmdLine@App@WUIF@@3QEA_WEA			  DQ 0000000000000000H
?winversion@App@WUIF@@3W4OSVersion@2@B    DD 00H
?hInstance@App@WUIF@@3QEAUHINSTANCE__@@EA DQ 0000000000000000H
ifndef DEBUG
?GFXflags@App@WUIF@@3EB					  DB 00H
else
?GFXflags@App@WUIF@@3V?$checked_bit_field@$1?ui_GFX_Flags@FLAGS@WUIF@@3Ubitfield_unique_id@@AE@@B DB 00H DUP (?)
	ALIGN	4
endif

.code
PUBLIC changeconst
changeconst PROC
	push rbp
	mov  rbp, rsp			  ;prologue
	mov  rax, rcx			  ;mov const variable's to change address to rax
	mov  rcx, qword ptr [rdx] ;dereference the value and mov to rcx
	mov  qword ptr [rax], rcx ;move the value into the dereferenced variable
	pop  rbp
	ret
changeconst ENDP

END