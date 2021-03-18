; lab1 - 문자열 출력하기
; 첫 시작 스택은 3번입니다.
; 0: stdin, 1: stdout

; RALSEI
; 82 65 76 83 69 73
push 2, 30	; $ 60
dup 5, 3	; $ 60 60 60 60 60 60

push 1, 22	; $ 22 60 60 60 60 60 60
popa 2, 1	; $ 60 60 60 60 60, R
push 1, 5
popa 2, 1	; $ 60 60 60 60, RA
push 4, 4
popa 2, 1	; $ 60 60 60, RAL
push 1, 23
popa 2, 1	; $ 60 60, RALS
push 3, 3
popa 2, 1	; $ 60, RALSE
push 1, 13
popa 2, 1	; $ , RALSEI

; END
push 4, 3
push 1, 1
popa 2, 3
dup 1, 1
popa 1, 3

