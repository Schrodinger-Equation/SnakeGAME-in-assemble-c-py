.global start_random_asm
.global get_random_num_asm
.global calc_score_asm

.data
    # storing the seed here so it doesnt reset every frame
    current_seed: .quad 1

.text

# function to start the randomizer
# we pass the time into rcx from C
start_random_asm:
    movq %rcx, current_seed(%rip)
    ret

# getting the actual random coordinate
get_random_num_asm:
    pushq %rbp
    movq %rsp, %rbp

    # grabbing seed and multiplying by a big prime number
    movq current_seed(%rip), %rax
    movq $1103515245, %r8
    imulq %r8, %rax
    addq $12345, %rax
    
    # save it for next time
    movq %rax, current_seed(%rip)
    
    # remove negative sign just in case
    andq $0x7FFFFFFF, %rax
    
    # divide by the grid size we passed from C
    xorq %rdx, %rdx
    divq %rcx
    
    # return the remainder back to C
    movq %rdx, %rax
    
    popq %rbp
    ret

# calculating points when we eat something
# rcx = base points, rdx = sugar rush check, r8 = snake length
calc_score_asm:
    pushq %rbp
    movq %rsp, %rbp

    movq %rcx, %rax    

    # check if sugar rush is active
    cmpq $1, %rdx
    jne check_snake_len
    
    # double the points if it is
    imulq $2, %rax

check_snake_len:
    # no bonus if less than 10 length
    cmpq $10, %r8
    jl finish_calc
    
    addq $5, %rax

    # no ultra bonus if less than 20
    cmpq $20, %r8
    jl finish_calc

    addq $10, %rax

finish_calc:
    popq %rbp
    ret