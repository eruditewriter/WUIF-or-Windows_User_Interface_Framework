.686P
.model flat

PUBLIC ?nCmdShow@App@WUIF@@3HB				   ; WUIF::App::nCmdShow
PUBLIC ?lpCmdLine@App@WUIF@@3QA_WA             ; WUIF::App::lpCmdLine
PUBLIC ?winversion@App@WUIF@@3W4OSVersion@2@B  ; WUIF::App::winversion
PUBLIC ?hInstance@App@WUIF@@3QAUHINSTANCE__@@A ; WUIF::App::hInstance
ifndef DEBUG ; WUIF::App::GFXflags
PUBLIC	?GFXflags@App@WUIF@@3EB
else
PUBLIC	?GFXflags@App@WUIF@@3V?$checked_bit_field@$1?ui_GFX_Flags@FLAGS@WUIF@@3Ubitfield_unique_id@@AE@@B
endif

.data
?nCmdShow@App@WUIF@@3HB					DD 00H
?lpCmdLine@App@WUIF@@3QA_WA				DD 00H
?winversion@App@WUIF@@3W4OSVersion@2@B  DD 00H
?hInstance@App@WUIF@@3QAUHINSTANCE__@@A	DD 00H
ifndef DEBUG
?GFXflags@App@WUIF@@3EB				    DB 00H
else
?GFXflags@App@WUIF@@3V?$checked_bit_field@$1?ui_GFX_Flags@FLAGS@WUIF@@3Ubitfield_unique_id@@AE@@B DB 00H DUP (?)
    ALIGN	4
endif

.code
PUBLIC _changeconst
_changeconst PROC
     push ebp					 ;prologue
     mov  ebp,esp
     sub  esp,0C0h
     push ebx
     push esi
     push edi
     mov  eax,dword ptr [ebp+12] ;move value to eax
     mov  ecx, [eax]			 ;dereference the value and mov to ecx
     mov  eax, dword ptr [ebp+8] ;mov const variable's to change address to eax
     mov  dword ptr [eax], ecx   ;move value in eax to variable
     mov  esp,ebp
     pop  ebp
     ret
_changeconst ENDP

END
