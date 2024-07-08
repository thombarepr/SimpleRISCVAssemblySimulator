import sys
import argparse
import os
import os.path
import string
import re
from enum import Enum, auto, IntEnum
import struct
import numpy as np

class opcodes(IntEnum):
    instr_movw = 0
    instr_movt = auto()
    instr_add = auto()
    instr_sub = auto()
    instr_and = auto()
    instr_orr = auto()
    instr_eor = auto()
    instr_bic = auto()
    instr_ldr = auto()
    instr_str = auto()
    instr_b = auto()
    instr_nop = auto()
    instr_halt = auto()
    instr_bkpt = auto()

def parse_mov_operands(instr):
    y=re.match("^[a-zA-Z]+\s*r(\d+)\s*,\s*#(\d+)$", instr)
    z=re.match("^[a-zA-Z]+\s*r(\d+)\s*,\s*r(\d+)$", instr)
    if y != None:
        imm=int(1)
        val=int(y.group(2))
        if val > 0x3fff:
            err="immediate out of range: "+line
            raise ValueError(err)
        rd=int(y.group(1))
        if rd>int(15):
            err="invalid operand: "+line
            raise ValueError(err)
    elif z != None:
        imm=int(0)
        val=int(z.group(2))
        if val>int(15):
            err="invalid operand: "+line
            raise ValueError(err)
        rd=int(z.group(1))
        if rd>int(15):
            err="invalid operand: "+line
            raise ValueError(err)
    else:
        err="invalid instruction: "+line
        raise ValueError(err)
    return rd, imm, val

def parse_ld_operands(instr):
    y=re.match("^[a-zA-Z]+\s+r(\d+)\s*,\s*\[\s*r(\d+)\s*[,\s*#(\d+)\s*]?\]$", instr)
    z=re.match("^[a-zA-Z]+\s+r(\d+)\s*,\s*\[\s*r(\d+)\s*[,\s*r(\d+)\s*]?\]$", instr)
    if y != None:
        imm=int(1)
        if len(y.groups()) >= 3:
            val=int(y.group(3))
            if val > 0x3fff:
                err="immediate out of range: "+line
                raise ValueError(err)
        else:
            imm=int(0)
            val=int(0)
        rd=int(y.group(1))
        if rd>int(15):
            err="invalid operand: "+line
            raise ValueError(err)
        rs1=int(y.group(2))
        if rs1>int(15):
            err="invalid operand: "+line
            raise ValueError(err)
    elif z != None:
        imm=int(0)
        if len(z.groups()) >= 3:
            val=int(z.group(3))
            if val>int(15):
                err="invalid operand: "+line
                raise ValueError(err)
        else:
            imm=int(0)
            val=int(0)
        rd=int(z.group(1))
        if rd>int(15):
            err="invalid operand: "+line
            raise ValueError(err)
        rs1=int(z.group(2))
        if rs1>int(15):
            err="invalid operand: "+line
            raise ValueError(err)
    else:
        err="invalid instruction: "+line
        raise ValueError(err)
    return rd, rs1, imm, val

def parse_operands(instr):
    y=re.match("^[a-zA-Z]+\s+r(\d+)\s*,\s*r(\d+)\s*,\s*#(\d+)$", instr)
    z=re.match("^[a-zA-Z]+\s+r(\d+)\s*,\s*r(\d+)\s*,\s*r(\d+)$", instr)
    if y != None:
        imm=int(1)
        val=int(y.group(3))
        if val > 0x3fff:
            err="immediate out of range: "+line
            raise ValueError(err)
        rd=int(y.group(1))
        if rd>int(15):
            err="invalid operand: "+line
            raise ValueError(err)
        rs1=int(y.group(2))
        if rs1>int(15):
            err="invalid operand: "+line
            raise ValueError(err)
    elif z != None:
        imm=int(0)
        val=int(z.group(3))
        if val>int(15):
            err="invalid operand: "+line
            raise ValueError(err)
        rd=int(z.group(1))
        if rd>int(15):
            err="invalid operand: "+line
            raise ValueError(err)
        rs1=int(z.group(2))
        if rs1>int(15):
            err="invalid operand: "+line
            raise ValueError(err)
    else:
        err="invalid instruction: "+line
        raise ValueError(err)
    return rd, rs1, imm, val

if __name__=="__main__":
    try:
        parser = argparse.ArgumentParser(description="Demo risc machine assembler")
        parser.add_argument('-i', help='assmebly source file path', type=str)
        parser.add_argument('-o', help='output file name', type=str, default='out.bin')
        args = parser.parse_args()
        fi = open(args.i, "r")
        fo = open(args.o, "w+b")
        fo.write(bytes('code', 'utf-8'))
        fo.seek(4, 1)
        fo.write(bytes('data', 'utf-8'))
        fo.seek(4, 1)
        lines = fi.read().splitlines()
        label_offset=0
        line_number=0
        labels={}
        branches={}
        for line in lines:
            line_number+=1
            line=line.strip()
            if line == '':
                continue;
            #skip commented lines
            x=re.match("^\#.*", line)
            if x != None:
                continue;
            #collect labels to resolve branch addresses later
            x=re.match("^([a-zA-Z]+[0-9]*)\s*\:", line)
            if x != None:
                label=str(x.group(1))
                if label in labels:
                    err="duplicate labels: "+label+" at line "+str(line_number)
                    raise ValueError(err)
                labels[label]=label_offset
                continue

            #handle conditional execution codes
            x=re.match("^[a-zA-Z]+(eq|ne|cs|cc)\s+.*", line)
            if x != None:
                ce=int(1)
                match x.group(1):
                    case "cs":
                        cc=int(0)
                        line=line.replace("ce",'')
                    case "cc":
                        cc=int(1)
                        line=line.replace("cc",'')
                    case "eq":
                        cc=int(2)
                        line=line.replace("eq",'')
                    case "ne":
                        cc=int(3)
                        line=line.replace("ne",'')
                    case _:
                        err="invalid conditional code: "
                        raise ValueError(err+x.group(1))
            else:
                ce=int(0)
                cc=int(0)

            #handle opcode and operands
            x=re.match("^([a-zA-Z]+)\s*.*", line)
            if x != None:
                label_offset+=4
                instr=0
                opcode=x.group(1)
                opcode.lower()
                match opcode:
                    case "movw":
                        instr=opcodes.instr_movw
                        rd,imm,val=parse_mov_operands(line)
                        instr|=imm<<4
                        instr|=ce<<5
                        instr|=cc<<6
                        instr|=rd<<10
                        instr|=val<<18
                    case "mov":
                        instr=opcodes.instr_movw
                        rd,imm,val=parse_mov_operands(line)
                        instr|=imm<<4
                        instr|=ce<<5
                        instr|=cc<<6
                        instr|=rd<<10
                        instr|=val<<18
                    case "movt":
                        instr=(instr & ~0xf) | opcodes.instr_movt
                        rd,imm,val=parse_mov_operands(line)
                        instr|=imm<<4
                        instr|=ce<<5
                        instr|=cc<<6
                        instr|=rd<<10
                        instr|=val<<18
                    case "add":
                        instr=(instr & ~0xf) | opcodes.instr_add
                        rd,rs1,imm,val=parse_operands(line)
                        instr|=imm<<4
                        instr|=ce<<5
                        instr|=cc<<6
                        instr|=rd<<10
                        instr|=rs1<<14
                        instr|=val<<18
                    case "sub":
                        instr=(instr & ~0xf) | opcodes.instr_sub
                        rd,rs1,imm,val=parse_operands(line)
                        instr|=imm<<4
                        instr|=ce<<5
                        instr|=cc<<6
                        instr|=rd<<10
                        instr|=rs1<<14
                        instr|=val<<18
                    case "and":
                        instr=(instr & ~0xf) | opcodes.instr_and
                        rd,rs1,imm,val=parse_operands(line)
                        instr|=imm<<4
                        instr|=ce<<5
                        instr|=cc<<6
                        instr|=rd<<10
                        instr|=rs1<<14
                        instr|=val<<18
                    case "orr":
                        instr=(instr & ~0xf) | opcodes.instr_orr
                        rd,rs1,imm,val=parse_operands(line)
                        instr|=imm<<4
                        instr|=ce<<5
                        instr|=cc<<6
                        instr|=rd<<10
                        instr|=rs1<<14
                        instr|=val<<18
                    case "eor":
                        instr=(instr & ~0xf) | opcodes.instr_eor
                        rd,rs1,imm,val=parse_operands(line)
                        instr|=imm<<4
                        instr|=ce<<5
                        instr|=cc<<6
                        instr|=rd<<10
                        instr|=rs1<<14
                        instr|=val<<18
                    case "bic":
                        instr=(instr & ~0xf) | opcodes.instr_bic
                        rd,rs1,imm,val=parse_operands(line)
                        instr|=imm<<4
                        instr|=ce<<5
                        instr|=cc<<6
                        instr|=rd<<10
                        instr|=rs1<<14
                        instr|=val<<18
                    case "ldr":
                        instr=(instr & ~0xf) | opcodes.instr_ldr
                        rd,rs1,imm,val=parse_ld_operands(line)
                        instr|=imm<<4
                        instr|=ce<<5
                        instr|=cc<<6
                        instr|=rd<<10
                        instr|=rs1<<14
                        instr|=val<<18
                    case "str":
                        instr=(instr & ~0xf) | opcodes.instr_str
                        rd,rs1,imm,val=parse_ld_operands(line)
                        instr|=imm<<4
                        instr|=ce<<5
                        instr|=cc<<6
                        instr|=rd<<10
                        instr|=rs1<<14
                        instr|=val<<18
                    case "b":
                        x=re.match("^[b|bl|bx|bxl]\s+([a-zA-Z]+[0-9]*)", line)
                        if x != None:
                            target_label=str(x.group(1))
                            branches[target_label]=label_offset-4
                        else:
                            err="invalid branch label: "
                            raise ValueError(err+line)
                        instr|=1<<4
                        instr=(instr & ~0xf) | opcodes.instr_b
                        instr|=ce<<5
                        instr|=cc<<6
                    case "nop":
                        instr=(instr & ~0xf) | opcodes.instr_nop
                    case "halt":
                        instr=(instr & ~0xf) | opcodes.instr_halt
                    case "bkpt":
                        instr=(instr & ~0xf) | opcodes.instr_bkpt
                    case _:
                        err="invalid opcode: "+opcode
                        raise ValueError(err)
                fo.write(instr.to_bytes(4, byteorder='little'))
            else:
                err="invalid instruction: "+line
                raise ValueError(err)

        #resolve label address
        for key in branches:
            if key in labels:
                fo.seek(branches[key] + 16, 0)
                instr=struct.unpack('I', fo.read(4))[0]
                offset=labels[key]-branches[key]
                offset=int.from_bytes(offset.to_bytes(4, 'little', signed=True), 'little', signed=False);
                offset&=0x3fff
                instr|=(offset<<18)
                fo.seek(branches[key] + 16, 0)
                fo.write(instr.to_bytes(4, byteorder='little'))
            else:
                err="unknown branch label: "+key
                raise ValueError(err)
        fo.seek(4, 0)
        fo.write(label_offset.to_bytes(4, byteorder='little'))
        fi.close()
        fo.close()

    except OSError as err:
        print("OS error: ", err)
    except ValueError as err:
        print(err)
        fi.close()
        fo.close()
