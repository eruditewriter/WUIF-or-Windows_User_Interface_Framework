PUBLIC  ?nCmdShow@App@WUIF@@3HD					                                  ; WUIF::App::nCmdShow
ifndef UNICODE
PUBLIC	?lpCmdLine@App@WUIF@@3SEADEA			                                  ; MBCS WUIF::App::lpCmdLine
else
PUBLIC  ?lpCmdLine@App@WUIF@@3SEA_WEA                                             ; UNICODE WUIF::App::lpCmdLine
endif
PUBLIC	?winversion@App@WUIF@@3W4OSVersion@2@D	                                  ; WUIF::App::winversion
PUBLIC	?hInstance@App@WUIF@@3SEAUHINSTANCE__@@EA                                 ; WUIF::App::hInstance
PUBLIC  ?processdpiawareness@App@WUIF@@3SEDW4PROCESS_DPI_AWARENESS@@ED            ; WUIF::App::processdpiawareness
PUBLIC  ?processdpiawarenesscontext@App@WUIF@@3SEDSEAUDPI_AWARENESS_CONTEXT__@@ED ; WUIF::App::processdpiawarenesscontext
ifndef DEBUG
PUBLIC	?GFXflags@App@WUIF@@3ED                   ; release WUIF::App::GFXflags
else
PUBLIC	?GFXflags@App@WUIF@@3V?$checked_bit_field@$1?ui_GFX_Flags@FLAGS@WUIF@@3Ubitfield_unique_id@@AE@@D ; debug WUIF::App::GFXflags
endif


.data
?nCmdShow@App@WUIF@@3HD					                                  DD 00H
ifndef UNICODE
?lpCmdLine@App@WUIF@@3SEADEA			                                  DQ 0000000000000000H
else
?lpCmdLine@App@WUIF@@3SEA_WEA			                                  DQ 0000000000000000H
endif
?winversion@App@WUIF@@3W4OSVersion@2@D                                    DD 00H
?hInstance@App@WUIF@@3SEAUHINSTANCE__@@EA                                 DQ 0000000000000000H
?processdpiawareness@App@WUIF@@3SEDW4PROCESS_DPI_AWARENESS@@ED            DQ 0000000000000000H
?processdpiawarenesscontext@App@WUIF@@3SEDSEAUDPI_AWARENESS_CONTEXT__@@ED DQ 0000000000000000H
ifndef DEBUG
?GFXflags@App@WUIF@@3ED					  DB 00H
else
?GFXflags@App@WUIF@@3V?$checked_bit_field@$1?ui_GFX_Flags@FLAGS@WUIF@@3Ubitfield_unique_id@@AE@@D DB 00H DUP (?)
	ALIGN	4
endif

.code
PUBLIC ?changeconst@WUIF@@YAXPEAXPEBX@Z
?changeconst@WUIF@@YAXPEAXPEBX@Z PROC
	mov  rax,qword ptr [rdx] ;rdx is the address for the value - derefence and move to rax
	mov  qword ptr [rcx],rax ;rcx is the address for the variable - store value in variable
	ret
?changeconst@WUIF@@YAXPEAXPEBX@Z ENDP

END