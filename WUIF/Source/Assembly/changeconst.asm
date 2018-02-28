.686P
.model flat

PUBLIC ?nCmdShow@App@WUIF@@3HD				                                  ; WUIF::App::nCmdShow
ifndef UNICODE
PUBLIC ?lpCmdLine@App@WUIF@@3SADA                                             ; MBCS WUIF::App::lpCmdLine
else
PUBLIC ?lpCmdLine@App@WUIF@@3SA_WA                                            ; UNICODE WUIF::App::lpCmdLine
endif
PUBLIC ?winversion@App@WUIF@@3W4OSVersion@2@D                                 ; WUIF::App::winversion
PUBLIC ?hInstance@App@WUIF@@3SAUHINSTANCE__@@A                                ; WUIF::App::hInstance
PUBLIC ?processdpiawareness@App@WUIF@@3SDW4PROCESS_DPI_AWARENESS@@D           ; WUIF::App::processdpiawareness
PUBLIC ?processdpiawarenesscontext@App@WUIF@@3SDSAUDPI_AWARENESS_CONTEXT__@@D ; WUIF::App::processdpiawarenesscontext
ifndef DEBUG
PUBLIC	?GFXflags@App@WUIF@@3ED				   ; release WUIF::App::GFXflags
else
PUBLIC	?GFXflags@App@WUIF@@3V?$checked_bit_field@$1?ui_GFX_Flags@FLAGS@WUIF@@3Ubitfield_unique_id@@AE@@D ;debug WUIF::App::GFXflags
endif

.data
?nCmdShow@App@WUIF@@3HD					                               DD 00H
ifndef UNICODE
?lpCmdLine@App@WUIF@@3SADA				                               DD 00H
else
?lpCmdLine@App@WUIF@@3SA_WA				                               DD 00H
endif
?winversion@App@WUIF@@3W4OSVersion@2@D                                 DD 00H
?hInstance@App@WUIF@@3SAUHINSTANCE__@@A	                               DD 00H
?processdpiawareness@App@WUIF@@3SDW4PROCESS_DPI_AWARENESS@@D           DD 00H
?processdpiawarenesscontext@App@WUIF@@3SDSAUDPI_AWARENESS_CONTEXT__@@D DD 00H
ifndef DEBUG
?GFXflags@App@WUIF@@3ED				    DB 00H
else
?GFXflags@App@WUIF@@3V?$checked_bit_field@$1?ui_GFX_Flags@FLAGS@WUIF@@3Ubitfield_unique_id@@AE@@D DB 00H DUP (?)
    ALIGN	4
endif

.code
PUBLIC ?changeconst@WUIF@@YIXPAXPBX@Z
?changeconst@WUIF@@YIXPAXPBX@Z PROC
     ;non-fastcall
     ;push ebp					  ;prologue
     ;mov  ebp,esp
     ;sub  esp,0C0h
     ;mov  edx,dword ptr [ebp+12] ;move value to eax
     ;mov  ecx,dword ptr [ebp+8]  ;mov const variable's to change address to eax
     ;mov  eax, [edx]			  ;dereference the value and mov to eax
     ;mov  dword ptr [ecx],eax    ;move value in eax to variable
     ;mov  esp,ebp
     ;pop  ebp
     ;end non-fast call
     ;start fast-call
     mov eax,dword ptr [edx] ;dereference the value and mov to eax
     mov dword ptr [ecx],eax ;move value in eax to variable
     ;end fast-call
     ret
?changeconst@WUIF@@YIXPAXPBX@Z ENDP

END