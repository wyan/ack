#include "mcg.h"

static ARRAYOF(struct ir) pending;
static ARRAYOF(struct ir) promotable;

static void addall(struct ir* ir)
{
    if (array_appendu(&pending, ir))
        return;

    if (ir->left)
        addall(ir->left);
    if (ir->right)
        addall(ir->right);
}

static void collect_irs(void)
{
    int i;
    
    pending.count = 0;
    promotable.count = 0;
	for (i=0; i<cfg.preorder.count; i++)
    {
        struct basicblock* bb = cfg.preorder.item[i];
        int j;

        for (j=0; j<bb->irs.count; j++)
            addall(bb->irs.item[j]);
    }
}

static void promote(struct ir* ir)
{
    switch (ir->opcode)
    {
        case IR_CONST:
        case IR_POP:
        case IR_LOAD:
            array_appendu(&promotable, ir);
            break;

        case IR_NOP:
            promote(ir->left);
            break;

        case IR_PHI:
        {
            int i;
            for (i=0; i<ir->u.phivalue.count; i++)
                promote(ir->u.phivalue.item[i].right);
            break;
        }
    }
}

static void search_for_promotable_irs(void)
{
    int i;

    for (i=0; i<pending.count; i++)
    {
        struct ir* ir = pending.item[i];

        switch (ir->opcode)
        {
            case IR_ADDF:
            case IR_SUBF:
            case IR_MULF:
            case IR_DIVF:
            case IR_NEGF:
            case IR_COMPAREF1:
            case IR_COMPAREF2:
            case IR_COMPAREF4:
            case IR_COMPAREF8:
            case IR_CFF1:
            case IR_CFF2:
            case IR_CFF4:
            case IR_CFF8:
                if (ir->left)
                    promote(ir->left);
                if (ir->right)
                    promote(ir->right);
                break;
        }
    }
}

static void modify_promotable_irs(void)
{
    int i;

    for (i=0; i<promotable.count; i++)
    {
        struct ir* ir = promotable.item[i];
        
        switch (ir->opcode)
        {
            case IR_ADDF:
            case IR_SUBF:
            case IR_MULF:
            case IR_DIVF:
            case IR_NEGF:
            case IR_PHI:
            case IR_NOP:
                break;

            default:
                ir->opcode++;
                break;
        }
    }
}

void pass_promote_float_ops(void)
{
	collect_irs();
    search_for_promotable_irs();
    modify_promotable_irs();
}

/* vim: set sw=4 ts=4 expandtab : */