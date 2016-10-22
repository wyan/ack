#include "mcg.h"

static struct basicblock* current_bb;

static int stackptr;
static struct ir* stack[64];
static struct ir* lastcall;

static struct ir* convert(struct ir* src, int destsize, int opcodebase);
static struct ir* appendir(struct ir* ir);

static void reset_stack(void)
{
    stackptr = 0;
}

static void push(struct ir* ir)
{
    if (stackptr == sizeof(stack)/sizeof(*stack))
        fatal("stack overflow");

    /* If we try to push something which is too small, convert it to a word
     * first. */

    if (ir->size < EM_wordsize)
        ir = convert(ir, EM_wordsize, IR_CIU1);

    stack[stackptr++] = ir;
}

static struct ir* pop(int size)
{
    if (stackptr == 0)
    {
        /* Nothing in our fake stack, so we have to read from the real stack. */

        if (size < EM_wordsize)
            size = EM_wordsize;
        return
            appendir(
                new_ir0(
                    IR_POP, size
                )
            );
    }
    else
    {
        struct ir* ir = stack[--stackptr];

        /* If we try to pop something which is smaller than a word, convert it first. */
        
        if (size < EM_wordsize)
            ir = convert(ir, size, IR_CIU1);

        if (ir->size != size)
            fatal("expected an item on stack of size %d, but got %d\n", size, ir->size);
        return ir;
    }
}

static void print_stack(void)
{
    int i;

    tracef('E', "E: stack:");
    for (i=0; i<stackptr; i++)
    {
        struct ir* ir = stack[i];
        tracef('E', " $%d.%d", ir->id, ir->size);
    }
    tracef('E', "  (top)\n");
}

static struct ir* appendir(struct ir* ir)
{
    int i;

    assert(current_bb != NULL);
    array_appendu(&current_bb->irs, ir);

    ir_print('0', ir);
    return ir;
}

static void materialise_stack(void)
{
    int i;

    for (i=0; i<stackptr; i++)
    {
        struct ir* ir = stack[i];
        appendir(
            new_ir1(
                IR_PUSH, ir->size,
                ir
            )
        );
    }

    reset_stack();
}

void tb_filestart(void)
{
}

void tb_fileend(void)
{
}

void tb_regvar(struct procedure* procedure, arith offset, int size, int type, int priority)
{
    struct local* local = calloc(1, sizeof(*local));
    local->size = size;
    local->offset = offset;
    local->is_register = true;
    imap_put(&procedure->locals, offset, local);
}

static struct ir* address_of_external(const char* label, arith offset)
{
    if (offset != 0)
        return
            new_ir2(
                IR_ADD, EM_pointersize,
                new_labelir(label),
                new_wordir(offset)
            );
    else
        return
            new_labelir(label);
}

static struct ir* convert(struct ir* src, int destsize, int opcode)
{
    switch (src->size)
    {
        case 1: opcode += 0; break;
        case 2: opcode += 1; break;
        case 4: opcode += 2; break;
        case 8: opcode += 3; break;
        default:
            fatal("can't convert from things of size %d", src->size);
    }

    return
        new_ir1(
            opcode, destsize,
            src
        );
}

static struct ir* compare(struct ir* left, struct ir* right,
        int size, int opcode)
{
    switch (size)
    {
        case 1: opcode += 0; break;
        case 2: opcode += 1; break;
        case 4: opcode += 2; break;
        case 8: opcode += 3; break;
        default:
            fatal("can't compare things of size %d", size);
    }

    return
        new_ir2(
            opcode, EM_wordsize,
            left, right
        );
}

static struct ir* tristate_compare(int size, int opcode)
{
    struct ir* right = pop(size);
    struct ir* left = pop(size);

    return compare(left, right, size, opcode);
}

static struct ir* tristate_compare0(int size, int opcode)
{
    struct ir* right = new_wordir(0);
    struct ir* left = pop(size);

    return compare(left, right, size, opcode);
}

static void simple_convert(int opcode)
{
    struct ir* destsize = pop(EM_wordsize);
    struct ir* srcsize = pop(EM_wordsize);
    struct ir* value;

    assert(srcsize->opcode == IR_CONST);
    assert(destsize->opcode == IR_CONST);

    value = pop(srcsize->u.ivalue);
    push(
        convert(value, destsize->u.ivalue, opcode)
    );
}

static void simple_branch2(int opcode, int size,
    struct basicblock* truebb, struct basicblock* falsebb,
    int irop)
{
    struct ir* right = pop(size);
    struct ir* left = pop(size);

    materialise_stack();
    appendir(
        new_ir2(
            irop, 0,
            compare(left, right, size, IR_COMPARES1),
            new_ir2(
                IR_PAIR, 0,
                new_bbir(truebb),
                new_bbir(falsebb)
            )
        )
    );
}

static void compare0_branch2(int opcode,
    struct basicblock* truebb, struct basicblock* falsebb,
    int irop)
{
    push(
        new_wordir(0)
    );

    simple_branch2(opcode, EM_wordsize, truebb, falsebb, irop);
}

static void simple_test(int size, int irop)
{
    push(
        new_ir1(
            irop, EM_wordsize,
            tristate_compare0(size, IR_COMPARES1)
        )
    );
}

static void simple_test_neg(int size, int irop)
{
    simple_test(size, irop);

    push(
        new_ir1(
            IR_NOT, EM_wordsize,
            pop(EM_wordsize)
        )
    );
}

static void insn_simple(int opcode)
{
    switch (opcode)
    {
        case op_bra:
        {
            struct ir* dest = pop(EM_pointersize);

            materialise_stack();
            appendir(
                new_ir1(
                    IR_JUMP, 0,
                    dest
                )
            );
            break;
        }
            
        case op_cii: simple_convert(IR_CII1); break;
        case op_ciu: simple_convert(IR_CIU1); break;
        case op_cui: simple_convert(IR_CUI1); break;
        case op_cfi: simple_convert(IR_CFI1); break;
        case op_cif: simple_convert(IR_CIF1); break;
        case op_cff: simple_convert(IR_CFF1); break;

        case op_cmp:
            push(
                tristate_compare(EM_pointersize, IR_COMPAREU1)
            );
            break;

        case op_teq: simple_test(EM_wordsize, IR_IFEQ); break;
        case op_tne: simple_test_neg(EM_wordsize, IR_IFEQ); break;

        case op_cai:
        {
            struct ir* dest = pop(EM_pointersize);

            materialise_stack();
            lastcall = appendir(
                new_ir1(
                    IR_CALL, 0,
                    dest
                )
            );
            break;
        }

        case op_inc:
        {
            push(
                new_ir2(
                    IR_ADD, EM_wordsize,
                    pop(EM_wordsize),
                    new_wordir(1)
                )
            );
            break;
        }

        case op_dec:
        {
            push(
                new_ir2(
                    IR_SUB, EM_wordsize,
                    pop(EM_wordsize),
                    new_wordir(1)
                )
            );
            break;
        }

        case op_lim:
        {
            push(
                new_ir1(
                    IR_LOAD, 2,
                    new_labelir(".ignmask")
                )
            );
            break;
        }

        case op_sim:
        {
            appendir(
                new_ir2(
                    IR_STORE, 2,
                    new_labelir(".ignmask"),
                    pop(EM_wordsize)
                )
            );
            break;
        }

        case op_trp:
        {
            materialise_stack();
            appendir(
                new_ir1(
                    IR_CALL, 0,
                    new_labelir(".trp")
                )
            );
            break;
        }

        case op_lni:
        {
            /* Increment line number --- ignore. */
            break;
        }

        default:
            fatal("treebuilder: unknown simple instruction '%s'",
                em_mnem[opcode - sp_fmnem]);
    }
}

static void insn_bvalue(int opcode, struct basicblock* leftbb, struct basicblock* rightbb)
{
    switch (opcode)
    {
        case op_zeq: compare0_branch2(opcode, leftbb, rightbb, IR_CJUMPEQ); break;
        case op_zlt: compare0_branch2(opcode, leftbb, rightbb, IR_CJUMPLT); break;
        case op_zle: compare0_branch2(opcode, leftbb, rightbb, IR_CJUMPLE); break;

        case op_zne: compare0_branch2(opcode, rightbb, leftbb, IR_CJUMPEQ); break;
        case op_zge: compare0_branch2(opcode, rightbb, leftbb, IR_CJUMPLT); break;
        case op_zgt: compare0_branch2(opcode, rightbb, leftbb, IR_CJUMPLE); break;

        case op_beq: simple_branch2(opcode, EM_wordsize, leftbb, rightbb, IR_CJUMPEQ); break;
        case op_blt: simple_branch2(opcode, EM_wordsize, leftbb, rightbb, IR_CJUMPLT); break;
        case op_ble: simple_branch2(opcode, EM_wordsize, leftbb, rightbb, IR_CJUMPLE); break;

        case op_bne: simple_branch2(opcode, EM_wordsize, rightbb, leftbb, IR_CJUMPEQ); break;
        case op_bge: simple_branch2(opcode, EM_wordsize, rightbb, leftbb, IR_CJUMPLT); break;
        case op_bgt: simple_branch2(opcode, EM_wordsize, rightbb, leftbb, IR_CJUMPLE); break;

        case op_bra:
        {
            materialise_stack();

            appendir(
                new_ir1(
                    IR_JUMP, 0,
                    new_bbir(leftbb)
                )
            );
            break;
        }

        case op_lae:
            push(
                new_bbir(leftbb)
            );
            break;

        default:
            fatal("treebuilder: unknown bvalue instruction '%s'",
                em_mnem[opcode - sp_fmnem]);
    }
}

static void simple_alu1(int opcode, int size, int irop)
{
    struct ir* val = pop(size);

    push(
        new_ir1(
            irop, size,
            val
        )
    );
}

static void simple_alu2(int opcode, int size, int irop)
{
    struct ir* right = pop(size);
    struct ir* left = pop(size);

    push(
        new_ir2(
            irop, size,
            left, right
        )
    );
}

static struct ir* extract_block_refs(struct basicblock* bb)
{
    struct ir* outir = NULL;
    int i;

    for (i=0; i<bb->ems.count; i++)
    {
        struct em* em = bb->ems.item[i];
        assert(em->opcode == op_bra);
        assert(em->paramtype == PARAM_BVALUE);

        outir = new_ir2(
            IR_PAIR, 0,
            new_bbir(em->u.bvalue.left),
            outir
        );
    }

    return outir;
}

static void change_by(struct ir* address, int amount)
{
    appendir(
        new_ir2(
            IR_STORE, EM_wordsize,
            address,
            new_ir2(
                IR_ADD, EM_wordsize,
                new_ir1(
                    IR_LOAD, EM_wordsize,
                    address
                ),
                new_wordir(amount)
            )
        )
    );
}

static struct ir* ptradd(struct ir* address, int offset)
{
    if (offset == 0)
        return address;

    return
        new_ir2(
            IR_ADD, EM_pointersize,
            address,
            new_wordir(offset)
        );
}

static void insn_ivalue(int opcode, arith value)
{
    switch (opcode)
    {
        case op_adi: simple_alu2(opcode, value, IR_ADD); break;
        case op_sbi: simple_alu2(opcode, value, IR_SUB); break;
        case op_mli: simple_alu2(opcode, value, IR_MUL); break;
        case op_dvi: simple_alu2(opcode, value, IR_DIV); break;
        case op_rmi: simple_alu2(opcode, value, IR_MOD); break;
        case op_sli: simple_alu2(opcode, value, IR_ASL); break;
        case op_sri: simple_alu2(opcode, value, IR_ASR); break;
        case op_ngi: simple_alu1(opcode, value, IR_NEG); break;

        case op_adu: simple_alu2(opcode, value, IR_ADD); break;
        case op_sbu: simple_alu2(opcode, value, IR_SUB); break;
        case op_mlu: simple_alu2(opcode, value, IR_MUL); break;
        case op_slu: simple_alu2(opcode, value, IR_LSL); break;
        case op_sru: simple_alu2(opcode, value, IR_LSR); break;
        case op_rmu: simple_alu2(opcode, value, IR_MODU); break;
        case op_dvu: simple_alu2(opcode, value, IR_DIVU); break;

        case op_and: simple_alu2(opcode, value, IR_AND); break;
        case op_ior: simple_alu2(opcode, value, IR_OR); break;
        case op_xor: simple_alu2(opcode, value, IR_EOR); break;
        case op_com: simple_alu1(opcode, value, IR_NOT); break;

        case op_adf: simple_alu2(opcode, value, IR_ADDF); break;
        case op_sbf: simple_alu2(opcode, value, IR_SUBF); break;
        case op_mlf: simple_alu2(opcode, value, IR_MULF); break;
        case op_dvf: simple_alu2(opcode, value, IR_DIVF); break;
        case op_ngf: simple_alu1(opcode, value, IR_NEGF); break;

        case op_cmu: /* fall through */
        case op_cms: push(tristate_compare(value, IR_COMPAREU1)); break;
        case op_cmi: push(tristate_compare(value, IR_COMPARES1)); break;
        case op_cmf: push(tristate_compare(value, IR_COMPAREF1)); break;

        case op_lol:
            push(
                new_ir1(
                    IR_LOAD, EM_wordsize,
                    new_localir(value)
                )
            );
            break;

        case op_ldl:
            push(
                new_ir1(
                    IR_LOAD, EM_wordsize*2,
                    new_localir(value)
                )
            );
            break;

        case op_stl:
            appendir(
                new_ir2(
                    IR_STORE, EM_wordsize,
                    new_localir(value),
                    pop(EM_wordsize)
                )
            );
            break;

        case op_sdl:
            appendir(
                new_ir2(
                    IR_STORE, EM_wordsize*2,
                    new_localir(value),
                    pop(EM_wordsize*2)
                )
            );
            break;

        case op_lal:
            push(
                new_localir(value)
            );
            break;

        case op_lil:
            push(
                new_ir1(
                    IR_LOAD, EM_wordsize,
                    new_ir1(
                        IR_LOAD, EM_pointersize,
                        new_localir(value)
                    )
                )
            );
            break;

        case op_sil:
            appendir(
                new_ir2(
                    IR_STORE, EM_wordsize,
                    new_ir1(
                        IR_LOAD, EM_pointersize,
                        new_localir(value)
                    ),
                    pop(EM_wordsize)
                )
            );
            break;

        case op_inl:
            change_by(new_localir(value), 1);
            break;

        case op_del:
            change_by(new_localir(value), -1);
            break;

        case op_zrl:
            appendir(
                new_ir2(
                    IR_STORE, EM_wordsize,
                    new_localir(value),
                    new_wordir(0)
                )
            );
            break;

        case op_zrf:
        {
            struct ir* ir = new_constir(value, 0);
            ir->opcode = IR_CONSTF;
            push(ir);
            break;
        }

        case op_loe:
            push(
                new_ir1(
                    IR_LOAD, EM_wordsize,
                    new_ir2(
                        IR_ADD, EM_pointersize,
                        new_labelir(".hol0"),
                        new_wordir(value)
                    )
                )
            );
            break;

        case op_ste:
            appendir(
                new_ir2(
                    IR_STORE, EM_wordsize,
                    new_ir2(
                        IR_ADD, EM_pointersize,
                        new_labelir(".hol0"),
                        new_wordir(value)
                    ),
                    pop(EM_wordsize)
                )
            );
            break;

        case op_zre:
            appendir(
                new_ir2(
                    IR_STORE, EM_wordsize,
                    new_ir2(
                        IR_ADD, EM_pointersize,
                        new_labelir(".hol0"),
                        new_wordir(value)
                    ),
                    new_wordir(0)
                )
            );
            break;
                
        case op_loc:
            push(
                new_wordir(value)
            );
            break;

        case op_loi:
        {
            struct ir* ptr = pop(EM_pointersize);
            int offset = 0;

            /* FIXME: this is awful; need a better way of dealing with
             * non-standard EM sizes. */
            if (value > (EM_wordsize*2))
                appendir(ptr);

            while (value > 0)
            {
                int s;
                if (value > (EM_wordsize*2))
                    s = EM_wordsize*2;
                else
                    s = value;

                push(
                    new_ir1(
                        IR_LOAD, s,
                        ptradd(ptr, offset)
                    )
                );

                value -= s;
                offset += s;
            }

            assert(value == 0);
            break;
        }

        case op_lof:
        {
            struct ir* ptr = pop(EM_pointersize);

            push(
                new_ir1(
                    IR_LOAD, EM_wordsize,
                    new_ir2(
                        IR_ADD, EM_pointersize,
                        ptr,
                        new_wordir(value)
                    )
                )
            );
            break;
        }

        case op_sti:
        {
            struct ir* ptr = pop(EM_pointersize);
            struct ir* val = pop(value);

            appendir(
                new_ir2(
                    IR_STORE, value,
                    ptr, val
                )
            );
            break;
        }

        case op_stf:
        {
            struct ir* ptr = pop(EM_pointersize);
            struct ir* val = pop(EM_wordsize);

            appendir(
                new_ir2(
                    IR_STORE, EM_wordsize,
                    new_ir2(
                        IR_ADD, EM_pointersize,
                        ptr,
                        new_wordir(value)
                    ),
                    val
                )
            );
            break;
        }

        case op_ads:
        {
            struct ir* off = pop(value);
            struct ir* ptr = pop(EM_pointersize);

            if (value != EM_pointersize)
                off = convert(off, EM_pointersize, IR_CII1);

            push(
                new_ir2(
                    IR_ADD, EM_pointersize,
                    ptr, off
                )
            );
            break;
        }

        case op_adp:
        {
            struct ir* ptr = pop(EM_pointersize);

            push(
                new_ir2(
                    IR_ADD, EM_pointersize,
                    ptr,
                    new_wordir(value)
                )
            );
            break;
        }

        case op_sbs:
        {
            struct ir* right = pop(EM_pointersize);
            struct ir* left = pop(EM_pointersize);

            struct ir* delta = 
                new_ir2(
                    IR_SUB, EM_pointersize,
                    left, right
                );

            if (value != EM_pointersize)
                delta = convert(delta, value, IR_CII1);

            push(delta);
            break;
        }
            
        case op_dup:
        {
            struct ir* v = pop(value);
            appendir(v);
            push(v);
            push(v);
            break;
        }

        case op_asp:
        {
            switch (value)
            {
                case 0:
                    break;

                case -1:
                case -2:
                case -4:
                case -8:
                    push(new_anyir(-value));
                    break;

                default:
                    while ((value > 0) && (stackptr > 0))
                    {
                        struct ir* ir = pop(stack[stackptr-1]->size);
                        value -= ir->size;
                    }

                    if (value != 0)
                    {
                        appendir(
                            new_ir1(
                                IR_STACKADJUST, EM_pointersize,
                                new_wordir(value)
                            )
                        );
                    }
                    break;
            }
            break;
        }

        case op_ass:
            appendir(
                new_ir1(
                    IR_STACKADJUST, EM_pointersize,
                    pop(value)
                )
            );
            break;

        case op_ret:
        {
            if (value > 0)
            {
                struct ir* retval = pop(value);
                materialise_stack();
                appendir(
                    new_ir1(
                        IR_SETRET, value,
                        retval
                    )
                );
            }

            if (!current_proc->exit)
            {
                current_proc->exit = bb_get(NULL);
                array_append(&current_proc->blocks, current_proc->exit);

                /* This is actually ignored --- the entire block gets special
                 * treatment. But a lot of the rest of the code assumes that
                 * all basic blocks have one instruction, so we insert one. */

                array_append(&current_proc->exit->irs,
                    new_ir0(
                        IR_RET, 0
                    )
                );
            }

            appendir(
                new_ir1(
                    IR_JUMP, 0,
                    new_bbir(current_proc->exit)
                )
            );
            break;
        }
                    
        case op_lfr:
        {
            assert(lastcall != NULL);
            lastcall->size = value;
            push(lastcall);
            break;
        }

        case op_csa:
        case op_csb:
        {
            const char* helper = aprintf(".%s%d",
                (opcode == op_csa) ? "csa" : "csb",
                value);
            struct ir* descriptor = pop(EM_pointersize);

            if (descriptor->opcode != IR_LABEL)
                fatal("csa/csb are only supported if they refer "
                    "directly to a descriptor block");

            push(descriptor);
            materialise_stack();
            appendir(
                new_ir2(
                    IR_JUMP, 0,
                    new_labelir(helper),
                    extract_block_refs(bb_get(descriptor->u.lvalue))
                )
            );
            break;
        }

        case op_sar:
        case op_lar:
        case op_aar:
        {
            const char* helper;
            if (value != EM_wordsize)
                fatal("sar/lar/aar are only supported when using "
                    "word-size descriptors");

            switch (opcode)
            {
                case op_sar: helper = ".sar4"; break;
                case op_lar: helper = ".lar4"; break;
                case op_aar: helper = ".aar4"; break;
            }

            materialise_stack();
            push(
                appendir(
                    new_ir1(
                        IR_CALL, EM_wordsize,
                        new_labelir(helper)
                    )
                )
            );
            break;
        }

        case op_lxl:
        {
            struct ir* ir;

            /* Walk the static chain. */

            ir = new_ir0(
                IR_GETFP, EM_pointersize
            );

            while (value--)
            {
                ir = new_ir1(
                    IR_CHAINFP, EM_pointersize,
                    ir
                );
            }

            push(ir);
            break;
        }

        case op_lxa:
        {
            struct ir* ir;

            /* Walk the static chain. */

            ir = new_ir0(
                IR_GETFP, EM_pointersize
            );

            while (value--)
            {
                ir = new_ir1(
                    IR_CHAINFP, EM_pointersize,
                    ir
                );
            }

            push(
                new_ir1(
                    IR_FPTOARGS, EM_pointersize,
                    ir
                )
            );
            break;
        }

        case op_fef:
        {
            struct ir* f = pop(value);

            /* fef is implemented by calling a helper function which then mutates
             * the stack. We read the return values off the stack when retracting
             * the stack pointer. */

            push(f);
            push(new_wordir(0));

            materialise_stack();
            appendir(
                new_ir1(
                    IR_CALL, 0,
                    new_labelir((value == 4) ? ".fef4" : ".fef8")
                )
            );
                    
            /* exit, leaving an int and then a float (or double) on the stack. */
            break;
        }
            
        case op_fif:
        {
            /* fif is implemented by calling a helper function which then mutates
             * the stack. We read the return values off the stack when retracting
             * the stack pointer. */

            /* We start with two floats on the stack. */

            materialise_stack();
            appendir(
                new_ir1(
                    IR_CALL, 0,
                    new_labelir((value == 4) ? ".fif4" : ".fif8")
                )
            );
                    
            /* exit, leaving two floats (or doubles) on the stack. */
            break;
        }
            
        case op_lor:
        {
            switch (value)
            {
                case 1:
                    appendir(
                        new_ir0(
                            IR_GETSP, EM_pointersize
                        )
                    );
                    break;

                default:
                    fatal("'lor %d' not supported", value);
            }

            break;
        }

        case op_str:
        {
            switch (value)
            {
                case 1:
                    appendir(
                        new_ir1(
                            IR_SETSP, EM_pointersize,
                            pop(EM_pointersize)
                        )
                    );
                    break;

                default:
                    fatal("'str %d' not supported", value);
            }

            break;
        }

        case op_blm:
        {
            /* Input stack: ( src dest -- ) */
            /* Memmove stack: ( size src dest -- ) */
            struct ir* dest = pop(EM_pointersize);
            struct ir* src = pop(EM_pointersize);

            push(new_wordir(value));
            push(src);
            push(dest);

            materialise_stack();
            appendir(
                new_ir1(
                    IR_CALL, 0,
                    new_labelir("memmove")
                )
            );
            appendir(
                new_ir1(
                    IR_STACKADJUST, EM_pointersize,
                    new_wordir(EM_pointersize*2 + EM_wordsize)
                )
            );
            break;
        }

        case op_lin:
        {
            /* Set line number --- ignore. */
            break;
        }

        default:
            fatal("treebuilder: unknown ivalue instruction '%s'",
                em_mnem[opcode - sp_fmnem]);
    }
}

static void insn_lvalue(int opcode, const char* label, arith offset)
{
    switch (opcode)
    {
        case op_lpi:
        case op_lae:
            push(
                address_of_external(label, offset)
            );
            break;

        case op_loe:
            push(
                new_ir1(
                    IR_LOAD, EM_wordsize,
                    address_of_external(label, offset)
                )
            );
            break;

        case op_lde:
            push(
                new_ir1(
                    IR_LOAD, EM_wordsize*2,
                    address_of_external(label, offset)
                )
            );
            break;

        case op_ste:
            appendir(
                new_ir2(
                    IR_STORE, EM_wordsize,
                    address_of_external(label, offset),
                    pop(EM_wordsize)
                )
            );
            break;

        case op_sde:
            appendir(
                new_ir2(
                    IR_STORE, EM_wordsize*2,
                    address_of_external(label, offset),
                    pop(EM_wordsize*2)
                )
            );
            break;

        case op_zre:
            appendir(
                new_ir2(
                    IR_STORE, EM_wordsize,
                    address_of_external(label, offset),
                    new_wordir(0)
                )
            );
            break;
                
        case op_ine:
            appendir(
                new_ir2(
                    IR_STORE, EM_wordsize,
                    address_of_external(label, offset),
                    new_ir2(
                        IR_ADD, EM_wordsize,
                        new_ir1(
                            IR_LOAD, EM_wordsize,
                            address_of_external(label, offset)
                        ),
                        new_wordir(1)
                    )
                )
            );
            break;

        case op_dee:
            appendir(
                new_ir2(
                    IR_STORE, EM_wordsize,
                    address_of_external(label, offset),
                    new_ir2(
                        IR_ADD, EM_wordsize,
                        new_ir1(
                            IR_LOAD, EM_wordsize,
                            address_of_external(label, offset)
                        ),
                        new_wordir(-1)
                    )
                )
            );
            break;

        case op_cal:
            assert(offset == 0);
            materialise_stack();
            lastcall = appendir(
                new_ir1(
                    IR_CALL, 0,
                    new_labelir(label)
                )
            );
            break;

        case op_bra:
            assert(offset == 0);
            materialise_stack();
            appendir(
                new_ir1(
                    IR_JUMP, 0,
                    new_labelir(label)
                )
            );
            break;

        case op_fil:
        {
            /* Set filename --- ignore. */
            break;
        }
                    
        default:
            fatal("treebuilder: unknown lvalue instruction '%s'",
                em_mnem[opcode - sp_fmnem]);
    }
}

static void generate_tree(struct basicblock* bb)
{
    int i;

    tracef('0', "0: block %s\n", bb->name);

    current_bb = bb;
    reset_stack();

    for (i=0; i<bb->ems.count; i++)
    {
        struct em* em = bb->ems.item[i];
        tracef('E', "E: read %s ", em_mnem[em->opcode - sp_fmnem]);
        switch (em->paramtype)
        {
            case PARAM_NONE:
                tracef('E', "\n");
                insn_simple(em->opcode);
                break;

            case PARAM_IVALUE:
                tracef('E', "value=%d\n", em->u.ivalue);
                insn_ivalue(em->opcode, em->u.ivalue);
                break;

            case PARAM_LVALUE:
                tracef('E', "label=%s offset=%d\n", 
                    em->u.lvalue.label, em->u.lvalue.offset);
                insn_lvalue(em->opcode, em->u.lvalue.label, em->u.lvalue.offset);
                break;

            case PARAM_BVALUE:
                tracef('E', "true=%s", em->u.bvalue.left->name);
                if (em->u.bvalue.right)
                    tracef('E', " false=%s", em->u.bvalue.right->name);
                tracef('E', "\n");
                insn_bvalue(em->opcode, em->u.bvalue.left, em->u.bvalue.right);
                break;

            default:
                assert(0);
        }

        if (tracing('E'))
            print_stack();
    }

    assert(stackptr == 0);
}

void tb_procedure(void)
{
    int i;

    for (i=0; i<current_proc->blocks.count; i++)
        generate_tree(current_proc->blocks.item[i]);

}

/* vim: set sw=4 ts=4 expandtab : */
