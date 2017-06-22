# this file has been automatically generated by the OpenQL compiler please do not modify it manually.
# auto-generated micro code from rb.qasm by OpenQL driver, please don't modify it manually 
mov r11, 0       # counter
mov r3,  10      # max iterations
mov r0,  20000   # relaxation time / 2
loop:

     waitreg r0
     waitreg r0

     waitreg r0
     waitreg r0

     pulse 1001 0000 1001
     wait 10
     pulse 1010 0000 1010
     wait 10





     wait 60
     pulse 0000 1111 1111
     wait 50
     measure

     # calibration points :
     waitreg r0               # prepz q0 (+100us) 
     waitreg r0               # prepz q0 (+100us) 
     wait 60 
     pulse 0000 1111 1111 
     wait 50
     measure
     waitreg r0               # prepz q0 (+100us) 
     waitreg r0               # prepz q0 (+100us) 
     wait 60 
     pulse 0000 1111 1111 
     wait 50
     measure
     waitreg r0               # prepz q0 (+100us) 
     waitreg r0               # prepz q0 (+100us) 
     pulse 1001 0000 1001     # X180 
     wait 10
     wait 60
     pulse 0000 1111 1111
     wait 50
     measure
     waitreg r0               # prepz q0 (+100us) 
     waitreg r0               # prepz q0 (+100us) 
     pulse 1001 0000 1001     # X180 
     wait 10
     wait 60
     pulse 0000 1111 1111
     wait 50
     measure
     beq  r3,  r3, loop   # infinite loop