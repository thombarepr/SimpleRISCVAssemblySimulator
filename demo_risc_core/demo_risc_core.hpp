#include "bits/stdc++.h"
#include "iostream"
#include "cstdint"
#include "climits"
#include "string"
#include "fstream"

#define PC_IDX	(15U)
#define _PC	(_R[PC_IDX])

typedef enum {
	OPERAND_TYPE_NONE,
	OPERAND_TYPE_REG,
	OPERAND_TYPE_IMM,
	OPERAND_TYPE_MEM,
} operand_type_e;

typedef struct {
	operand_type_e type;
	uint16_t reg;
	uint32_t val;
} operand_t;

#define align(a, b) ((a + b) & ~(b - 1))

//opcode - allowing 16 instructions
#define OPCODE(x) (x & 0xf)
//immediate value as operand
#define IMM_OP(x)	((x >> 4) & 1)
//conditional execution needed
#define COND_EXE(x)	((x >> 5) & 1)
//conditional execution code
#define COND_CODE(x)	((x >> 6) & 0x7)
//status word update needed
#define swu(x)	((x >> 9) & 1)
//dest reg index
#define DEST_REG(x)	((x >> 10) & 0xf)
//source reg1 index
#define SRC1_REG(x)	((x >> 14) & 0xf)
//14bit signed ImmValue or offset OR source reg0 index
#define IMM_VAL(x)	((x >> 18) & 0x3fff)

#define ERR(err) cout<<"Error: "<<err<<endl;
#define WARN(msg) cout<<"Warning: "<<msg<<endl;
#define DBG_STR(msg) cout<<"Debug: "<<msg<<endl;
#define DBG() cout<<__func__<<":"<<std:dec<<__LINE__<<endl;
#define DBG_HEX(val)	cout<<#val<<" = 0x"<<std::hex<<val<<endl;
#define DBG_DEC(val)	cout<<#val<<" = "<<std::dec<<val<<endl;

typedef struct {
	uint8_t opcode;
	uint8_t finished;
	uint8_t stalled;
	uint8_t overflow;
	operand_t src0;
	operand_t src1;
	operand_t dest;
	uint32_t val;
} sDecodedInstr;

#define BIT(x) (1 << x)
#define PSR_CARRY_BIT	(_PSR & BIT(31U))
#define PSR_SET_CARRY_BIT (_PSR |= BIT(31U))
#define PSR_CLR_CARRY_BIT (_PSR &= ~BIT(31U))

#define PSR_ZERO_BIT	(_PSR & BIT(30U))
#define PSR_SET_ZERO_BIT (_PSR |= BIT(30U))
#define PSR_CLR_ZERO_BIT (_PSR &= ~BIT(30U))

typedef enum {
	CC_CS,
	CC_CL,
	CC_EQ,
	CC_NE,
} CC_e;

typedef enum {
	INSTR_MOVW = 0,
	INSTR_MOVT,
	INSTR_ADD,
	INSTR_SUB,
	INSTR_AND,
	INSTR_ORR,
	INSTR_EOR,
	INSTR_BIC,
	INSTR_LDR,
	INSTR_STR,
	INSTR_BRANCH,
	INSTR_NOP,
	INSTR_HALT,
	INSTR_BKPT,
	INSTR_MAX,
} instr_e;

using namespace std;
class _demo_risc_core {
	private:
	uint32_t _R[16];
	uint32_t _PSR;
	uint32_t _PC_HIGH;

	uintptr_t CodeMemBase;
	uintptr_t CodeMemCeil;
	uintptr_t DataMemBase;
	uintptr_t DataMemCeil;
	uint32_t CodeMemSize;
	uint32_t DataMemSize;

	uint32_t aInstr[2];
	sDecodedInstr asDInstr[4];

	void fetch(void)
	{
		uintptr_t _PC0 = (((uintptr_t)_PC_HIGH<<32)|_PC);

		if (_PC0 == 0 ||
		    _PC0 < CodeMemBase ||
		    _PC0 > CodeMemCeil)
		{
			ERR("Prefetch abort");
			DBG_HEX(CodeMemBase);
			DBG_HEX(CodeMemCeil);
			DBG_HEX(_PC_HIGH);
			DBG_HEX(_PC);
			DBG_HEX(_PC0);
			while (1);
		}
		aInstr[0] = *(reinterpret_cast<uint32_t*>(_PC0));
		_PC0 += 4;
		_PC_HIGH = (_PC0>>32) & 0xffffffff;
		_PC = (_PC0 & 0xffffffff);
	}

	void print_context()
	{
		for (int i = 0; i < 16; i++) {
			cout<<"R["<<std::dec<<i<<"] = 0x"<<std::hex<<_R[i];
			cout<<((i+1)%4 ? "\t" : "\n");
		}
		DBG_HEX(_PSR);
	}

	void decode(void)
	{
		uint32_t instr = aInstr[1];
		sDecodedInstr *pDecode = &asDInstr[0];

		if ((COND_EXE(instr) && 
			((COND_CODE(instr) == CC_CS && !PSR_CARRY_BIT) ||
			 (COND_CODE(instr) == CC_CL && PSR_CARRY_BIT) ||
			 (COND_CODE(instr) == CC_EQ && !PSR_ZERO_BIT) ||
			 (COND_CODE(instr) == CC_NE && PSR_ZERO_BIT))) ||
		    (OPCODE(instr) == INSTR_NOP))
		{
			pDecode->finished = 1;
			return;
		}

		if (OPCODE(instr) == INSTR_HALT)
		{
			print_context();
			WARN("HALT");
			while (1);
		}

		if (OPCODE(instr) == INSTR_BKPT)
		{
			DBG_STR("BKPT");
			print_context();
			pDecode->finished = 1;
			return;
		}

		pDecode->opcode = OPCODE(instr);

		pDecode->dest.type = OPERAND_TYPE_REG;
		pDecode->dest.reg = DEST_REG(instr);
		pDecode->dest.val = _R[DEST_REG(instr)];
		pDecode->src1.type = OPERAND_TYPE_REG;
		pDecode->src1.reg = SRC1_REG(instr);
		pDecode->src1.val = _R[SRC1_REG(instr)];
		if (IMM_OP(instr)) {
			pDecode->src0.type = OPERAND_TYPE_IMM;
			pDecode->src0.val = IMM_VAL(instr);
			// sign extend 14 bit offset or imm value
			if (pDecode->src0.val & BIT(13))
				pDecode->src0.val |= 0xffffc000;
		} else {
			pDecode->src0.type = OPERAND_TYPE_REG;
			pDecode->src0.reg = IMM_VAL(instr);
			pDecode->src0.val = _R[IMM_VAL(instr)];
		}

		if (pDecode->opcode == INSTR_BRANCH) {
			uintptr_t _PC0 = ((((uintptr_t)_PC_HIGH<<32))|_PC);
			uintptr_t _offset = pDecode->src0.val;
			if (_offset & BIT(13))
				_offset |= ((uintptr_t)0xffffffff<<32);
			_PC0 -= 4; //Adjusting PC increament in fetch
			_PC0 = _PC0 + _offset;
			_PC_HIGH = (_PC0>>32) & 0xffffffff;
			_PC = (_PC0 & 0xffffffff);
			pDecode->finished = 1;
			return;
		}
	}

	void execute(void)	
	{
		sDecodedInstr *pDecode = &asDInstr[1];
		long long res;
		uint32_t val;
		uint32_t src1 = pDecode->src1.val;
		uint32_t src0 = pDecode->src0.val;
		uint32_t dst = pDecode->dest.val;

		if (pDecode->finished)
			return;

		switch (pDecode->opcode) {
			case INSTR_MOVW:
			res = (src0 & 0xffff) | (dst & ~0xffff);
			break;
			case INSTR_MOVT:
			res = ((src0 & 0xffff) << 16) | (dst & 0xffff);
			break;
			case INSTR_ADD:
			res = (long long)src1 + (long long)src0;
			break;
			case INSTR_SUB:
			res = (long long)src1 - (long long)src0;
			break;
			case INSTR_AND:
			res = src0 & src1;
			break;
			case INSTR_ORR:
			res = src0 | src1;
			break;
			case INSTR_EOR:
			res = src0 ^ src1;
			break;
			case INSTR_BIC:
			res = src1 & ~src0;
			break;
			case INSTR_LDR:
			case INSTR_STR:
			res = src0 + src1;
			break;
		}
		
		if (res > INT_MAX || res < INT_MIN)
			pDecode->overflow = 1;
		//if (pDecode->swu)
		{
			pDecode->overflow ? PSR_SET_CARRY_BIT : PSR_CLR_CARRY_BIT;
			(res == 0) ? PSR_SET_ZERO_BIT : PSR_CLR_ZERO_BIT;
		}
		pDecode->val = res;
	}

	void mem_access(void)	
	{
		sDecodedInstr *pDecode = &asDInstr[2];

		if (pDecode->finished)
			return;

		switch (pDecode->opcode) {
			case INSTR_LDR:
			pDecode->val = *(reinterpret_cast<uint32_t*>(pDecode->val));
			break;
			case INSTR_STR:
			if (pDecode->val < DataMemBase ||
			    pDecode->val > DataMemCeil)
			{
				ERR("Data abort");
				while (1);
			}
			*(reinterpret_cast<uint32_t*>(pDecode->val)) = pDecode->dest.val;
			pDecode->finished = 1;
			break;
		}
	}

	void writeback(void)	
	{
		sDecodedInstr *pDecode = &asDInstr[3];

		if (pDecode->finished)
			return;

		switch (pDecode->opcode) {
			case INSTR_MOVW:
			case INSTR_MOVT:
			case INSTR_ADD:
			case INSTR_SUB:
			case INSTR_AND:
			case INSTR_ORR:
			case INSTR_EOR:
			case INSTR_BIC:
			case INSTR_LDR:
			{
				_R[pDecode->dest.reg] = pDecode->val;
				pDecode->finished = 1;
			}
			break;
		}
	}

	public:
	_demo_risc_core() 
	{
		static_assert(INSTR_MAX < 16, "Insufficient opcode size");
		memset(_R, 0, sizeof(_R));
		_PSR = 0;
		CodeMemBase = 0;
		DataMemBase = 0;
	}

	_demo_risc_core(string binary_file) : _demo_risc_core()
	{
		char header[16];
		uint32_t code_size, data_size;
		uint32_t *code, *data;
		ifstream fi(binary_file, ios::binary);

		if (!fi) {
			ERR("File I/O error");
			return;
		}

		fi.read(header, sizeof(header));
		if (header[0] != 'c' ||
		    header[1] != 'o' ||
		    header[2] != 'd' ||
		    header[3] != 'e' ||
		    header[8] != 'd' ||
		    header[9] != 'a' ||
		    header[10] != 't'||
		    header[11] != 'a') {
			fi.close();
			ERR("Invalid binary file");
			return;
		}

		code_size = static_cast<uint32_t>(header[4]);
		if ((code_size<0) || (code_size%4)) {
			fi.close();
			ERR("Invalid binary file");
			return;
		}
		code = new uint32_t[align(code_size, 8)/4];
		fi.read((char*)code, code_size);
		CodeMemBase = reinterpret_cast<uintptr_t>(code);
		CodeMemCeil = CodeMemBase + code_size;

		data_size = static_cast<uint32_t>(header[12]);
		if (data_size > 0) {
			if (data_size%4) {
				delete reinterpret_cast<uint32_t*>(CodeMemBase);
				CodeMemBase = 0;
				fi.close();
				ERR("Invalid binary file");
				return;
			}
			DataMemSize = data_size;
			data = new uint32_t[align(data_size, 8)/4];
			fi.read((char*)data, data_size);
			DataMemBase = reinterpret_cast<uintptr_t>(data);
			DataMemCeil = DataMemBase + data_size;
		} else {
			DataMemSize = 0;
		}
		_PC_HIGH = (CodeMemBase>>32) & 0xffffffff;
		_PC = (CodeMemBase & 0xffffffff);
		fi.close();
	}

	~_demo_risc_core()
	{
		if (CodeMemBase)	
			delete reinterpret_cast<uint32_t*>(CodeMemBase);
		if (DataMemBase)
			delete reinterpret_cast<uint32_t*>(DataMemBase);
	}

	void clock()
	{
		fetch();
		aInstr[1] = aInstr[0];
		memset(&asDInstr[0], 0, sizeof(asDInstr[0]));
		decode();
		memcpy(&asDInstr[1], &asDInstr[0], sizeof(asDInstr[0]));
		execute();
		memcpy(&asDInstr[2], &asDInstr[1], sizeof(asDInstr[0]));
		mem_access();
		memcpy(&asDInstr[3], &asDInstr[2], sizeof(asDInstr[0]));
		writeback();
	}
};
